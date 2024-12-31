#include "RestServer.h"
#include "DatabaseManager.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include "include/bcrypt/bcrypt.h"
#include <jwt-cpp/jwt.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

std::string RestServer::hashPassword(const std::string& password) {
    try {
        char salt[BCRYPT_HASHSIZE];
        char hash[BCRYPT_HASHSIZE];
        // Generate a salt and hash the password
        int ret = bcrypt_gensalt(12, salt);

        if (ret != 0) {
            throw std::runtime_error("Failed to generate salt");
        }

        int ret_ = bcrypt_hashpw(password.c_str(), salt, hash);

        if (ret_ != 0) {
            throw std::runtime_error("Failed to hash password");
        }

        return hash;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to hash password: " + std::string(e.what()));
    }
}


bool RestServer::checkPassword(const std::string& password, const std::string& hash) {
    try {
        int ret = bcrypt_checkpw(password.c_str(), hash.c_str());
        if (ret >= 0) {
            return ret == 0;
        }

        throw std::runtime_error("Failed to check password");

    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to check password: " + std::string(e.what()));
    }
}

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
        [this, socket](boost::system::error_code ec) {  // Capture socket ?úng cách
            if (!ec) {
                threadPool_.enqueueTask([this, socket = std::move(socket)]() mutable {
                    handleRequest(socket);
                    });
            }
            else {
                std::cerr << "Failed to accept connection: " << ec.message() << std::endl;
            }
            doAccept();
        });
}


void RestServer::handleRequest(std::shared_ptr<tcp::socket> socket) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(*socket, buffer, req);

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");

        if (req.method() == http::verb::post && req.target() == "/api/login") {
            // Handle login
            json response = handleLogin(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::post && req.target() == "/api/register") {
            // Handle register
            json response = handleRegister(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::post && req.target() == "/api/logout") {
            // Handle logout
            json response = handleLogout(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::get && req.target() == "/api/users") {
            // Handle get users
            json response = handleGetUsers(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::get && req.target() == "/api/rooms") {
            // Handle get rooms
            json response = handleGetRooms(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::get && req.target() == "/api/invite") {
            // Handle get rooms
            json response = handleInviteFriend(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::get && req.target().starts_with("/api/messages/")) {
            // Handle get messages
            std::string roomId = req.target().substr(std::string("/api/messages/").length());
            json response = handleGetMessages(req, roomId);
            res.body() = response.dump();
        }
        else {
            res.result(http::status::not_found);
            res.body() = "Not Found";
        }

        res.prepare_payload();
        http::write(*socket, res);
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

json RestServer::handleLogin(const http::request<http::string_body>& req) {
    json response;
    try {
        // Parse the request body
        json requestBody = json::parse(req.body());

        // Extract email and password
        std::string email = requestBody.at("email").get<std::string>();
        std::string password = requestBody.at("password").get<std::string>();

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        // Check if the email exists in the database
        if (!dbManager.emailExists(email)) {
            response["message"] = "Invalid email or password";
            response["status"] = "error";
        }
        else {
            // Check the password (you can use a library like bcrypt for this)
            std::string storedHash = dbManager.getPasswordHash(email);
            if (checkPassword(password, storedHash)) {
                // Generate a token
                std::string token = generateToken(email);

                // Update user status to 'online'
                dbManager.updateUserStatus(email, "online");
                response["message"] = "Login successful";
                response["status"] = "success";
                response["token"] = token;
            }
            else {
                response["message"] = "Invalid email or password";
                response["status"] = "error";
            }
        }
    }
    catch (const std::exception& e) {
        response["message"] = "Invalid request";
        response["status"] = "error";
    }
    return response;
}


std::string RestServer::generateToken(const std::string& email) {
    auto token = jwt::create()
        .set_issuer("auth0")
        .set_type("JWS")
        .set_payload_claim("email", jwt::claim(email))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{ 24 })
        .sign(jwt::algorithm::hs256{ "secret" });
    return token;
}

json RestServer::handleRegister(const http::request<http::string_body>& req) {
    json response;
    try {
        // Parse the request body
        json requestBody = json::parse(req.body());

        // Extract email and password
        std::string email = requestBody.at("email").get<std::string>();
        std::string password = requestBody.at("password").get<std::string>();

        // Hash the password (you can use a library like bcrypt for this)
        std::string passwordHash = hashPassword(password.c_str());

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        // Check if the email already exists in the database
        if (dbManager.emailExists(email)) {
            response["message"] = "Email already exists";
            response["status"] = "error";
        }
        else {
            // Register the new user
            if (dbManager.registerUser(email, passwordHash)) {
                std::string token = generateToken(email);

                // Update user status to 'online'
				dbManager.updateUserStatus(email, "online");
                response["message"] = "Registration successful";
                response["status"] = "success";
                response["token"] = token;

            }
            else {
                response["message"] = "Registration failed";
                response["status"] = "error";
            }
        }
    }
    catch (const std::exception& e) {
        response["message"] = "Invalid request";
        response["status"] = "error";
    }
    return response;
}

json RestServer::handleLogout(const http::request<http::string_body>& req) {
    json response;
    try {
        // Extract the token from the request headers
        auto authHeader = req[http::field::authorization];
        if (authHeader.empty()) {
            response["message"] = "Authorization header missing";
            response["status"] = "error";
            return response;
        }

        std::string token = authHeader.substr(7); // Remove "Bearer " prefix

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        // Invalidate the token
        if (dbManager.invalidateToken(token)) {
            // Extract email from token
            auto decoded = jwt::decode(token);
            std::string email = decoded.get_payload_claim("email").as_string();
            // Update user status to 'offline'
            dbManager.updateUserStatus(email, "offline")
            response["message"] = "Logout successful";
            response["status"] = "success";
        }
        else {
            response["message"] = "Logout failed";
            response["status"] = "error";
        }
    }
    catch (const std::exception& e) {
        response["message"] = "Invalid request";
        response["status"] = "error";
    }
    return response;
}

json RestServer::handleGetUsers(const http::request<http::string_body>& req) {
    json response;
    try {
        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        // Retrieve list of users
        std::vector<std::vector<std::string>> users = dbManager.getUsers();
        response["users"] = json::array();
        for (const auto& user : users) {
            json userJson;
            userJson["username"] = user[0];
            userJson["email"] = user[1];
            userJson["profile_picture"] = user[2];
            userJson["status"] = user[3];
            userJson["created_at"] = user[4];
            response["users"].push_back(userJson);
        }
        response["status"] = "success";
    }
    catch (const std::exception& e) {
        response["message"] = "Failed to retrieve users";
        response["status"] = "error";
    }

    return response;
}

json RestServer::handleGetRooms(const http::request<http::string_body>& req) {
    // Retrieve list of chat rooms
    // Return JSON response
    json response;
    response["rooms"] = json::array();
    return response;
}

json RestServer::handleGetMessages(const http::request<http::string_body>& req, const std::string& roomId) {
    // Retrieve message history for the specified room
    // Return JSON response
    json response;
    response["messages"] = json::array();
    return response;
}

void RestServer::fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}