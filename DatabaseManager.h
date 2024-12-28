#pragma once
#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>
#include <string>
#include <vector>
#include <mutex>

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

class DatabaseManager {
public:
    static DatabaseManager& getInstance(const std::string& conninfo = "");

    bool executeQuery(const std::string& query);
    std::vector<std::vector<std::string>> fetchQuery(const std::string& query);
    bool deleteData(const std::string& table, const std::string& condition);
    bool insertData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values);
    bool updateData(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values, const std::string& condition);

private:
    DatabaseManager(const std::string& conninfo);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    soci::session sql;
    bool openDatabase(const std::string& conninfo);
    void closeDatabase();
    void handleError(const std::string& errorMessage);

    static std::mutex mutex_;
    static DatabaseManager* instance_;
};

#endif //DATABASEMANAGER_H