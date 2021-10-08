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
 * @file EventSub.cpp
 * @author: octopus
 * @date 2021-09-26
 */
#include <bcos-boostssl/websocket/WsSession.h>
#include <bcos-framework/libutilities/Common.h>
#include <bcos-rpc/event/Common.h>
#include <bcos-rpc/event/EventSub.h>
#include <bcos-rpc/event/EventSubMatcher.h>
#include <bcos-rpc/event/EventSubRequest.h>
#include <bcos-rpc/event/EventSubResponse.h>
#include <memory>
#include <shared_mutex>

using namespace bcos;
using namespace bcos::event;

void EventSub::start()
{
    if (m_running.load())
    {
        EVENT_PUSH(INFO) << LOG_BADGE("start") << LOG_DESC("amop is running");
        return;
    }

    m_running.store(true);

    EVENT_PUSH(INFO) << LOG_BADGE("start") << LOG_DESC("start event sub successfully");
}

void EventSub::stop()
{
    if (!m_running.load())
    {
        EVENT_PUSH(INFO) << LOG_BADGE("stop") << LOG_DESC("event sub is not running");
        return;
    }

    for (auto esp : m_groups)
    {
        esp.second->stop();
    }

    EVENT_PUSH(INFO) << LOG_BADGE("stop") << LOG_DESC("stop event sub successfully");
}

bool EventSub::addGroup(
    const std::string& _group, bcos::ledger::LedgerInterface::Ptr _ledgerInterface)
{
    std::unique_lock lock(x_groups);
    auto it = m_groups.find(_group);
    if (it != m_groups.end())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("addGroup") << LOG_DESC("event sub group has been exist")
                            << LOG_KV("group", _group);
        return false;
    }

    auto matcher = std::make_shared<EventSubMatcher>();
    auto esGroup = std::make_shared<EventSubGroup>(_group);

    esGroup->setGroup(_group);
    esGroup->setLedger(_ledgerInterface);
    esGroup->setMatcher(matcher);
    esGroup->start();

    m_groups[_group] = esGroup;

    EVENT_PUSH(INFO) << LOG_BADGE("addGroup") << LOG_DESC("add event sub group successfully")
                     << LOG_KV("group", _group);
    return true;
}

bool EventSub::removeGroup(const std::string& _group)
{
    std::unique_lock lock(x_groups);
    auto it = m_groups.find(_group);
    if (it == m_groups.end())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("removeGroup") << LOG_DESC("event sub group is not exist")
                            << LOG_KV("group", _group);
        return false;
    }

    auto esGroup = it->second;
    m_groups.erase(it);
    esGroup->stop();

    EVENT_PUSH(INFO) << LOG_BADGE("removeGroup") << LOG_DESC("remove event sub group successfully")
                     << LOG_KV("group", _group);
    return true;
}

EventSubGroup::Ptr EventSub::getGroup(const std::string& _group)
{
    std::shared_lock lock(x_groups);
    auto it = m_groups.find(_group);
    if (it == m_groups.end())
    {
        return nullptr;
    }

    return it->second;
}

// register this function to blockNumber notify
bool EventSub::notifyBlockNumber(
    const std::string& _group, bcos::protocol::BlockNumber _blockNumber)
{
    auto esGroup = getGroup(_group);
    if (esGroup)
    {
        esGroup->setLatestBlockNumber(_blockNumber);
        EVENT_PUSH(DEBUG) << LOG_BADGE("notifyBlockNumber") << LOG_KV("group", _group)
                          << LOG_KV("blockNumber", _blockNumber);
        return true;
    }
    else
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("notifyBlockNumber") << LOG_DESC("group is not exist")
                            << LOG_KV("group", _group) << LOG_KV("blockNumber", _blockNumber);
        return false;
    }
}

