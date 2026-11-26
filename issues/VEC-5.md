# [VEC-5] Search on all cores
labels: comp:vec, size:S
milestone: v1.0 Fast, and powers the RAG

**Component:** VEC · **Size:** S · **Needs:** POOL-5, VEC-1
**Your code:** `src/vec/flat_index.cpp`; one line in `apps/forgedb.cpp` (APP reviews)

## What is this, in plain words?
`make_flat_index(dim, pool)` already takes a pool. Use it: split the rows
into pieces with POOL's `parallel_for`, find the top k of each piece, and
merge them. One big search then uses every core.

## What you must give back
1. When the index has a pool and more than 20,000 rows, `search` uses `parallel_for` with one piece per pool thread (fewer rows: one thread is faster than the coordination).
2. Each piece keeps its own top-k (no shared state, no locks); then merge the pieces' lists, and keep the best k with `better`.
3. `forgedb` passes its pool to `make_flat_index`.
4. A unit test with a `ThreadPool`: results equal the single-thread search exactly, for random data.

## Watch out for
- Each piece writes only to its own result vector (e.g. `std::vector<std::vector<ScoredRow>> per_piece(pieces)`): no locks needed, no races.

## Done when
- [ ] Contract tests pass with and without a pool, on `debug` and `tsan`; the PR has before/after numbers

## Learn first
- POOL-5's issue, to know exactly what `parallel_for` promises.
