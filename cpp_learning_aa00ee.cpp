// Learning Objective:
// To understand and implement a thread-safe, generic producer-consumer queue
// using C++ synchronization primitives: std::mutex and std::condition_variable.
// This tutorial demonstrates how to coordinate multiple threads safely when
// they share a common resource (the queue) to prevent race conditions and
// manage workflow (producers wait if full, consumers wait if empty).

#include <queue>              // For std::queue
#include <mutex>              // For std::mutex, std::unique_lock
#include <condition_variable> // For std::condition_variable
#include <iostream>           // For std::cout
#include <thread>             // For std::thread
#include <vector>             // For std::vector
#include <chrono>             // For std::chrono::milliseconds

// Define a generic template for our thread-safe queue.
// This allows the queue to store any type T, like int, std::string, or custom objects.
template<typename T>
class ProducerConsumerQueue {
private:
    std::queue<T> q_;                      // The underlying queue to store elements.
    std::mutex mtx_;                       // Mutex to protect access to the queue (q_).
                                           // WHY: Prevents multiple threads from modifying q_ simultaneously,
                                           //      which would lead to data corruption (race conditions).
    std::condition_variable cv_empty_;     // Condition variable for consumers to wait when the queue is empty.
                                           // WHY: Consumers should block and wait efficiently if there's nothing
                                           //      to process, instead of constantly checking (spinning), which wastes CPU.
    std::condition_variable cv_full_;      // Condition variable for producers to wait when the queue is full.
                                           // WHY: Producers should block and wait efficiently if there's no
                                           //      space to add new items, preventing unbounded memory growth.
    size_t max_size_;                      // Maximum capacity of the queue.

public:
    // Constructor to initialize the queue with a maximum capacity.
    explicit ProducerConsumerQueue(size_t max_size) : max_size_(max_size) {}

    // Method for producers to add an item to the queue.
    void push(T item) {
        // WHAT: Acquire a unique_lock on the mutex.
        // WHY: `std::unique_lock` automatically locks the mutex upon creation
        //      and unlocks it when it goes out of scope (RAII - Resource Acquisition Is Initialization).
        //      It's also required by `std::condition_variable::wait()` because `wait()` temporarily
        //      releases the lock, and `unique_lock` manages this re-locking automatically.
        std::unique_lock<std::mutex> lock(mtx_);

        // WHAT: Wait if the queue is full.
        // WHY: Producers must not add items to a full queue. `cv_full_.wait()`
        //      atomically releases the lock and puts the current thread to sleep.
        //      When notified (or spuriously woken), it re-acquires the lock
        //      and re-evaluates the predicate (the lambda function).
        //      The predicate `[this]{ return q_.size() < max_size_; }`
        //      ensures we only proceed if there's actual space, protecting against spurious wakeups.
        cv_full_.wait(lock, [this]{ return q_.size() < max_size_; });

        // WHAT: Add the item to the queue.
        // WHY: This operation is now safe because the mutex is locked, guaranteeing exclusive access.
        q_.push(std::move(item)); // Use std::move for efficiency, especially with complex objects,
                                  // avoiding unnecessary copies.
        std::cout << "Produced: " << item << " (Queue size: " << q_.size() << ")\n";

        // WHAT: Notify one waiting consumer that an item is now available.
        // WHY: A consumer thread waiting on `cv_empty_` can now potentially wake up
        //      and process the newly added item. `notify_one()` is often
        //      sufficient, as only one consumer needs to know there's an item to proceed.
        cv_empty_.notify_one();
    }

    // Method for consumers to retrieve an item from the queue.
    T pop() {
        // WHAT: Acquire a unique_lock on the mutex.
        // WHY: Protects access to the queue during popping to prevent race conditions.
        std::unique_lock<std::mutex> lock(mtx_);

        // WHAT: Wait if the queue is empty.
        // WHY: Consumers must not try to retrieve items from an empty queue.
        //      Similar to `push`, `cv_empty_.wait()` blocks until an item
        //      is available, releasing and re-acquiring the lock as needed,
        //      and checking the predicate `!q_.empty()`.
        cv_empty_.wait(lock, [this]{ return !q_.empty(); });

        // WHAT: Retrieve the item from the front of the queue.
        // WHY: This is safe as the mutex is locked and we know the queue isn't empty (due to the wait).
        T item = std::move(q_.front()); // Move the item out of the queue for efficiency.
        q_.pop();                        // Remove the item from the queue.
        std::cout << "Consumed: " << item << " (Queue size: " << q_.size() << ")\n";

        // WHAT: Notify one waiting producer that space is now available.
        // WHY: If producers were blocked because the queue was full, one can
        //      now potentially wake up and add an item.
        cv_full_.notify_one();

        return item; // Return the consumed item.
    }
};

// Example usage demonstrating producers and consumers.
int main() {
    // WHAT: Create an instance of our thread-safe queue with a maximum capacity of 5.
    // WHY: A limited capacity demonstrates the "producer waits if full" logic.
    ProducerConsumerQueue<int> queue(5);

    // Number of items each producer will try to add.
    const int items_per_producer = 10;
    // Total items consumers expect to consume (ensure producers make enough).
    // With 2 producers making 10 items each, total is 20.
    const int total_items_to_consume = 20;

    // WHAT: Create a vector to hold producer threads.
    std::vector<std::thread> producers;
    // WHAT: Create a vector to hold consumer threads.
    std::vector<std::thread> consumers;

    // WHAT: Launch two producer threads.
    // WHY: Demonstrates multiple threads concurrently adding items to the shared queue.
    for (int i = 0; i < 2; ++i) {
        // Use a lambda to define the thread's task. `[&queue]` captures the queue by reference.
        producers.emplace_back([&queue, i, items_per_producer]() {
            for (int j = 0; j < items_per_producer; ++j) {
                // Simulate some work or variable production rate before producing.
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                // Add an item to the queue. The producer will wait if the queue is full.
                queue.push(i * items_per_producer + j + 1); // Generate unique item values
            }
        });
    }

    // WHAT: Launch two consumer threads.
    // WHY: Demonstrates multiple threads concurrently retrieving items from the shared queue.
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&queue, i, total_items_to_consume]() {
            // Each consumer tries to consume a portion of the total items.
            // This is a simple way to stop consumers for this example.
            for (int j = 0; j < total_items_to_consume / 2; ++j) {
                // Simulate some work or variable consumption rate after consuming.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                // Retrieve and print an item from the queue. The consumer will wait if the queue is empty.
                queue.pop();
            }
        });
    }

    // WHAT: Wait for all producer threads to finish their work.
    // WHY: `join()` ensures the main thread waits until the producer thread completes
    //      its execution, preventing the main thread from exiting prematurely while
    //      producers are still running or have not yet produced all their items.
    for (std::thread& p : producers) {
        p.join();
    }

    // WHAT: Wait for all consumer threads to finish their work.
    // WHY: `join()` ensures the main thread waits until the consumer thread completes.
    //      This makes sure all intended items are processed before the program terminates.
    for (std::thread& c : consumers) {
        c.join();
    }

    std::cout << "\nAll producers and consumers have finished.\n";
    return 0;
}