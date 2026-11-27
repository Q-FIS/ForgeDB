# [VEC-3] Cache-friendly search
labels: comp:vec, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** VEC · **Size:** M · **Needs:** VEC-2
**Your code:** `src/vec/flat_index.cpp`

## What is this, in plain words?
For large n, search speed is limited by **memory**, not arithmetic: the CPU
waits for vectors to arrive from RAM. The ways to win are about memory: read
it in order (so the hardware prefetcher helps), read less of it, and use each
byte you fetch as much as possible.

## What you must give back
At least two of these, each measured with `bench_flat_index` and `perf stat`, kept only if it helps:
1. Make sure the inner loop auto-vectorises: compile with `-O3 -march=native` in a local test build, and check with `-fopt-info-vec` or by reading the assembly (`objdump -d`, or godbolt.org).
2. **Several queries at once** (a batch): read each vector once and compare it with 8 queries while it's in the cache. Add `search_batch` if you do this (agree it with the lead).
3. Store vectors as 16-bit floats or 8-bit integers (quantisation): half or a quarter of the memory to read. Measure the speed-up **and** how much the results change (top-10 overlap with the exact float version).
4. Your own idea, backed by a `perf stat -e cache-misses,cache-references,instructions,cycles` comparison.

## Done when
- [ ] The contract tests still pass; the PR shows before/after numbers and `perf stat` output for each change

## Learn first
- "What every programmer should know about memory" (Ulrich Drepper), sections 3 and 6: skim, read the pictures.
- What is a cache line? Why does reading memory in order matter so much?
