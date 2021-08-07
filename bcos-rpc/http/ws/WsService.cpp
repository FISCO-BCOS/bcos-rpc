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
 * @file WsService.cpp
 * @author: octopus
 * @date 2021-07-28
 */
#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/libutilities/Common.h>
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-rpc/amop/AMOP.h>
#include <bcos-rpc/amop/TopicManager.h>
#include <bcos-rpc/http/ws/Common.h>
#include <bcos-rpc/http/ws/WsMessageType.h>
#include <bcos-rpc/http/ws/WsService.h>
#include <bcos-rpc/http/ws/WsSession.h>
#include <boost/core/ignore_unused.hpp>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#define WS_SERVICE_DO_LOOP_PERIOD (10000)

using namespace bcos;
using namespace bcos::ws;

void WsService::start()
{
    if (m_running)
    {
        WEBSOCKET_SERVICE(INFO) << LOG_BADGE("start") << LOG_DESC("websocket service is running");
        return;
    }
    m_running = true;
    doLoop();
    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("start")
                            << LOG_DESC("start websocket service successfully");
}

void WsService::stop()
{
    if (!m_running)
    {
        WEBSOCKET_SERVICE(INFO) << LOG_BADGE("stop")
                                << LOG_DESC("websocket service has been stopped");
        return;
    }
    m_running = false;

    if (m_loopTimer)
    {
        m_loopTimer->cancel();
    }

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("stop") << LOG_DESC("stop websocket service successfully");
}

void WsService::doLoop()
{
    m_loopTimer = std::make_shared<boost::asio::deadline_timer>(boost::asio::make_strand(*m_ioc),
        boost::posix_time::milliseconds(WS_SERVICE_DO_LOOP_PERIOD));

    auto self = std::weak_ptr<WsService>(shared_from_this());
    m_loopTimer->async_wait([self](const boost::system::error_code&) {
        auto service = self.lock();
        if (!service)
        {
            return;
        }

        auto ss = service->sessions();
        for (auto const& session : ss)
        {
            boost::ignore_unused(session);
            // NOTE: server should send heartbeat message
        }
        WEBSOCKET_SERVICE(INFO) << LOG_BADGE("doLoop") << LOG_KV("connected sdk count", ss.size());
        service->doLoop();
    });
}

void WsService::initMethod()
{
    m_msgType2Method.clear();

    auto self = std::weak_ptr<WsService>(shared_from_this());
    m_msgType2Method[WsMessageType::HANDESHAKE] = [self](std::shared_ptr<WsMessage> _msg,
                                                      std::shared_ptr<WsSession> _session) {
        auto service = self.lock();
        if (service)
        {
            service->onRecvHandshake(_msg, _session);
        }
    };
    m_msgType2Method[WsMessageType::RPC_REQUEST] = [self](std::shared_ptr<WsMessage> _msg,
                                                       std::shared_ptr<WsSession> _session) {
        auto service = self.lock();
        if (service)
        {
            service->onRecvRPCRequest(_msg, _session);
        }
    };
    m_msgType2Method[WsMessageType::AMOP_SUBTOPIC] = [self](std::shared_ptr<WsMessage> _msg,
                                                         std::shared_ptr<WsSession> _session) {
        auto service = self.lock();
        if (service)
        {
            service->onRecvSubTopics(_msg, _session);
        }
    };
    m_msgType2Method[WsMessageType::AMOP_REQUEST] = [self](std::shared_ptr<WsMessage> _msg,
                                                        std::shared_ptr<WsSession> _session) {
        auto service = self.lock();
        if (service)
        {
            service->onRecvAMOPRequest(_msg, _session);
        }
    };
    m_msgType2Method[WsMessageType::AMOP_BROADCAST] = [self](std::shared_ptr<WsMessage> _msg,
                                                          std::shared_ptr<WsSession> _session) {
        auto service = self.lock();
        if (service)
        {
            service->onRecvAMOPBroadcast(_msg, _session);
        }
    };

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("initMethod")
                            << LOG_KV("methods", m_msgType2Method.size());
    for (const auto& method : m_msgType2Method)
    {
        WEBSOCKET_SERVICE(INFO) << LOG_BADGE("initMethod") << LOG_KV("type", method.first);
    }
}

