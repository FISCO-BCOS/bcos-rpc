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
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/executor/ExecutorInterface.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/sync/BlockSyncInterface.h>
#include <bcos-framework/interfaces/txpool/TxPoolInterface.h>
#include <bcos-rpc/rpc/Rpc.h>
#include <bcos-rpc/rpc/jsonrpc/JsonRpcImpl_2_0.h>

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

public:
    JsonRpcInterface::Ptr buildJsonRpc(const NodeInfo& _nodeInfo);
    /**
     * @brief: Rpc
     * @param _rpcConfig: rpc config
     * @param _nodeInfo: node config
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const RpcConfig& _rpcConfig, const NodeInfo& _nodeInfo);

    /**
     * @brief: Rpc
     * @param _configPath: rpc config path
     * @param _nodeInfo: node config
     * @return Rpc::Ptr:
     */
    Rpc::Ptr buildRpc(const std::string& _configPath, const NodeInfo& _nodeInfo);

public:
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

    void setTransactionFactory(bcos::protocol::TransactionFactory::Ptr _transactionFactory)
    {
        m_transactionFactory = _transactionFactory;
    }
    bcos::protocol::TransactionFactory::Ptr transactionFactory() const
    {
        return m_transactionFactory;
    }

public:
    void checkParams();

private:
    bcos::ledger::LedgerInterface::Ptr m_ledgerInterface;
    std::shared_ptr<bcos::executor::ExecutorInterface> m_executorInterface;
    bcos::txpool::TxPoolInterface::Ptr m_txPoolInterface;
    bcos::consensus::ConsensusInterface::Ptr m_consensusInterface;
    bcos::sync::BlockSyncInterface::Ptr m_blockSyncInterface;
    bcos::gateway::GatewayInterface::Ptr m_gatewayInterface;
    bcos::protocol::TransactionFactory::Ptr m_transactionFactory;
};
}  // namespace rpc
}  // namespace bcos