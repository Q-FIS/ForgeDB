# [POOL-2] The thread pool
labels: comp:pool, size:M
milestone: v0.1 First search

**Component:** POOL · **Size:** M (2 days) · **Needs:** POOL-1
**Contract:** `include/forge/thread_pool.hpp` · **Your code:** `src/pool/thread_pool.cpp` · **Tests:** `tests/contract/pool/thread_pool_test.cpp`

## What is this, in plain words?
Starting a thread for every request is slow, and with 10,000 requests you'd
have 10,000 threads. A **thread pool** starts N threads once; they loop
forever, taking tasks from your `BoundedQueue` and running them. The server
submits each connection as a task. If the queue is full, `submit` says no
right away, and the client gets a 503.

## What you must give back
`ThreadPool`'s constructor, destructor, `submit`, `stop` and `threads`, as the header describes.

## Step by step
1. Delete `src/pool/stub_thread_pool.cpp`; create `src/pool/thread_pool.cpp`.
2. Members (in the header): `BoundedQueue<std::function<void()>> queue_;` and `std::vector<std::jthread> workers_;`. Declare the queue **before** the workers: members are built in the order they're declared, and the workers use the queue.
3. Constructor: check the sizes; start the workers. Each worker runs `while (auto task = queue_.pop()) (*task)();`
4. `submit`: `queue_.try_push(std::move(task))`: never blocks.
5. `stop`: `queue_.close()` (the workers finish what's queued, then `pop()` returns `nullopt` and the loops end), then join every worker. Make a second call harmless.

## Watch out for
- A task that throws inside a `std::jthread` ends the whole program (`std::terminate`). POOL-4 deals with it; for now just know.
- `std::jthread` joins itself when destroyed: handy, but `stop()` must still close the queue first, or the join waits forever.

## How to test it
```bash
ctest --preset debug -R contract.pool.ThreadPool
ctest --preset tsan  -R contract.pool.ThreadPool
```

## Done when
- [ ] Every `contract.pool.ThreadPool.*` test passes on `debug` and `tsan`

## Learn first
- cppreference: `std::jthread` (and how it differs from `std::thread`), `std::function`.
- Why do servers use pools instead of one thread per request? What does a thread cost (memory for its stack; time to create it)?
