# [POOL-1] A thread-safe queue with a size limit: `BoundedQueue<T>`
labels: comp:pool, size:M
milestone: v0.1 First search

**Component:** POOL (concurrency) · **Size:** M (2 days) · **Needs:** nothing, start on day 1
**Contract and your code:** `include/forge/bounded_queue.hpp` (a template: the code goes in the header) · **Tests:** `tests/contract/pool/bounded_queue_test.cpp`

## What is this, in plain words?
The thread pool (POOL-2) needs a queue where the server puts tasks and the
workers take them, from many threads at once. Two rules make it useful:
- **bounded**: at most `capacity` items. When it's full, the server answers 503 instead of piling up work until memory runs out (that's "backpressure");
- **blocking**: a worker with nothing to do sleeps in `pop()` (using no CPU) until an item arrives.

This is the classic producer-consumer problem, solved with one mutex and two
condition variables. It's the most important pattern in concurrent programming.

## What you must give back
Every member function of `BoundedQueue<T>`, as the comments in the header describe.

## Step by step
1. Delete `src/pool/stub_bounded_queue.cpp` (the tests switch on).
2. Members: `mutable std::mutex mutex_` (`mutable` so `size()` can lock it in a `const` function), `std::condition_variable not_empty_, not_full_`, `std::deque<T> items_`, `std::size_t capacity_`, `bool closed_ = false`.
3. `push`: lock; `not_full_.wait(lock, [&]{ return closed_ || items_.size() < capacity_; })`; if closed, return false; `push_back(std::move(item))`; unlock; `not_empty_.notify_one()`.
4. `pop`: the mirror image. Return `std::nullopt` only when closed **and** empty.
5. `try_push`: like `push`, but never waits.
6. `close`: set `closed_` under the lock, then `notify_all()` on **both** condition variables.
7. Write the definitions below the class, each starting with `template <class T>`.

## Watch out for
- Always wait with a **predicate** (the lambda). Condition variables can wake up for no reason ("spurious wakeups"), and the predicate re-checks.
- Don't hold the lock longer than needed, and never call user code (like a task) while holding it.
- Use `std::move` when pushing and popping: the tests use `std::unique_ptr`, which can't be copied.

## How to test it
```bash
ctest --preset debug -R contract.pool.BoundedQueue
ctest --preset tsan  -R contract.pool.BoundedQueue    # ThreadSanitizer: the real test for this code
```

## Done when
- [ ] Every `contract.pool.BoundedQueue.*` test passes on `debug` **and** `tsan`
- [ ] In the PR, explain in 3 sentences why `close()` must use `notify_all` and not `notify_one`

## Learn first
- *Operating Systems: Three Easy Pieces*, chapter 30, "Condition Variables" (the producer/consumer section).
- cppreference: `std::condition_variable::wait` with a predicate.
- Lab 3 (threads) covers the same ideas: do them side by side.
