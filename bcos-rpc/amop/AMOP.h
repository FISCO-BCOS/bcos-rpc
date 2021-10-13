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
 * @file AMOP.h
 * @author: octopus
 * @date 2021-06-21
 */
#pragma once

#include <bcos-framework/interfaces/amop/AMOPInterface.h>
#include <bcos-framework/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/libutilities/ThreadPool.h>
#include <bcos-rpc/amop/AMOPMessage.h>
#include <bcos-rpc/amop/AMOPRequest.h>
#include <bcos-rpc/amop/TopicManager.h>
#include <boost/asio.hpp>

namespace bcos
{
namespace boostssl
{
namespace ws
{
class WsService;
class WsSession;
class WsMessage;
class WsMessageFactory;
}  // namespace ws
}  // namespace boostssl
namespace amop
{
class AMOP : public std::enable_shared_from_this<AMOP>
{
public:
    using Ptr = std::shared_ptr<AMOP>;

public:
    AMOP(std::shared_ptr<bcos::boostssl::ws::WsService> _wsService,
        std::shared_ptr<bcos::amop::MessageFactory> _messageFactory,
        std::shared_ptr<amop::TopicManager> _topicManager,
        std::shared_ptr<AMOPRequestFactory> _requestFactory,
        bcos::crypto::KeyFactory::Ptr _keyFactory)
      : m_messageFactory(_messageFactory),
        m_topicManager(_topicManager),
        m_requestFactory(_requestFactory),
        m_keyFactory(_keyFactory)
    {
        m_wsService = std::weak_ptr<boostssl::ws::WsService>(_wsService);
        m_wsMessageFactory = _wsService->messageFactory();
        m_ioc = _wsService->ioc();
        m_wsService = _wsService;
    }

    virtual ~AMOP() { stop(); }

    void initMsgHandler();

public:
    virtual void start();
    virtual void stop();

    /**
     * @brief: async receive message from front service
     * @param _nodeID: the message sender nodeID
     * @param _id: the id of this message, it can by used to send response to the peer
     * @param _data: the message data
     * @return void
     */
    virtual void asyncNotifyAmopMessage(bcos::crypto::NodeIDPtr _nodeID, const std::string& _id,
        bcos::bytesConstRef _data, std::function<void(bcos::Error::Ptr _error)> _callback);
    /**
     * @brief: async receive nodeIDs from front service
     * @param _nodeIDs: the nodeIDs
     * @param _callback: callback
     * @return void
     */
    virtual void asyncNotifyAmopNodeIDs(std::shared_ptr<const crypto::NodeIDs> _nodeIDs,
        std::function<void(bcos::Error::Ptr _error)> _callback);

    /**
     * @brief: async send message to random node subscribe _topic
     * @param _topic: topic
     * @param _data: message data
     * @param _respFunc: callback
     * @return void
     */
    virtual void asyncSendMessage(const std::string& _topic, bcos::bytesConstRef _data,
        std::function<void(bcos::Error::Ptr _error, bcos::bytesConstRef _data)> _respFunc);
    /**
     * @brief: async send message to all nodes subscribe _topic
     * @param _topic: topic
     * @param _data: message data
     * @return void
     */
    virtual void asyncSendBroadbastMessage(const std::string& _topic, bcos::bytesConstRef _data);

public:
    /**
     * @brief: create message and encode the message to bytes
     * @param _type: message type
     * @param _data: message data
     * @return std::shared_ptr<bytes>
     */
    std::shared_ptr<bytes> buildAndEncodeMessage(uint32_t _type, bcos::bytesConstRef _data);
    /**
     * @brief: periodically send topicSeq to all other nodes
     * @return void
     */
    void broadcastTopicSeq();

