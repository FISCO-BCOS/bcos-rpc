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
 *  m_limitations under the License.
 *
 * @file HttpHttpServer.h
 * @author: octopus
 * @date 2021-07-08
 */
#pragma once

#include <bcos-rpc/rpc/http/HttpSession.h>
#include <exception>
#include <thread>

namespace bcos
{
namespace http
{
// The http server impl
class HttpServer : public std::enable_shared_from_this<HttpServer>
{
public:
    using Ptr = std::shared_ptr<HttpServer>;

public:
    HttpServer(const std::string& _listenIP, uint16_t _listenPort)
      : m_listenIP(_listenIP), m_listenPort(_listenPort)
    {}

    ~HttpServer() { stop(); }

public:
    // start http server
    void startListen();
    void stop();

    // accept connection
    void doAccept();
    // handle connection
    void onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

public:
    RequestHandler requestHandler() const { return m_requestHandler; }
    void setRequestHandler(RequestHandler requestHandler) { m_requestHandler = requestHandler; }
    HttpSessionFactory::Ptr sessionFactory() const { return m_sessionFactory; }
    void setSessionFactory(HttpSessionFactory::Ptr sessionFactory)
    {
        m_sessionFactory = sessionFactory;
    }
    std::shared_ptr<boost::asio::io_context> ioc() const { return m_ioc; }
    void setIoc(std::shared_ptr<boost::asio::io_context> _ioc) { m_ioc = _ioc; }
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor() const { return m_acceptor; }
    void setAcceptor(std::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor)
    {
        m_acceptor = _acceptor;
    }
    std::shared_ptr<std::vector<std::thread>> threads() { return m_threads; }
    void setThreads(std::shared_ptr<std::vector<std::thread>> _threads) { m_threads = _threads; }

    void setThreadCount(uint32_t _threadCount) { m_threadCount = _threadCount; }
    uint32_t getThreadCount() { return m_threadCount; }

private:
    std::string m_listenIP;
    uint16_t m_listenPort;
    uint32_t m_threadCount;
    RequestHandler m_requestHandler;

    HttpSessionFactory::Ptr m_sessionFactory;
    std::shared_ptr<boost::asio::io_context> m_ioc;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
    std::shared_ptr<std::vector<std::thread>> m_threads;
};

// The http server factory
class HttpServerFactory : public std::enable_shared_from_this<HttpServerFactory>
{
public:
    using Ptr = std::shared_ptr<HttpServerFactory>;

public:
    /**
     * @brief: create http server
     * @param _listenIP: listen ip
     * @param _listenPort: listen port
     * @param _threadCount: thread count
     * @return HttpServer::Ptr:
     */
    HttpServer::Ptr buildHttpServer(
        const std::string& _listenIP, uint16_t _listenPort, std::size_t _threadCount);
};

}  // namespace http
}  // namespace bcos