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
#include <bcos-rpc/http/HttpServer.h>
#include <bcos-rpc/http/ws/WsService.h>
#include <bcos-rpc/jsonrpc/Common.h>
#include <iterator>
#include <thread>

namespace bcos
{
namespace rpc
{
class Rpc : public RPCInterface,
            public amop::AMOPInterface,
            public std::enable_shared_from_this<Rpc>
{
public:
    using Ptr = std::shared_ptr<Rpc>;

    Rpc() = default;
    virtual ~Rpc() { stop(); }

public:
    virtual void start() override;
    virtual void stop() override;

    void startThread()
    {
        // start threads
        if (m_threads->empty())
        {
            auto& ioc = m_ioc;
            m_threads->reserve(m_threadC);
            for (auto i = m_threadC; i > 0; --i)
            {
                m_threads->emplace_back([&ioc]() { ioc->run(); });
            }
        }
    }

public:
    /**
     * @brief: notify blockNumber to rpc
     * @param _blockNumber: blockNumber
     * @param _callback: resp callback
     * @return void
     */
    virtual void asyncNotifyBlockNumber(bcos::protocol::BlockNumber _blockNumber,
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
        std::function<void(Error::Ptr&&)> _callback) override
    {
        m_wsService->jsonRpcInterface()->updateGroupInfo(_groupInfo);
        BCOS_LOG(INFO) << LOG_DESC("asyncNotifyGroupInfo: update the groupInfo")
                       << printGroupInfo(_groupInfo);
        if (_callback)
        {
            _callback(nullptr);
        }
    }

public:
    bcos::http::HttpServer::Ptr httpServer() const { return m_httpServer; }
    void setHttpServer(bcos::http::HttpServer::Ptr _httpServer) { m_httpServer = _httpServer; }

    bcos::ws::WsService::Ptr wsService() const { return m_wsService; }
    void setWsService(bcos::ws::WsService::Ptr _wsService) { m_wsService = _wsService; }

    bcos::amop::AMOP::Ptr AMOP() const { return m_AMOP; }
    void setAMOP(bcos::amop::AMOP::Ptr _AMOP) { m_AMOP = _AMOP; }

    std::shared_ptr<boost::asio::io_context> ioc() const { return m_ioc; }
    void setIoc(std::shared_ptr<boost::asio::io_context> _ioc) { m_ioc = _ioc; }

    void setThreadC(std::size_t _threadC) { m_threadC = _threadC; }
    std::size_t threadC() const { return m_threadC; }

    std::shared_ptr<std::vector<std::thread>> threads() const { return m_threads; }
    void setThreads(std::shared_ptr<std::vector<std::thread>> _threads) { m_threads = _threads; }

private:
    bcos::http::HttpServer::Ptr m_httpServer;
    bcos::ws::WsService::Ptr m_wsService;
    bcos::amop::AMOP::Ptr m_AMOP;
    std::shared_ptr<boost::asio::io_context> m_ioc;

    std::size_t m_threadC;
    std::shared_ptr<std::vector<std::thread>> m_threads;
};

}  // namespace rpc
}  // namespace bcos