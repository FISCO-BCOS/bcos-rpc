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

#include "libutilities/Common.h"
#include <bcos-rpc/http/ws/WsMessage.h>
#include <bcos-rpc/http/ws/WsMessageType.h>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

//------------------------------------------------------------------------------

// Report a failure
void fail(boost::beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
private:
    boost::asio::ip::tcp::resolver m_resolver;
    boost::beast::websocket::stream<boost::beast::tcp_stream> m_wsStream;
    boost::beast::flat_buffer m_buffer;
    std::string host_;
    bcos::ws::WsMessageFactory::Ptr m_wsMessageFactory;
    std::vector<std::shared_ptr<bcos::bytes>> m_queue;

public:
    bcos::ws::WsMessageFactory::Ptr wsMessageFactory() { return m_wsMessageFactory; }
    void setWsMessageFactory(bcos::ws::WsMessageFactory::Ptr _wsMessageFactory)
    {
        m_wsMessageFactory = _wsMessageFactory;
    }

public:
    // Resolver and socket require an io_context
    explicit session(boost::asio::io_context& ioc)
      : m_resolver(boost::asio::make_strand(ioc)), m_wsStream(boost::asio::make_strand(ioc))
    {}

    // Start the asynchronous operation
    void run(char const* host, char const* port)
    {
        // Save these for later
        host_ = host;

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

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        boost::beast::get_lowest_layer(m_wsStream).expires_never();

        // Set suggested timeout settings for the websocket
        m_wsStream.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        m_wsStream.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req) {
                req.set(boost::beast::http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
            }));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the websocket handshake
        m_wsStream.async_handshake(host_, "/",
            boost::beast::bind_front_handler(&session::on_handshake, shared_from_this()));
    }

    void on_handshake(boost::beast::error_code ec)
    {
        if (ec)
            return fail(ec, "handshake");

        auto message = m_wsMessageFactory->buildMessage();
        message->setType(bcos::ws::WsMessageType::HANDESHAKE);
        /*
        std::string request =
            "{\"jsonrpc\":\"2.0\",\"method\":\"getNodeInfo\",\"params\":[],\"id\":1}";
        message->setData(std::make_shared<bcos::bytes>(request.begin(), request.end()));
        */
        auto buffer = std::make_shared<bcos::bytes>();
        message->encode(*buffer);
        m_queue.push_back(buffer);

        // Send the message
        m_wsStream.async_write(boost::asio::buffer(*m_queue.front()),
            boost::beast::bind_front_handler(&session::on_write, shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        m_queue.erase(m_queue.begin());
        // Read a message into our buffer
        m_wsStream.async_read(
            m_buffer, boost::beast::bind_front_handler(&session::on_read, shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        // Close the WebSocket connection
        m_wsStream.async_close(boost::beast::websocket::close_code::normal,
            boost::beast::bind_front_handler(&session::on_close, shared_from_this()));
    }

    void on_close(boost::beast::error_code ec)
    {
        if (ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << boost::beast::make_printable(m_buffer.data()) << std::endl;
    }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc != 3)
    {
        std::cerr << "Usage: ws-client <host> <port>\n"
                  << "Example:\n"
                  << "    ./ws-client 127.0.0.1 20200\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    auto messageFactory = std::make_shared<bcos::ws::WsMessageFactory>();

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(host, port);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
}