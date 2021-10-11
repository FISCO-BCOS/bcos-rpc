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
 * @brief GroupManager.h
 * @file GroupManager.h
 * @author: yujiechen
 * @date 2021-10-11
 */
#pragma once
#include "NodeService.h"
#include <bcos-framework/interfaces/multigroup/GroupInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/GroupManagerInterface.h>
namespace bcos
{
namespace rpc
{
class GroupManager
{
public:
    using Ptr = std::shared_ptr<GroupManager>;
    GroupManager(std::string const& _chainID, NodeServiceFactory::Ptr _nodeServiceFactory)
      : m_chainID(_chainID), m_nodeServiceFactory(_nodeServiceFactory)
    {}
    virtual ~GroupManager() {}

    virtual void updateGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo);

    virtual NodeService::Ptr getNodeService(
        std::string const& _groupID, std::string const& _nodeName) const;

    std::string const& chainID() const { return m_chainID; }

    bcos::group::GroupInfo::Ptr getGroupInfo(std::string const& _groupID)
    {
        ReadGuard l(x_nodeServiceList);
        if (m_groupInfos.count(_groupID))
        {
            return m_groupInfos[_groupID];
        }
        return nullptr;
    }

    bcos::group::ChainNodeInfo::Ptr getNodeInfo(
        std::string const& _groupID, std::string const& _nodeName)
    {
        ReadGuard l(x_nodeServiceList);
        if (!m_groupInfos.count(_groupID))
        {
            return nullptr;
        }
        auto groupInfo = m_groupInfos[_groupID];
        return groupInfo->nodeInfo(_nodeName);
    }

    bcos::group::GroupManagerInterface::Ptr groupMgrClient() { return m_groupMgrClient; }
    bcos::group::GroupInfoFactory::Ptr groupInfoFactory() { return m_groupInfoFactory; };
    bcos::group::ChainNodeInfoFactory::Ptr chainNodeInfoFactory()
    {
        return m_chainNodeInfoFactory;
    };

protected:
    void updateNodeServiceWithoutLock(
        std::string const& _groupID, bcos::group::ChainNodeInfo::Ptr _nodeInfo);

private:
    std::string m_chainID;
    NodeServiceFactory::Ptr m_nodeServiceFactory;
    // TODO: set m_groupMgrClient
    bcos::group::GroupManagerInterface::Ptr m_groupMgrClient;
    bcos::group::GroupInfoFactory::Ptr m_groupInfoFactory;
    bcos::group::ChainNodeInfoFactory::Ptr m_chainNodeInfoFactory;

    // map between groupID to groupInfo
    std::map<std::string, bcos::group::GroupInfo::Ptr> m_groupInfos;

    // map between serviceName to NodeService
    std::map<std::string, NodeService::Ptr> m_nodeServiceList;
    mutable SharedMutex x_nodeServiceList;
};
}  // namespace rpc
}  // namespace bcos