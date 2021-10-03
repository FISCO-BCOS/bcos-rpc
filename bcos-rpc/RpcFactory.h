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
#include <bcos-boostssl/websocket/WsConfig.h>
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/interfaces/executor/ExecutorInterface.h>
#include <bcos-framework/interfaces/front/FrontServiceInterface.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/sync/BlockSyncInterface.h>
#include <bcos-framework/interfaces/txpool/TxPoolInterface.h>
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

public:
    std::shared_ptr<boostssl::ws::WsConfig> initConfig(const std::string& _configPath);
    std::shared_ptr<boostssl::ws::WsService> buildWsService(
        bcos::boostssl::ws::WsConfig::Ptr _config);
    bcos::amop::AMOP::Ptr buildAMOP(std::shared_ptr<boostssl::ws::WsService> _wsService);
    bcos::rpc::JsonRpcImpl_2_0::Ptr buildJsonRpc(
        const NodeInfo& _nodeInfo, std::shared_ptr<boostssl::ws::WsService> _wsService);
    bcos::event::EventSub::Ptr buildEventSub(std::shared_ptr<boostssl::ws::WsService> _wsService);

    /**
     * @brief: Rpc
     * @param _config: WsConfig
     * @param _nodeInfo: node info
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(bcos::boostssl::ws::WsConfig::Ptr _config, const NodeInfo& _nodeInfo);

    /**
     * @brief: Rpc
     * @param _configPath: rpc config path
     * @param _nodeInfo: node config
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const std::string& _configPath, const NodeInfo& _nodeInfo);

public:
    bcos::boostssl::ws::WsConfig::Ptr config() const { return m_config; }
    void setConfig(bcos::boostssl::ws::WsConfig::Ptr _config) { m_config = _config; }

    bcos::ledger::LedgerInterface::Ptr ledger() const { return m_ledgerInterface; }
    void setLedger(bcos::ledger::LedgerInterface::Ptr _ledgerInterface)
    {
        m_ledgerInterface = _ledgerInterface;
    }

    std::shared_ptr<bcos::executor::ExecutorInterface> executorInterface() const
    {
        return m_executorInterface;
    }
    void setExecutorInterface(std::shared_ptr<bcos::executor::ExecutorInterface> _executorInterface)
    {
        m_executorInterface = _executorInterface;
    }

    bcos::txpool::TxPoolInterface::Ptr txPoolInterface() const { return m_txPoolInterface; }
    void setTxPoolInterface(bcos::txpool::TxPoolInterface::Ptr _txPoolInterface)
    {
        m_txPoolInterface = _txPoolInterface;
    }

    bcos::consensus::ConsensusInterface::Ptr consensusInterface() const
    {
        return m_consensusInterface;
    }
    void setConsensusInterface(bcos::consensus::ConsensusInterface::Ptr _consensusInterface)
    {
        m_consensusInterface = _consensusInterface;
    }

    bcos::sync::BlockSyncInterface::Ptr blockSyncInterface() const { return m_blockSyncInterface; }
    void setBlockSyncInterface(bcos::sync::BlockSyncInterface::Ptr _blockSyncInterface)
    {
        m_blockSyncInterface = _blockSyncInterface;
    }

    bcos::gateway::GatewayInterface::Ptr gatewayInterface() const { return m_gatewayInterface; }
    void setGatewayInterface(bcos::gateway::GatewayInterface::Ptr _gatewayInterface)
    {
        m_gatewayInterface = _gatewayInterface;
    }

    bcos::front::FrontServiceInterface::Ptr frontServiceInterface() const
    {
        return m_frontServiceInterface;
    }
    void setFrontServiceInterface(bcos::front::FrontServiceInterface::Ptr _frontServiceInterface)
    {
        m_frontServiceInterface = _frontServiceInterface;
    }

    void setTransactionFactory(bcos::protocol::TransactionFactory::Ptr _transactionFactory)
    {
        m_transactionFactory = _transactionFactory;
    }
    bcos::protocol::TransactionFactory::Ptr transactionFactory() const
    {
        return m_transactionFactory;
    }

    std::shared_ptr<bcos::crypto::KeyFactory> keyFactory() { return m_keyFactory; }
    void setKeyFactory(std::shared_ptr<bcos::crypto::KeyFactory> _keyFactory)
    {
        m_keyFactory = _keyFactory;
    }

public:
    void checkParams();

private:
    std::shared_ptr<bcos::crypto::KeyFactory> m_keyFactory;
    bcos::ledger::LedgerInterface::Ptr m_ledgerInterface;
    std::shared_ptr<bcos::executor::ExecutorInterface> m_executorInterface;
    bcos::txpool::TxPoolInterface::Ptr m_txPoolInterface;
    bcos::consensus::ConsensusInterface::Ptr m_consensusInterface;
    bcos::sync::BlockSyncInterface::Ptr m_blockSyncInterface;
    bcos::front::FrontServiceInterface::Ptr m_frontServiceInterface;
    bcos::gateway::GatewayInterface::Ptr m_gatewayInterface;
    bcos::protocol::TransactionFactory::Ptr m_transactionFactory;
    bcos::boostssl::ws::WsConfig::Ptr m_config;
};
}  // namespace rpc
}  // namespace bcos