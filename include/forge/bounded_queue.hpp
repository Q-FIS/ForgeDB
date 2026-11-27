// A thread-safe queue with a maximum size.   (component POOL; issue POOL-1)
//
// A template, so all its code lives in this header: write the member
// functions below the class. You may add private members.
#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

namespace forge {

template <class T>
class BoundedQueue {
public:
    /// capacity >= 1, else std::invalid_argument.
    explicit BoundedQueue(std::size_t capacity);

    /// Adds an item, waiting while the queue is full. Returns false (and
    /// drops the item) if the queue is closed, even while waiting.
    bool push(T item);

    /// Adds an item only if there's room right now. Never waits. Returns
    /// false (and drops the item) if the queue is full or closed.
    bool try_push(T item);

    /// Takes the oldest item, waiting while the queue is empty. Returns
    /// std::nullopt only when the queue is closed AND empty: items pushed
    /// before close() are still handed out.
    std::optional<T> pop();

    /// No more pushes. Wakes every thread waiting in push() or pop().
    /// Calling it twice is fine.
    void close();

    std::size_t size() const;
    std::size_t capacity() const;

private:
    // POOL-1: your members go here, e.g. a std::mutex, two
    // std::condition_variable, a std::deque<T>, the capacity, a closed flag.
};

// POOL-1: write the member functions here, e.g.
//   template <class T>
//   bool BoundedQueue<T>::push(T item) { ... }

}  // namespace forge
