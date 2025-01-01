#include "RestServer.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <jwt-cpp/jwt.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/*
    The RestServer class is responsible for handling RESTful API requests.
    It listens for incoming HTTP requests and processes them accordingly.
    The RestServer class uses the Boost.Beast library to handle HTTP requests and responses.
*/


/*
Function of acceptor
 Accepting Connections:
   The acceptor listens on a specified address and port, and when a client tries to connect, the acceptor accepts the connection.
 Socket Management:
   When a connection is accepted, the acceptor creates a new socket to communicate with the client. This socket will be used to send and receive data.
 Asynchronous operation:
    Acceptors often support asynchronous operations, allowing the server to continue performing other tasks without being blocked while waiting for a connection.
 */

RestServer::RestServer(net::io_context& ioc, tcp::endpoint endpoint, ThreadPool& threadPool)
    : acceptor_(ioc), threadPool_(threadPool) {

  	//The ec variable is used to save the error code during operations with the socket.
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      	// If an error occurs, the fail function is called to print the error message.
        fail(ec, "open");
        return;
    }

    // Set the option to reuse the address
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
       // If an error occurs, the fail function is called to print the error message.
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    // Allows the server to listen for connections to the specified address and port.
    acceptor_.bind(endpoint, ec);
    if (ec) {
      	// If an error occurs, the fail function is called to print the error message.
        fail(ec, "bind");
        return;
    }

    // Start listening for incoming connections with the maximum number of connections specified.
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
    doAccept();
}

void RestServer::doAccept() {
    // 'async_accept' is used to accept a new connection from a client.
    // When a client tries to connect to the server,
    // async_accept will accept the connection and provide a socket to communicate with the client.
    auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
    acceptor_.async_accept(
        *socket,
        [this, socket](boost::system::error_code ec) {  // Capture socket ?úng cách
            if (!ec) {
              // Enqueue the task to handle the client connection if there is no error
                threadPool_.enqueueTask([this, socket = std::move(socket)]() mutable {
                    handleRequest(socket);
                    });
            }
            else {
                std::cerr << "Failed to accept connection: " << ec.message() << std::endl;
            }
            // Continue to accept new connections
            doAccept();
        });
}


