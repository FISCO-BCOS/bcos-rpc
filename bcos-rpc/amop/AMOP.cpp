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

#include "libutilities/Log.h"
#include <bcos-framework/interfaces/front/FrontServiceInterface.h>
#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/interfaces/protocol/Protocol.h>
#include <bcos-framework/libutilities/DataConvertUtility.h>
#include <bcos-framework/libutilities/Exceptions.h>
#include <bcos-rpc/amop/AMOP.h>
#include <bcos-rpc/amop/AMOPMessage.h>
#include <bcos-rpc/amop/Common.h>
#include <bcos-rpc/http/ws/WsService.h>
#include <algorithm>
#include <random>

#define TOPIC_SYNC_PERIOD (2000)

using namespace bcos;
using namespace bcos::amop;

void AMOP::initMsgHandler()
{
    auto self = std::weak_ptr<AMOP>(shared_from_this());
    m_messageHandler[AMOPMessageType::TopicSeq] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveTopicSeqMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessageType::RequestTopic] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveRequestTopicMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessageType::ResponseTopic] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveResponseTopicMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessageType::AMOPRequest] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveAMOPMessage(_nodeID, _id, _msg);
            }
        };

    m_messageHandler[AMOPMessageType::AMOPBroadcast] =
        [self](bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg) {
            auto amop = self.lock();
            if (amop)
            {
                amop->onReceiveAMOPBroadcastMessage(_nodeID, _id, _msg);
            }
        };
}

void AMOP::start()
{
    if (m_run)
    {
        AMOP_LOG(INFO) << LOG_DESC("amop is running");
        return;
    }

    m_run = true;
    // init message handler
    initMsgHandler();
    // broadcast topic seq periodically
    broadcastTopicSeq();
    AMOP_LOG(INFO) << LOG_DESC("start amop successfully");
}

void AMOP::stop()
{
    if (!m_run)
    {
        AMOP_LOG(INFO) << LOG_DESC("amop is not running");
        return;
    }

    m_run = false;
    m_messageHandler.clear();

    if (m_timer)
    {
        m_timer->cancel();
    }

    AMOP_LOG(INFO) << LOG_DESC("stop amop successfully");
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
        AMOPMessageType::TopicSeq, bytesConstRef((byte*)topicSeq.data(), topicSeq.size()));
    m_frontServiceInterface->asyncSendBroadcastMessage(
        bcos::protocol::ModuleID::AMOP, bytesConstRef(buffer->data(), buffer->size()));

    AMOP_LOG(DEBUG) << LOG_DESC("broadcastTopicSeq") << LOG_KV("topicSeq", topicSeq);

    auto self = std::weak_ptr<AMOP>(shared_from_this());
    m_timer = std::make_shared<boost::asio::deadline_timer>(
        boost::asio::make_strand(*m_ioService), boost::posix_time::milliseconds(TOPIC_SYNC_PERIOD));
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
            AMOP_LOG(TRACE) << LOG_DESC("onReceiveTopicSeqMessage")
                            << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                            << LOG_KV("topicSeq", topicSeq);
            return;
        }

        AMOP_LOG(INFO) << LOG_DESC("onReceiveTopicSeqMessage") << LOG_KV("nodeID", _nodeID->hex())
                       << LOG_KV("id", _id) << LOG_KV("topicSeq", topicSeq);

        auto buffer = buildAndEncodeMessage(AMOPMessageType::RequestTopic, bytesConstRef());
        m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP, _nodeID,
            bytesConstRef(buffer->data(), buffer->size()), 0,
            [_nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr, bytesConstRef,
                const std::string& _id, bcos::front::ResponseFunc) {
                if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                {
                    AMOP_LOG(WARNING) << LOG_DESC("onReceiveTopicSeqMessage response")
                                      << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                                      << LOG_KV("errorCode", _error->errorCode())
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

        AMOP_LOG(INFO) << LOG_DESC("onReceiveRequestTopicMessage")
                       << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                       << LOG_KV("topicJson", topicJson);

        auto buffer = buildAndEncodeMessage(AMOPMessageType::ResponseTopic,
            bytesConstRef((byte*)topicJson.data(), topicJson.size()));
        m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP, _nodeID,
            bytesConstRef(buffer->data(), buffer->size()), 0,
            [_nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr, bytesConstRef,
                const std::string& _id, bcos::front::ResponseFunc) {
                if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                {
                    AMOP_LOG(WARNING) << LOG_DESC("onReceiveRequestTopicMessage response")
                                      << LOG_KV("nodeID", _nodeID->hex()) << LOG_KV("id", _id)
                                      << LOG_KV("errorCode", _error->errorCode())
                                      << LOG_KV("errorMessage", _error->errorMessage());
                }
            });
    }
    catch (const std::exception& e)
    {
        AMOP_LOG(ERROR) << LOG_DESC("onReceiveRequestTopicMessage")
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
        AMOP_LOG(ERROR) << LOG_DESC("onReceiveResponseTopicMessage")
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

    AMOP_LOG(TRACE) << LOG_DESC("onReceiveAMOPMessage") << LOG_KV("nodeID", _nodeID->hex())
                    << LOG_KV("id", _id);

    auto frontService = m_frontServiceInterface;
    wsService->onRecvAMOPMessage(
        _msg->data(), _nodeID->hex(), [_id, _nodeID, frontService](bytesConstRef _data) {
            frontService->asyncSendResponse(_id, bcos::protocol::ModuleID::AMOP, _nodeID, _data,
                [_nodeID, _id](Error::Ptr _error) {
                    if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                    {
                        // send response failed???
                        AMOP_LOG(WARNING)
                            << LOG_BADGE("onReceiveAMOPMessage") << LOG_DESC("send response error")
                            << LOG_KV("id", _id) << LOG_KV("nodeID", _nodeID->hex());
                    }
                });
        });
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

    wsService->onRecvAMOPBroadcastMessage(_msg->data());
    AMOP_LOG(TRACE) << LOG_DESC("onReceiveAMOPBroadcastMessage") << LOG_KV("nodeID", _nodeID->hex())
                    << LOG_KV("id", _id);
}

