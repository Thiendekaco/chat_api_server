#include "RestServer.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

RestServer::RestServer(net::io_context& ioc, tcp::endpoint endpoint, ThreadPool& threadPool)
    : acceptor_(ioc), threadPool_(threadPool) {

    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
    doAccept();
}

void RestServer::doAccept() {
    auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
    acceptor_.async_accept(
        *socket,
        [this, socket](boost::system::error_code ec) {  // Capture socket đúng cách
            if (!ec) {
                threadPool_.enqueueTask([this, socket = std::move(socket)]() mutable {
                    handleRequest(socket);
                });
            } else {
                std::cerr << "Failed to accept connection: " << ec.message() << std::endl;
            }
            doAccept();
        });
}


void RestServer::handleRequest(std::shared_ptr<tcp::socket> socket) {
    try {
        std::cout << "Handling request\n";
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(*socket, buffer, req);

        if (req.method() == http::verb::get && req.target() == "/login") {
            // Handle login
        } else if (req.method() == http::verb::post && req.target() == "/send_message") {
            // Handle send message
        } else {
            // Handle other requests
        }

        // Send response
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Request processed";
        res.prepare_payload();
        http::write(*socket, res);
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}
void RestServer::fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}