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
 * @file AMOP.cpp
 * @author: octopus
 * @date 2021-06-21
 */

#include <bcos-boostssl/websocket/WsMessage.h>
#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-framework/interfaces/front/FrontServiceInterface.h>
#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/interfaces/protocol/Protocol.h>
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Exceptions.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-rpc/amop/AMOP.h>
#include <bcos-rpc/amop/AMOPMessage.h>
#include <bcos-rpc/amop/Common.h>
#include <boost/core/ignore_unused.hpp>
#include <algorithm>
#include <random>

#define TOPIC_SYNC_PERIOD (2000)

using namespace bcos;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;
using namespace bcos::amop;

void AMOP::initMsgHandler()
{
    auto self = std::weak_ptr<AMOP>(shared_from_this());
    m_messageHandler[AMOPMessage::Type::TopicSeq] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveTopicSeqMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessage::Type::RequestTopic] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveRequestTopicMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessage::Type::ResponseTopic] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveResponseTopicMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessage::Type::AMOPRequest] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveAMOPMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessage::Type::AMOPBroadcast] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveAMOPBroadcastMessage(_nodeID, _id, _msg);
            }
        };

    AMOP_LOG(INFO) << LOG_BADGE("initMsgHandler")
                   << LOG_KV("front service message handler size", m_messageHandler.size());
}

void AMOP::start()
{
    if (m_run)
    {
        AMOP_LOG(INFO) << LOG_BADGE("start") << LOG_DESC("amop is running");
        return;
    }

    m_run = true;
    // init message handler
    initMsgHandler();
    // broadcast topic seq periodically
    broadcastTopicSeq();
    AMOP_LOG(INFO) << LOG_BADGE("start") << LOG_DESC("start amop successfully");
}

void AMOP::stop()
{
    if (!m_run)
    {
        AMOP_LOG(INFO) << LOG_BADGE("stop") << LOG_DESC("amop is not running");
        return;
    }

    m_run = false;
    m_messageHandler.clear();

    if (m_timer)
    {
        m_timer->cancel();
    }

    AMOP_LOG(INFO) << LOG_BADGE("stop") << LOG_DESC("stop amop successfully");
}

/**
 * @brief: create message and encode the message to bytes
 * @param _type: message type
 * @param _data: message data
 * @return std::shared_ptr<bytes>
 */
std::shared_ptr<bytes> AMOP::buildAndEncodeMessage(uint32_t _type, bcos::bytesConstRef _data)
{
    auto message = m_messageFactory->buildMessage();
    message->setType(_type);
    message->setData(_data);
    auto buffer = std::make_shared<bytes>();
    message->encode(*buffer.get());
    return buffer;
}

/**
 * @brief: periodically send topicSeq to all other nodes
 * @return void
 */
void AMOP::broadcastTopicSeq()
{
    auto topicSeq = std::to_string(m_topicManager->topicSeq());
    auto buffer = buildAndEncodeMessage(
        AMOPMessage::Type::TopicSeq, bytesConstRef((byte*)topicSeq.data(), topicSeq.size()));
    m_frontServiceInterface->asyncSendBroadcastMessage(
        bcos::protocol::ModuleID::AMOP, bytesConstRef(buffer->data(), buffer->size()));

    AMOP_LOG(TRACE) << LOG_BADGE("broadcastTopicSeq") << LOG_KV("topicSeq", topicSeq);

    auto self = std::weak_ptr<AMOP>(shared_from_this());
    m_timer = std::make_shared<boost::asio::deadline_timer>(
        boost::asio::make_strand(*m_ioc), boost::posix_time::milliseconds(TOPIC_SYNC_PERIOD));
    m_timer->async_wait([self](boost::system::error_code e) {
        if (e)
        {
            return;
        }

        auto s = self.lock();
        if (s)
        {
            s->broadcastTopicSeq();
        }
    });
}

/**
 * @brief: receive topicSeq from other nodes
 * @param _nodeID: the sender nodeID
 * @param _id: the message id
 * @param _msg: message
 * @return void
 */
