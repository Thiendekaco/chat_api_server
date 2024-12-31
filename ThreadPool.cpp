#include "ThreadPool.h"

/*
    The ThreadPool class is responsible for managing a pool of worker threads.
    It allows tasks to be enqueued and executed by the worker threads.
    The ThreadPool class uses the std::thread, std::mutex, and std::condition_variable classes to manage the worker threads.
*/

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
  // Create list of worker threads
  // Each thread will execute the workerThread function
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
      	// Lock the mutex before setting stop to true to prevent data
        std::unique_lock<std::mutex> lock(queueMutex);
        // Set stop to true to signal to all worker threads to stop, stop to accept new tasks
        stop = true;
    }
    // Wake up all the threads waiting on the condition.
    // When these threads are awakened,
    // they will check the condition they are waiting on
    // (for example: checking the value of the stop variable)
    condition.notify_all();
    for (std::thread& worker : workers) {
      	// Wait for all worker threads to finish before free its
        worker.join();
    }
}

void ThreadPool::enqueueTask(std::function<void()> task) {
    {
       // Mutex is used to protect access to the task queue to prevent race conditions,
       // ensuring that only one thread can access the queue at a time.
       // and avoid the case of multiple threads accessing the queue at the same time then they get the same task
        std::unique_lock<std::mutex> lock(queueMutex);

        // Add the task to the queue
        tasks.push(std::move(task));
    }
    // Notify one of the waiting threads that there is a new task to be executed
    condition.notify_one();
}

// Each thread will execute this function
void ThreadPool::workerThread() {
    // This loop allows the thread to continue running continuously, waiting for new tasks to execute.
    while (true) {
        std::function<void()> task;
        {
            // Lock the mutex to access the task queue
            std::unique_lock<std::mutex> lock(queueMutex);
            // Wait until there is a task to execute or the thread pool is stopped
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            // If the thread pool is stopped and there are no tasks to execute, the thread will exit
            if (stop && tasks.empty()) return;
            // Get the next task from the queue
            task = std::move(tasks.front());
            tasks.pop();
        }
        // Execute the task
        task();
    }
}