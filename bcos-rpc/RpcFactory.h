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
#include <bcos-framework/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-rpc/Rpc.h>
#include <bcos-rpc/amop/AMOP.h>
#include <bcos-rpc/http/ws/WsService.h>
#include <bcos-rpc/http/ws/WsSession.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>

namespace bcos
{
namespace rpc
{
// rpc config
struct RpcConfig
{
    std::string m_listenIP;
    uint16_t m_listenPort;
    std::size_t m_threadCount;

    void initConfig(const std::string& _configPath);
};

class RpcFactory : public std::enable_shared_from_this<RpcFactory>
{
public:
    using Ptr = std::shared_ptr<RpcFactory>;
    RpcFactory(std::string const& _chainID, bcos::gateway::GatewayInterface::Ptr _gatewayInterface,
        bcos::crypto::KeyFactory::Ptr _keyFactory);
    virtual ~RpcFactory() {}

    bcos::amop::AMOP::Ptr buildAMOP();
    JsonRpcInterface::Ptr buildJsonRpc();
    ws::WsSession::Ptr buildWsSession(
        boost::asio::ip::tcp::socket&& _socket, std::weak_ptr<ws::WsService> _wsService);

    /**
     * @brief: Rpc
     * @param _rpcConfig: rpc config
     * @param _nodeInfo: node config
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const RpcConfig& _rpcConfig);

    /**
     * @brief: Rpc
     * @param _configPath: rpc config path
     * @param _nodeInfo: node config
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const std::string& _configPath);

private:
    bcos::gateway::GatewayInterface::Ptr m_gatewayInterface;
    std::shared_ptr<bcos::crypto::KeyFactory> m_keyFactory;
    GroupManager::Ptr m_groupManager;
};
}  // namespace rpc
}  // namespace bcos