/**
 * @brief: async receive message from front service
 * @param _nodeID: the message sender nodeID
 * @param _id: the id of this message, it can by used to send response to the peer
 * @param _data: the message data
 * @return void
 */
void AMOP::asyncNotifyAmopMessage(
    bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, bcos::bytesConstRef _data)
{
    auto message = m_messageFactory->buildMessage();
    auto size = message->decode(_data);
    if (size < 0)
    {  // invalid format packet
        AMOP_LOG(ERROR) << LOG_DESC("asyncNotifyAmopMessage illegal packet")
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
        AMOP_LOG(ERROR) << LOG_DESC("asyncNotifyAmopMessage unrecognized message type")
                        << LOG_KV("type", message->type()) << LOG_KV("nodeID", _nodeID->hex())
                        << LOG_KV("id", _id) << LOG_KV("data", *toHexString(_data));
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
    AMOP_LOG(INFO) << LOG_DESC("asyncNotifyAmopNodeIDs")
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
        // TODO: to define the specific error code
        auto errorPtr = std::make_shared<Error>(bcos::protocol::CommonError::TIMEOUT,
            "there has no node follow the topic, topic: " + _topic);
        if (_respFunc)
        {
            _respFunc(errorPtr, bytesConstRef());
        }

        AMOP_LOG(WARNING) << LOG_DESC(
                                 "asyncSendMessage there has no node follow the topic, topic: ")
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

    auto buffer = buildAndEncodeMessage(AMOPMessageType::AMOPRequest, _data);
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
                // TODO: to define the specific error code
                auto errorPtr = std::make_shared<Error>(
                    bcos::protocol::CommonError::TIMEOUT, "failed to send the message after retry");
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

            auto weakPtr = std::weak_ptr<RetrySender>(shared_from_this());
            // try to send message to node
            m_frontServiceInterface->asyncSendMessageByNodeID(bcos::protocol::ModuleID::AMOP,
                nodeID, bytesConstRef(m_buffer->data(), m_buffer->size()), 0,
                [weakPtr, nodeID](Error::Ptr _error, bcos::crypto::NodeIDPtr _nodeID,
                    bytesConstRef _data, const std::string& _id,
                    bcos::front::ResponseFunc _respFunc) {
                    (void)_respFunc;
                    (void)_nodeID;
                    auto self = weakPtr.lock();
                    if (!self)
                    {
                        return;
                    }
                    if (_error && (_error->errorCode() != bcos::protocol::CommonError::SUCCESS))
                    {
                        AMOP_LOG(DEBUG) << LOG_DESC("RetrySender::sendMessage response error")
                                        << LOG_KV("nodeID", nodeID->hex())
                                        << LOG_KV("errorCode", _error->errorCode())
                                        << LOG_KV("errorMessage", _error->errorMessage());
                        // try again to send to another node
                        self->sendMessage();
                    }
                    else
                    {
                        AMOP_LOG(DEBUG) << LOG_DESC("RetrySender::sendMessage response ok")
                                        << LOG_KV("nodeID", nodeID->hex()) << LOG_KV("id", _id)
                                        << LOG_KV("data size", _data.size());
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
        AMOP_LOG(WARNING) << LOG_DESC("asyncSendBroadbastMessage no node follow topic")
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

    auto buffer = buildAndEncodeMessage(AMOPMessageType::AMOPBroadcast, _data);
    m_frontServiceInterface->asyncSendMessageByNodeIDs(
        bcos::protocol::ModuleID::AMOP, nodeIDs, bytesConstRef(buffer->data(), buffer->size()));

    AMOP_LOG(DEBUG) << LOG_DESC("asyncSendBroadbastMessage") << LOG_KV("topic", _topic)
                    << LOG_KV("data size", _data.size());
}