void RestServer::handleRequest(std::shared_ptr<tcp::socket> socket) {
    try {
        // This buffer helps to manage the size and calculate memory efficiently.
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        // Read a request from the client
        http::read(*socket, buffer, req);

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");

        // Handle different API requests based on the request method and target
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
        else if (req.method() == http::verb::post && req.target() == "/api/invite") {
            // Handle invite friend
            json response = handleInviteFriend(req);
            res.body() = response.dump();
        }
        else if (req.method() == http::verb::post && req.target() == "/api/accept-invite") {
            // Handle invite friend
            json response = handleInviteFriend(req);
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

        // Prepare the response payload
        // Write the response to the client
        res.prepare_payload();
        http::write(*socket, res);
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

bool RestServer::isTokenValid(const std::string& token, std::string& email) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ "secret" })
            .with_issuer("auth0");

        verifier.verify(decoded);
        email = decoded.get_payload_claim("email").as_string();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Token validation failed: " << e.what() << "\n";
        return false;
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
            if (Utils::checkPassword(password, storedHash)) {
                // Generate a token
                std::string token = Utils::generateToken(email);

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

json RestServer::handleRegister(const http::request<http::string_body>& req) {
    json response;
    try {
        // Parse the request body
        json requestBody = json::parse(req.body());

        // Extract email and password
        std::string email = requestBody.at("email").get<std::string>();
        std::string password = requestBody.at("password").get<std::string>();

        // Hash the password (you can use a library like bcrypt for this)
        std::string passwordHash = Utils::hashPassword(password.c_str());

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
                std::string token = Utils::generateToken(email);

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
            dbManager.updateUserStatus(email, "offline");
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
        // Extract the token from the request headers
        auto authHeader = req[http::field::authorization];
        if (authHeader.empty()) {
            response["message"] = "Authorization header missing";
            response["status"] = "error";
            return response;
        }

        std::string token = authHeader.substr(7); // Remove "Bearer " prefix
        std::string email;
        if (!isTokenValid(token, email)) {
            response["message"] = "Invalid token";
            response["status"] = "error";
            return response;
        }

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        // Retrieve list of users
        std::vector<std::string> user = dbManager.getUserByEmail(email);
        response["username"] = user[0];
        response["email"] = user[1];
        response["profile_picture"] = user[2];
        response["status"] = user[3];
        response["created_at"] = user[4];

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
    try {
        // Extract the token from the request headers
        auto authHeader = req[http::field::authorization];
        if (authHeader.empty()) {
            response["message"] = "Authorization header missing";
            response["status"] = "error";
            return response;
        }

        std::string token = authHeader.substr(7); // Remove "Bearer " prefix
        std::string email;
        if (!isTokenValid(token, email)) {
            response["message"] = "Invalid token";
            response["status"] = "error";
            return response;
        }

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        std::string userId = json::parse(req.body())["user_id"];

        // Retrieve list of chat rooms
        std::vector<std::vector<std::string>> rooms = dbManager.getRoomsByUserId(std::stoi(userId));
        response["rooms"] = json::array();
        for (const auto& room : rooms) {
            json roomJson;
            roomJson["room_id"] = room[0];
            roomJson["room_name"] = room[1];
            roomJson["created_at"] = room[2];
            response["rooms"].push_back(roomJson);
        }
        response["status"] = "success";
    }
    catch (const std::exception& e) {
        response["message"] = "Failed to retrieve rooms";
        response["status"] = "error";
    }

    return response;
}

json RestServer::handleGetMessages(const http::request<http::string_body>& req, const std::string& roomId) {
  // Retrieve message history for the specified room
  // Return JSON response
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
        std::string email;
        if (!isTokenValid(token, email)) {
            response["message"] = "Invalid token";
            response["status"] = "error";
            return response;
        }


		// Get the singleton instance of DatabaseManager
		DatabaseManager& dbManager = DatabaseManager::getInstance();

        std::vector<std::vector<std::string>> messages = dbManager.getMessages(std::stoi(roomId));

		for (const auto& message : messages) {
			json messageJson;
			messageJson["message_id"] = message[0];
			messageJson["sender_id"] = message[1];
			messageJson["content"] = message[2];
			messageJson["is_read"] = message[3];
			messageJson["created_at"] = message[4];
			response["messages"].push_back(messageJson);
		}

		std::vector<std::string> room = dbManager.getRoomById(std::stoi(roomId));
		json roomJson;
		roomJson["room_id"] = room[0];
		roomJson["user_id_1"] = room[1];
        roomJson["user_id_2"] = room[2];
		roomJson["last_message_at"] = room[3];
        roomJson["created_at"] = room[4];

        // Retrieve message history for the specified room
        response["messages"] = json::array();
		response["room"] = roomJson;
        response["status"] = "success";
    }
    catch (const std::exception& e) {
        response["message"] = "Failed to retrieve messages";
        response["status"] = "error";
    }

    return response;
}

json RestServer::handleInviteFriend(const http::request<http::string_body>& req) {
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
        std::string email;
        if (!isTokenValid(token, email)) {
            response["message"] = "Invalid token";
            response["status"] = "error";
            return response;
        }

		std::string friendId = json::parse(req.body())["friend_id"];
		std::string userId = json::parse(req.body())["user_id"];

		// Get the singleton instance of DatabaseManager
		DatabaseManager& dbManager = DatabaseManager::getInstance();

		std::vector<std::string> result = dbManager.updateFriendRequest(std::stoi(userId), std::stoi(friendId));

		for (const auto& row : result) {
			json friendRequest;
			friendRequest["user_id_1"] = row[0];
			friendRequest["user_id_2"] = row[1];
			friendRequest["is_accepted"] = row[2];
			response["friend_requests"].push_back(friendRequest);
		}



        // Invite a friend to join a chat room
        response["message"] = "Invite sent";
        response["status"] = "success";
    }
    catch (const std::exception& e) {
        response["message"] = "Failed to send invite";
        response["status"] = "error";
    }

    return response;
}


json RestServer::handleGetFriend(const http::request<http::string_body>& req) {
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
        std::string email;
        if (!isTokenValid(token, email)) {
            response["message"] = "Invalid token";
            response["status"] = "error";
            return response;
        }

		// Get the singleton instance of DatabaseManager
		DatabaseManager& dbManager = DatabaseManager::getInstance();

		std::string userId = json::parse(req.body())["user_id"];

        std::vector<std::vector<std::string>> result = dbManager.getFriends(std::stoi(userId));

		for (const auto& row : result) {
			json friend_;
            friend_["user_name"] = row[0];
            friend_["email"] = row[1];
            friend_["profile_picture"] = row[2];
            friend_["status"] = row[3];
            friend_["created_at"] = row[4];
			response["friends"].push_back(friend_);
		}
		response["status"] = "success";

	}
    catch (const std::exception& e) {
        response["message"] = "Failed to retrieve friend";
        response["status"] = "error";
    }

	return response;

};