void AMOP::onReceiveTopicSeqMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg)
{
    try
    {
        uint32_t topicSeq =
            boost::lexical_cast<uint32_t>(std::string(_msg->data().begin(), _msg->data().end()));
        if (!m_topicManager->checkTopicSeq(_nodeID, topicSeq))
        {
            AMOP_LOG(TRACE) << LOG_BADGE("onReceiveTopicSeqMessage")
                            << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                            << LOG_KV("topicSeq", topicSeq);
            return;
        }

        AMOP_LOG(INFO) << LOG_BADGE("onReceiveTopicSeqMessage") << LOG_KV("nodeID", _nodeID->hex())
                       << LOG_KV("id", _id) << LOG_KV("topicSeq", topicSeq);

        auto buffer = buildAndEncodeMessage(AMOPMessage::Type::RequestTopic, bytesConstRef());
        m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP, _nodeID,
            bytesConstRef(buffer->data(), buffer->size()), 0,
            [_nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr, bytesConstRef,
                const std::string& _id, bcos::front::ResponseFunc) {
                if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                {
                    AMOP_LOG(WARNING)
                        << LOG_BADGE("onReceiveTopicSeqMessage")
                        << LOG_DESC("receive error callback") << LOG_KV("nodeID", _nodeID->hex())
                        << LOG_KV("id", _id) << LOG_KV("errorCode", _error->errorCode())
                        << LOG_KV("errorMessage", _error->errorMessage());
                }
            });
    }
    catch (const std::exception& e)
    {
        AMOP_LOG(ERROR) << LOG_DESC("onReceiveTopicSeqMessage") << LOG_KV("nodeID", _nodeID->hex())
                        << LOG_KV("error", boost::diagnostic_information(e));
    }
}

/**
 * @brief: receive request topic message from other nodes
 * @param _nodeID: the sender nodeID
 * @param _id: the message id
 * @param _msg: message
 * @return void
 */
void AMOP::onReceiveRequestTopicMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg)
{
    (void)_msg;
    try
    {
        std::string topicJson = m_topicManager->queryTopicsSubByClient();

        AMOP_LOG(INFO) << LOG_BADGE("onReceiveRequestTopicMessage")
                       << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                       << LOG_KV("topicJson", topicJson);

        auto buffer = buildAndEncodeMessage(AMOPMessage::Type::ResponseTopic,
            bytesConstRef((byte*)topicJson.data(), topicJson.size()));
        m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP, _nodeID,
            bytesConstRef(buffer->data(), buffer->size()), 0,
            [_nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr, bytesConstRef,
                const std::string& _id, bcos::front::ResponseFunc) {
                if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                {
                    AMOP_LOG(WARNING)
                        << LOG_BADGE("onReceiveRequestTopicMessage")
                        << LOG_DESC("callback respones error") << LOG_KV("nodeID", _nodeID->hex())
                        << LOG_KV("id", _id) << LOG_KV("errorCode", _error->errorCode())
                        << LOG_KV("errorMessage", _error->errorMessage());
                }
            });
    }
    catch (const std::exception& e)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onReceiveRequestTopicMessage")
                        << LOG_KV("nodeID", _nodeID->hex())
                        << LOG_KV("error", boost::diagnostic_information(e));
    }
}

/**
 * @brief: receive topic response message from other nodes
 * @param _nodeID: the sender nodeID
 * @param _id: the message id
 * @param _msg: message
 * @return void
 */
void AMOP::onReceiveResponseTopicMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg)
{
    try
    {
        uint32_t topicSeq;
        TopicItems topicItems;
        std::string topicJson = std::string(_msg->data().begin(), _msg->data().end());
        if (m_topicManager->parseTopicItemsJson(topicSeq, topicItems, topicJson))
        {
            m_topicManager->updateSeqAndTopicsByNodeID(_nodeID, topicSeq, topicItems);
        }
    }
    catch (const std::exception& e)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onReceiveResponseTopicMessage")
                        << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                        << LOG_KV("error", boost::diagnostic_information(e));
    }
}

/**
 * @brief: receive amop message
 * @param _nodeID: the sender nodeID
 * @param _id: the message id
 * @param _msg: message
 * @return void
 */