void WsService::addSession(std::shared_ptr<WsSession> _session)
{
    {
        std::unique_lock lock(x_mutex);
        m_sessions[_session->remoteEndPoint()] = _session;
    }

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("addSession")
                            << LOG_KV("endpoint",
                                   _session ? _session->remoteEndPoint() : std::string(""));
}

void WsService::removeSession(const std::string& _endPoint)
{
    {
        std::unique_lock lock(x_mutex);
        m_sessions.erase(_endPoint);
    }

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("removeSession") << LOG_KV("endpoint", _endPoint);
}

std::shared_ptr<WsSession> WsService::getSession(const std::string& _endPoint)
{
    std::shared_lock lock(x_mutex);
    return m_sessions[_endPoint];
}

WsSessions WsService::sessions()
{
    WsSessions sessions;
    {
        std::shared_lock lock(x_mutex);
        for (const auto& session : m_sessions)
        {
            if (session.second->isConnected())
            {
                sessions.push_back(session.second);
            }
            else
            {
                WEBSOCKET_SERVICE(DEBUG)
                    << LOG_BADGE("sessions") << LOG_DESC("the session is disconnected")
                    << LOG_KV("endpoint", session.second->remoteEndPoint());
            }
        }
    }

    WEBSOCKET_SERVICE(TRACE) << LOG_BADGE("sessions") << LOG_KV("size", sessions.size());
    return sessions;
}

/**
 * @brief: websocket session disconnect
 * @param _msg: received message
 * @param _error:
 * @param _session: websocket session
 * @return void:
 */
void WsService::onDisconnect(Error::Ptr _error, std::shared_ptr<WsSession> _session)
{
    boost::ignore_unused(_error);
    std::string endpoint = "";
    if (_session)
    {
        endpoint = _session->remoteEndPoint();
    }

    // clear the session
    removeSession(endpoint);
    // clear the topics the sdk sub
    m_topicManager->removeTopicsByClient(endpoint);
    // Add additional disconnect logic

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("onDisconnect") << LOG_KV("endpoint", endpoint);
}

/**
 * @brief: recv message from sdk
 * @param _error: error
 * @param _msg: receive message
 * @param _session: websocket session
 * @return void:
 */
void WsService::onRecvClientMessage(
    Error::Ptr _error, std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvClientMessage")
                                 << LOG_KV("endpoint",
                                        _session ? _session->remoteEndPoint() : std::string(""))
                                 << LOG_KV("errorCode", _error->errorCode())
                                 << LOG_KV("errorMessage", _error->errorMessage());
        _session->drop();
        return;
    }

    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());

    WEBSOCKET_SERVICE(TRACE) << LOG_BADGE("onRecvClientMessage") << LOG_KV("type", _msg->type())
                             << LOG_KV("seq", seq) << LOG_KV("endpoint", _session->remoteEndPoint())
                             << LOG_KV("data size", _msg->data()->size());

    auto it = m_msgType2Method.find(_msg->type());
    if (it != m_msgType2Method.end())
    {
        auto callback = it->second;
        callback(_msg, _session);
    }
    else
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvClientMessage")
                                 << LOG_DESC("unrecognized message type")
                                 << LOG_KV("type", _msg->type())
                                 << LOG_KV("endpoint", _session->remoteEndPoint())
                                 << LOG_KV("seq", seq) << LOG_KV("data size", _msg->data()->size());
    }
}

/**
 * @brief: receive ws handshake message from sdk
 */
void WsService::onRecvHandshake(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    // getNodeInfo
    m_jsonRpcInterface->getNodeInfo([_msg, _session, this](
                                        bcos::Error::Ptr _error, Json::Value& _jNodeInfo) {
        boost::ignore_unused(_error);
        // getBlockNumber
        m_jsonRpcInterface->getBlockNumber([_msg, _session, _jNodeInfo](bcos::Error::Ptr _error,
                                               Json::Value& _jResp) mutable {
            if (!_error || _error->errorCode() == bcos::protocol::CommonError::SUCCESS)
            {
                _jNodeInfo["blockNumber"] = _jResp.asInt64();
            }
            else
            {
                // NOTE: give default 0 value or not give blockNumber field
                _jNodeInfo["blockNumber"] = 0;
                WEBSOCKET_SERVICE(ERROR)
                    << LOG_BADGE("onRecvHandshake") << LOG_DESC("get block number failed")
                    << LOG_KV("errorCode", _error ? _error->errorCode() : -1)
                    << LOG_KV("errorMessage", _error ? _error->errorMessage() : std::string(""));
            }

            Json::FastWriter writer;
            std::string resp = writer.write(_jNodeInfo);
            _msg->setData(std::make_shared<bcos::bytes>(resp.begin(), resp.end()));
            _session->asyncSendMessage(_msg);
        });
    });
}

