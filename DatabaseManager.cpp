#include <iostream>
#include <stdexcept>
#include "DatabaseManager.h"


std::mutex DatabaseManager::mutex_;
DatabaseManager* DatabaseManager::instance_ = nullptr;

DatabaseManager& DatabaseManager::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        instance_ = new DatabaseManager("host=localhost port=5432 dbname=chat_message_db user=postgres password =root");
    }
    return *instance_;
}

DatabaseManager::DatabaseManager(const std::string& conninfo) {
    if (!openDatabase(conninfo)) {
        std::cerr << "Failed to open database: " << conninfo << std::endl;
        throw std::runtime_error("Failed to open database");
    }
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase(const std::string& conninfo) {
    try {
        conn = new pqxx::connection(conninfo);
    } catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase() {
    if (conn) {
        conn->close();
        delete conn;
        conn = nullptr;
    }
}

bool DatabaseManager::executeQuery(const std::string& query) {
    try {
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();
    } catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
    return true;
}

std::vector<std::vector<std::string>> DatabaseManager::fetchQuery(const std::string& query) {
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
    } catch (const std::exception& e) {
        handleError(e.what());
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

bool DatabaseManager::emailExists(const std::string& email) {
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("SELECT 1 FROM users WHERE email = " + txn.quote(email));
        return !result.empty();
    }
    catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
}

bool DatabaseManager::registerUser(const std::string& email, const std::string& passwordHash) {
    try {
        pqxx::work txn(*conn);
        // Insert into users table
        pqxx::result result = txn.exec("INSERT INTO users (email, username) VALUES (" + txn.quote(email) + ", " + txn.quote(email) + ") RETURNING user_id");
        if (result.empty()) {
            return false;
        }
        int userId = result[0][0].as<int>();

        // Insert into authentication table
        txn.exec("INSERT INTO authentication (user_id, password) VALUES (" + txn.quote(userId) + ", " + txn.quote(passwordHash) + ")");
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
}

std::string DatabaseManager::getPasswordHash(const std::string& email) {
    try {
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec("SELECT password_hash FROM authentication WHERE user_id = (SELECT user_id FROM users WHERE email = " + txn.quote(email) + ")");
        if (result.empty()) {
            return "";
        }
        return result[0][0].as<std::string>();
    }
    catch (const std::exception& e) {
        handleError(e.what());
        return "";
    }
}

bool DatabaseManager::invalidateToken(const std::string& token) {
    try {
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO token_blacklist (token) VALUES (" + txn.quote(token) + ")");
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
}


void DatabaseManager::handleError(const std::string& errorMessage) {
    std::cerr << "Database error: " << errorMessage << std::endl;
    throw std::runtime_error("Database error: " + errorMessage);
}

bool DatabaseManager::updateUserStatus(const std::string& email, const std::string& status) {
    try {
        pqxx::work txn(*conn);
        txn.exec("UPDATE users SET status = " + txn.quote(status) + " WHERE email = " + txn.quote(email));
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
}

