# [POOL-4] A tougher pool: failing tasks, statistics, and a stress test
labels: comp:pool, size:S
milestone: v0.2 Survives a crash

**Component:** POOL · **Size:** S · **Needs:** POOL-2
**Your code:** `include/forge/thread_pool.hpp`, `src/pool/thread_pool.cpp` · **Tests:** `tests/unit/pool/` (yours)

## What is this, in plain words?
Make the pool safe to run for days: a task that throws must not kill the
server, and the pool should report how busy it is, so `/metrics` (APP-6) can
show it and the experiments can explain their numbers.

## What you must give back
1. A task that throws is caught in the worker; the worker logs one line to stderr and keeps going.
2. Statistics, added to the header (agreed with the lead in this issue):
   ```cpp
   struct PoolStats {
       std::size_t queued;     // waiting in the queue now
       std::size_t running;    // being run by a worker now
       std::uint64_t done;     // finished since start
       std::uint64_t rejected; // refused by submit() since start
   };
   PoolStats stats() const;
   ```
   Use `std::atomic` counters, so reading them never blocks the workers.
3. Unit tests in `tests/unit/pool/`: throwing tasks don't stop the pool; the stats add up (`done + rejected` = everything submitted, after `stop()`); a stress test: 8 threads submitting 100,000 tasks while the pool runs them.

## Watch out for
- `queued` and `running` change all the time: a snapshot of them is approximate, and that's fine. `done` and `rejected` must be exact.
- `std::memory_order_relaxed` is enough for counters you only read for reporting. Explain why in the PR.

## How to test it
`ctest --preset tsan -R "pool"` (and `debug`).

## Done when
- [ ] Your tests pass on `debug` and `tsan`, and the contract tests still pass

## Learn first
- cppreference: `std::atomic`, `fetch_add`, and the memory orders (at least: relaxed vs seq_cst, what each promises).
