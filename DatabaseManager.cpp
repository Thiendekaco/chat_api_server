#include "DatabaseManager.h"
#include <iostream>
#include <stdexcept>
#include <mutex>

/*
    The DatabaseManager class is a singleton class that provides an interface to interact with the database.
    It manages the connection pool and provides methods to execute queries, fetch data, and perform CRUD operations.
    The DatabaseManager should be a singleton class to maintain a single connectionPool throughout the application's lifecycle.
*/

std::unique_ptr<DatabaseManager> DatabaseManager::instance_ = nullptr;
std::once_flag DatabaseManager::initInstanceFlag;

// Get the singleton instance of DatabaseManager
DatabaseManager& DatabaseManager::getInstance() {
  // Use std::call_once to ensure that the instance is created only once
  // This is thread-safe and guarantees that the instance is created in a multi-threaded environment
    std::call_once(initInstanceFlag, []() {
        instance_ = std::unique_ptr<DatabaseManager>(new DatabaseManager());
    });
    return *instance_;
}

// Constructor to initialize the connection pool
// The connection pool is created with the connection information and pool size
DatabaseManager::DatabaseManager()
    : connectionPool_(std::make_unique<ConnectionPool>(
        "host=localhost port=5432 dbname=chat_message_db user=postgres password=root",
        150
    )) {
}

DatabaseManager::~DatabaseManager() {}

// Get a connection from the connection pool
std::shared_ptr<pqxx::connection> DatabaseManager::getConnection() {
    return connectionPool_->getConnection();
}


// Release the connection back to the connection pool
// This allows other threads to use the connection
void DatabaseManager::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    connectionPool_->releaseConnection(conn);
}

// Excuting the query
bool DatabaseManager::executeQuery(const std::string& query) {
     // Get a connection from the connection pool
     // The connection is automatically released back to the pool when the function exits
    auto conn = getConnection();
    try {
        // Start a transaction
        // The transaction is automatically committed when the function exits
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();

        // When the transaction is committed, the connection is released back to the pool
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        // If an error occurs, the connection is released back to the pool
        releaseConnection(conn);
        return false;
    }
}

std::vector<std::vector<std::string>> DatabaseManager::fetchQuery(const std::string& query) {
    // Get a connection from the connection pool
    // The connection is automatically released back to the pool when the function exits
    auto conn = getConnection();
    std::vector<std::vector<std::string>> results;
    try {
        pqxx::work txn(*conn);
        pqxx::result res = txn.exec(query);
        for (const auto& row : res) {
            std::vector<std::string> resultRow;
            for (const auto& field : row) {
                resultRow.push_back(field.c_str());
            }
            results.push_back(resultRow);
        }
        releaseConnection(conn);
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
    }
    return results;
}

bool DatabaseManager::deleteData(const std::string& table, const std::string& condition) {
    std::string query = "DELETE FROM " + table + " WHERE " + condition;
    return executeQuery(query);
}

bool DatabaseManager::insertData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values) {
    std::string query = "INSERT INTO " + table + " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        query += columns[i];
        if (i < columns.size() - 1) {
            query += ", ";
        }
    }
    query += ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        query += "'" + values[i] + "'";
        if (i < values.size() - 1) {
            query += ", ";
        }
    }
    query += ")";
    return executeQuery(query);
}

bool DatabaseManager::updateData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values, const std::string& condition) {
    std::string query = "UPDATE " + table + " SET ";
    for (size_t i = 0; i < columns.size(); ++i) {
        query += columns[i] + " = '" + values[i] + "'";
        if (i < columns.size() - 1) {
            query += ", ";
        }
    }
    query += " WHERE " + condition;
    return executeQuery(query);
}

/*
    The query methods below will be built according to the basic flow as follows:
      1. Get a connection from the connection pool
      2. Execute the query
      3. Release the connection back to the pool
      4. If an exception occurs, handle the error and release the connection back to the pool
      5. Return the result of the query
*/

bool DatabaseManager::emailExists(const std::string& email) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("SELECT 1 FROM users WHERE email = " + txn.quote(email));
        releaseConnection(conn);
        return !result.empty();
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

bool DatabaseManager::registerUser(const std::string& email, const std::string& passwordHash) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("INSERT INTO users (email, username) VALUES (" + txn.quote(email) + ", " + txn.quote(email) + ") RETURNING user_id");
        if (result.empty()) {
            releaseConnection(conn);
            return false;
        }
        int userId = result[0][0].as<int>();
        txn.exec("INSERT INTO authentication (user_id, password_hash) VALUES (" + txn.quote(userId) + ", " + txn.quote(passwordHash) + ")");
        txn.commit();
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

std::string DatabaseManager::getPasswordHash(const std::string& email) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("SELECT password_hash FROM authentication WHERE user_id = (SELECT user_id FROM users WHERE email = " + txn.quote(email) + ")");
        releaseConnection(conn);
        if (result.empty()) {
            return "";
        }
        return result[0][0].as<std::string>();
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return "";
    }
}

bool DatabaseManager::invalidateToken(const std::string& token) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO token_blacklist (token) VALUES (" + txn.quote(token) + ")");
        txn.commit();
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

bool DatabaseManager::updateUserStatus(const std::string& email, const std::string& status) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        txn.exec("UPDATE users SET status = " + txn.quote(status) + " WHERE email = " + txn.quote(email));
        txn.commit();
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

std::vector<std::vector<std::string>> DatabaseManager::getUsers() {
    auto conn = getConnection();
    std::vector<std::vector<std::string>> users;
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("SELECT username, email, profile_picture, status, created_at FROM users");
        for (const auto& row : result) {
            std::vector<std::string> user;
            user.push_back(row["username"].c_str());
            user.push_back(row["email"].c_str());
            user.push_back(row["profile_picture"].c_str());
            user.push_back(row["status"].c_str());
            user.push_back(row["created_at"].c_str());
            users.push_back(user);
        }
        releaseConnection(conn);
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
    }
    return users;
}

bool DatabaseManager::saveMessage(int roomId, int senderId, const std::string& content) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO messages (room_id, sender_id, content) VALUES (" + txn.quote(roomId) + ", " + txn.quote(senderId) + ", " + txn.quote(content) + ")");
        txn.commit();
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

bool DatabaseManager::updateMessageStatus(int messageId, int userId, const std::string& status) {
    auto conn = getConnection();
    try {
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO message_status (message_id, user_id, status) VALUES (" + txn.quote(messageId) + ", " + txn.quote(userId) + ", " + txn.quote(status) + ") ON CONFLICT (message_id, user_id) DO UPDATE SET status = " + txn.quote(status) + ", updated_at = CURRENT_TIMESTAMP");
        txn.commit();
        releaseConnection(conn);
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        releaseConnection(conn);
        return false;
    }
}

void DatabaseManager::handleError(const std::string& errorMessage) {
    std::cerr << "Database error: " << errorMessage << std::endl;
    throw std::runtime_error("Database error: " + errorMessage);
}


