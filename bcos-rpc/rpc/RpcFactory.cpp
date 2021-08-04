/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief RpcFactory
 * @file RpcFactory.h
 * @author: octopus
 * @date 2021-07-15
 */

#include "bcos-rpc/amop/TopicManager.h"
#include "bcos-rpc/http/ws/Common.h"
#include "libutilities/Log.h"
#include "libutilities/ThreadPool.h"
#include <bcos-framework/libutilities/Exceptions.h>
#include <bcos-framework/libutilities/FileUtility.h>
#include <bcos-rpc/http/HttpServer.h>
#include <bcos-rpc/http/ws/WsMessage.h>
#include <bcos-rpc/http/ws/WsSession.h>
#include <bcos-rpc/rpc/RpcFactory.h>
#include <bcos-rpc/rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <utility>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::http;

void RpcConfig::initConfig(const std::string& _configPath)
{
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(_configPath, pt);
        /*
        [rpc]
            listen_ip=0.0.0.0
            listen_port=30300
            thread_count=16
        */
        std::string listenIP = pt.get<std::string>("rpc.listen_ip", "0.0.0.0");
        int listenPort = pt.get<int>("rpc.listen_port", 20200);
        int threadCount = pt.get<int>("rpc.thread_count", 8);

        auto validPort = [](int port) -> bool { return (port <= 65535 && port > 1024); };
        if (!validPort(listenPort))
        {
            BOOST_THROW_EXCEPTION(
                InvalidParameter() << errinfo_comment(
                    "initConfig: invalid rpc listen port, port=" + std::to_string(listenPort)));
        }

        m_listenIP = listenIP;
        m_listenPort = listenPort;
        m_threadCount = threadCount;

        RPC_FACTORY(INFO) << LOG_DESC("initConfig") << LOG_KV("listenIP", listenIP)
                          << LOG_KV("listenPort", listenPort) << LOG_KV("threadCount", threadCount);
    }
    catch (const std::exception& e)
    {
        boost::filesystem::path full_path(boost::filesystem::current_path());
        RPC_FACTORY(ERROR) << LOG_DESC("initConfig") << LOG_KV("configPath", _configPath)
                           << LOG_KV("currentPath", full_path.string())
                           << LOG_KV("error: ", boost::diagnostic_information(e));
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment("initConfig: currentPath:" + full_path.string() +
                                                  " ,error:" + boost::diagnostic_information(e)));
    }
}

void RpcFactory::checkParams()
{
    if (!m_ledgerInterface)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams ledgerInterface is uninitialized"));
    }

    if (!m_executorInterface)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams executorInterface is uninitialized"));
    }

    if (!m_txPoolInterface)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams txPoolInterface is uninitialized"));
    }

    if (!m_consensusInterface)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams consensusInterface is uninitialized"));
    }

    if (!m_blockSyncInterface)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams blockSyncInterface is uninitialized"));
    }

    if (!m_transactionFactory)
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "RpcFactory::checkParams transactionFactory is uninitialized"));
    }
    return;
}

JsonRpcInterface::Ptr RpcFactory::buildJsonRpc(const NodeInfo& _nodeInfo)
{
    // JsonRpcImpl_2_0
    auto jsonRpcInterface = std::make_shared<bcos::rpc::JsonRpcImpl_2_0>();
    jsonRpcInterface->setNodeInfo(_nodeInfo);
    jsonRpcInterface->setLedger(m_ledgerInterface);
    jsonRpcInterface->setTxPoolInterface(m_txPoolInterface);
    jsonRpcInterface->setExecutorInterface(m_executorInterface);
    jsonRpcInterface->setConsensusInterface(m_consensusInterface);
    jsonRpcInterface->setBlockSyncInterface(m_blockSyncInterface);
    jsonRpcInterface->setTransactionFactory(m_transactionFactory);
    jsonRpcInterface->setGatewayInterface(m_gatewayInterface);
    return jsonRpcInterface;
}

