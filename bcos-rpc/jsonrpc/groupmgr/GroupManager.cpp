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
 * @brief GroupManager.cpp
 * @file GroupManager.cpp
 * @author: yujiechen
 * @date 2021-10-11
 */
#include "GroupManager.h"
#include <bcos-framework/interfaces/protocol/ServiceDesc.h>
using namespace bcos;
using namespace bcos::group;
using namespace bcos::rpc;
using namespace bcos::protocol;

void GroupManager::updateGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo)
{
    WriteGuard l(x_nodeServiceList);
    auto nodeInfos = _groupInfo->nodeInfos();
    auto const& groupID = _groupInfo->groupID();
    for (auto const& it : nodeInfos)
    {
        updateNodeServiceWithoutLock(groupID, it.second);
    }
    // TODO: notify the groupInfo to SDK
}

void GroupManager::updateNodeServiceWithoutLock(
    std::string const& _groupID, ChainNodeInfo::Ptr _nodeInfo)
{
    auto nodeAppName = getApplicationName(m_chainID, _groupID, _nodeInfo->nodeName());
    // the node has not been started
    if (_nodeInfo->status() != GroupStatus::Started)
    {
        if (m_nodeServiceList.count(nodeAppName))
        {
            m_nodeServiceList.erase(nodeAppName);
            BCOS_LOG(INFO) << LOG_DESC("Erase the node service for the node not started")
                           << printNodeInfo(_nodeInfo);
        }
        return;
    }
    // a started node
    if (!m_nodeServiceList.count(nodeAppName))
    {
        auto nodeService = m_nodeServiceFactory->buildNodeService(m_chainID, _groupID, _nodeInfo);
        m_nodeServiceList[nodeAppName] = nodeService;
        BCOS_LOG(INFO) << LOG_DESC("buildNodeService for the started new node")
                       << printNodeInfo(_nodeInfo);
    }
}

NodeService::Ptr GroupManager::getNodeService(
    std::string const& _groupID, std::string const& _nodeName) const
{
    auto appName = getApplicationName(m_chainID, _groupID, _nodeName);
    ReadGuard l(x_nodeServiceList);
    if (m_nodeServiceList.count(appName))
    {
        return m_nodeServiceList.at(appName);
    }
    return nullptr;
}