void AMOP::onReceiveAMOPMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg)
{
    auto wsService = m_wsService.lock();
    if (!wsService)
    {
        return;
    }

    AMOP_LOG(TRACE) << LOG_BADGE("onReceiveAMOPMessage") << LOG_KV("nodeID", _nodeID->hex())
                    << LOG_KV("id", _id);

    auto frontService = m_frontServiceInterface;
    auto _callback = [_id, _nodeID, frontService](bytesConstRef _data) {
        frontService->asyncSendResponse(
            _id, bcos::protocol::ModuleID::AMOP, _nodeID, _data, [_nodeID, _id](Error::Ptr _error) {
                if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                {
                    // send response failed???
                    AMOP_LOG(WARNING)
                        << LOG_BADGE("onReceiveAMOPMessage") << LOG_DESC("send response error")
                        << LOG_KV("id", _id) << LOG_KV("nodeID", _nodeID->hex());
                }
            });
    };

    auto _data = _msg->data();
    auto message = m_wsMessageFactory->buildMessage();
    auto size = message->decode(_data.data(), _data.size());
    if (size < 0)
    {
        WEBSOCKET_SERVICE(ERROR) << LOG_BADGE("onRecvAMOPMessage")
                                 << LOG_DESC("decode message failed") << LOG_KV("nodeID", _nodeID)
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
                                 << LOG_KV("nodeID", _nodeID)
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
        auto buffer = std::make_shared<bcos::bytes>();
        message->setStatus(bcos::protocol::CommonError::NotFoundClientByTopicDispatchMsg);
        message->data()->clear();
        message->setType(bcos::amop::MessageType::AMOP_RESPONSE);
        message->data()->clear();
        message->encode(*buffer);

        m_threadPool->enqueue(
            [buffer, _callback]() { _callback(bytesConstRef(buffer->data(), buffer->size())); });

        WEBSOCKET_SERVICE(WARNING)
            << LOG_BADGE("onRecvAMOPMessage") << LOG_DESC("no client subscribe the topic")
            << LOG_KV("topic", topic) << LOG_KV("nodeID", _nodeID);
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
        std::function<void(std::shared_ptr<bcos::bytes> _data)> m_callback;

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
                if (!session || !session->isConnected())
                {
                    WEBSOCKET_SERVICE(ERROR)
                        << LOG_BADGE("onRecvAMOPMessage")
                        << LOG_DESC("cannot get session by endpoint") << LOG_KV("endpoint", client)
                        << LOG_KV("topic", m_topic) << LOG_KV("nodeID", m_nodeID);
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
                m_message->setStatus(bcos::protocol::CommonError::NotFoundClientByTopicDispatchMsg);
                m_message->data()->clear();
                m_message->setType(bcos::amop::MessageType::AMOP_RESPONSE);
                m_message->encode(*buffer);

                m_callback(buffer);
                return;
            }

            auto self = shared_from_this();
            // Note: how to set the timeout
            session->asyncSendMessage(m_message, Options(30000),
                [self](bcos::Error::Ptr _error, std::shared_ptr<WsMessage> _msg,
                    std::shared_ptr<WsSession> _session) {
                    // try again when send message to the session failed
                    if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                    {
                        WEBSOCKET_SERVICE(WARNING)
                            << LOG_BADGE("onRecvAMOPMessage")
                            << LOG_DESC("asyncSendMessage callback error and try to send again")
                            << LOG_KV(
                                   "endpoint", (_session ? _session->endPoint() : std::string("")))
                            << LOG_KV("topic", self->m_topic) << LOG_KV("nodeID", self->m_nodeID)
                            << LOG_KV("errorCode", _error ? _error->errorCode() : -1)
                            << LOG_KV("errorMessage", _error ? _error->errorMessage() : "success");
                        return self->sendMessage();
                    }

                    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());
                    auto data = _msg->data();

                    WEBSOCKET_SERVICE(TRACE)
                        << LOG_BADGE("onRecvAMOPMessage")
                        << LOG_DESC("asyncSendMessage callback response") << LOG_KV("seq", seq)
                        << LOG_KV("data size", data->size());

                    auto buffer = std::make_shared<bcos::bytes>();
                    _msg->encode(*buffer);
                    self->m_callback(buffer);
                });
        }
    };

    auto retry = std::make_shared<Retry>();
    retry->m_topic = topic;
    retry->m_nodeID = _nodeID->hex();
    retry->m_clients = clients;
    retry->m_message = message;
    retry->m_wsService = m_wsService.lock();
    retry->m_callback = [_callback](std::shared_ptr<bcos::bytes> _data) {
        _callback(bytesConstRef(_data->data(), _data->size()));
    };
    retry->sendMessage();
}

