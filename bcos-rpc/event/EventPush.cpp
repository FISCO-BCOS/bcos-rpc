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
 * @file EventPush.cpp
 * @author: octopus
 * @date 2021-09-26
 */
#include <bcos-framework/libutilities/Common.h>
#include <bcos-rpc/event/Common.h>
#include <bcos-rpc/event/EventPush.h>
#include <bcos-rpc/event/EventPushMatcher.h>
#include <bcos-rpc/event/EventPushRequest.h>
#include <bcos-rpc/event/EventPushResponse.h>
#include <bcos-rpc/http/ws/WsMessageType.h>
#include <memory>
#include <shared_mutex>

using namespace bcos;
using namespace bcos::event;

void EventPush::start()
{
    if (m_running.load())
    {
        EVENT_PUSH(INFO) << LOG_BADGE("start") << LOG_DESC("amop is running");
        return;
    }

    m_running.store(true);

    EVENT_PUSH(INFO) << LOG_BADGE("start") << LOG_DESC("start event push successfully");
}

void EventPush::stop()
{
    if (!m_running.load())
    {
        EVENT_PUSH(INFO) << LOG_BADGE("stop") << LOG_DESC("event push is not running");
        return;
    }


    EVENT_PUSH(INFO) << LOG_BADGE("stop") << LOG_DESC("stop event push successfully");
}

bool EventPush::addGroup(
    const std::string& _group, bcos::ledger::LedgerInterface::Ptr _ledgerInterface)
{
    std::unique_lock lock(x_groups);
    auto it = m_groups.find(_group);
    if (it != m_groups.end())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("addGroup") << LOG_DESC("event push group has been exist")
                            << LOG_KV("group", _group);
        return false;
    }

    auto matcher = std::make_shared<EventPushMatcher>();
    auto epGroup = std::make_shared<EventPushGroup>(_group);

    epGroup->setGroup(_group);
    epGroup->setLedger(_ledgerInterface);
    epGroup->setMatcher(matcher);
    epGroup->start();

    m_groups[_group] = epGroup;

    EVENT_PUSH(INFO) << LOG_BADGE("addGroup") << LOG_DESC("add event push group successfully")
                     << LOG_KV("group", _group);
    return true;
}

bool EventPush::removeGroup(const std::string& _group)
{
    std::unique_lock lock(x_groups);
    auto it = m_groups.find(_group);
    if (it == m_groups.end())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("removeGroup") << LOG_DESC("event push group is not exist")
                            << LOG_KV("group", _group);
        return false;
    }

    auto epGroup = it->second;
    m_groups.erase(it);
    epGroup->stop();

    EVENT_PUSH(INFO) << LOG_BADGE("removeGroup") << LOG_DESC("remove event push group successfully")
                     << LOG_KV("group", _group);
    return true;
}

EventPushGroup::Ptr EventPush::getGroup(const std::string& _group)
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
bool EventPush::notifyBlockNumber(
    const std::string& _group, bcos::protocol::BlockNumber _blockNumber)
{
    auto epGroup = getGroup(_group);
    if (epGroup)
    {
        epGroup->setLatestBlockNumber(_blockNumber);
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

void EventPush::onRecvSubscribeEvent(
    std::shared_ptr<ws::WsMessage> _msg, std::shared_ptr<ws::WsSession> _session)
{
    std::string request = std::string(_msg->data()->begin(), _msg->data()->end());

    EVENT_PUSH(TRACE) << LOG_BADGE("onRecvSubscribeEvent") << LOG_KV("request", request)
                      << LOG_KV("endpoint", _session->endPoint());

    auto epReq = std::make_shared<EventPushSubRequest>();
    if (!epReq->fromJson(request))
    {
        sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::INVALID_PARAMS);
        return;
    }

    auto epGroup = getGroup(epReq->group());
    if (!epGroup)
    {
        sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::GROUP_NOT_EXIST);
        return;
    }
    auto task = std::make_shared<EventPushTask>();
    task->setGroup(epReq->group());
    task->setId(epReq->id());
    task->setParams(epReq->params());

    // TODO: check request parameters
    auto self = std::weak_ptr<EventPush>(shared_from_this());
    task->setCallback([self, _session](const std::string& _id, const Json::Value& _result) -> bool {
        auto ep = self.lock();
        if (!ep)
        {
            return false;
        }

        return ep->sendEvents(_session, _id, _result);
    });

    epGroup->subEventPushTask(task);
    sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::SUCCESS);
    return;
}

