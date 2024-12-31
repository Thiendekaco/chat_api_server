#pragma once
#include <string>
#include <vector>
#include <memory>
#include "ConnectionPool.h"

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

/*
    The DatabaseManager class is a singleton class that provides an interface to interact with the database.
    It manages the connection pool and provides methods to execute queries, fetch data, and perform CRUD operations.
    The DatabaseManager should be a singleton class to maintain a single connectionPool throughout the application's lifecycle.
*/

class DatabaseManager {
public:
    static DatabaseManager& getInstance();

    bool executeQuery(const std::string& query);
    std::vector<std::vector<std::string>> fetchQuery(const std::string& query);
    bool deleteData(const std::string& table, const std::string& condition);
    bool insertData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values);
    bool updateData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values, const std::string& condition);
    bool emailExists(const std::string& email);
    bool registerUser(const std::string& email, const std::string& passwordHash);
    bool invalidateToken(const std::string& token);
    bool updateUserStatus(const std::string& email, const std::string& status);
    std::vector<std::vector<std::string>> getUsers();
    std::string getPasswordHash(const std::string& email);
    bool saveMessage(int roomId, int senderId, const std::string& content);
    bool updateMessageStatus(int messageId, int userId, const std::string& status);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    std::shared_ptr<pqxx::connection> getConnection();
    void releaseConnection(std::shared_ptr<pqxx::connection> conn);
    void handleError(const std::string& errorMessage);

    std::unique_ptr<ConnectionPool> connectionPool_;
    static std::unique_ptr<DatabaseManager> instance_;
    static std::once_flag initInstanceFlag;
};

#endif //DATABASEMANAGER_H