/**
 * @brief: receive amop broadcast message
 * @param _nodeID: the sender nodeID
 * @param _id: the message id
 * @param _msg: message
 * @return void
 */
void AMOP::onReceiveAMOPBroadcastMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg)
{
    auto wsService = m_wsService.lock();
    if (!wsService)
    {
        return;
    }

    bytesConstRef _data = _msg->data();

    // WsMessage
    auto message = m_wsMessageFactory->buildMessage();
    auto size = message->decode(_data.data(), _data.size());
    if (size < 0)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                        << LOG_DESC("decode message failed") << LOG_KV("data", *toHexString(_data));
        return;
    }

    // AMOPRequest
    auto request = m_requestFactory->buildRequest();
    size = request->decode(bytesConstRef(message->data()->data(), message->data()->size()));
    if (size < 0)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onRecvAMOPBroadcastMessage")
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
        AMOP_LOG(WARNING) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                          << LOG_DESC("no client subscribe the topic") << LOG_KV("seq", seq)
                          << LOG_KV("topic", topic);
        return;
    }

    for (const auto& client : clients)
    {
        auto session = wsService->getSession(client);
        if (!session || !session->isConnected())
        {
            continue;
        }

        AMOP_LOG(TRACE) << LOG_BADGE("onRecvAMOPBroadcastMessage")
                        << LOG_DESC("push message to client") << LOG_KV("seq", seq)
                        << LOG_KV("topic", topic) << LOG_KV("endpoint", session->endPoint());

        session->asyncSendMessage(message);
    }
    AMOP_LOG(TRACE) << LOG_DESC("onReceiveAMOPBroadcastMessage") << LOG_KV("nodeID", _nodeID->hex())
                    << LOG_KV("id", _id);
}


/**
 * @brief: receive sub topic message from sdk
 */
void AMOP::onRecvSubTopics(std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto request = std::string(_msg->data()->begin(), _msg->data()->end());
    auto endpoint = _session->endPoint();
    m_topicManager->subTopic(endpoint, request);

    AMOP_LOG(INFO) << LOG_BADGE("onRecvSubTopics") << LOG_KV("request", request)
                   << LOG_KV("endpoint", endpoint);
}

/**
 * @brief: receive amop request message from sdk
 */
void AMOP::onRecvAMOPRequest(std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto factory = std::make_shared<AMOPRequestFactory>();
    auto request = factory->buildRequest();
    auto size = request->decode(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    if (size < 0)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onRecvAMOPRequest")
                        << LOG_DESC("decode amop request message failed")
                        << LOG_KV("endpoint", _session ? _session->endPoint() : std::string(""))
                        << LOG_KV("data", *toHexString(_msg->data()->begin(), _msg->data()->end()));
        return;
    }
    auto buffer = std::make_shared<bcos::bytes>();
    _msg->encode(*buffer);
    auto topic = request->topic();
    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());
    auto type = _msg->type();

    AMOP_LOG(DEBUG) << LOG_BADGE("onRecvAMOPRequest") << LOG_KV("seq", seq) << LOG_KV("type", type)
                    << LOG_KV("topic", topic);

    asyncSendMessage(topic, bcos::bytesConstRef(buffer->data(), buffer->size()),
        [_msg, _session, topic, seq, type](bcos::Error::Ptr _error, bcos::bytesConstRef _data) {
            if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
            {
                _msg->setStatus(_error->errorCode());
                _msg->data()->clear();

                AMOP_LOG(ERROR) << LOG_BADGE("onRecvAMOPRequest")
                                << LOG_DESC("AMOP async send message callback")
                                << LOG_KV("seq", seq) << LOG_KV("type", type)
                                << LOG_KV("topic", topic)
                                << LOG_KV("errorCode", _error->errorCode())
                                << LOG_KV("errorMessage", _error->errorMessage());
                return _session->asyncSendMessage(_msg);
            }

            // Note:
            auto size = _msg->decode(_data.data(), _data.size());
            AMOP_LOG(DEBUG) << LOG_BADGE("onRecvAMOPRequest")
                            << LOG_DESC("AMOP async send message callback") << LOG_KV("seq", seq)
                            << LOG_KV("type", type) << LOG_KV("topic", topic)
                            << LOG_KV("size", size) << LOG_KV("data size", _data.size());
            _session->asyncSendMessage(_msg);
        });
}

