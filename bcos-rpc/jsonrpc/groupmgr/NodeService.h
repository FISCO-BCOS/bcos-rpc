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
 * @brief NodeService.h
 * @file NodeService.h
 * @author: yujiechen
 * @date 2021-10-11
 */
#pragma once
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/executor/ExecutorInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/multigroup/ChainNodeInfo.h>
#include <bcos-framework/interfaces/multigroup/GroupInfo.h>
#include <bcos-framework/interfaces/protocol/BlockFactory.h>
#include <bcos-framework/interfaces/protocol/ServiceDesc.h>
#include <bcos-framework/interfaces/sync/BlockSyncInterface.h>
#include <bcos-framework/interfaces/txpool/TxPoolInterface.h>
#include <tarscpp/servant/Application.h>
namespace bcos
{
namespace rpc
{
class NodeService
{
public:
    using Ptr = std::shared_ptr<NodeService>;
    NodeService(bcos::ledger::LedgerInterface::Ptr _ledger,
        std::shared_ptr<bcos::executor::ExecutorInterface> _executor,
        bcos::txpool::TxPoolInterface::Ptr _txpool,
        bcos::consensus::ConsensusInterface::Ptr _consensus,
        bcos::sync::BlockSyncInterface::Ptr _sync, bcos::protocol::BlockFactory::Ptr _blockFactory)
      : m_ledger(_ledger),
        m_executor(_executor),
        m_txpool(_txpool),
        m_consensus(_consensus),
        m_sync(_sync),
        m_blockFactory(_blockFactory)
    {}
    virtual ~NodeService() {}

    bcos::ledger::LedgerInterface::Ptr ledger() { return m_ledger; }
    std::shared_ptr<bcos::executor::ExecutorInterface> executor() { return m_executor; }
    bcos::txpool::TxPoolInterface::Ptr txpool() { return m_txpool; }
    bcos::consensus::ConsensusInterface::Ptr consensus() { return m_consensus; }
    bcos::sync::BlockSyncInterface::Ptr sync() { return m_sync; }
    bcos::protocol::BlockFactory::Ptr blockFactory() { return m_blockFactory; }

private:
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    std::shared_ptr<bcos::executor::ExecutorInterface> m_executor;
    bcos::txpool::TxPoolInterface::Ptr m_txpool;
    bcos::consensus::ConsensusInterface::Ptr m_consensus;
    bcos::sync::BlockSyncInterface::Ptr m_sync;
    bcos::protocol::BlockFactory::Ptr m_blockFactory;
};

class NodeServiceFactory
{
public:
    using Ptr = std::shared_ptr<NodeServiceFactory>;
    NodeServiceFactory() = default;
    virtual ~NodeServiceFactory() {}
    NodeService::Ptr buildNodeService(std::string const& _chainID, std::string const& _groupID,
        bcos::group::ChainNodeInfo::Ptr _nodeInfo);

    template <typename T, typename S, typename... Args>
    std::shared_ptr<T> createServiceClient(
        std::string const& _appName, std::string const& _serviceName, const Args&... _args)
    {
        auto servantName = bcos::protocol::getPrxDesc(_appName, _serviceName);
        auto prx = Application::getCommunicator()->stringToProxy<S>(servantName);
        return std::make_shared<T>(prx, _args...);
    }
};
}  // namespace rpc
}  // namespace bcos