#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#ifndef THREADPOOL_H
#define THREADPOOL_H

/*
    The ThreadPool class is responsible for managing a pool of worker threads.
    It allows tasks to be enqueued and executed by the worker threads.
    The ThreadPool class uses the std::thread, std::mutex, and std::condition_variable classes to manage the worker threads.
*/

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueueTask(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;

    void workerThread();
};



#endif //THREADPOOL_H#include <vector>

