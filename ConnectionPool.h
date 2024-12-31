#pragma once
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

/*
    The purpose of a connection pool is to
    optimise the management and use of connections to the database (DB) in the thread pool server.
 */

class ConnectionPool {
public:
    ConnectionPool(const std::string& conninfo, size_t poolSize);
    ~ConnectionPool();

    std::shared_ptr<pqxx::connection> getConnection();
    void releaseConnection(std::shared_ptr<pqxx::connection> conn);

private:
    std::queue<std::shared_ptr<pqxx::connection>> pool_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::string conninfo_;
    size_t poolSize_;
};

