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
 * @brief AMOP client
 * @file AMOPClient.cpp
 * @author: yujiechen
 * @date 2021-10-28
 */
#include "AMOPClient.h"
#include <bcos-tars-protocol/client/GatewayServiceClient.h>
#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/interfaces/gateway/GatewayTypeDef.h>
#include <bcos-framework/libprotocol/amop/TopicItem.h>
#include <bcos-rpc/Common.h>
using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::boostssl::ws;
using namespace bcostars;
using namespace tars;
using namespace bcos::gateway;
using namespace bcos::protocol;

void AMOPClient::initMsgHandler()
{
    m_wsService->registerMsgHandler(AMOPClientMessageType::AMOP_SUBTOPIC,
        boost::bind(&AMOPClient::onRecvSubTopics, this, boost::placeholders::_1, boost::placeholders::_2));
    m_wsService->registerMsgHandler(
        AMOPClientMessageType::AMOP_REQUEST, boost::bind(&AMOPClient::onRecvAMOPRequest, this,
                                           boost::placeholders::_1, boost::placeholders::_2));
    m_wsService->registerMsgHandler(
        AMOPClientMessageType::AMOP_BROADCAST, boost::bind(&AMOPClient::onRecvAMOPBroadcast, this,
                                             boost::placeholders::_1, boost::placeholders::_2));
    m_wsService->registerDisconnectHandler(
        boost::bind(&AMOPClient::onClientDisconnect, this, boost::placeholders::_1));
}

bool AMOPClient::updateTopicInfos(
    std::string const& _topicInfo, std::shared_ptr<WsSession> _session)
{
    TopicItems topicItems;
    auto ret = parseSubTopicsJson(_topicInfo, topicItems);
    if (!ret)
    {
        return false;
    }
    WriteGuard l(x_topicToSessions);
    for (auto const& item : topicItems)
    {
        m_topicToSessions[item.topicName()][_session->endPoint()] = _session;
    }
    return true;
}
/**
 * @brief: receive sub topic message from sdk
 */
void AMOPClient::onRecvSubTopics(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto topicInfo = std::string(_msg->data()->begin(), _msg->data()->end());
    auto ret = updateTopicInfos(topicInfo, _session);
    if (!ret)
    {
        BCOS_LOG(WARNING) << LOG_BADGE("onRecvSubTopics: invalid topic info")
                          << LOG_KV("topicInfo", topicInfo)
                          << LOG_KV("endpoint", _session->endPoint());
        return;
    }
    subscribeTopicToAllNodes(topicInfo);
    BCOS_LOG(INFO) << LOG_BADGE("onRecvSubTopics") << LOG_KV("topicInfo", topicInfo)
                   << LOG_KV("endpoint", _session->endPoint());
}

/**
 * @brief: receive amop request message from sdk
 */
void AMOPClient::onRecvAMOPRequest(
    std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession> _session)
{
    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());
    auto msgData = std::make_shared<bytes>();
    _msg->encode(*msgData);

    auto amopReq =
        m_requestFactory->buildRequest(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    m_gateway->asyncSendMessageByTopic(amopReq->topic(),
        bytesConstRef(msgData->data(), msgData->size()),
        [this, seq, _session](
            bcos::Error::Ptr&& _error, int16_t _type, bytesPointer _responseData) {
            try
            {
                auto responseMsg = m_wsMessageFactory->buildMessage();
                if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                {
                    responseMsg->setStatus(_error->errorCode());
                    BCOS_LOG(ERROR) << LOG_BADGE("onRecvAMOPRequest error")
                                    << LOG_DESC("AMOP async send message callback")
                                    << LOG_KV("seq", seq) << LOG_KV("code", _error->errorCode())
                                    << LOG_KV("msg", _error->errorMessage());
                    _session->asyncSendMessage(responseMsg);
                    return;
                }
                if (_type == bcos::gateway::MessageType::AMOPMessageType)
                {
                    responseMsg->setData(_responseData);
                    BCOS_LOG(INFO) << LOG_BADGE("onRecvAMOPRequest")
                                   << LOG_DESC("receive AMOP response data from the p2pNode");
                }
                if (_type == bcos::gateway::MessageType::WSMessageType)
                {
                    auto size = responseMsg->decode(_responseData->data(), _responseData->size());
                    BCOS_LOG(DEBUG)
                        << LOG_BADGE("onRecvAMOPRequest")
                        << LOG_DESC("AMOP async send message: receive message response for sdk")
                        << LOG_KV("seq", seq) << LOG_KV("size", size);
                }
                else
                {
                    responseMsg->setStatus(bcos::protocol::CommonError::UnSupportedPacketType);
                    BCOS_LOG(WARNING) << LOG_DESC("onRecvAMOPRequest: unknown packet type")
                                      << LOG_KV("type", _type);
                }
                _session->asyncSendMessage(responseMsg);
            }
            catch (std::exception const& e)
            {
                BCOS_LOG(WARNING) << LOG_DESC("onRecvAMOPRequest exception")
                                  << LOG_KV("error", boost::diagnostic_information(e));
            }
        });
}

