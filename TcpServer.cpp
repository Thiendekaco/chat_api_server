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

void TcpServer::handleClient(std::shared_ptr<tcp::socket> socket) {
    try {
        for (;;) {
            char data[1024];
            boost::system::error_code error;
            size_t length = socket->read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof) break; // Connection closed cleanly by peer.
            else if (error) throw boost::system::system_error(error); // Some other error.

            // Process the received message
            std::string message(data, length);
            processMessage(message, socket);
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

void TcpServer::processMessage(const std::string& message, std::shared_ptr<tcp::socket> socket) {
    try {
        json jsonMessage = json::parse(message);
        std::string type = jsonMessage["type"];

        if (type == "connect") {
            handleConnect(jsonMessage, socket);
        }
        else if (type == "disconnect") {
            handleDisconnect(jsonMessage, socket);
        }
        else if (type == "message") {
            handleMessage(jsonMessage, socket);
        }
        else if (type == "joinRoom") {
            handleJoinRoom(jsonMessage, socket);
        }
        else if (type == "leaveRoom") {
            handleLeaveRoom(jsonMessage, socket);
        }
        else if (type == "typing") {
            handleTyping(jsonMessage, socket);
        }
        else if (type == "stopTyping") {
            handleStopTyping(jsonMessage, socket);
        }
        else if (type == "userStatus") {
            handleUserStatus(jsonMessage, socket);
        }
        else if (type == "messageReceipt") {
            handleMessageReceipt(jsonMessage, socket);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to process message: " << e.what() << "\n";
    }
}

void TcpServer::handleConnect(const json& message, std::shared_ptr<tcp::socket> socket) {
    try {
        std::string token = message["token"];
        if (!isTokenValid(token)) {
            std::cerr << "Invalid token. Closing connection.\n";
            socket->close();
            return;
        }

        // Handle client connection
        std::cout << "Client connected: " << message["username"] << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to handle connect: " << e.what() << "\n";
        socket->close();
    }
}

void TcpServer::handleDisconnect(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle client disconnection
    std::cout << "Client disconnected: " << message["username"] << "\n";
}

void TcpServer::handleMessage(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle message sending/receiving
    std::cout << "Message from " << message["username"] << ": " << message["content"] << "\n";
}

void TcpServer::handleJoinRoom(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle joining a chat room
    std::cout << "Client " << message["username"] << " joined room: " << message["room"] << "\n";
}

void TcpServer::handleLeaveRoom(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle leaving a chat room
    std::cout << "Client " << message["username"] << " left room: " << message["room"] << "\n";
}

void TcpServer::handleTyping(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle typing status
    std::cout << "Client " << message["username"] << " is typing in room: " << message["room"] << "\n";
}

void TcpServer::handleStopTyping(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle stop typing status
    std::cout << "Client " << message["username"] << " stopped typing in room: " << message["room"] << "\n";
}

void TcpServer::handleUserStatus(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle user status update
    std::cout << "User status update: " << message["username"] << " is " << message["status"] << "\n";
}

void TcpServer::handleMessageReceipt(const json& message, std::shared_ptr<tcp::socket> socket) {
    // Handle message receipt confirmation
    std::cout << "Message receipt from " << message["username"] << " for message ID: " << message["messageId"] << "\n";
}

bool TcpServer::isTokenValid(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ "secret" })
            .with_issuer("auth0");

        verifier.verify(decoded);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Token validation failed: " << e.what() << "\n";
        return false;
    }
}