/**
 * @brief: receive ws rpc request message from sdk
 */
void WsService::onRecvRPCRequest(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto request = std::string(_msg->data()->begin(), _msg->data()->end());
    auto weak = std::weak_ptr<WsSession>(_session);
    m_jsonRpcInterface->onRPCRequest(request, [_msg, request, weak](const std::string& _resp) {
        auto session = weak.lock();
        if (!session)
        {
            return;
        }

        _msg->setData(std::make_shared<bcos::bytes>(_resp.begin(), _resp.end()));
        session->asyncSendMessage(_msg);

        WEBSOCKET_SERVICE(DEBUG) << LOG_BADGE("onRecvRPCRequest") << LOG_KV("request", request)
                                 << LOG_KV("response", _resp);
    });
}

/**
 * @brief: receive sub topic message from sdk
 */
void WsService::onRecvSubTopics(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto request = std::string(_msg->data()->begin(), _msg->data()->end());
    auto endpoint = _session->remoteEndPoint();
    m_topicManager->subTopic(endpoint, request);

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("onRecvSubTopics") << LOG_KV("request", request)
                            << LOG_KV("endpoint", endpoint);
}

/**
 * @brief: receive amop request message from sdk
 */
void WsService::onRecvAMOPRequest(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto factory = std::make_shared<AMOPRequestFactory>();
    auto request = factory->buildRequest();
    auto size = request->decode(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPRequest")
                                 << LOG_DESC("decode amop request message failed")
                                 << LOG_KV("endpoint",
                                        _session ? _session->remoteEndPoint() : std::string(""))
                                 << LOG_KV("data",
                                        *toHexString(_msg->data()->begin(), _msg->data()->end()));
        return;
    }
    auto buffer = std::make_shared<bcos::bytes>();
    _msg->encode(*buffer);
    auto topic = request->topic();
    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());

    WEBSOCKET_SERVICE(DEBUG) << LOG_BADGE("onRecvAMOPRequest") << LOG_KV("seq", _msg->seq())
                             << LOG_KV("topic", topic);
    auto AMOP = m_AMOP.lock();
    if (!AMOP)
    {
        return;
    }

    AMOP->asyncSendMessage(topic, bcos::bytesConstRef(buffer->data(), buffer->size()),
        [_msg, _session, topic, seq](bcos::Error::Ptr _error, bcos::bytesConstRef _data) {
            if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
            {
                _msg->setStauts(_error->errorCode());
                _msg->data()->clear();

                WEBSOCKET_SERVICE(ERROR)
                    << LOG_BADGE("onRecvAMOPRequest")
                    << LOG_DESC("AMOP async send message callback") << LOG_KV("seq", seq)
                    << LOG_KV("topic", topic) << LOG_KV("errorCode", _error->errorCode())
                    << LOG_KV("errorMessage", _error->errorMessage());
            }
            else
            {
                _msg->setData(std::make_shared<bcos::bytes>(_data.begin(), _data.end()));

                WEBSOCKET_SERVICE(INFO)
                    << LOG_BADGE("onRecvAMOPRequest")
                    << LOG_DESC("AMOP async send message callback") << LOG_KV("seq", seq)
                    << LOG_KV("topic", topic) << LOG_KV("data size", _data.size());
            }

            // send response back to sdk
            _session->asyncSendMessage(_msg);
        });
}

/**
 * @brief: receive amop broadcast message from sdk
 */
