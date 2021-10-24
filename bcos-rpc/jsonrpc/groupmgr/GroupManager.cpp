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
    auto const& groupID = _groupInfo->groupID();
    if (!m_groupInfos.count(groupID))
    {
        m_groupInfos[groupID] = _groupInfo;
        GROUP_LOG(INFO) << LOG_DESC("updateGroupInfo") << printGroupInfo(_groupInfo);
        return;
    }
    auto nodeInfos = _groupInfo->nodeInfos();
    for (auto const& it : nodeInfos)
    {
        updateNodeServiceWithoutLock(groupID, it.second);
    }
}

void GroupManager::updateNodeServiceWithoutLock(
    std::string const& _groupID, ChainNodeInfo::Ptr _nodeInfo)
{
    auto nodeAppName = _nodeInfo->nodeName();
    // a started node
    if (!m_nodeServiceList.count(nodeAppName))
    {
        auto nodeService = m_nodeServiceFactory->buildNodeService(m_chainID, _groupID, _nodeInfo);
        if (!nodeService)
        {
            return;
        }
        m_nodeServiceList[nodeAppName] = nodeService;
        auto groupInfo = m_groupInfos[_groupID];
        groupInfo->appendNodeInfo(_nodeInfo);
        BCOS_LOG(INFO) << LOG_DESC("buildNodeService for the started new node")
                       << printNodeInfo(_nodeInfo) << printGroupInfo(groupInfo);
    }
}

NodeService::Ptr GroupManager::selectNode(std::string const& _groupID) const
{
    auto nodeName = selectNodeByBlockNumber(_groupID);
    if (nodeName.size() == 0)
    {
        return selectNodeRandomly(_groupID);
    }
    return queryNodeService(nodeName);
}
std::string GroupManager::selectNodeByBlockNumber(std::string const& _groupID) const
{
    ReadGuard l(x_groupBlockInfos);
    if (!m_nodesWithLatestBlockNumber.count(_groupID))
    {
        return "";
    }
    srand(utcTime());
    auto const& nodesList = m_nodesWithLatestBlockNumber.at(_groupID);
    auto selectNodeIndex = rand() % nodesList.size();
    auto it = nodesList.begin();
    std::advance(it, selectNodeIndex);
    return *it;
}

NodeService::Ptr GroupManager::selectNodeRandomly(std::string const& _groupID) const
{
    ReadGuard l(x_nodeServiceList);
    if (!m_groupInfos.count(_groupID))
    {
        return nullptr;
    }
    auto const& groupInfo = m_groupInfos.at(_groupID);
    auto const& nodeInfos = groupInfo->nodeInfos();
    for (auto const& it : nodeInfos)
    {
        auto const& node = it.second;
        if (m_nodeServiceList.count(node->nodeName()))
        {
            return m_nodeServiceList.at(node->nodeName());
        }
    }
    return nullptr;
}

NodeService::Ptr GroupManager::queryNodeService(std::string const& _nodeName) const
{
    ReadGuard l(x_nodeServiceList);
    if (m_nodeServiceList.count(_nodeName))
    {
        return m_nodeServiceList.at(_nodeName);
    }
    return nullptr;
}

NodeService::Ptr GroupManager::getNodeService(
    std::string const& _groupID, std::string const& _nodeName) const
{
    if (_nodeName.size() > 0)
    {
        return queryNodeService(_nodeName);
    }
    return selectNode(_groupID);
}

void GroupManager::updateGroupStatus()
{
    m_groupStatusUpdater->restart();
    std::vector<std::string> unreachableNodes;
    {
        UpgradableGuard l(x_nodeServiceList);
        for (auto const& it : m_groupInfos)
        {
            bool groupInfoUpdated = false;
            auto groupInfo = it.second;
            auto const& groupNodeList = groupInfo->nodeInfos();
            for (auto const& nodeInfo : groupNodeList)
            {
                if (!m_nodeServiceList.count(nodeInfo.first))
                {
                    groupInfo->removeNodeInfo(nodeInfo.second);
                    groupInfoUpdated = true;
                    unreachableNodes.emplace_back(nodeInfo.first);
                    continue;
                }
                auto nodeService = m_nodeServiceList.at(nodeInfo.first);
                if (nodeService->unreachable())
                {
                    GROUP_LOG(INFO)
                        << LOG_DESC("erase the node service for unreachable")
                        << LOG_KV("group", groupInfo->groupID()) << LOG_KV("node", nodeInfo.first);
                    groupInfo->removeNodeInfo(nodeInfo.second);
                    groupInfoUpdated = true;
                    unreachableNodes.emplace_back(nodeInfo.first);
                }
            }
            // notify the updated groupInfo to the sdk
            if (m_groupInfoNotifier && groupInfoUpdated)
            {
                m_groupInfoNotifier(groupInfo);
            }
        }
        if (unreachableNodes.size() == 0)
        {
            return;
        }
        // update m_nodeServiceList
        UpgradeGuard ul(l);
        for (auto const& node : unreachableNodes)
        {
            m_nodeServiceList.erase(node);
        }
    }
    WriteGuard l(x_groupBlockInfos);
    for (auto const& node : unreachableNodes)
    {
        m_groupBlockInfos.erase(node);
        m_nodesWithLatestBlockNumber.erase(node);
    }
}