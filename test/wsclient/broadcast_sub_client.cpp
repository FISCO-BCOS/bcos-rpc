//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket client, asynchronous
//
//------------------------------------------------------------------------------

#include <bcos-framework/libutilities/Common.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-rpc/http/ws/WsMessage.h>
#include <bcos-rpc/http/ws/WsMessageType.h>
#include <json/json.h>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

//------------------------------------------------------------------------------

void fail(boost::beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
    std::exit(-1);
}

// AMOP echo sample
class session : public std::enable_shared_from_this<session>
{
private:
    boost::asio::ip::tcp::resolver m_resolver;
    boost::beast::websocket::stream<boost::beast::tcp_stream> m_wsStream;
    boost::beast::flat_buffer m_buffer;
    std::string m_host;
    bcos::ws::WsMessageFactory::Ptr m_messageFactory;
    std::vector<std::shared_ptr<bcos::bytes>> m_queue;

    std::string m_topic;
    std::string m_message;

public:
    bcos::ws::WsMessageFactory::Ptr messageFactory() { return m_messageFactory; }
    void setMessageFactory(bcos::ws::WsMessageFactory::Ptr _messageFactory)
    {
        m_messageFactory = _messageFactory;
    }

    void setTopic(const std::string& _topic) { m_topic = _topic; }
    std::string topic() const { return m_topic; }

    std::string message() const { return m_message; }
    void setMessage(const std::string& _message) { m_message = _message; }

public:
    // Resolver and socket require an io_context
    explicit session(boost::asio::io_context& ioc)
      : m_resolver(boost::asio::make_strand(ioc)), m_wsStream(boost::asio::make_strand(ioc))
    {}

    // Start the asynchronous operation
    void run(char const* host, char const* port)
    {
        // Save these for later
        m_host = host;

        // Look up the domain name
        m_resolver.async_resolve(
            host, port, boost::beast::bind_front_handler(&session::on_resolve, shared_from_this()));
    }

    void on_resolve(
        boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        boost::beast::get_lowest_layer(m_wsStream).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(m_wsStream)
            .async_connect(results,
                boost::beast::bind_front_handler(&session::on_connect, shared_from_this()));
    }

    void on_connect(
        boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep)
    {
        if (ec)
            return fail(ec, "connect");

        // turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        boost::beast::get_lowest_layer(m_wsStream).expires_never();

        // set suggested timeout settings for the websocket
        m_wsStream.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::client));

        // set a decorator to change the User-Agent of the handshake
        m_wsStream.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req) {
                req.set(boost::beast::http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-amop-sample");
            }));

        m_host += ':' + std::to_string(ep.port());

        // perform the websocket handshake
        m_wsStream.async_handshake(m_host, "/",
            boost::beast::bind_front_handler(&session::do_subscribe, shared_from_this()));
    }

    void do_subscribe(boost::beast::error_code ec)
    {
        if (ec)
            return fail(ec, "subscribe topic");

        Json::Value jRequest;
        Json::Value jTopics(Json::arrayValue);

        std::string topic = m_topic;
        jTopics.append(topic);

        jRequest["topics"] = jTopics;

        Json::FastWriter writer;
        std::string request = writer.write(jRequest);

        auto message = m_messageFactory->buildMessage();
        message->setType(bcos::ws::WsMessageType::AMOP_SUBTOPIC);
        message->setData(std::make_shared<bcos::bytes>(request.begin(), request.end()));

        BCOS_LOG(INFO) << "     ==> subscribe topic: " << m_topic << std::endl;
        BCOS_LOG(INFO) << "         ==> seq: "
                       << std::string(message->seq()->begin(), message->seq()->end()) << std::endl;

        auto buffer = std::make_shared<bcos::bytes>();
        message->encode(*buffer);
        m_queue.push_back(buffer);

        // send the message
        m_wsStream.async_write(boost::asio::buffer(*m_queue.front()),
            boost::beast::bind_front_handler(&session::on_subscribe, shared_from_this()));
    }

    void on_subscribe(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        m_queue.erase(m_queue.begin());
        // read a message into our buffer
        m_wsStream.async_read(
            m_buffer, boost::beast::bind_front_handler(&session::on_read, shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        auto data =
            boost::asio::buffer_cast<bcos::byte*>(boost::beast::buffers_front(m_buffer.data()));
        auto size = boost::asio::buffer_size(m_buffer.data());

        auto message = m_messageFactory->buildMessage();
        message->decode(data, size);
        auto seq = std::string(message->seq()->begin(), message->seq()->end());
        auto requestFactory = std::make_shared<bcos::ws::AMOPRequestFactory>();
        auto request = requestFactory->buildRequest();
        request->decode(bcos::bytesConstRef(message->data()->data(), message->data()->size()));

        BCOS_LOG(INFO) << "     receive: ";
        BCOS_LOG(INFO) << "         seq: " << seq;
        BCOS_LOG(INFO) << "         size: " << request->data().size();
        BCOS_LOG(INFO) << "         data: "
                       << std::string(request->data().begin(), request->data().end());
        m_buffer.consume(m_buffer.size());

        // read a message into our buffer
        m_wsStream.async_read(
            m_buffer, boost::beast::bind_front_handler(&session::on_read, shared_from_this()));
    }
};

void usage()
{
    std::cerr << "Usage: broadcast-sub-client <host> <port> <topic>\n"
              << "Example:\n"
              << "    ./broadcast-sub-client 127.0.0.1 20200 topic\n";
    std::exit(0);
}

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc != 4)
    {
        usage();
    }

    auto const host = argv[1];
    auto const port = argv[2];
    std::string topic = argv[3];

    BCOS_LOG(INFO) << LOG_DESC("amop broadcast sample") << LOG_KV("ip", host)
                   << LOG_KV("port", port) << LOG_KV("topic", topic);

    boost::asio::io_context ioc;
    auto s = std::make_shared<session>(ioc);
    auto factory = std::make_shared<bcos::ws::WsMessageFactory>();
    s->setTopic(topic);
    s->setMessageFactory(factory);
    s->run(host, port);

    BCOS_LOG(INFO) << LOG_DESC(" ==> subscible ") << LOG_KV("topic", topic);

    // run the I/O service. the call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
}