json RestServer::handleGetPendingInvitedFriend(const http::request<http::string_body>& req) {

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
		std::string email;
		if (!isTokenValid(token, email)) {
			response["message"] = "Invalid token";
			response["status"] = "error";
			return response;
		}
		// Get the singleton instance of DatabaseManager
		DatabaseManager& dbManager = DatabaseManager::getInstance();
		std::string userId = json::parse(req.body())["user_id"];
		std::vector<std::vector<std::string>> result = dbManager.getFriendRequestPending(std::stoi(userId));
		for (const auto& row : result) {
			json friend_;
			friend_["user_name"] = row[0];
			friend_["email"] = row[1];
			friend_["profile_picture"] = row[2];
			friend_["status"] = row[3];
			friend_["created_at"] = row[4];
			response["friends"].push_back(friend_);
		}
		response["status"] = "success";
	}
	catch (const std::exception& e) {
		response["message"] = "Failed to retrieve friend";
		response["status"] = "error";
	}
	return response;


};

json RestServer::handleGetFriendIniviteRequest(const http::request<http::string_body>& req) {
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
		std::string email;
		if (!isTokenValid(token, email)) {
			response["message"] = "Invalid token";
			response["status"] = "error";
			return response;
		}
		// Get the singleton instance of DatabaseManager
		DatabaseManager& dbManager = DatabaseManager::getInstance();
		std::string userId = json::parse(req.body())["user_id"];
		std::vector<std::vector<std::string>> result = dbManager.getFriendRequests(std::stoi(userId));
		for (const auto& row : result) {
			json friend_;
			friend_["user_name"] = row[0];
			friend_["email"] = row[1];
			friend_["profile_picture"] = row[2];
			friend_["status"] = row[3];
			friend_["created_at"] = row[4];
			response["friends"].push_back(friend_);
		}
		response["status"] = "success";
	}
	catch (const std::exception& e) {
		response["message"] = "Failed to retrieve friend";
		response["status"] = "error";
	}
	return response;
};


bool RestServer::handleAcceptInviteFriend(const http::request<http::string_body>& req) {
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
		std::string email;
		if (!isTokenValid(token, email)) {
			response["message"] = "Invalid token";
			response["status"] = "error";
			return response;
		}
        std::string friendId = json::parse(req.body())["friend_id"];
        std::string userId = json::parse(req.body())["user_id"];

        // Get the singleton instance of DatabaseManager
        DatabaseManager& dbManager = DatabaseManager::getInstance();

        std::vector<std::string> result = dbManager.updateFriendRequest(std::stoi(userId), std::stoi(friendId));

        for (const auto& row : result) {
            json friendRequest;
            friendRequest["user_id_1"] = row[0];
            friendRequest["user_id_2"] = row[1];
            friendRequest["is_accepted"] = row[2];
            response["friend_requests"].push_back(friendRequest);
        }



        // Invite a friend to join a chat room
        response["message"] = "Invite sent";
        response["status"] = "success";
    }
    catch (const std::exception& e) {
        response["message"] = "Failed to send invite";
        response["status"] = "error";
    }

    return response;
}

void RestServer::fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}