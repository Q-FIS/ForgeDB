# L6 · How the CPU actually runs your code
**Week 6 · C++ · about 4 hours** · *The test arrives before week 6.*

## What is this, in plain words?
Two programs that do the **same** amount of work can differ in speed by 10x,
just because of the order in which they touch memory. The CPU reads memory in
**cache lines** (64 bytes at a time), and keeps recently used lines close by.
Code that walks memory in order is fast; code that jumps around is slow. You'll
predict, measure, and explain.

## Build
Four benchmarks in `bench.cpp`, each timed with `std::chrono::steady_clock` (release build):
1. **Matrix:** sum a 4096 × 4096 `float` matrix row by row, then column by column.
2. **Array of structs vs struct of arrays:** 10 million particles with `x, y, z, mass`; sum only `mass`. Compare `std::vector<Particle>` with four separate `std::vector<float>`s.
3. **False sharing:** two threads each increment their own counter 100 million times. Counters next to each other in memory vs each one padded to its own cache line (`alignas(64)`).
4. **List vs array:** sum 10 million numbers stored in a `std::vector` vs a `std::list`.

## Before you run anything
Write your **predictions** in `NOTES.md`: for each benchmark, which is faster, and by roughly how much. Commit them before you measure. Being wrong is fine: it's the most useful part of the lab.

## Measure
`perf stat -e cache-misses,cycles,instructions ./bench <n>` for each benchmark. Explain every result using cache lines, prefetching and memory latency.

## Done when
- [ ] Predictions committed **before** the results (git history shows it)
- [ ] `NOTES.md`: predictions, measurements, `perf` numbers, explanations

## Extra
Paste the inner loop of benchmark 2 into Compiler Explorer (godbolt.org) with `-O3 -march=native`, and find the SIMD instructions (`vaddps`...). Why can the struct-of-arrays version use them?

## Read
- *Computer Systems: A Programmer's Perspective*, chapter 6 ("The Memory Hierarchy").
- "Latency Numbers Every Programmer Should Know".
