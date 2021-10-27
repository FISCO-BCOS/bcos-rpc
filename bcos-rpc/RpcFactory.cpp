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
#include <bcos-framework/libprotocol/amop/AMOPRequest.h>
#include <bcos-framework/libutilities/Exceptions.h>
#include <bcos-framework/libutilities/FileUtility.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-rpc/RpcFactory.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <string>
#include <utility>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::protocol;
using namespace bcos::crypto;
using namespace bcos::gateway;
using namespace bcos::group;
using namespace bcos::boostssl::ws;
using namespace bcos::protocol;

RpcFactory::RpcFactory(std::string const& _chainID, GatewayInterface::Ptr _gatewayInterface,
    KeyFactory::Ptr _keyFactory, std::string const& _clientID,
    std::string const& _gatewayServiceName)
  : m_gatewayInterface(_gatewayInterface),
    m_keyFactory(_keyFactory),
    m_clientID(_clientID),
    m_gatewayServiceName(_gatewayServiceName)
{
    auto nodeServiceFactory = std::make_shared<NodeServiceFactory>();
    m_groupManager = std::make_shared<GroupManager>(_chainID, nodeServiceFactory);
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

void RpcFactory::registerHandlers(std::shared_ptr<boostssl::ws::WsService> _wsService,
    bcos::rpc::JsonRpcImpl_2_0::Ptr _jsonRpcInterface)
{
    _wsService->registerMsgHandler(bcos::rpc::MessageType::RPC_REQUEST,
        [_jsonRpcInterface](std::shared_ptr<boostssl::ws::WsMessage> _msg,
            std::shared_ptr<boostssl::ws::WsSession> _session) {
            if (!_jsonRpcInterface)
            {
                return;
            }
            std::string req = std::string(_msg->data()->begin(), _msg->data()->end());
            _jsonRpcInterface->onRPCRequest(req, [req, _msg, _session](const std::string& _resp) {
                if (_session && _session->isConnected())
                {
                    auto buffer = std::make_shared<bcos::bytes>(_resp.begin(), _resp.end());
                    _msg->setData(buffer);
                    _session->asyncSendMessage(_msg);
                }
                else
                {
                    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());
                    BCOS_LOG(WARNING)
                        << LOG_DESC("[RPC][FACTORY][buildJsonRpc]")
                        << LOG_DESC("unable to send response for session has been inactive")
                        << LOG_KV("req", req) << LOG_KV("resp", _resp) << LOG_KV("seq", seq)
                        << LOG_KV("endpoint", _session ? _session->endPoint() : std::string(""));
                }
            });
        });
}
bcos::rpc::JsonRpcImpl_2_0::Ptr RpcFactory::buildJsonRpc(
    std::shared_ptr<boostssl::ws::WsService> _wsService)
{
    assert(m_groupManager);
    // JsonRpcImpl_2_0
    auto jsonRpcInterface =
        std::make_shared<bcos::rpc::JsonRpcImpl_2_0>(m_groupManager, m_gatewayInterface);
    auto httpServer = _wsService->httpServer();
    if (httpServer)
    {
        httpServer->setHttpReqHandler(std::bind(&bcos::rpc::JsonRpcInterface::onRPCRequest,
            jsonRpcInterface, std::placeholders::_1, std::placeholders::_2));
    }
    registerHandlers(_wsService, jsonRpcInterface);
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
Rpc::Ptr RpcFactory::buildRpc(const std::string& _configPath)
{
    auto config = initConfig(_configPath);
    return buildRpc(config);
}

/**
 * @brief: Rpc
 * @param _config: WsConfig
 * @param _nodeInfo: node info
 * @return Rpc::Ptr:
 */
Rpc::Ptr RpcFactory::buildRpc(bcos::boostssl::ws::WsConfig::Ptr _config)
{
    // checkParams();

    auto wsService = buildWsService(_config);

    // JsonRpc
    auto jsonRpc = buildJsonRpc(wsService);
    // EventSub
    auto es = buildEventSub(wsService);

    auto wsFactory = std::make_shared<WsMessageFactory>();
    auto requestFactory = std::make_shared<AMOPRequestFactory>();
    auto rpc = std::make_shared<Rpc>(wsService, jsonRpc, es, wsFactory, requestFactory,
        m_gatewayInterface, m_clientID, m_gatewayServiceName);
    BCOS_LOG(INFO) << LOG_DESC("[RPC][FACTORY][buildRpc]")
                   << LOG_KV("listenIP", _config->listenIP())
                   << LOG_KV("listenPort", _config->listenPort())
                   << LOG_KV("threadCount", _config->threadPoolSize())
                   << LOG_KV("clientID", m_clientID);
    return rpc;
}