#pragma once
#pragma once
#include <string>
#include <pqxx/pqxx>
#include <vector>
#include <mutex>

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

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
    DatabaseManager(const std::string& conninfo);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    pqxx::connection* conn;
    bool openDatabase(const std::string& conninfo);
    void closeDatabase();
    void handleError(const std::string& errorMessage);

    static std::mutex mutex_;
    static DatabaseManager* instance_;
};

#endif //DATABASEMANAGER_H

