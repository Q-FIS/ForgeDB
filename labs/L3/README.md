# L3 · Threads: races, locks and a queue
**Week 3 · C++ · about 4 hours**

## What is this, in plain words?
Threads share memory. That makes them fast, and it makes them dangerous: two
threads doing `count++` at the same time can **lose updates**, because `++` is
really three steps (read, add, write). You'll create that bug on purpose,
watch ThreadSanitizer catch it, fix it three ways, and then build the most
useful tool in concurrent programming: a queue that threads can safely share.

## Build
1. **`counter MODE THREADS ITERS`**: start THREADS `std::thread`s; each adds 1 to a shared counter ITERS times. Print the final value (just the number).
   - `MODE` = `none`: a plain `long`, no protection (the bug);
   - `mutex`: protected by a `std::mutex` with `std::lock_guard`;
   - `atomic`: a `std::atomic<long>`.
2. **`queue_test PRODUCERS CONSUMERS ITEMS`**: a **bounded queue** (capacity 1024) built with `std::mutex` and two `std::condition_variable`s (one for "not full", one for "not empty").
   - Producers push the numbers 0 to ITEMS-1 between them (each number exactly once). Consumers pop until everything has been consumed. Use a "poison pill" or a `closed` flag to tell consumers to stop.
   - Each consumer records what it saw. At the end, check every number arrived exactly once, and print exactly one line:
     `produced <N> consumed <N> missing <M> duplicates <D>`

## Measure
With the **release** build, time `counter` for each mode at 1, 2, 4 and 8 threads (ITERS = 10,000,000). Make a table. Why does `mutex` get **slower** with more threads? Why is `atomic` faster than `mutex`, but still slower with 8 threads than with 1?

## Observe
- Build with the `tsan` preset and run `counter none 4 100000`. Read ThreadSanitizer's report: which line does it point at, and what does "data race" mean?
- `strace -f -e trace=clone,clone3,futex ./counter mutex 4 1000000`: find the `clone` calls behind `std::thread`, and the `futex` calls behind `std::mutex` when threads have to wait.

## The test
```bash
python3 L3/test_l3.py build/debug/L3
```
`mutex` and `atomic` must give exactly THREADS × ITERS. `queue_test` must move 10 million items with 4 producers and 4 consumers: nothing missing, nothing duplicated. Run it on the `tsan` build too: `python3 L3/test_l3.py build/tsan/L3`.

## Done when
- [ ] The test passes on `debug` and on `tsan`
- [ ] `NOTES.md`: your table, your answers, and the TSan report explained

## Extra
Two threads, two mutexes, locked in opposite orders: make a deadlock, find it with `gdb` (`thread apply all bt`), and fix it with a fixed lock order (or `std::scoped_lock`). Then read about `std::memory_order`.

## Read
- *Operating Systems: Three Easy Pieces*, chapters 26-30 (concurrency, locks, condition variables).
- *A Tour of C++*, chapter 18 ("Concurrency").