/**
 * @brief: receive amop broadcast message from sdk
 */
void AMOPClient::onRecvAMOPBroadcast(std::shared_ptr<WsMessage> _msg, std::shared_ptr<WsSession>)
{
    auto seq = std::string(_msg->seq()->begin(), _msg->seq()->end());
    auto msgData = std::make_shared<bytes>();
    _msg->encode(*msgData);

    auto amopReq =
        m_requestFactory->buildRequest(bytesConstRef(_msg->data()->data(), _msg->data()->size()));
    m_gateway->asyncSendBroadbastMessageByTopic(
        amopReq->topic(), bytesConstRef(msgData->data(), msgData->size()));
    BCOS_LOG(DEBUG) << LOG_BADGE("onRecvAMOPBroadcast") << LOG_KV("seq", seq);
}

void AMOPClient::asyncNotifyAMOPMessage(std::string const& _topic, bytesConstRef _amopRequestData,
    std::function<void(Error::Ptr&&, bytesPointer)> _callback)
{
    auto clientSession = randomChooseSession(_topic);
    auto buffer = std::make_shared<bcos::bytes>();
    if (!clientSession)
    {
        auto responseMessage = m_wsMessageFactory->buildMessage();
        responseMessage->setStatus(bcos::protocol::CommonError::NotFoundClientByTopicDispatchMsg);
        responseMessage->setType(AMOPClientMessageType::AMOP_RESPONSE);
        responseMessage->encode(*buffer);
        _callback(std::make_shared<Error>(CommonError::NotFoundClientByTopicDispatchMsg,
                      "NotFoundClientByTopicDispatchMsg"),
            buffer);
        return;
    }
    auto requestMsg = m_wsMessageFactory->buildMessage();
    requestMsg->decode(_amopRequestData.data(), _amopRequestData.size());
    clientSession->asyncSendMessage(requestMsg, Options(30000),
        [_topic, buffer, _callback](bcos::Error::Ptr _error,
            std::shared_ptr<WsMessage> _responseMsg, std::shared_ptr<WsSession> _session) {
            // try again when send message to the session failed
            if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
            {
                BCOS_LOG(WARNING) << LOG_BADGE("asyncNotifyAMOPMessage")
                                  << LOG_DESC("asyncSendMessage callback error")
                                  << LOG_KV("endpoint",
                                         (_session ? _session->endPoint() : std::string("")))
                                  << LOG_KV("topic", _topic)
                                  << LOG_KV("errorCode", _error ? _error->errorCode() : -1)
                                  << LOG_KV("errorMessage",
                                         _error ? _error->errorMessage() : "success");
            }
            auto seq = std::string(_responseMsg->seq()->begin(), _responseMsg->seq()->end());
            BCOS_LOG(TRACE) << LOG_BADGE("asyncNotifyAMOPMessage")
                            << LOG_DESC("asyncSendMessage callback response") << LOG_KV("seq", seq)
                            << LOG_KV("data size", _responseMsg->data()->size());

            _responseMsg->encode(*buffer);
            _callback(std::move(_error), buffer);
        });
}

