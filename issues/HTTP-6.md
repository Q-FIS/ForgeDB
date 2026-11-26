# [HTTP-6] Experiment: how fast is our parser, and can we make it faster?
labels: comp:http, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** HTTP · **Size:** S · **Needs:** HTTP-4
**Your output:** `bench/http_parser.cpp`, `docs/experiments/HTTP-6-parser-speed.md`

## What is this, in plain words?
Every request goes through the parser. How many requests per second can it
parse on one core, where does the time go, and can one change (measured, not
guessed) make it faster?

## What you must give back
1. `bench/http_parser.cpp`: parses a typical 300-byte search request a million times; prints requests/s and ns/request. (It's built as `bench_http_parser`.)
2. A `perf record` / `perf report` of it: the three functions where the time goes.
3. One optimisation (e.g. fewer copies, `reserve`, `memchr` instead of a loop, no `std::string` for header names), with before/after numbers, and the contract tests still passing.
4. The report: `docs/experiments/HTTP-6-parser-speed.md`.

## Step by step
```bash
cmake --preset release && cmake --build --preset release --target bench_http_parser
./build/release/bench_http_parser
perf record -g ./build/release/bench_http_parser && perf report
```

## Watch out for
- The compiler may delete work whose result isn't used. Use the result (e.g. add up `consumed` and print it).
- Run each measurement 5 times; report the median.

## Done when
- [ ] The benchmark, the optimisation (with tests passing) and the report are merged

## Learn first
- Brendan Gregg's `perf` examples page: `perf stat`, `perf record`, `perf report`.
