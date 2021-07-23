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
 * @file HttpSession.h
 * @author: octopus
 * @date 2021-07-08
 */
#pragma once
#include <bcos-rpc/rpc/http/Common.h>
#include <bcos-rpc/rpc/http/HttpQueue.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket.hpp>

namespace bcos
{
namespace http
{
// The http session for connection
class HttpSession : public std::enable_shared_from_this<HttpSession>
{
public:
    using Ptr = std::shared_ptr<HttpSession>;

public:
    HttpSession(boost::asio::ip::tcp::socket&& socket) : m_stream(std::move(socket))
    {
        HTTP_SESSION(DEBUG) << LOG_KV("[NEWOBJ][HTTPSESSION]", this);
    }

    virtual ~HttpSession()
    {
        doClose();
        HTTP_SESSION(DEBUG) << LOG_KV("[DELOBJ][HTTPSESSION]", this);
    }

    // start the HttpSession
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this HttpSession. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(m_stream.get_executor(),
            boost::beast::bind_front_handler(&HttpSession::doRead, shared_from_this()));
    }

    void doRead()
    {
        m_parser.emplace();
        // set limit to http request size, 10m
        m_parser->body_limit(10 * 1024 * 1024);
        // set the timeout, 60s
        m_stream.expires_after(std::chrono::seconds(60));
        // read the coming http request
        boost::beast::http::async_read(m_stream, m_buffer, *m_parser,
            boost::beast::bind_front_handler(&HttpSession::onRead, shared_from_this()));
    }

    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // the peer client closed the connection
        if (ec == boost::beast::http::error::end_of_stream)
        {
            HTTP_SESSION(TRACE) << LOG_BADGE("onRead") << LOG_DESC("end of stream");
            return doClose();
        }

        if (ec)
        {
            HTTP_SESSION(ERROR) << LOG_BADGE("onRead") << LOG_DESC("close the connection")
                                << LOG_KV("error", ec);
            return doClose();
        }

        if (boost::beast::websocket::is_upgrade(m_parser->get()))
        {
            // TODO: See if it is a WebSocket Upgrade
            HTTP_SESSION(INFO) << LOG_BADGE("onRead") << LOG_DESC("websocket upgrade");
        }

        auto self = std::weak_ptr<HttpSession>(shared_from_this());
        handleRequest(m_parser->release(), [self](auto&& msg) {
            auto session = self.lock();
            if (!session)
            {
                HTTP_SESSION(WARNING) << LOG_DESC("session is not exist");
                return;
            }

            // put the response into the queue and waiting to be send
            session->queue()->enqueue(msg);
        });

        if (!m_queue->isFull())
        {
            doRead();
        }
    }

    void onWrite(bool close, boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            HTTP_SESSION(ERROR) << LOG_BADGE("onWrite") << LOG_DESC("close the connection")
                                << LOG_KV("error", ec);
            return doClose();
        }

        if (close)
        {
            // we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return doClose();
        }

        if (m_queue->onWrite())
        {
            // read the next request
            doRead();
        }
    }

    void doClose()
    {
        boost::beast::error_code ec;
        m_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // at this point the connection is closed gracefully
    }

    /**
     * @brief: handle http request and send the response
     * @param req: http request object
     * @param send: http response sender
     * @return void:
     */
    template <class Send>
    void handleRequest(HttpRequest&& req, Send&& send)
    {
        HTTP_SESSION(DEBUG) << LOG_BADGE("handleRequest") << LOG_DESC("request")
                            << LOG_KV("method", req.method_string())
                            << LOG_KV("target", req.target()) << LOG_KV("body", req.body())
                            << LOG_KV("keep_alive", req.keep_alive())
                            << LOG_KV("need_eof", req.need_eof());

        std::chrono::high_resolution_clock::time_point start =
            std::chrono::high_resolution_clock::now();

        std::string request = req.body();

        m_requestHandler(request, [this, req, send, start](const std::string& _content) {
            std::chrono::high_resolution_clock::time_point end =
                std::chrono::high_resolution_clock::now();

            auto resp = buildHttpResp(req, _content);
            send(resp);

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            HTTP_SESSION(DEBUG) << LOG_BADGE("handleRequest") << LOG_DESC("response")
                                << LOG_KV("body", resp->body())
                                << LOG_KV("keep_alive", resp->keep_alive()) << LOG_KV("ms", ms);
        });
    }

    /**
     * @brief: build http response object
     * @param req: http request object
     * @param content: http response content
     * @return HttpResponsePtr:
     */
    HttpResponsePtr buildHttpResp(const HttpRequest& req, boost::beast::string_view content)
    {
        auto msg = std::make_shared<HttpResponse>(boost::beast::http::status::ok, req.version());
        msg->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        msg->set(boost::beast::http::field::content_type, "application/json");
        msg->keep_alive(req.keep_alive());
        msg->body() = std::string(content);
        msg->prepare_payload();
        return msg;
    }

    boost::beast::tcp_stream& stream() { return m_stream; }
    RequestHandler requestHandler() const { return m_requestHandler; }
    void setRequestHandler(RequestHandler requestHandler) { m_requestHandler = requestHandler; }
    std::shared_ptr<Queue> queue() { return m_queue; }
    void setQueue(std::shared_ptr<Queue> _queue) { m_queue = _queue; }

private:
    boost::beast::tcp_stream m_stream;
    boost::beast::flat_buffer m_buffer;
    RequestHandler m_requestHandler;
    std::shared_ptr<Queue> m_queue;

    // the parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_parser;
};

class HttpSessionFactory : public std::enable_shared_from_this<HttpSessionFactory>
{
public:
    using Ptr = std::shared_ptr<HttpSessionFactory>;

public:
    /**
     * @brief: create http session
     * @param socket: socket
     * @return HttpSession::Ptr:
     */
    HttpSession::Ptr createSession(boost::asio::ip::tcp::socket&& socket)
    {
        auto session = std::make_shared<bcos::http::HttpSession>(std::move(socket));
        auto queue = std::make_shared<bcos::http::Queue>();
        auto self = std::weak_ptr<bcos::http::HttpSession>(session);
        queue->setSender([self](bcos::http::HttpResponsePtr resp) {
            auto session = self.lock();
            if (!session)
            {
                return;
            }

            HTTP_SESSION(DEBUG) << LOG_BADGE("Queue::Write") << LOG_KV("resp", resp->body())
                                << LOG_KV("keep_alive", resp->keep_alive());

            boost::beast::http::async_write(session->stream(), *resp,
                boost::beast::bind_front_handler(
                    &bcos::http::HttpSession::onWrite, session, resp->need_eof()));
        });

        session->setQueue(queue);
        return session;
    }
};
}  // namespace http
}  // namespace bcos