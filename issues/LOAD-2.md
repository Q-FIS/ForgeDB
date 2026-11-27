# [LOAD-2] `forge-load`: a load generator
labels: comp:load, size:M
milestone: v0.2 Survives a crash

**Component:** LOAD · **Size:** M (3 days) · **Needs:** CONN-1 (`Socket`)
**Your code:** `apps/forge_load.cpp` (+ `src/load/` if you like) · **Tests:** `tests/unit/load/` (yours)

## What is this, in plain words?
Every experiment needs the same tool: hit the server with N connections for T
seconds, and report how many requests per second it handled and how long they
took. Tools like `wrk` exist, but writing your own teaches you how
measurement goes wrong, and it speaks ForgeDB's API (searches and puts with
random vectors).

## What you must give back
`forge-load --port 8080 --connections 64 --duration 10 --mode search --dim 384 [--k 10] [--host H]`:
- `--mode`: `health`, `search` (random vectors), `put` (random vectors, unique ids like `load-<thread>-<n>`), or `keyword` (random words from a small list).
- Each connection: its own thread, keep-alive, send a request, wait for the response, repeat ("closed loop").
- The first second is warm-up: not counted.
- Record every latency (per-thread `std::vector`, merged at the end); errors (non-2xx, broken connections) counted separately.
- Output: a readable table, then one JSON line for scripts:
  `{"mode": "search", "connections": 64, "seconds": 10, "requests": 51234, "errors": 0, "rps": 5123.4, "p50_ms": 11.8, "p90_ms": 15.0, "p99_ms": 22.1, "max_ms": 48.3}`

## Step by step
1. Command-line parsing (like APP-1's: strict, clear errors, exit 2).
2. One connection's loop using `Socket`, `send_all`, `recv_some`; reading a response like CLI-1 does (pair with the CLI owner: maybe share the code in `src/load/` or `src/cli/`).
3. Percentiles: sort all latencies; p99 is the value at position `0.99 × (count − 1)`.
4. Unit tests for the percentile function and the response reader.

## Watch out for
- Measure with `std::chrono::steady_clock`, from just before `send` to the last byte of the response.
- Run the load generator on a release build: a slow load generator measures itself.
- Closed-loop tools hide slowness: if the server stalls for 1 s, your connections stop sending, and those lost requests never appear in the stats. LOAD-3 is about exactly this.

## Done when
- [ ] `forge-load` runs against `forgedb` and the fake server; unit tests pass; an example run is in the PR

## Learn first
- `wrk`'s README (how it works), and Gil Tene's talk "How NOT to Measure Latency" (first 20 minutes).
