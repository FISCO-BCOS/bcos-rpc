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
#include <bcos-framework/interfaces/amop/AMOPInterface.h>
#include <bcos-framework/interfaces/rpc/RPCInterface.h>
#include <bcos-rpc/amop/AMOP.h>
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
class Rpc : public RPCInterface,
            public amop::AMOPInterface,
            public std::enable_shared_from_this<Rpc>
{
public:
    using Ptr = std::shared_ptr<Rpc>;

    Rpc(std::shared_ptr<boostssl::ws::WsService> _wsService,
        bcos::rpc::JsonRpcImpl_2_0::Ptr _jsonRpcImpl, bcos::event::EventSub::Ptr _eventSub,
        bcos::amop::AMOP::Ptr _AMOP)
      : m_wsService(_wsService), m_jsonRpcImpl(_jsonRpcImpl), m_eventSub(_eventSub), m_AMOP(_AMOP)
    {}

    virtual ~Rpc() { stop(); }

    virtual void start() override;
    virtual void stop() override;

    virtual void init();

    /**
     * @brief: notify blockNumber to rpc
     * @param _blockNumber: blockNumber
     * @param _callback: resp callback
     * @return void
     */
    virtual void asyncNotifyBlockNumber(std::string const& _groupID, std::string const& _nodeName,
        bcos::protocol::BlockNumber _blockNumber,
        std::function<void(Error::Ptr)> _callback) override;

    /**
     * @brief: async receive message from front service
     * @param _nodeID: the message sender nodeID
     * @param _id: the id of this message, it can by used to send response to the peer
     * @param _data: the message data
     * @return void
     */
    virtual void asyncNotifyAmopMessage(bcos::crypto::NodeIDPtr _nodeID, const std::string& _id,
        bcos::bytesConstRef _data, std::function<void(Error::Ptr _error)> _onRecv) override;
    /**
     * @brief: async receive nodeIDs from front service
     * @param _nodeIDs: the nodeIDs
     * @param _callback: callback
     * @return void
     */
    virtual void asyncNotifyAmopNodeIDs(std::shared_ptr<const bcos::crypto::NodeIDs> _nodeIDs,
        std::function<void(bcos::Error::Ptr _error)> _callback) override;

    void asyncNotifyGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo,
        std::function<void(Error::Ptr&&)> _callback) override;

    std::shared_ptr<boostssl::ws::WsService> wsService() const { return m_wsService; }
    bcos::amop::AMOP::Ptr AMOP() const { return m_AMOP; }
    bcos::rpc::JsonRpcImpl_2_0::Ptr jsonRpcImpl() const { return m_jsonRpcImpl; }
    bcos::event::EventSub::Ptr eventSub() const { return m_eventSub; }

protected:
    virtual void notifyGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo);

private:
    std::shared_ptr<boostssl::ws::WsService> m_wsService;
    bcos::rpc::JsonRpcImpl_2_0::Ptr m_jsonRpcImpl;
    bcos::event::EventSub::Ptr m_eventSub;
    bcos::amop::AMOP::Ptr m_AMOP;
};

}  // namespace rpc
}  // namespace bcos