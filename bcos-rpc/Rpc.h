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
 * @file Rpc.h
 * @author: octopus
 * @date 2021-07-15
 */

#pragma once
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-framework/interfaces/rpc/RPCInterface.h>
#include <bcos-framework/libprotocol/amop/AMOPRequest.h>
#include <bcos-rpc/event/EventSub.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>

namespace bcos
{
namespace boostssl
{
namespace ws
{
class WsSession;
class WsService;
}  // namespace ws
}  // namespace boostssl
namespace rpc
{
class Rpc : public RPCInterface, public std::enable_shared_from_this<Rpc>
{
public:
    using Ptr = std::shared_ptr<Rpc>;

    Rpc(std::shared_ptr<boostssl::ws::WsService> _wsService,
        bcos::rpc::JsonRpcImpl_2_0::Ptr _jsonRpcImpl, bcos::event::EventSub::Ptr _eventSub,
        std::shared_ptr<bcos::boostssl::ws::WsMessageFactory> _wsMessageFactory,
        std::shared_ptr<bcos::protocol::AMOPRequestFactory> _requestFactory,
        bcos::gateway::GatewayInterface::Ptr _gateway, std::string const& _clientID,
        std::string const& _gatewayServiceName)
      : m_wsService(_wsService),
        m_jsonRpcImpl(_jsonRpcImpl),
        m_eventSub(_eventSub),
        m_wsMessageFactory(_wsMessageFactory),
        m_requestFactory(_requestFactory),
        m_gateway(_gateway),
        m_clientID(_clientID),
        m_gatewayServiceName(_gatewayServiceName)
    {
        m_jsonRpcImpl->groupManager()->registerGroupInfoNotifier(
            [this](bcos::group::GroupInfo::Ptr _groupInfo) { notifyGroupInfo(_groupInfo); });
        initMsgHandler();
    }

    virtual ~Rpc() { stop(); }

    virtual void start() override;
    virtual void stop() override;

    /**
     * @brief: notify blockNumber to rpc
     * @param _blockNumber: blockNumber
     * @param _callback: resp callback
     * @return void
     */
    virtual void asyncNotifyBlockNumber(std::string const& _groupID, std::string const& _nodeName,
        bcos::protocol::BlockNumber _blockNumber,
        std::function<void(Error::Ptr)> _callback) override;


    void asyncNotifyGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo,
        std::function<void(Error::Ptr&&)> _callback) override;

    std::shared_ptr<boostssl::ws::WsService> wsService() const { return m_wsService; }
    bcos::rpc::JsonRpcImpl_2_0::Ptr jsonRpcImpl() const { return m_jsonRpcImpl; }
    bcos::event::EventSub::Ptr eventSub() const { return m_eventSub; }


    /**
     * @brief receive amop request message from the gateway
     *
     */
    void asyncNotifyAMOPMessage(int16_t _type, std::string const& _topic, bytesConstRef _data,
        std::function<void(Error::Ptr&&, bytesPointer)> _callback) override
    {
        try
        {
            switch (_type)
            {
            case AMOPNotifyMessageType::Unicast:
                asyncNotifyAMOPMessage(_topic, _data, _callback);
                break;
            case AMOPNotifyMessageType::Broadcast:
                asyncNotifyAMOPBroadcastMessage(_topic, _data, _callback);
                break;
            default:
                BCOS_LOG(WARNING) << LOG_DESC("asyncNotifyAMOPMessage: unknown message type")
                                  << LOG_KV("type", _type);
            }
        }
        catch (std::exception const& e)
        {
            BCOS_LOG(WARNING) << LOG_DESC("asyncNotifyAMOPMessage exception")
                              << LOG_KV("error", boost::diagnostic_information(e));
        }
    }

protected:
    /// for AMOP requests from SDK
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


    virtual void notifyGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo);
    std::shared_ptr<boostssl::ws::WsSession> randomChooseSession(std::string const& _topic);

    virtual void asyncNotifyAMOPMessage(std::string const& _topic, bytesConstRef _data,
        std::function<void(Error::Ptr&&, bytesPointer)> _callback);
    virtual void asyncNotifyAMOPBroadcastMessage(std::string const& _topic, bytesConstRef _data,
        std::function<void(Error::Ptr&&, bytesPointer)> _callback);

    std::map<std::string, std::shared_ptr<boostssl::ws::WsSession>> querySessionsByTopic(
        std::string const& _topic) const
    {
        ReadGuard l(x_topicToSessions);
        return m_topicToSessions.at(_topic);
    }

    virtual void initMsgHandler();
    void onClientDisconnect(std::shared_ptr<boostssl::ws::WsSession> _session);

    bool updateTopicInfos(
        std::string const& _topicInfo, std::shared_ptr<boostssl::ws::WsSession> _session);

    // TODO: sync from dev
    void asyncNotifyTransactionResult(const std::string_view&, bcos::crypto::HashType,
        bcos::protocol::TransactionSubmitResult::Ptr) override
    {}

    std::vector<tars::EndpointInfo> getActiveGatewayEndPoints();
    virtual void subscribeTopicToAllNodes(std::string const& _topicInfo);
    virtual void removeTopicFromAllNodes(std::vector<std::string> const& _topicName);
    std::string endPointToString(std::string const& _serviceName, TC_Endpoint const& _endPoint)
    {
        return _serviceName + "@tcp -h " + _endPoint.getHost() + " -p " +
               boost::lexical_cast<std::string>(_endPoint.getPort());
    }

private:
    std::shared_ptr<boostssl::ws::WsService> m_wsService;
    bcos::rpc::JsonRpcImpl_2_0::Ptr m_jsonRpcImpl;
    bcos::event::EventSub::Ptr m_eventSub;

    std::shared_ptr<bcos::boostssl::ws::WsMessageFactory> m_wsMessageFactory;
    std::shared_ptr<bcos::protocol::AMOPRequestFactory> m_requestFactory;
    bcos::gateway::GatewayInterface::Ptr m_gateway;
    std::string m_clientID;
    std::string m_gatewayServiceName;

    // for AMOP
    std::map<std::string, std::map<std::string, std::shared_ptr<boostssl::ws::WsSession>>>
        m_topicToSessions;
    mutable SharedMutex x_topicToSessions;
};

}  // namespace rpc
}  // namespace bcos