# [APP-6] `/metrics`: what is the server doing?
labels: comp:app, size:M
milestone: v0.2 Survives a crash

**Component:** APP · **Size:** M · **Needs:** APP-4 (POOL-4 for the pool's numbers)
**Your code:** `src/app/metrics.cpp` + header changes · **Tests:** `tests/unit/app/` (yours)

## What is this, in plain words?
You can't fix what you can't see. `GET /metrics` reports how many requests
each route got (by status code), how long they took, and how busy the pool
is, in the plain-text Prometheus format that monitoring tools read. Every
experiment will use it.

## What you must give back
1. A `Metrics` class (in `include/forge/app.hpp`, agreed in this issue) that's safe to update from many threads without a big lock:
   - `forge_requests_total{route,status}`: a counter per (route, status);
   - `forge_request_seconds_bucket{route,le}`: a histogram per route, buckets 0.0005, 0.001, 0.0025, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, +Inf seconds, plus `_sum` and `_count`;
   - `forge_chunks`, and the pool's `queued`, `running`, `rejected` (from POOL-4).
2. The API times every request and records it. Use the **route pattern** (`/v1/chunks/{id}`), not the actual path, or you get one counter per chunk id! One way: in `Api`'s constructor, wrap each handler before you `router_.add` it, in a lambda that notes which pattern matched (e.g. in a `thread_local` variable); then `handle`, which turns exceptions into status codes, records the time and the **final** status under that pattern. Requests that match no route (404/405) are recorded as `route="other"`.
3. `GET /metrics` returns it with `Content-Type: text/plain; version=0.0.4`.
4. Unit tests: after N requests the counters say N; the histogram buckets add up; 8 threads × 10,000 records give exact totals.

## Watch out for
- Updating counters is on the hot path of every request: use `std::atomic` with relaxed order, or one counter set per thread. Measure the cost (HTTP-6's benchmark style).
- Buckets are **cumulative** in Prometheus (`le="0.01"` counts everything ≤ 10 ms).

## Done when
- [ ] `curl localhost:8080/metrics` shows real numbers while `forge-load` runs
- [ ] Your unit tests pass on `debug` and `tsan`

## Learn first
- prometheus.io: "Exposition formats" (the text format) and "Histograms and summaries".
