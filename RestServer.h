#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include "ThreadPool.h"
#include "include/nlohmann/json.hpp"
#include <jwt-cpp/jwt.h>

#ifndef RESTSERVER_H
#define RESTSERVER_H

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class RestServer {
public:
    RestServer(net::io_context& ioc, tcp::endpoint endpoint, ThreadPool& threadPool);

private:
    void doAccept();
    void handleRequest(std::shared_ptr<tcp::socket> socket);
    void fail(beast::error_code ec, char const* what);

    // New methods for handling API requests
    json handleLogin(const http::request<http::string_body>& req);
    json handleRegister(const http::request<http::string_body>& req);
    json handleLogout(const http::request<http::string_body>& req);
    json handleGetUsers(const http::request<http::string_body>& req);
    json handleGetRooms(const http::request<http::string_body>& req);
    json handleGetUsers(const http::request<http::string_body>& req);
    json handleGetMessages(const http::request<http::string_body>& req, const std::string& roomId);
    std::string hashPassword(const std::string& password);

    std::string generateToken(const std::string& email);
    bool checkPassword(const std::string& password, const std::string& hash);
    tcp::acceptor acceptor_;
    ThreadPool& threadPool_;
};

#endif // RESTSERVER_H
