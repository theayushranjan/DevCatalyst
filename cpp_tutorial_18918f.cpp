// Learning Objective: Understand how to build a basic C++ thread pool to efficiently
// manage and execute concurrent tasks using std::thread, std::mutex, and
// std::condition_variable for synchronization and task distribution.

#include <vector>                  // For std::vector to hold worker threads
#include <queue>                   // For std::queue to hold tasks
#include <thread>                  // For std::thread to create worker threads
#include <mutex>                   // For std::mutex to protect shared data
#include <condition_variable>      // For std::condition_variable to signal workers
#include <functional>              // For std::function to store arbitrary tasks
#include <iostream>                // For example output (std::cout)
#include <stdexcept>               // For std::runtime_error

// The ThreadPool class manages a collection of worker threads
// and a queue of tasks for them to execute.
class ThreadPool {
public:
    // Constructor: Initializes the thread pool with a specified number of threads.
    ThreadPool(size_t num_threads) : stop(false) {
        // We create 'num_threads' worker threads. Each thread will run an infinite loop
        // to pick up and execute tasks from the shared queue.
        for (size_t i = 0; i < num_threads; ++i) {
            // emplace_back constructs the std::thread object directly in the vector.
            // Each thread executes a lambda function as its entry point.
            workers.emplace_back([this] { // Capture 'this' by value to access member variables
                for (;;) { // Infinite loop for worker threads to continuously look for tasks
                    std::function<void()> task; // Placeholder for the task to be executed

                    { // This block defines a scope for the std::unique_lock
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // Wait until either the stop flag is set OR there are tasks in the queue.
                        // The lambda predicate prevents spurious wakeups and ensures the condition is met.
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });

                        // If the stop flag is true AND the task queue is empty,
                        // it means the pool is shutting down and there are no more tasks to process.
                        // This thread can now safely exit its loop and terminate.
                        if (this->stop && this->tasks.empty())
                            return; // Worker thread exits

                        // Retrieve the next task from the front of the queue.
                        // std::move is used for efficiency, as we are taking ownership of the task.
                        task = std::move(this->tasks.front());
                        this->tasks.pop(); // Remove the task from the queue
                    } // The unique_lock goes out of scope here, releasing the mutex.
                      // This allows other threads to access the queue while the current
                      // thread executes its task.

                    // Execute the retrieved task.
                    task();
                }
            });
        }
    }

    // Destructor: Ensures all worker threads are gracefully stopped and joined.
    ~ThreadPool() {
        { // This block defines a scope for the std::unique_lock
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true; // Set the stop flag to true, signaling all workers to terminate
        }
        condition.notify_all(); // Wake up all waiting worker threads so they can check the 'stop' flag

        // Iterate through all worker threads and join them.
        // Joining ensures that each thread completes its current task and
        // reaches its termination condition (the `return` statement in its loop)
        // before the ThreadPool object is destroyed.
        for (std::thread& worker : workers) {
            worker.join(); // Wait for each thread to finish
        }
    }

    // Enqueue method: Adds a new task to the task queue.
    // It uses a template to accept any callable object (function, lambda, functor).
    template<class F>
    void enqueue(F&& f) {
        { // This block defines a scope for the std::unique_lock
            std::unique_lock<std::mutex> lock(queue_mutex);

            // If the pool is in the process of stopping, prevent new tasks from being enqueued.
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // Add the task to the queue. std::forward ensures perfect forwarding,
            // preserving the value category (lvalue/rvalue) of 'f'.
            // std::function will then copy or move the callable as needed.
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one(); // Wake up one waiting worker thread to process the new task
    }

private:
    std::vector<std::thread> workers;               // Collection of worker threads
    std::queue<std::function<void()>> tasks;        // Queue of tasks (functions without return values)

    std::mutex queue_mutex;                         // Mutex to protect access to the task queue
    std::condition_variable condition;              // Condition variable to signal workers about new tasks

    bool stop;                                      // Flag to signal worker threads to stop
};

// --- Example Usage ---
int main() {
    std::cout << "--- Thread Pool Tutorial ---" << std::endl;

    // 1. Create a Thread Pool:
    // We instantiate a ThreadPool with 4 worker threads. These threads start immediately
    // and wait for tasks.
    std::cout << "Initializing Thread Pool with 4 threads..." << std::endl;
    ThreadPool pool(4);

    // 2. Enqueue Tasks:
    // We add 10 tasks to the pool. Each task is a lambda function that prints a message
    // and simulates some work with a short sleep.
    std::cout << "Enqueuing 10 tasks..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i] { // This is a lambda function acting as our task.
            std::cout << "Task " << i
                      << " is running in thread ID: " << std::this_thread::get_id()
                      << std::endl;
            // Simulate some work being done by the task
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
    }

    std::cout << "All tasks enqueued. Main thread continues..." << std::endl;

    // 3. Waiting for Tasks (Simplified):
    // In this example, the main thread will pause for a moment to allow tasks to run.
    // When `main` exits, the `pool` object's destructor will be automatically called,
    // which then gracefully stops and joins all worker threads. This ensures all
    // enqueued tasks (if any are still being processed) have a chance to complete.
    // For more robust waiting in a real application, you might use std::future or
    // a task counter. For this tutorial, the graceful shutdown on destruction is key.
    std::cout << "Main thread sleeping for 3 seconds to allow tasks to complete..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "Main thread done sleeping. Thread pool will now be destroyed." << std::endl;

    // When 'pool' goes out of scope here, its destructor will be called,
    // which will stop all worker threads and join them.
    std::cout << "--- Thread Pool Demonstration Complete ---" << std::endl;

    return 0;
}