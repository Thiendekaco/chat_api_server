#include "TcpServer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;

TcpServer::TcpServer(boost::asio::io_context& io_context, short port, ThreadPool& threadPool)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), threadPool_(threadPool) {
    socket_ = std::make_shared<tcp::socket>(io_context);
    doAccept();
}

void TcpServer::doAccept() {
    acceptor_.async_accept(*socket_,
        [this](boost::system::error_code ec) {
            if (!ec) {
                threadPool_.enqueueTask([this]() mutable {
                    handleClient();
                });
            }
            doAccept();
        });
}

void TcpServer::handleClient() {
    try {
        for (;;) {
            char data[1024];
            boost::system::error_code error;
            size_t length = socket_->read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof) break; // Connection closed cleanly by peer.
            else if (error) throw boost::system::system_error(error); // Some other error.

            // Process data and send response
            std::string response = "Received: " + std::string(data, length);
            boost::asio::write(*socket_, boost::asio::buffer(response), error);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}