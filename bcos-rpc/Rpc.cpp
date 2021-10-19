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
 * @file Rpc.cpp
 * @author: octopus
 * @date 2021-07-15
 */

#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-rpc/Rpc.h>
#include <future>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::group;

void Rpc::start()
{
    // start amop
    // TODO: move AMOP to the gateway
    // m_AMOP->start();
    // start event sub
    // m_eventSub->start();
    // start websocket service
    m_wsService->start();
    BCOS_LOG(INFO) << LOG_DESC("[RPC][RPC][start]") << LOG_DESC("start rpc successfully");
}

void Rpc::stop()
{
    // stop ws service
    if (m_wsService)
    {
        m_wsService->stop();
    }

    // stop event sub
    if (m_eventSub)
    {
        m_eventSub->stop();
    }

    // stop amop
    if (m_AMOP)
    {
        m_AMOP->stop();
    }

    BCOS_LOG(INFO) << LOG_DESC("[RPC][RPC][stop]") << LOG_DESC("stop rpc successfully");
}

void Rpc::init()
{
    BCOS_LOG(INFO) << LOG_DESC("init Rpc");
    auto initRet = std::make_shared<std::promise<Error::Ptr>>();
    auto future = initRet->get_future();
    auto chainID = m_jsonRpcImpl->groupManager()->chainID();
    auto groupMgrClient = m_jsonRpcImpl->groupManager()->groupMgrClient();
    groupMgrClient->asyncGetGroupList(chainID, [this, chainID, initRet, groupMgrClient](
                                                   Error::Ptr&& _error,
                                                   std::set<std::string>&& _groupList) {
        if (_error)
        {
            BCOS_LOG(ERROR) << LOG_DESC("Rpc init failed for getGroupList from GroupManager failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
            initRet->set_value(std::move(_error));
            return;
        }
        // get and update all groupInfos
        BCOS_LOG(INFO) << LOG_DESC("Rpc init: fetch groupList success, fetch group information now")
                       << LOG_KV("groupSize", _groupList.size());
        std::vector<std::string> groupList(_groupList.begin(), _groupList.end());
        groupMgrClient->asyncGetGroupInfos(chainID, groupList,
            [this, initRet](Error::Ptr&& _error, std::vector<GroupInfo::Ptr>&& _groupInfos) {
                if (_error)
                {
                    BCOS_LOG(ERROR) << LOG_DESC("Rpc init failed for get group informations failed")
                                    << LOG_KV("code", _error->errorCode())
                                    << LOG_KV("msg", _error->errorMessage());
                    initRet->set_value(std::move(_error));
                    return;
                }
                BCOS_LOG(INFO) << LOG_DESC("Rpc init: fetch group information succcess");
                for (auto const& groupInfo : _groupInfos)
                {
                    m_jsonRpcImpl->groupManager()->updateGroupInfo(groupInfo);
                }
                initRet->set_value(nullptr);
            });
    });
    auto error = future.get();
    if (error)
    {
        BCOS_LOG(WARNING) << LOG_DESC("Rpc init error for get group informations failed")
                          << LOG_KV("code", error->errorCode())
                          << LOG_KV("msg", error->errorMessage());
    }
    BCOS_LOG(INFO) << LOG_DESC("Rpc init success");
}

/**
 * @brief: notify blockNumber to rpc
 * @param _blockNumber: blockNumber
 * @param _callback: resp callback
 * @return void
 */
void Rpc::asyncNotifyBlockNumber(std::string const& _groupID, std::string const& _nodeName,
    bcos::protocol::BlockNumber _blockNumber, std::function<void(Error::Ptr)> _callback)
{
    auto ss = m_wsService->sessions();
    for (const auto& s : ss)
    {
        if (s && s->isConnected())
        {
            std::string group;
            // eg: {"blockNumber": 11, "group": "group"}
            Json::Value response;
            response["group"] = _groupID;
            response["nodeName"] = _nodeName;
            response["blockNumber"] = _blockNumber;
            auto resp = response.toStyledString();
            auto message =
                m_wsService->messageFactory()->buildMessage(bcos::rpc::MessageType::BLOCK_NOTIFY,
                    std::make_shared<bcos::bytes>(resp.begin(), resp.end()));
            s->asyncSendMessage(message);
        }
    }

    if (_callback)
    {
        _callback(nullptr);
    }

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("asyncNotifyBlockNumber")
                            << LOG_KV("blockNumber", _blockNumber) << LOG_KV("ss size", ss.size());
}

/**
 * @brief: async receive message from front service
 * @param _nodeID: the message sender nodeID
 * @param _id: the id of this message, it can by used to send response to the peer
 * @param _data: the message data
 * @return void
 */
void Rpc::asyncNotifyAmopMessage(bcos::crypto::NodeIDPtr _nodeID, const std::string& _id,
    bcos::bytesConstRef _data, std::function<void(Error::Ptr _error)> _onRecv)
{
    m_AMOP->asyncNotifyAmopMessage(_nodeID, _id, _data, _onRecv);
}

/**
 * @brief: async receive nodeIDs from front service
 * @param _nodeIDs: the nodeIDs
 * @param _callback: callback
 * @return void
 */
void Rpc::asyncNotifyAmopNodeIDs(std::shared_ptr<const bcos::crypto::NodeIDs> _nodeIDs,
    std::function<void(bcos::Error::Ptr _error)> _callback)
{
    m_AMOP->asyncNotifyAmopNodeIDs(_nodeIDs, _callback);
}

void Rpc::asyncNotifyGroupInfo(
    bcos::group::GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    m_jsonRpcImpl->groupManager()->updateGroupInfo(_groupInfo);
    BCOS_LOG(INFO) << LOG_DESC("asyncNotifyGroupInfo: update the groupInfo")
                   << printGroupInfo(_groupInfo);
    if (_callback)
    {
        _callback(nullptr);
    }
    notifyGroupInfo(_groupInfo);
}

void Rpc::notifyGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo)
{
    // notify the groupInfo to SDK
    auto sdkSessions = m_wsService->sessions();
    for (auto const& session : sdkSessions)
    {
        if (!session || !session->isConnected())
        {
            continue;
        }
        Json::Value groupInfoJson;
        groupInfoToJson(groupInfoJson, _groupInfo);
        auto response = groupInfoJson.toStyledString();
        auto message =
            m_wsService->messageFactory()->buildMessage(bcos::rpc::MessageType::GROUP_NOTIFY,
                std::make_shared<bcos::bytes>(response.begin(), response.end()));
        session->asyncSendMessage(message);
    }
}