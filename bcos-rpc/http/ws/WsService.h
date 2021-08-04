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
 * @file WsService.h
 * @author: octopus
 * @date 2021-07-28
 */
#pragma once

#include <bcos-framework/interfaces/protocol/ProtocolTypeDef.h>
#include <bcos-rpc/http/ws/Common.h>
#include <bcos-rpc/rpc/jsonrpc/JsonRpcInterface.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace bcos
{
class ThreadPool;
namespace amop
{
class TopicManager;
class AMOPMessage;
class AMOPInterface;
class AMOP;
}  // namespace amop
namespace ws
{
class WsSession;
class WsMessage;
class WsMessageFactory;

using WsSessions = std::vector<std::shared_ptr<WsSession>>;
using WsMsgHandler = std::function<void(std::shared_ptr<WsMessage>, std::shared_ptr<WsSession>)>;

class WsService : public std::enable_shared_from_this<WsService>
{
public:
    using Ptr = std::shared_ptr<WsService>;
    WsService() = default;
    virtual ~WsService() { stop(); }
    void initMethod();

public:
    virtual void start();
    virtual void stop();
    virtual void doLoop();

public:
    std::shared_ptr<WsSession> getSession(const std::string& _endPoint);
    void addSession(std::shared_ptr<WsSession> _session);
    void removeSession(const std::string& _endPoint);
    WsSessions sessions();

public:
    /**
     * @brief: websocket session disconnect
     * @param _msg: received message
     * @param _error:
     * @param _session: websocket session
     * @return void:
     */
    virtual void onDisconnect(Error::Ptr _error, std::shared_ptr<WsSession> _session);

    //------------------------------------------------------------------------------------------
    //-------------- sdk message begin ---------------------------------------------------------
    /**
     * @brief: receive message from sdk
     * @param _error: error
     * @param _msg: received message
     * @param _session: websocket session
     * @return void:
     */
    virtual void onRecvClientMessage(
        Error::Ptr _error, std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    /**
     * @brief: receive handshake message from sdk
     */
    virtual void onRecvHandshake(
        std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    /**
     * @brief: receive rpc request message from sdk
     */
    virtual void onRecvRPCRequest(
        std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    /**
     * @brief: receive sub topic message from sdk
     */
    virtual void onRecvSubTopics(
        std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    /**
     * @brief: receive amop request message from sdk
     */
    virtual void onRecvAMOPRequest(
        std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    /**
     * @brief: receive amop broadcast message from sdk
     */
    virtual void onRecvAMOPBroadcast(
        std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session);
    //-------------- sdk message end ---------------------------------------------------------
    //----------------------------------------------------------------------------------------

    /**
     * @brief: receive message from front service
     * @param _error: error
     * @param _msg:  message received from front service
     * @return void:
     */
    virtual void onRecvAMOPMessage(Error::Ptr _error, std::shared_ptr<amop::AMOPMessage> _msg);

    /**
     * @brief: push blocknumber to _session
     * @param _session:
     * @param _blockNumber:
     * @return void:
     */
    void notifyBlockNumberToClient(
        std::shared_ptr<WsSession> _session, bcos::protocol::BlockNumber _blockNumber);
    /**
     * @brief: push blocknumber to all active sessions
     * @param _blockNumber:
     * @return void:
     */
    void notifyBlockNumberToClient(bcos::protocol::BlockNumber _blockNumber);

public:
    std::shared_ptr<WsMessageFactory> messageFactory() { return m_messageFactory; }
    void setMessageFactory(std::shared_ptr<WsMessageFactory> _messageFactory)
    {
        m_messageFactory = _messageFactory;
    }

    std::shared_ptr<bcos::ThreadPool> threadPool() const { return m_threadPool; }
    void setThreadPool(std::shared_ptr<bcos::ThreadPool> _threadPool)
    {
        m_threadPool = _threadPool;
    }

    std::shared_ptr<bcos::rpc::JsonRpcInterface> jsonRpcInterface() { return m_jsonRpcInterface; }
    void setJsonRpcInterface(std::shared_ptr<bcos::rpc::JsonRpcInterface> _jsonRpcInterface)
    {
        m_jsonRpcInterface = _jsonRpcInterface;
    }

    std::shared_ptr<amop::TopicManager> topicManager() { return m_topicManager; }
    void setTopicManager(std::shared_ptr<amop::TopicManager> _topicManager)
    {
        m_topicManager = _topicManager;
    }

    std::shared_ptr<bcos::amop::AMOP> AMOP() const { return m_AMOP; }
    void setAMOP(std::shared_ptr<bcos::amop::AMOP> _AMOP) { m_AMOP = _AMOP; }

    std::shared_ptr<boost::asio::io_context> ioc() const { return m_ioc; }
    void setIoc(std::shared_ptr<boost::asio::io_context> _ioc) { m_ioc = _ioc; }

private:
    bool m_running{false};
    // WsMessageFactory
    std::shared_ptr<WsMessageFactory> m_messageFactory;
    // ThreadPool
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
    // JsonRpcInterface
    std::shared_ptr<bcos::rpc::JsonRpcInterface> m_jsonRpcInterface;
    // AMOPInterface
    std::shared_ptr<bcos::amop::AMOP> m_AMOP;
    // TopicManager
    std::shared_ptr<amop::TopicManager> m_topicManager;
    // mutex for m_sessions
    mutable std::shared_mutex x_mutex;
    // all active sessions
    std::unordered_map<std::string, std::shared_ptr<WsSession>> m_sessions;
    // io context
    std::shared_ptr<boost::asio::io_context> m_ioc;
    std::shared_ptr<boost::asio::deadline_timer> m_loopTimer;
    // type => handler
    std::unordered_map<uint32_t,
        std::function<void(std::shared_ptr<WsMessage>, std::shared_ptr<WsSession>)>>
        m_msgType2Method;
};

}  // namespace ws
}  // namespace bcos