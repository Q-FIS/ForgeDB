// A fixed set of worker threads that run submitted tasks.   (component POOL; issue POOL-2)
#pragma once

#include <cstddef>
#include <functional>

#include "forge/executor.hpp"

namespace forge {

class ThreadPool final : public Executor {
public:
    /// Starts `threads` workers (>= 1). At most `queue_capacity` (>= 1) tasks
    /// can wait for a worker. Bad sizes: std::invalid_argument.
    ThreadPool(std::size_t threads, std::size_t queue_capacity);

    /// Calls stop().
    ~ThreadPool() override;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Queues the task for a worker. Never blocks: returns false if the queue
    /// is full or the pool is stopped (the server then answers 503).
    bool submit(std::function<void()> task) override;

    /// Takes no more tasks, lets the workers finish every task already
    /// queued, then joins them. Safe to call twice.
    void stop();

    std::size_t threads() const;

private:
    // POOL-2: your members go here, e.g. a BoundedQueue<std::function<void()>>
    // and a std::vector<std::jthread>.
};

}  // namespace forge
