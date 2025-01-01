#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include "ThreadPool.h"
#include <nlohmann/json.hpp> // For JSON handling
#include <jwt-cpp/jwt.h> // For JWT handling

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

/*
    The RestServer class is responsible for handling RESTful API requests.
    It listens for incoming HTTP requests and processes them accordingly.
    The RestServer class uses the Boost.Beast library to handle HTTP requests and responses.
*/


class RestServer {
public:
    RestServer(net::io_context& ioc, tcp::endpoint endpoint, ThreadPool& threadPool);

private:
    void doAccept();
    void handleRequest(std::shared_ptr<tcp::socket> socket);
    void fail(beast::error_code ec, char const* what);

    json handleLogin(const http::request<http::string_body>& req);
    json handleRegister(const http::request<http::string_body>& req);
    json handleLogout(const http::request<http::string_body>& req);
    json handleGetUsers(const http::request<http::string_body>& req);
    json handleGetRooms(const http::request<http::string_body>& req);
    json handleGetMessages(const http::request<http::string_body>& req, const std::string& roomId);
    json handleInviteFriend(const http::request<http::string_body>& req);
    json handleGetFriend(const http::request<http::string_body>& req);
    json handleGetPendingInvitedFriend(const http::request<http::string_body>& req);
    json handleGetFriendIniviteRequest(const http::request<http::string_body>& req);
    bool handleAcceptInviteFriend(const http::request<http::string_body>& req);

    bool isTokenValid(const std::string& token, std::string& email);

    tcp::acceptor acceptor_;
    ThreadPool& threadPool_;
};


