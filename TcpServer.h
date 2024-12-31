#pragma once
#include <boost/asio.hpp>
#include "ThreadPool.h"
#include "include/nlohmann/json.hpp"

#ifndef TCPSERVER_H
#define TCPSERVER_H

using boost::asio::ip::tcp;

class TcpServer {
public:
    TcpServer(boost::asio::io_context& io_context, short port, ThreadPool& threadPool);

private:
    void doAccept();
    void handleClient();

    void processMessage(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void handleConnect(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleDisconnect(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleMessage(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleJoinRoom(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleLeaveRoom(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleTyping(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleStopTyping(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleUserStatus(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleMessageReceipt(const json& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    bool isTokenValid(const std::string& token);
    tcp::acceptor acceptor_; // Member variable to accept incoming connections
    ThreadPool& threadPool_;
    std::shared_ptr<tcp::socket> socket_; // Member variable for the socket
};

#endif // TCPSERVER_H