    //----------------------------------------------------------------------------------------
    //-------------- front service message begin----------------------------------------------
    /**
     * @brief: receive topicSeq from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveTopicSeqMessage(
        bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg);
    /**
     * @brief: receive request topic message from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveRequestTopicMessage(
        bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg);
    /**
     * @brief: receive topic response message from other nodes
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveResponseTopicMessage(
        bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg);
    /**
     * @brief: receive amop message
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveAMOPMessage(
        bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg);
    /**
     * @brief: receive broadcast message
     * @param _nodeID: the sender nodeID
     * @param _id: the message id
     * @param _msg: message
     * @return void
     */
    void onReceiveAMOPBroadcastMessage(
        bcos::crypto::NodeIDPtr _nodeID, const std::string& _id, AMOPMessage::Ptr _msg);

    //-------------- front service message end----------------------------------------------
    //----------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------
    //-------------- sdk message begin ---------------------------------------------------------
    /**
     * @brief: receive sub topic message from sdk
     */
    virtual void onRecvSubTopics(std::shared_ptr<boostssl::ws::WsMessage> _msg,
        std::shared_ptr<boostssl::ws::WsSession> _session);
    /**
     * @brief: receive amop request message from sdk
     */
    virtual void onRecvAMOPRequest(std::shared_ptr<boostssl::ws::WsMessage> _msg,
        std::shared_ptr<boostssl::ws::WsSession> _session);
    /**
     * @brief: receive amop broadcast message from sdk
     */
    virtual void onRecvAMOPBroadcast(std::shared_ptr<boostssl::ws::WsMessage> _msg,
        std::shared_ptr<boostssl::ws::WsSession> _session);
    //-------------- sdk message end ---------------------------------------------------------
    //----------------------------------------------------------------------------------------

public:
    decltype(auto) messageHandler() { return m_messageHandler; }

    std::shared_ptr<boost::asio::io_context> ioc() const { return m_ioc; }
    void setIoc(const std::shared_ptr<boost::asio::io_context> _ioc) { m_ioc = _ioc; }
    std::shared_ptr<MessageFactory> messageFactory() const { return m_messageFactory; }
    std::shared_ptr<bcos::boostssl::ws::WsMessageFactory> wsMessageFactory() const
    {
        return m_wsMessageFactory;
    }
    std::shared_ptr<TopicManager> topicManager() const { return m_topicManager; }
    std::shared_ptr<bcos::front::FrontServiceInterface> frontServiceInterface() const
    {
        return m_frontServiceInterface;
    }
    void setFrontServiceInterface(
        std::shared_ptr<bcos::front::FrontServiceInterface> _frontServiceInterface)
    {
        m_frontServiceInterface = _frontServiceInterface;
    }

    std::weak_ptr<bcos::boostssl::ws::WsService> wsService() { return m_wsService; }
    std::shared_ptr<bcos::crypto::KeyFactory> keyFactory() { return m_keyFactory; }

    std::shared_ptr<AMOPRequestFactory> requestFactory() const { return m_requestFactory; }
    std::shared_ptr<bcos::ThreadPool> threadPool() const { return m_threadPool; }
    void setThreadPool(std::shared_ptr<bcos::ThreadPool> _threadPool)
    {
        m_threadPool = _threadPool;
    }

private:
    bool m_run = false;
    std::weak_ptr<bcos::boostssl::ws::WsService> m_wsService;
    std::shared_ptr<MessageFactory> m_messageFactory;
    std::shared_ptr<bcos::boostssl::ws::WsMessageFactory> m_wsMessageFactory;
    std::shared_ptr<TopicManager> m_topicManager;
    std::shared_ptr<AMOPRequestFactory> m_requestFactory;
    std::shared_ptr<bcos::crypto::KeyFactory> m_keyFactory;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
    std::shared_ptr<bcos::front::FrontServiceInterface> m_frontServiceInterface;

    std::shared_ptr<boost::asio::deadline_timer> m_timer;
    std::shared_ptr<boost::asio::io_context> m_ioc;

    std::unordered_map<uint16_t, std::function<void(bcos::crypto::NodeIDPtr _nodeID,
                                     const std::string& _id, AMOPMessage::Ptr _msg)>>
        m_messageHandler;
};

}  // namespace amop
}  // namespace bcos