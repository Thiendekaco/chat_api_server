#include "ConnectionPool.h"

/*
    The purpose of a connection pool is to
    optimise the management and use of connections to the database (DB) in the thread pool server.
 */


// Constructor to initialize the connection pool with the connection information and pool size
ConnectionPool::ConnectionPool(const std::string& conninfo, size_t poolSize)
    : conninfo_(conninfo), poolSize_(poolSize) {
    for (size_t i = 0; i < poolSize_; ++i) {
        pool_.emplace(std::make_shared<pqxx::connection>(conninfo_));
    }
}

// Destructor to clear the connection pool
ConnectionPool::~ConnectionPool() {
    // Lock the mutex to access the pool, ensuring that no other thread is using the pool
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.pop();
    }
}

std::shared_ptr<pqxx::connection> ConnectionPool::getConnection() {
    // Lock the mutex to access the pool, ensuring that no other thread is using the pool,
    // avoid the case of multiple threads accessing the pool at the same time and then they get the same connection
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]() { return !pool_.empty(); });

    // Get the connection from the front of the pool
    auto conn = pool_.front();
    pool_.pop();
    return conn;
}

// Release the connection back to the pool
void ConnectionPool::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    // Notify one of the waiting threads that there is a connection available
    condition_.notify_one();
}
