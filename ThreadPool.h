#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#ifndef THREADPOOL_H
#define THREADPOOL_H



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

