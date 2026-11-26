# [VEC-2] Measure the flat index
labels: comp:vec, size:S
milestone: v0.2 Survives a crash

**Component:** VEC · **Size:** S · **Needs:** VEC-1
**Your code:** `bench/flat_index.cpp`

## What is this, in plain words?
Before making anything faster, measure it. A benchmark program that answers:
how many searches per second, at what size? It's your baseline for VEC-3,
VEC-4 and VEC-5, and every later number is compared with it.

## What you must give back
`bench/flat_index.cpp` (built as `bench_flat_index`): random vectors, dim
384, for n = 10,000, 100,000 and 1,000,000. For each n: time to add them all,
and the median / p99 time of one search (k = 10) over 200 queries. Print a
table, and also the **memory bandwidth** you reached: bytes of vectors read
per second (n × 384 × 4 bytes per search ÷ time).

## Step by step
1. Use `std::mt19937` with a fixed seed, so runs are comparable.
2. Time with `std::chrono::steady_clock`.
3. Release build only: `cmake --preset release && cmake --build --preset release --target bench_flat_index`.
4. Put the table in the PR, with your CPU (`lscpu`).

## Watch out for
- Use the results (e.g. add up the top scores and print them), or the optimiser may skip the work.
- 1,000,000 × 384 floats = 1.5 GB. Make sure your machine has the RAM, or stop at 300,000.

## Done when
- [ ] The benchmark is merged, with your baseline numbers in the PR

## Learn first
- What is memory bandwidth, and roughly how much does your machine have? (Look up your RAM type: DDR4 ≈ 20-25 GB/s per channel.)
