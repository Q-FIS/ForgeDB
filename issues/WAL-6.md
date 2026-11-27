# [WAL-6] Experiment: fsync policies, speed vs safety
labels: comp:wal, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** WAL · **Size:** S · **Needs:** WAL-3, LOAD-2
**Your output:** `docs/experiments/WAL-6-fsync.md`

## What is this, in plain words?
Measure what each `--fsync` policy costs, and explain what each one buys.

## What you must give back
A report with:
1. Puts per second and put latency (p50, p99) with 1, 8 and 64 concurrent clients, for `always`, `batch`, `never` (`forge-load --mode put`, release build).
2. How long one `fsync` takes on your disk (`bench/fsync.cpp`: write 4 KB + fsync, 1,000 times; report the median). Compare SSD vs your laptop vs a tmpfs (`/dev/shm`: what happens there, and why?).
3. With `batch`: how many puts share one fsync, on average, at each concurrency? (Count both in the Journal.)
4. A table "what can each policy lose" for: `kill -9`, kernel panic, power cut.

## Done when
- [ ] The report and `bench/fsync.cpp` are merged, and presented

## Learn first
- Why is an fsync so slow compared to a write? What is the disk doing?
