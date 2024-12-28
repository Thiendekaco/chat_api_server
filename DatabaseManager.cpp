#include "DatabaseManager.h"
#include <iostream>
#include <stdexcept>

std::mutex DatabaseManager::mutex_;
DatabaseManager* DatabaseManager::instance_ = nullptr;

DatabaseManager& DatabaseManager::getInstance(const std::string& conninfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        instance_ = new DatabaseManager(conninfo);
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
        sql.open(soci::postgresql, conninfo);
    } catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase() {
    sql.close();
}

bool DatabaseManager::executeQuery(const std::string& query) {
    try {
        sql << query;
    } catch (const std::exception& e) {
        handleError(e.what());
        return false;
    }
    return true;
}

std::vector<std::vector<std::string>> DatabaseManager::fetchQuery(const std::string& query) {
    std::vector<std::vector<std::string>> results;
    try {
        soci::rowset<soci::row> rs = (sql.prepare << query);
        for (const auto& row : rs) {
            std::vector<std::string> resultRow;
            for (std::size_t i = 0; i != row.size(); ++i) {
                resultRow.push_back(row.get<std::string>(i));
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

void DatabaseManager::handleError(const std::string& errorMessage) {
    std::cerr << "Database error: " << errorMessage << std::endl;
    throw std::runtime_error("Database error: " + errorMessage);
}