void WsService::onRecvAMOPBroadcast(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    boost::ignore_unused(_session);

    auto request = m_requestFactory->buildRequest();
    auto size = request->decode(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPBroadcast")
                                 << LOG_DESC("decode amop broadcast message failed")
                                 << LOG_KV("endpoint",
                                        _session ? _session->remoteEndPoint() : std::string(""))
                                 << LOG_KV("data",
                                        *toHexString(_msg->data()->begin(), _msg->data()->end()));
        return;
    }

    auto topic = request->topic();
    WEBSOCKET_SERVICE(DEBUG) << LOG_BADGE("onRecvAMOPBroadcast") << LOG_KV("seq", _msg->seq())
                             << LOG_KV("topic", topic);

    auto buffer = std::make_shared<bcos::bytes>();
    _msg->encode(*buffer);

    auto AMOP = m_AMOP.lock();
    if (AMOP)
    {
        AMOP->asyncSendBroadbastMessage(topic, bcos::bytesConstRef(buffer->data(), buffer->size()));
    }
}

void WsService::onRecvAMOPMessage(bytesConstRef _data, const std::string& nodeID,
    std::function<void(bytesConstRef _data)> _callback)
{
    // WsMessage
    auto message = m_messageFactory->buildMessage();
    auto size = message->decode(_data.data(), _data.size());
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPMessage")
                                 << LOG_DESC("decode message failed") << LOG_KV("nodeID", nodeID)
                                 << LOG_KV("data", *toHexString(_data));
        return;
    }

    // AMOPRequest
    auto request = m_requestFactory->buildRequest();
    size = request->decode(bytesConstRef(message->data()->data(), message->data()->size()));
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPMessage")
                                 << LOG_DESC("decode amop message failed")
                                 << LOG_KV("nodeID", nodeID) << LOG_KV("data", *toHexString(_data));
        return;
    }

    // message seq
    std::string topic = request->topic();
    std::string seq = std::string(message->seq()->begin(), message->seq()->end());

    std::vector<std::string> clients;
    m_topicManager->queryClientsByTopic(topic, clients);
    if (clients.empty())
    {
        auto buffer = std::make_shared<bcos::bytes>();
        // TODO: set the error code
        message->setStauts(bcos::protocol::CommonError::TIMEOUT);
        message->data()->clear();
        message->setType(WsMessageType::AMOP_RESPONSE);
        message->encode(*buffer);

        _callback(bytesConstRef(buffer->data(), buffer->size()));

        WEBSOCKET_SERVICE(WARNING)
            << LOG_BADGE("onRecvAMOPMessage") << LOG_DESC("no client subscribe the topic")
            << LOG_KV("topic", topic) << LOG_KV("nodeID", nodeID);
        return;
    }

    class Retry : public std::enable_shared_from_this<Retry>
    {
    public:
        std::string m_topic;
        std::string m_nodeID;
        std::shared_ptr<WsMessage> m_message;
        std::vector<std::string> m_clients;
        std::shared_ptr<WsService> m_wsService;
        std::function<void(bytesConstRef _data)> m_callback;

    public:
        WsSession::Ptr chooseSession()
        {
            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine e(seed);
            std::shuffle(m_clients.begin(), m_clients.end(), e);

            while (!m_clients.empty())
            {
                auto client = *m_clients.begin();
                m_clients.erase(m_clients.begin());

                auto session = m_wsService->getSession(client);
                if (!session->isConnected())
                {
                    continue;
                }

                return session;
            }

            return nullptr;
        }

        void sendMessage()
        {
            auto session = chooseSession();
            if (!session)
            {
                WEBSOCKET_SERVICE(WARNING)
                    << LOG_BADGE("onRecvAMOPMessage")
                    << LOG_DESC("send message to client by topic failed")
                    << LOG_KV("topic", m_topic) << LOG_KV("nodeID", m_nodeID);

                auto buffer = std::make_shared<bcos::bytes>();
                m_message->setStauts(bcos::protocol::CommonError::TIMEOUT);
                m_message->data()->clear();
                m_message->setType(WsMessageType::AMOP_RESPONSE);
                m_message->encode(*buffer);

                m_callback(bytesConstRef(buffer->data(), buffer->size()));
                return;
            }

            auto self = std::weak_ptr<Retry>(shared_from_this());
            // Note: how to set the timeout
            session->asyncSendMessage(m_message, Options(30000),
                [self](bcos::Error::Ptr _error, std::shared_ptr<WsMessage> _msg,
                    std::shared_ptr<WsSession> _session) {
                    auto retry = self.lock();
                    if (!retry)
                    {
                        return;
                    }

                    // try again when send message to the session failed
                    if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                    {
                        WEBSOCKET_SERVICE(WARNING)
                            << LOG_BADGE("onRecvAMOPMessage")
                            << LOG_DESC("asyncSendMessage callback error and try to send again")
                            << LOG_KV("endpoint",
                                   (_session ? _session->remoteEndPoint() : std::string("")))
                            << LOG_KV("topic", retry->m_topic) << LOG_KV("nodeID", retry->m_nodeID)
                            << LOG_KV("errorCode", _error ? _error->errorCode() : -1)
                            << LOG_KV("errorMessage", _error ? _error->errorMessage() : "success");
                        return retry->sendMessage();
                    }

                    auto buffer = std::make_shared<bcos::bytes>();
                    _msg->encode(*buffer);
                    retry->m_callback(bytesConstRef(buffer->data(), buffer->size()));
                });
        }
    };

    auto retry = std::make_shared<Retry>();
    retry->m_topic = topic;
    retry->m_nodeID = nodeID;
    retry->m_clients = clients;
    retry->m_message = message;
    retry->m_wsService = shared_from_this();
    retry->m_callback = _callback;
    retry->sendMessage();
}

