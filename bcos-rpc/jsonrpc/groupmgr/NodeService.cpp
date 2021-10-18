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
 * @brief NodeService
 * @file NodeService.cpp
 * @author: yujiechen
 * @date 2021-10-11
 */
#include "NodeService.h"
#include "Common.h"
#include <bcos-framework/interfaces/protocol/ServiceDesc.h>
#include <bcos-tars-protocol/client/LedgerServiceClient.h>
#include <bcos-tars-protocol/client/PBFTServiceClient.h>
#include <bcos-tars-protocol/client/SchedulerServiceClient.h>
#include <bcos-tars-protocol/client/TxPoolServiceClient.h>
#include <tarscpp/servant/Application.h>
using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::crypto;
using namespace bcos::group;
using namespace bcos::protocol;

NodeService::Ptr NodeServiceFactory::buildNodeService(std::string const& _chainID,
    std::string const& _groupID, bcos::group::ChainNodeInfo::Ptr _nodeInfo)
{
    auto appName = getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
    // create cryptoSuite
    auto const& type = _nodeInfo->nodeType();
    CryptoSuite::Ptr cryptoSuite = nullptr;
    if (type == NodeType::SM_NODE)
    {
        cryptoSuite = createSMCryptoSuite();
    }
    else
    {
        cryptoSuite = createCryptoSuite();
    }
    auto blockFactory = createBlockFactory(cryptoSuite);
    auto ledgerClient =
        createServiceClient<bcostars::LedgerServiceClient, bcostars::LedgerServicePrx>(
            appName, LEDGER_SERVICE_NAME, blockFactory);
    auto schedulerClient =
        createServiceClient<bcostars::SchedulerServiceClient, bcostars::SchedulerServicePrx>(
            appName, SCHEDULER_SERVICE_NAME, cryptoSuite);

    // create txpool client
    auto txpoolClient =
        createServiceClient<bcostars::TxPoolServiceClient, bcostars::TxPoolServicePrx>(
            appName, TXPOOL_SERVICE_NAME, cryptoSuite, blockFactory);
    // create consensus client
    auto consensusClient =
        createServiceClient<bcostars::PBFTServiceClient, bcostars::PBFTServicePrx>(
            appName, CONSENSUS_SERVICE_NAME);
    // create sync client
    auto syncClient =
        createServiceClient<bcostars::BlockSyncServiceClient, bcostars::PBFTServicePrx>(
            appName, CONSENSUS_SERVICE_NAME);
    return std::make_shared<NodeService>(
        ledgerClient, schedulerClient, txpoolClient, consensusClient, syncClient, blockFactory);
}