void EventPush::onRecvUnsubscribeEvent(
    std::shared_ptr<ws::WsMessage> _msg, std::shared_ptr<ws::WsSession> _session)
{
    std::string request = std::string(_msg->data()->begin(), _msg->data()->end());
    EVENT_PUSH(TRACE) << LOG_BADGE("onRecvUnsubscribeEvent") << LOG_KV("request", request)
                      << LOG_KV("endpoint", _session->endPoint());


    auto epReq = std::make_shared<EventPushUnsubRequest>();
    if (!epReq->fromJson(request))
    {
        sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::INVALID_PARAMS);
        return;
    }

    auto epGroup = getGroup(epReq->group());
    if (!epGroup)
    {
        sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::GROUP_NOT_EXIST);
        return;
    }

    epGroup->unsubEventPushTask(epReq->id());
    sendResponse(_session, _msg, epReq->id(), EP_STATUS_CODE::SUCCESS);
    return;
}

/**
 * @brief: send response
 * @param _session: the peer session
 * @param _msg: the msg
 * @param _status: the response status
 * @return bool: if _session is inactive, false will be return
 */
bool EventPush::sendResponse(std::shared_ptr<ws::WsSession> _session,
    std::shared_ptr<ws::WsMessage> _msg, const std::string& _id, int32_t _status)
{
    if (!_session->isConnected())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("sendResponse") << LOG_DESC("session has been inactive")
                            << LOG_KV("id", _id) << LOG_KV("status", _status)
                            << LOG_KV("endpoint", _session->endPoint());
        return false;
    }

    auto epResp = std::make_shared<EventPushResponse>();
    epResp->setId(_id);
    epResp->setStatus(_status);
    auto result = epResp->generateJson();

    auto data = std::make_shared<bcos::bytes>(result.begin(), result.end());
    _msg->setData(data);

    _session->asyncSendMessage(_msg);
    return true;
}

/**
 * @brief: send event log list to client
 * @param _session: the peer
 * @param _id: the eventpush id
 * @param _result:
 * @return bool: if _session is inactive, false will be return
 */
bool EventPush::sendEvents(
    std::shared_ptr<ws::WsSession> _session, const std::string& _id, const Json::Value& _result)
{
    if (!_session->isConnected())
    {
        EVENT_PUSH(WARNING) << LOG_BADGE("sendEvents") << LOG_DESC("session has been inactive")
                            << LOG_KV("id", _id) << LOG_KV("endpoint", _session->endPoint());
        return false;
    }

    if (0 != _result.size())
    {  // if result is not null, send the events to client
        auto epResp = std::make_shared<EventPushResponse>();
        epResp->setId(_id);
        epResp->setStatus(EP_STATUS_CODE::SUCCESS);
        epResp->generateJson();

        auto jResp = epResp->jResp();
        jResp["result"] = _result;

        Json::FastWriter writer;
        std::string s = writer.write(jResp);
        auto data = std::make_shared<bcos::bytes>(s.begin(), s.end());

        auto msg = m_messageFactory->buildMessage();
        msg->setType(ws::WsMessageType::EVENT_LOG_PUSH);
        msg->setData(data);
        _session->asyncSendMessage(msg);

        EVENT_PUSH(TRACE) << LOG_BADGE("sendEvents") << LOG_DESC("send events to client")
                          << LOG_KV("endpoint", _session->endPoint()) << LOG_KV("id", _id)
                          << LOG_KV("events", s);
    }

    return true;
}