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
 * @brief interface for RPC
 * @file Rpc.h
 * @author: octopus
 * @date 2021-07-15
 */

#pragma once
#include "bcos-rpc/jsonrpc/groupmgr/GroupManager.h"
#include <bcos-boostssl/websocket/WsConfig.h>
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-rpc/Rpc.h>
#include <bcos-rpc/amop/AMOP.h>
#include <bcos-rpc/event/EventSub.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>

namespace bcos
{
namespace boostssl
{
namespace ws
{
class WsService;
class WsSession;
}  // namespace ws
}  // namespace boostssl

namespace rpc
{
class RpcFactory : public std::enable_shared_from_this<RpcFactory>
{
public:
    using Ptr = std::shared_ptr<RpcFactory>;
    RpcFactory(std::string const& _chainID, bcos::gateway::GatewayInterface::Ptr _gatewayInterface,
        bcos::group::GroupManagerInterface::Ptr _groupMgr,
        bcos::group::GroupInfoFactory::Ptr _groupInfoFactory,
        bcos::group::ChainNodeInfoFactory::Ptr _chainNodeInfoFactory,
        bcos::crypto::KeyFactory::Ptr _keyFactory);
    virtual ~RpcFactory() {}

    std::shared_ptr<boostssl::ws::WsConfig> initConfig(const std::string& _configPath);
    std::shared_ptr<boostssl::ws::WsService> buildWsService(
        bcos::boostssl::ws::WsConfig::Ptr _config);
    bcos::amop::AMOP::Ptr buildAMOP(std::shared_ptr<boostssl::ws::WsService> _wsService);
    bcos::rpc::JsonRpcImpl_2_0::Ptr buildJsonRpc(
        std::shared_ptr<boostssl::ws::WsService> _wsService);
    bcos::event::EventSub::Ptr buildEventSub(std::shared_ptr<boostssl::ws::WsService> _wsService);

    /**
     * @brief: Rpc
     * @param _config: WsConfig
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(bcos::boostssl::ws::WsConfig::Ptr _config);

    /**
     * @brief: Rpc
     * @param _configPath: rpc config path
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const std::string& _configPath);
    bcos::boostssl::ws::WsConfig::Ptr config() const { return m_config; }
    void setConfig(bcos::boostssl::ws::WsConfig::Ptr _config) { m_config = _config; }

    GroupManager::Ptr groupManager() { return m_groupManager; }

private:
    bcos::gateway::GatewayInterface::Ptr m_gatewayInterface;
    std::shared_ptr<bcos::crypto::KeyFactory> m_keyFactory;
    GroupManager::Ptr m_groupManager;
    bcos::boostssl::ws::WsConfig::Ptr m_config;
};
}  // namespace rpc
}  // namespace bcos