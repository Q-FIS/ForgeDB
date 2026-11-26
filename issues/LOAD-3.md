# [LOAD-3] Open-loop load, and the coordinated omission trap
labels: comp:load, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** LOAD · **Size:** M · **Needs:** LOAD-2
**Your code:** `apps/forge_load.cpp` · **Your output:** `docs/experiments/LOAD-3-open-loop.md`

## What is this, in plain words?
Real users don't wait for each other: they send requests at their own rate.
A closed-loop tool (LOAD-2) slows down when the server does, so it
under-reports exactly the bad moments ("coordinated omission"). An
**open-loop** tool sends on a fixed schedule (e.g. 2,000 requests/s), and
measures each latency from when the request **should** have been sent.

## What you must give back
1. `--rate R`: requests per second in total, spread evenly over the connections. Latency = response time − **intended** send time. If a connection is busy when its next request is due, the request waits, and that wait counts.
2. A histogram with constant memory instead of storing every latency: buckets growing by ~10% (like HdrHistogram), accurate to 1 % at any value. Unit-test its percentiles against the exact ones.
3. A short report: run `forgedb` near and above its capacity (from LOAD-2's numbers) in both modes. Show how closed-loop p99 and open-loop p99 differ, with a server that stalls (e.g. `kill -STOP` it for 1 s, then `kill -CONT`).

## Done when
- [ ] `--rate` and the histogram work, with tests; the report is merged and presented

## Learn first
- Gil Tene, "How NOT to Measure Latency" (the coordinated omission part), and the HdrHistogram README.