void EventSub::onRecvSubscribeEvent(std::shared_ptr<bcos::boostssl::ws::WsMessage> _msg,
    std::shared_ptr<bcos::boostssl::ws::WsSession> _session)
{
    std::string request = std::string(_msg->data()->begin(), _msg->data()->end());

    EVENT_PUSH(TRACE) << LOG_BADGE("onRecvSubscribeEvent") << LOG_KV("request", request)
                      << LOG_KV("endpoint", _session->endPoint());

    auto esRes = std::make_shared<EventSubSubRequest>();
    if (!esRes->fromJson(request))
    {
        sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::INVALID_PARAMS);
        return;
    }

    auto esGroup = getGroup(esRes->group());
    if (!esGroup)
    {
        sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::GROUP_NOT_EXIST);
        return;
    }

    auto task = std::make_shared<EventSubTask>();
    task->setGroup(esRes->group());
    task->setId(esRes->id());
    task->setParams(esRes->params());

    // TODO: check request parameters
    auto self = std::weak_ptr<EventSub>(shared_from_this());
    task->setCallback([self, _session](const std::string& _id, const Json::Value& _result) -> bool {
        auto es = self.lock();
        if (!es)
        {
            return false;
        }

        return es->sendEvents(_session, _id, _result);
    });

    esGroup->subEventSubTask(task);
    sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::SUCCESS);
    return;
}

void EventSub::onRecvUnsubscribeEvent(std::shared_ptr<bcos::boostssl::ws::WsMessage> _msg,
    std::shared_ptr<bcos::boostssl::ws::WsSession> _session)
{
    std::string request = std::string(_msg->data()->begin(), _msg->data()->end());
    EVENT_PUSH(TRACE) << LOG_BADGE("onRecvUnsubscribeEvent") << LOG_KV("request", request)
                      << LOG_KV("endpoint", _session->endPoint());


    auto esRes = std::make_shared<EventSubUnsubRequest>();
    if (!esRes->fromJson(request))
    {
        sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::INVALID_PARAMS);
        return;
    }

    auto esGroup = getGroup(esRes->group());
    if (!esGroup)
    {
        sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::GROUP_NOT_EXIST);
        return;
    }

    esGroup->unsubEventSubTask(esRes->id());
    sendResponse(_session, _msg, esRes->id(), EP_STATUS_CODE::SUCCESS);
    return;
}

/**
 * @brief: send response
 * @param _session: the peer session
 * @param _msg: the msg
 * @param _status: the response status
 * @return bool: if _session is inactive, false will be return
 */
bool EventSub::sendResponse(std::shared_ptr<bcos::boostssl::ws::WsSession> _session,
    std::shared_ptr<bcos::boostssl::ws::WsMessage> _msg, const std::string& _id, int32_t _status)
{
    if (!_session->isConnected())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("sendResponse") << LOG_DESC("session has been inactive")
                            << LOG_KV("id", _id) << LOG_KV("status", _status)
                            << LOG_KV("endpoint", _session->endPoint());
        return false;
    }

    auto esResp = std::make_shared<EventSubResponse>();
    esResp->setId(_id);
    esResp->setStatus(_status);
    auto result = esResp->generateJson();

    auto data = std::make_shared<bcos::bytes>(result.begin(), result.end());
    _msg->setData(data);

    _session->asyncSendMessage(_msg);
    return true;
}

/**
 * @brief: send event log list to client
 * @param _session: the peer
 * @param _id: the EventSub id
 * @param _result:
 * @return bool: if _session is inactive, false will be return
 */
bool EventSub::sendEvents(std::shared_ptr<bcos::boostssl::ws::WsSession> _session,
    const std::string& _id, const Json::Value& _result)
{
    if (!_session->isConnected())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("sendEvents") << LOG_DESC("session has been inactive")
                            << LOG_KV("id", _id) << LOG_KV("endpoint", _session->endPoint());
        return false;
    }

    if (0 == _result.size())
    {
        return true;
    }

    auto esResp = std::make_shared<EventSubResponse>();
    esResp->setId(_id);
    esResp->setStatus(EP_STATUS_CODE::SUCCESS);
    esResp->generateJson();

    auto jResp = esResp->jResp();
    jResp["result"] = _result;

    Json::FastWriter writer;
    std::string s = writer.write(jResp);
    auto data = std::make_shared<bcos::bytes>(s.begin(), s.end());

    auto msg = m_messageFactory->buildMessage();
    msg->setType(bcos::event::MessageType::EVENT_LOG_PUSH);
    msg->setData(data);
    _session->asyncSendMessage(msg);

    EVENT_PUSH(TRACE) << LOG_BADGE("sendEvents") << LOG_DESC("send events to client")
                      << LOG_KV("endpoint", _session->endPoint()) << LOG_KV("id", _id)
                      << LOG_KV("events", s);

    return true;
}