/**
 * @brief: receive amop broadcast message from sdk
 */
void AMOP::onRecvAMOPBroadcast(std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    boost::ignore_unused(_session);

    auto request = m_requestFactory->buildRequest();
    auto size = request->decode(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    if (size < 0)
    {
        AMOP_LOG(ERROR) << LOG_BADGE("onRecvAMOPBroadcast")
                        << LOG_DESC("decode amop broadcast message failed")
                        << LOG_KV("endpoint", _session ? _session->endPoint() : std::string(""))
                        << LOG_KV("data", *toHexString(_msg->data()->begin(), _msg->data()->end()));
        return;
    }

    auto topic = request->topic();
    AMOP_LOG(DEBUG) << LOG_BADGE("onRecvAMOPBroadcast") << LOG_KV("seq", _msg->seq())
                    << LOG_KV("topic", topic);

    auto buffer = std::make_shared<bcos::bytes>();
    _msg->encode(*buffer);

    asyncSendBroadbastMessage(topic, bcos::bytesConstRef(buffer->data(), buffer->size()));
}


/**
 * @brief: async receive message from front service
 * @param _nodeID: the message sender nodeID
 * @param _id: the id of this message, it can by used to send response to the peer
 * @param _data: the message data
 * @return void
 */
void AMOP::asyncNotifyAmopMessage(bcos::crypto::NodeIDPtr _nodeID, const std::string& _id,
    bcos::bytesConstRef _data, std::function<void(bcos::Error::Ptr _error)> _callback)
{
    if (_callback)
    {
        _callback(nullptr);
    }
    auto message = m_messageFactory->buildMessage();
    auto size = message->decode(_data);
    if (size < 0)
    {  // invalid format packet
        AMOP_LOG(ERROR) << LOG_BADGE("asyncNotifyAmopMessage") << LOG_BADGE("illegal packet")
                        << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                        << LOG_KV("data", *toHexString(_data));
        return;
    }

    auto it = m_messageHandler.find(message->type());
    if (it != m_messageHandler.end())
    {
        it->second(_nodeID, _id, message);
    }
    else
    {
        AMOP_LOG(ERROR) << LOG_BADGE("asyncNotifyAmopMessage")
                        << LOG_BADGE("unrecognized message type") << LOG_KV("type", message->type())
                        << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                        << LOG_KV("data", *toHexString(_data));
    }
}

/**
 * @brief: async receive nodeIDs from front service
 * @param _nodeIDs: the nodeIDs
 * @param _callback: callback
 * @return void
 */
void AMOP::asyncNotifyAmopNodeIDs(std::shared_ptr<const crypto::NodeIDs> _nodeIDs,
    std::function<void(bcos::Error::Ptr _error)> _callback)
{
    m_topicManager->notifyNodeIDs(_nodeIDs ? *_nodeIDs.get() : bcos::crypto::NodeIDs());
    if (_callback)
    {
        _callback(nullptr);
    }
    AMOP_LOG(INFO) << LOG_BADGE("asyncNotifyAmopNodeIDs")
                   << LOG_KV("nodeIDs size", (_nodeIDs ? _nodeIDs->size() : 0));
}

/**
 * @brief: async send message to random node subscribe _topic
 * @param _topic: topic
 * @param _data: message data
 * @param _respFunc: callback
 * @return void
 */
void AMOP::asyncSendMessage(const std::string& _topic, bcos::bytesConstRef _data,
    std::function<void(bcos::Error::Ptr _error, bcos::bytesConstRef _data)> _respFunc)
{
    std::vector<std::string> strNodeIDs;
    m_topicManager->queryNodeIDsByTopic(_topic, strNodeIDs);
    if (strNodeIDs.empty())
    {
        auto errorPtr =
            std::make_shared<Error>(bcos::protocol::CommonError::NotFoundPeerByTopicSendMsg,
                "there has no node subscribe this topic, topic: " + _topic);
        if (_respFunc)
        {
            _respFunc(errorPtr, bytesConstRef());
        }

        AMOP_LOG(WARNING) << LOG_BADGE("asyncSendMessage")
                          << LOG_DESC("there has no node subscribe the topic")
                          << LOG_KV("topic", _topic);
        return;
    }

    std::vector<bcos::crypto::NodeIDPtr> nodeIDs;
    auto keyFactory = m_keyFactory;
    std::for_each(
        strNodeIDs.begin(), strNodeIDs.end(), [&nodeIDs, keyFactory](const std::string& strNodeID) {
            auto nodeID = keyFactory->createKey(*fromHexString(strNodeID));
            nodeIDs.push_back(nodeID);
        });

    auto buffer = buildAndEncodeMessage(AMOPMessage::Type::AMOPRequest, _data);
    class RetrySender : public std::enable_shared_from_this<RetrySender>
    {
    public:
        bcos::crypto::NodeIDs m_nodeIDs;
        std::shared_ptr<bytes> m_buffer;
        std::shared_ptr<bcos::front::FrontServiceInterface> m_frontServiceInterface;
        std::function<void(bcos::Error::Ptr _error, bcos::bytesConstRef _data)> m_callback;

    public:
        void sendMessage()
        {
            if (m_nodeIDs.empty())
            {
                auto errorPtr =
                    std::make_shared<Error>(bcos::protocol::CommonError::AMOPSendMsgFailed,
                        "unable to send message to peer by topic");
                if (m_callback)
                {
                    m_callback(errorPtr, bytesConstRef());
                }

                return;
            }

            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine e(seed);
            std::shuffle(m_nodeIDs.begin(), m_nodeIDs.end(), e);

            auto nodeID = *m_nodeIDs.begin();
            m_nodeIDs.erase(m_nodeIDs.begin());

            auto self = shared_from_this();
            // try to send message to node
            m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP,
                nodeID, bytesConstRef(m_buffer->data(), m_buffer->size()), 0,
                [self, nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr _nodeID,
                    bytesConstRef _data, const std::string& _id,
                    bcos::front::ResponseFunc _respFunc) {
                    boost::ignore_unused(_nodeID, _data, _id, _respFunc);
                    if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                    {
                        AMOP_LOG(DEBUG)
                            << LOG_BADGE("RetrySender::sendMessage")
                            << LOG_DESC("asyncSendMessageByNodeID callback response error")
                            << LOG_KV("nodeID", nodeID->hex())
                            << LOG_KV("errorCode", _error->errorCode())
                            << LOG_KV("errorMessage", _error->errorMessage());
                        // try again to send to another node
                        self->sendMessage();
                    }
                    else
                    {
                        if (self->m_callback)
                        {
                            self->m_callback(nullptr, _data);
                        }
                    }
                });
        }
    };

    auto sender = std::make_shared<RetrySender>();
    sender->m_nodeIDs = nodeIDs;
    sender->m_buffer = buffer;
    sender->m_frontServiceInterface = m_frontServiceInterface;
    sender->m_callback = _respFunc;

    // send message
    sender->sendMessage();
}

