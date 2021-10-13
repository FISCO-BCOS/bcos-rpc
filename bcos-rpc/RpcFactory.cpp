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
#include <bcos-framework/interfaces/multigroup/ChainNodeInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/GroupInfoFactory.h>
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
using namespace bcos::protocol;
using namespace bcos::crypto;
using namespace bcos::gateway;
using namespace bcos::amop;
using namespace bcos::group;

RpcFactory::RpcFactory(std::string const& _chainID, GatewayInterface::Ptr _gatewayInterface,
    GroupManagerInterface::Ptr _groupMgr, GroupInfoFactory::Ptr _groupInfoFactory,
    ChainNodeInfoFactory::Ptr _chainNodeInfoFactory, KeyFactory::Ptr _keyFactory)
  : m_gatewayInterface(_gatewayInterface), m_keyFactory(_keyFactory)
{
    auto nodeServiceFactory = std::make_shared<NodeServiceFactory>();
    m_groupManager = std::make_shared<GroupManager>(
        _chainID, nodeServiceFactory, _groupMgr, _groupInfoFactory, _chainNodeInfoFactory);
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