void WsService::onRecvAMOPBroadcastMessage(bytesConstRef _data)
{
    // WsMessage
    auto message = m_messageFactory->buildMessage();
    auto size = message->decode(_data.data(), _data.size());
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                                 << LOG_DESC("decode message failed")
                                 << LOG_KV("data", *toHexString(_data));
        return;
    }

    // AMOPRequest
    auto request = m_requestFactory->buildRequest();
    size = request->decode(bytesConstRef(message->data()->data(), message->data()->size()));
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                                 << LOG_DESC("decode amop message failed")
                                 << LOG_KV("data", *toHexString(_data));
        return;
    }

    // message seq
    std::string topic = request->topic();
    std::string seq = std::string(message->seq()->begin(), message->seq()->end());

    std::vector<std::string> clients;
    m_topicManager->queryClientsByTopic(topic, clients);
    if (clients.empty())
    {
        WEBSOCKET_SERVICE(WARNING)
            << LOG_BADGE("onRecvAMOPBroadcastMessage") << LOG_DESC("no client subscribe the topic")
            << LOG_KV("seq", seq) << LOG_KV("topic", topic);
        return;
    }

    for (const auto& client : clients)
    {
        auto session = getSession(client);
        if (!session || !session->isConnected())
        {
            continue;
        }

        WEBSOCKET_SERVICE(TRACE) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                                 << LOG_DESC("push message to client") << LOG_KV("seq", seq)
                                 << LOG_KV("topic", topic)
                                 << LOG_KV("endpoint", session->remoteEndPoint());

        session->asyncSendMessage(message);
    }
}

/**
 * @brief: push blocknumber to _session
 * @param _session:
 * @param _blockNumber:
 * @return void:
 */
void WsService::notifyBlockNumberToClient(
    std::shared_ptr<WsSession> _session, bcos::protocol::BlockNumber _blockNumber)
{
    std::string resp = "{\"blockNumber\": " + std::to_string(_blockNumber) + "}";
    auto message = m_messageFactory->buildMessage(
        WsMessageType::BLOCK_NOTIFY, std::make_shared<bcos::bytes>(resp.begin(), resp.end()));
    _session->asyncSendMessage(message);

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("notifyBlockNumberToClient")
                            << LOG_KV("endpoint", _session->remoteEndPoint())
                            << LOG_KV("blockNumber", _blockNumber);
}

/**
 * @brief: push blocknumber to all active sessions
 * @param _blockNumber:
 * @return void:
 */
void WsService::notifyBlockNumberToClient(bcos::protocol::BlockNumber _blockNumber)
{
    auto allSessions = sessions();
    for (const auto& session : allSessions)
    {
        if (session->isConnected())
        {
            notifyBlockNumberToClient(session, _blockNumber);
        }
    }

    WEBSOCKET_SERVICE(INFO) << LOG_BADGE("notifyBlockNumberToClient")
                            << LOG_KV("blockNumber", _blockNumber)
                            << LOG_KV("sessions size", allSessions.size());
}