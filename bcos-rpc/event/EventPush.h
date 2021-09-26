/*
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
 * @file EvenPush.h
 * @author: octopus
 * @date 2021-09-26
 */

#pragma once

#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-rpc/event/EventPushGroup.h>
#include <json/value.h>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace bcos
{
namespace event
{
class EventPush : public std::enable_shared_from_this<EventPush>
{
public:
    using Ptr = std::shared_ptr<EventPush>;

    virtual ~EventPush() { stop(); }

public:
    virtual void start();
    virtual void stop();

public:
    EventPushGroup::Ptr getGroup(const std::string& _group);
    bool addGroup(const std::string& _group, bcos::ledger::LedgerInterface::Ptr _ledgerInterface);
    bool removeGroup(const std::string& _group);

    bool notifyBlockNumber(const std::string& _group, bcos::protocol::BlockNumber _blockNumber);

public:
    virtual void onRecvSubscribeEvent(
        std::shared_ptr<ws::WsMessage> _msg, std::shared_ptr<ws::WsSession> _session);
    virtual void onRecvUnsubscribeEvent(
        std::shared_ptr<ws::WsMessage> _msg, std::shared_ptr<ws::WsSession> _session);

public:
    /**
     * @brief: send response
     * @param _session: the peer
     * @param _msg: the msg
     * @param _id: the eventpush id
     * @param _status: the response status
     * @return bool: if _session is inactive, false will be return
     */
    bool sendResponse(std::shared_ptr<ws::WsSession> _session, std::shared_ptr<ws::WsMessage> _msg,
        const std::string& _id, int32_t _status);

    /**
     * @brief: send event log list to client
     * @param _session: the peer
     * @param _id: the eventpush id
     * @param _result:
     * @return bool: if _session is inactive, false will be return
     */
    bool sendEvents(std::shared_ptr<ws::WsSession> _session, const std::string& _id,
        const Json::Value& _result);

public:
    std::shared_ptr<ws::WsMessageFactory> messageFactory() const { return m_messageFactory; }
    void setMessageFactory(std::shared_ptr<ws::WsMessageFactory> _messageFactory)
    {
        m_messageFactory = _messageFactory;
    }

private:
    std::atomic<bool> m_running{false};

    // lock for m_groups
    mutable std::shared_mutex x_groups;
    std::unordered_map<std::string, EventPushGroup::Ptr> m_groups;

    std::shared_ptr<ws::WsMessageFactory> m_messageFactory;
};

}  // namespace event
}  // namespace bcos