/**
 * @brief: async send message to all nodes subscribe _topic
 * @param _topic: topic
 * @param _data: message data
 * @return void
 */
void AMOP::asyncSendBroadbastMessage(const std::string& _topic, bcos::bytesConstRef _data)
{
    std::vector<std::string> strNodeIDs;
    m_topicManager->queryNodeIDsByTopic(_topic, strNodeIDs);
    if (strNodeIDs.empty())
    {
        AMOP_LOG(WARNING) << LOG_BADGE("asyncSendBroadbastMessage")
                          << LOG_DESC("there no node subscribe this topic")
                          << LOG_KV("topic", _topic);
        return;
    }

    std::vector<bcos::crypto::NodeIDPtr> nodeIDs;
    auto keyFactory = m_keyFactory;
    std::for_each(
        strNodeIDs.begin(), strNodeIDs.end(), [&nodeIDs, keyFactory](const std::string& strNodeID) {
            auto nodeID = keyFactory->createKey(*fromHexString(strNodeID));
            nodeIDs.push_back(nodeID);
        });

    auto buffer = buildAndEncodeMessage(AMOPMessage::Type::AMOPBroadcast, _data);
    m_frontServiceInterface->asyncSendMessageByNodeIDs(
        bcos::protocol::ModuleID::AMOP, nodeIDs, bytesConstRef(buffer->data(), buffer->size()));

    AMOP_LOG(DEBUG) << LOG_BADGE("asyncSendBroadbastMessage") << LOG_DESC("send broadcast message")
                    << LOG_KV("topic", _topic) << LOG_KV("data size", _data.size());
}