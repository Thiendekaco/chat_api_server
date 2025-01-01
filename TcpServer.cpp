#include "TcpServer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <jwt-cpp/jwt.h>
#include "DatabaseManager.h"

/*
    The TcpServer class is responsible for handling TCP/IP connections.
    It listens for incoming connections and processes the messages received from the clients.
    The TcpServer class uses the Boost.Asio library to handle TCP/IP connections.
*/


// Constructor to initialize the acceptor and socket
TcpServer::TcpServer(boost::asio::io_context& io_context, short port, ThreadPool& threadPool)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), threadPool_(threadPool) {
   	// Create a new socket
    socket_ = std::make_shared<tcp::socket>(io_context);
    // Start accepting incoming connections
    doAccept();
}

void TcpServer::doAccept() {
    // 'async_accept' is used to accept a new connection from a client.
    // When a client tries to connect to the server,
    // async_accept will accept the connection and provide a socket to communicate with the client.
    acceptor_.async_accept(*socket_,
        [this](boost::system::error_code ec) {
            if (!ec) {
                // Enqueue the task to handle the client connection if there is no error
                threadPool_.enqueueTask([this]() mutable {
                    handleClient(socket_);
                    });
            }
            // Continue to accept new connections
            doAccept();
        });
}

void TcpServer::handleClient(std::shared_ptr<tcp::socket> socket) {
    try {
        for (;;) {
         	 // Read data from the socket
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
        std::string email;
        if (!isTokenValid(token, email)) {
            std::cerr << "Invalid token. Closing connection.\n";
            socket->close();
            return;
        }

        // Add client to the list of connected clients
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_[email].push_back(socket);
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
    std::string email;
    if (!isTokenValid(message["token"], email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    // Handle client disconnection
    std::cout << "Client disconnected: " << message["username"] << "\n";

    // Remove client from the list of connected clients
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto& sockets = clients_[email];
        sockets.erase(std::remove(sockets.begin(), sockets.end(), socket), sockets.end());
        if (sockets.empty()) {
            clients_.erase(email);


            DatabaseManager& dbManager = DatabaseManager::getInstance();

			std::vector<std::string> user = dbManager.getUserByEmail(email);
			std::string userId = user[0];


			json userStatusMessage;
			userStatusMessage["type"] = "userStatus";
			userStatusMessage["user_id"] = userId;
			userStatusMessage["user_status"] = "offline";

            dbManager.updateUserStatus(email, message["user_status"]);

            std::vector<std::string> user = dbManager.getUserByEmail(email);
            std::string userId = user[0];


            // Get the user's friends
            std::vector<std::vector<std::string>> friends = dbManager.getFriends(std::stoi(userId));

            std::vector<std::string> friendEmails;

            for (auto& friend_ : friends) {
                friendEmails.push_back(friend_[2]);
            }

            // Broadcast the user status update to all clients
            sendMessageToMultipleClients(friendEmails, userStatusMessage.dump());
        }
    }
}

void TcpServer::handleMessage(const json& message, std::shared_ptr<tcp::socket> socket) {
    std::string token = message["token"];
    std::string email;
    if (!isTokenValid(token, email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    // Handle message sending/receiving
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    std::vector<std::string> user = dbManager.getUserByEmail(email);
    std::string userId = user[0];
    std::string recipientId = message["recipient"];
    std::vector<std::string> room = dbManager.getRoomByUserIds(std::stoi(userId), std::stoi(recipientId));
    std::string roomId = room[0];

    bool rs = dbManager.saveMessage(std::stoi(roomId), std::stoi(userId), message["content"]);

    if (!rs) {
        std::cerr << "Failed to save message to the database.\n";
        return;
    }

    // Send the message to the clients in the same room
    std::string recipientEmail = dbManager.getUserById(std::stoi(recipientId))[2];
    sendMessageToClient(recipientEmail, message.dump());
}

void TcpServer::handleTyping(const json& message, std::shared_ptr<tcp::socket> socket) {
    std::string token = message["token"];
    std::string email;
    if (!isTokenValid(token, email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    std::string recipientId = message["recipient"];

    // Handle typing status
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    std::string recipientEmail = dbManager.getUserById(std::stoi(recipientId))[2];

    // Send the typing status to the clients in the same room
    sendMessageToClient(recipientEmail, message.dump());
}

void TcpServer::handleStopTyping(const json& message, std::shared_ptr<tcp::socket> socket) {
    std::string token = message["token"];
    std::string email;
    if (!isTokenValid(token, email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    std::string recipientId = message["recipient"];

    // Handle typing status
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    std::string recipientEmail = dbManager.getUserById(std::stoi(recipientId))[2];

    // Send the typing status to the clients in the same room
    sendMessageToClient(recipientEmail, message.dump());
}

void TcpServer::handleUserStatus(const json& message, std::shared_ptr<tcp::socket> socket) {
    std::string token = message["token"];
    std::string email;
    if (!isTokenValid(token, email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    // Handle user status update
	DatabaseManager& dbManager = DatabaseManager::getInstance();
	dbManager.updateUserStatus(email, message["user_status"]);

	std::vector<std::string> user = dbManager.getUserByEmail(email);
	std::string userId = user[0];


	// Get the user's friends
	std::vector<std::vector<std::string>> friends = dbManager.getFriends(std::stoi(userId));

	std::vector<std::string> friendEmails;

	for (auto& friend_ : friends) {
		friendEmails.push_back(friend_[2]);
	}

    // Broadcast the user status update to all clients
	sendMessageToMultipleClients(friendEmails, message.dump());
}

void TcpServer::handleMessageReceipt(const json& message, std::shared_ptr<tcp::socket> socket) {
    std::string token = message["token"];
    std::string email;
    if (!isTokenValid(token, email)) {
        std::cerr << "Invalid token. Closing connection.\n";
        socket->close();
        return;
    }

    // Handle message receipt confirmation
    std::cout << "Message receipt from " << message["username"] << " for message ID: " << message["messageId"] << "\n";
}

// Check if the token is valid
// After authentication using the RESTful API methods,
// each time the client sends a request to the socket,
// it needs to check the token to ensure that the client has been authenticated.
bool TcpServer::isTokenValid(const std::string& token) {
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

void TcpServer::sendMessageToClient(const std::string& email, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(email);
    if (it != clients_.end()) {
        for (const auto& socket : it->second) {
            boost::asio::async_write(*socket, boost::asio::buffer(message),
                [](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "Failed to send message: " << ec.message() << "\n";
                    }
                });
        }
    }
}

void TcpServer::sendMessageToMultipleClients(const std::vector<std::string>& emails, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (const auto& email : emails) {
        auto it = clients_.find(email);
        if (it != clients_.end()) {
            for (const auto& socket : it->second) {
                boost::asio::async_write(*socket, boost::asio::buffer(message),
                    [](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            std::cerr << "Failed to send message: " << ec.message() << "\n";
                        }
                    });
            }
        }
    }
}

void TcpServer::broadcastMessage(const std::string& message) {
    std::vector<std::shared_ptr<tcp::socket>> clients_copy;

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (const auto& client : clients_) {
            clients_copy.insert(clients_copy.end(), client.second.begin(), client.second.end());
        }
    }

    for (const auto& client : clients_copy) {
        boost::asio::async_write(*client, boost::asio::buffer(message),
            [this, client](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Failed to send message to client: " << ec.message() << "\n";

                    std::lock_guard<std::mutex> lock(clientsMutex_);
                    for (auto& [email, sockets] : clients_) {
                        sockets.erase(std::remove(sockets.begin(), sockets.end(), client), sockets.end());
                        if (sockets.empty()) {
                            clients_.erase(email);
                        }
                    }
                }
            });
    }
}

