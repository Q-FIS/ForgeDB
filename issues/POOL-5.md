# [POOL-5] `parallel_for`: split a loop across the pool
labels: comp:pool, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** POOL · **Size:** M · **Needs:** POOL-2
**Contract:** `include/forge/parallel.hpp` · **Your code:** `src/pool/parallel.cpp` · **Tests:** `tests/contract/pool/parallel_test.cpp`

## What is this, in plain words?
A vector search compares the query with every vector: 1 million comparisons
on one core. Split them into 8 pieces on 8 cores, and it's (nearly) 8× faster.
`parallel_for` is the tool: give it a range and a function, and it runs the
pieces on the pool at the same time and waits for all of them. VEC-5 uses it
for search.

## What you must give back
`parallel_for`, exactly as the header's comment describes, including its two hard parts:
1. **No deadlock when called from inside a pool task.** A search runs on a pool worker. If it then waits for pieces queued on the same pool, and every worker is waiting like this, nobody runs the pieces. Fix: the **caller works too**. Pieces are numbered; every runner (the caller and the pool tasks) repeatedly takes the next unclaimed number (`std::atomic<std::size_t>::fetch_add`) until none are left. The caller only waits for pieces that someone else has already started.
2. **Pool tasks may start after `parallel_for` has returned** (they were queued, but the caller finished everything). They find no pieces left and do nothing, but they still touch the shared state. So the state (the counter, the mutex, the "done" count) must live in a `std::shared_ptr`, not on the caller's stack.

## Step by step
1. Delete `src/pool/stub_parallel.cpp`; create `src/pool/parallel.cpp`.
2. Piece `i` covers `[i*n/pieces, (i+1)*n/pieces)`; skip empty ones.
3. Submit `pieces - 1` runner tasks (stop if `submit` refuses), then run the same runner on the caller.
4. Count finished pieces under a mutex; the caller waits on a condition variable until all are done; keep the first exception (`std::exception_ptr`) and rethrow it at the end.

## How to test it
```bash
ctest --preset debug -R contract.pool.ParallelFor
ctest --preset tsan  -R contract.pool.ParallelFor
```

## Done when
- [ ] Every `contract.pool.ParallelFor.*` test passes on `debug` and `tsan`

## Learn first
- cppreference: `std::exception_ptr`, `std::current_exception`, `std::rethrow_exception`.
- "Work stealing" (just the idea): why letting idle threads take work beats a fixed split.