/**
 * @brief: Rpc
 * @param _configPath: rpc config path
 * @return Rpc::Ptr:
 */
Rpc::Ptr RpcFactory::buildRpc(const std::string& _configPath, const NodeInfo& _nodeInfo)
{
    RpcConfig rpcConfig;
    rpcConfig.initConfig(_configPath);
    return buildRpc(rpcConfig, _nodeInfo);
}

/**
 * @brief: Rpc
 * @param _rpcConfig: rpc config
 * @return Rpc::Ptr:
 */
Rpc::Ptr RpcFactory::buildRpc(const RpcConfig& _rpcConfig, const NodeInfo& _nodeInfo)
{
    const std::string _listenIP = _rpcConfig.m_listenIP;
    uint16_t _listenPort = _rpcConfig.m_listenPort;
    std::size_t _threadCount = _rpcConfig.m_threadCount;

    checkParams();

    // JsonRpcImpl_2_0
    auto jsonRpcInterface = buildJsonRpc(_nodeInfo);

    // HttpServer
    auto httpServerFactory = std::make_shared<bcos::http::HttpServerFactory>();
    auto httpServer = httpServerFactory->buildHttpServer(_listenIP, _listenPort, _threadCount);
    httpServer->setRequestHandler(std::bind(&bcos::rpc::JsonRpcInterface::onRPCRequest,
        jsonRpcInterface, std::placeholders::_1, std::placeholders::_2));

    auto topicManager = std::make_shared<amop::TopicManager>();
    auto wsMessageFactory = std::make_shared<ws::WsMessageFactory>();
    auto threadPool = std::make_shared<bcos::ThreadPool>("ws-service", _threadCount);
    // WsService
    auto wsService = std::make_shared<ws::WsService>();
    auto weakWsService = std::weak_ptr<ws::WsService>(wsService);
    wsService->setJsonRpcInterface(jsonRpcInterface);
    wsService->setTopicManager(topicManager);
    wsService->setIoc(httpServer->ioc());
    wsService->setMessageFactory(wsMessageFactory);
    wsService->setThreadPool(threadPool);

    httpServer->setWsUpgradeHandler(
        [threadPool, wsMessageFactory, weakWsService](
            boost::asio::ip::tcp::socket&& _socket, HttpRequest&& _httpRequest) {
            auto session = std::make_shared<ws::WsSession>(std::move(_socket));
            session->setThreadPool(threadPool);
            session->setMessageFactory(wsMessageFactory);
            session->setAcceptHandler(
                [weakWsService](bcos::Error::Ptr _error, std::shared_ptr<ws::WsSession> _session) {
                    boost::ignore_unused(_error);
                    auto service = weakWsService.lock();
                    if (service)
                    {
                        service->addSession(_session);
                    }
                });
            session->setDisconnectHandler(
                [weakWsService](bcos::Error::Ptr _error, std::shared_ptr<ws::WsSession> _session) {
                    auto service = weakWsService.lock();
                    if (service)
                    {
                        service->onDisconnect(_error, _session);
                    }
                });
            session->setRecvMessageHandler(
                [weakWsService](bcos::Error::Ptr _error, std::shared_ptr<ws::WsMessage> _msg,
                    std::shared_ptr<ws::WsSession> _session) {
                    auto service = weakWsService.lock();
                    if (service)
                    {
                        service->onRecvClientMessage(_error, _msg, _session);
                    }
                });

            // start websocket handshake
            session->doAccept(std::move(_httpRequest));
        });
    wsService->initMethod();

    auto rpc = std::make_shared<Rpc>();
    rpc->setHttpServer(httpServer);
    rpc->setWsService(wsService);
    RPC_FACTORY(INFO) << LOG_BADGE("buildRpc") << LOG_KV("listenIP", _listenIP)
                      << LOG_KV("listenPort", _listenPort) << LOG_KV("threadCount", _threadCount);
    return rpc;
}