void AMOPClient::asyncNotifyAMOPBroadcastMessage(std::string const& _topic, bytesConstRef _data,
    std::function<void(Error::Ptr&&, bytesPointer)> _callback)
{
    auto sessions = querySessionsByTopic(_topic);
    auto requestMsg = m_wsMessageFactory->buildMessage();
    requestMsg->decode(_data.data(), _data.size());
    for (auto const& session : sessions)
    {
        session.second->asyncSendMessage(requestMsg, Options(30000));
    }
    if (_callback)
    {
        _callback(nullptr, nullptr);
    }
}

std::shared_ptr<WsSession> AMOPClient::randomChooseSession(std::string const& _topic)
{
    ReadGuard l(x_topicToSessions);
    if (!m_topicToSessions.count(_topic))
    {
        return nullptr;
    }
    std::shared_ptr<WsSession> selectedSession;
    auto const& sessions = m_topicToSessions[_topic];
    do
    {
        srand(utcTime());
        auto selectedClient = rand() % m_topicToSessions.size();
        auto it = sessions.begin();
        std::advance(it, selectedClient);
        selectedSession = it->second;
    } while (selectedSession && selectedSession->isConnected());
    return selectedSession;
}

void AMOPClient::onClientDisconnect(std::shared_ptr<WsSession> _session)
{
    std::vector<std::string> topicsToRemove;
    {
        WriteGuard l(x_topicToSessions);
        for (auto it = m_topicToSessions.begin(); it != m_topicToSessions.end();)
        {
            auto& sessions = it->second;
            if (sessions.count(_session->endPoint()))
            {
                sessions.erase(_session->endPoint());
            }
            if (sessions.size() == 0)
            {
                topicsToRemove.emplace_back(it->first);
                it = m_topicToSessions.erase(it);
                continue;
            }
            it++;
        }
    }
    if (topicsToRemove.size() == 0)
    {
        return;
    }
    removeTopicFromAllNodes(topicsToRemove);
}

std::vector<tars::EndpointInfo> AMOPClient::getActiveGatewayEndPoints()
{
    vector<EndpointInfo> activeEndPoints;
    vector<EndpointInfo> nactiveEndPoints;
    auto gatewayClient = std::dynamic_pointer_cast<bcostars::GatewayServiceClient>(m_gateway);
    gatewayClient->prx()->tars_endpointsAll(activeEndPoints, nactiveEndPoints);
    return activeEndPoints;
}

void AMOPClient::subscribeTopicToAllNodes(std::string const& _topicInfo)
{
    auto activeEndPoints = getActiveGatewayEndPoints();
    for (auto const& endPoint : activeEndPoints)
    {
        auto endPointStr = endPointToString(m_gatewayServiceName, endPoint.getEndpoint());
        auto servicePrx =
            Application::getCommunicator()->stringToProxy<GatewayServicePrx>(endPointStr);
        auto serviceClient = std::make_shared<GatewayServiceClient>(servicePrx);
        serviceClient->asyncSubscribeTopic(
            m_clientID, _topicInfo, [endPointStr](Error::Ptr&& _error) {
                if (_error)
                {
                    BCOS_LOG(WARNING)
                        << LOG_DESC("asyncSubScribeTopic error") << LOG_KV("gateway", endPointStr)
                        << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage());
                }
            });
    }
}
void AMOPClient::removeTopicFromAllNodes(std::vector<std::string> const& topicsToRemove)
{
    auto activeEndPoints = getActiveGatewayEndPoints();
    for (auto const& endPoint : activeEndPoints)
    {
        auto endPointStr = endPointToString(m_gatewayServiceName, endPoint.getEndpoint());
        auto servicePrx =
            Application::getCommunicator()->stringToProxy<GatewayServicePrx>(endPointStr);
        auto serviceClient = std::make_shared<GatewayServiceClient>(servicePrx);
        serviceClient->asyncRemoveTopic(
            m_clientID, topicsToRemove, [topicsToRemove, endPointStr](Error::Ptr&& _error) {
                BCOS_LOG(INFO) << LOG_DESC("asyncRemoveTopic") << LOG_KV("gateway", endPointStr)
                               << LOG_KV("removedSize", topicsToRemove.size())
                               << LOG_KV("code", _error ? _error->errorCode() : 0)
                               << LOG_KV("msg", _error ? _error->errorMessage() : "");
            });
    }
}