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

#include <bcos-boostssl/websocket/WsInitializer.h>
#include <bcos-boostssl/websocket/WsMessage.h>
#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-framework/libutilities/Exceptions.h>
#include <bcos-framework/libutilities/FileUtility.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-rpc/RpcFactory.h>
#include <bcos-rpc/amop/TopicManager.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <utility>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::amop;

void RpcFactory::checkParams()
{
    if (!m_frontServiceInterface)
    {
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment(
                "RpcFactory::checkParams frontServiceInterface is uninitialized"));
    }

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

std::shared_ptr<bcos::boostssl::ws::WsConfig> RpcFactory::initConfig(const std::string& _configPath)
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

        auto config = std::make_shared<boostssl::ws::WsConfig>();
        config->setModel(bcos::boostssl::ws::WsModel::Server);
        config->setListenIP(listenIP);
        config->setListenPort(listenPort);
        config->setThreadPoolSize(threadCount);

        BCOS_LOG(INFO) << LOG_DESC("[RPC][FACTORY][initConfig]") << LOG_KV("listenIP", listenIP)
                       << LOG_KV("listenPort", listenPort) << LOG_KV("threadCount", threadCount)
                       << LOG_KV("asServer", config->asServer());

        return config;
    }
    catch (const std::exception& e)
    {
        boost::filesystem::path full_path(boost::filesystem::current_path());
        BCOS_LOG(ERROR) << LOG_DESC("[RPC][FACTORY][initConfig]")
                        << LOG_KV("configPath", _configPath)
                        << LOG_KV("currentPath", full_path.string())
                        << LOG_KV("error: ", boost::diagnostic_information(e));
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment("initConfig: currentPath:" + full_path.string() +
                                                  " ,error:" + boost::diagnostic_information(e)));
    }
}

bcos::boostssl::ws::WsService::Ptr RpcFactory::buildWsService(
    bcos::boostssl::ws::WsConfig::Ptr _config)
{
    auto wsService = std::make_shared<bcos::boostssl::ws::WsService>();
    auto initializer = std::make_shared<bcos::boostssl::ws::WsInitializer>();

    initializer->setConfig(_config);
    initializer->initWsService(wsService);
    return wsService;
}

bcos::amop::AMOP::Ptr RpcFactory::buildAMOP(std::shared_ptr<boostssl::ws::WsService> _wsService)
{
    auto topicManager = std::make_shared<amop::TopicManager>();
    auto messageFactory = std::make_shared<bcos::amop::MessageFactory>();
    auto amop = std::make_shared<bcos::amop::AMOP>();
    auto requestFactory = std::make_shared<AMOPRequestFactory>();

    auto amopWeak = std::weak_ptr<bcos::amop::AMOP>(amop);
    auto wsServiceWeak = std::weak_ptr<boostssl::ws::WsService>(_wsService);

    amop->setFrontServiceInterface(m_frontServiceInterface);
    amop->setKeyFactory(m_keyFactory);
    amop->setMessageFactory(messageFactory);
    amop->setWsMessageFactory(_wsService->messageFactory());
    amop->setTopicManager(topicManager);
    amop->setIoc(_wsService->ioc());
    amop->setWsService(wsServiceWeak);
    amop->setRequestFactory(requestFactory);
    amop->setThreadPool(_wsService->threadPool());

    _wsService->registerMsgHandler(bcos::amop::MessageType::AMOP_SUBTOPIC,
        [amopWeak](std::shared_ptr<boostssl::ws::WsMessage> _msg,
            std::shared_ptr<boostssl::ws::WsSession> _session) {
            auto amop = amopWeak.lock();
            if (amop)
            {
                amop->onRecvSubTopics(_msg, _session);
            }
        });

    _wsService->registerMsgHandler(bcos::amop::MessageType::AMOP_REQUEST,
        [amopWeak](std::shared_ptr<boostssl::ws::WsMessage> _msg,
            std::shared_ptr<boostssl::ws::WsSession> _session) {
            auto amop = amopWeak.lock();
            if (amop)
            {
                amop->onRecvAMOPRequest(_msg, _session);
            }
        });

    _wsService->registerMsgHandler(bcos::amop::MessageType::AMOP_BROADCAST,
        [amopWeak](std::shared_ptr<boostssl::ws::WsMessage> _msg,
            std::shared_ptr<boostssl::ws::WsSession> _session) {
            auto amop = amopWeak.lock();
            if (amop)
            {
                amop->onRecvAMOPBroadcast(_msg, _session);
            }
        });

    return amop;
}

bcos::rpc::JsonRpcImpl_2_0::Ptr RpcFactory::buildJsonRpc(
    const NodeInfo& _nodeInfo, std::shared_ptr<boostssl::ws::WsService> _wsService)
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

    auto httpServer = _wsService->httpServer();
    if (httpServer)
    {
        httpServer->setHttpReqHandler(std::bind(&bcos::rpc::JsonRpcInterface::onRPCRequest,
            jsonRpcInterface, std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        BCOS_LOG(INFO) << LOG_DESC("[RPC][FACTORY][buildJsonRpc]")
                       << LOG_DESC("http server is null") << LOG_KV("model", m_config->model());
    }

    return jsonRpcInterface;
}

bcos::event::EventSub::Ptr RpcFactory::buildEventSub(
    std::shared_ptr<boostssl::ws::WsService> _wsService)
{
    // TODO:
    boost::ignore_unused(_wsService);
    return nullptr;
}

/**
 * @brief: Rpc
 * @param _configPath: rpc config path
 * @return Rpc::Ptr:
 */
Rpc::Ptr RpcFactory::buildRpc(const std::string& _configPath, const NodeInfo& _nodeInfo)
{
    auto config = initConfig(_configPath);
    return buildRpc(config, _nodeInfo);
}

/**
 * @brief: Rpc
 * @param _config: WsConfig
 * @param _nodeInfo: node info
 * @return Rpc::Ptr:
 */
Rpc::Ptr RpcFactory::buildRpc(bcos::boostssl::ws::WsConfig::Ptr _config, const NodeInfo& _nodeInfo)
{
    // checkParams();

    auto wsService = buildWsService(_config);

    // JsonRpc
    auto jsonRpc = buildJsonRpc(_nodeInfo, wsService);
    // AMOP
    auto amop = buildAMOP(wsService);
    // EventSub
    auto es = buildEventSub(wsService);

    auto rpc = std::make_shared<Rpc>();
    rpc->setWsService(wsService);
    rpc->setAMOP(amop);
    rpc->setEventSub(es);
    rpc->setJsonRpcImpl(jsonRpc);

    BCOS_LOG(INFO) << LOG_DESC("[RPC][FACTORY][buildRpc]")
                   << LOG_KV("listenIP", _config->listenIP())
                   << LOG_KV("listenPort", _config->listenPort())
                   << LOG_KV("threadCount", _config->threadPoolSize());
    return rpc;
}