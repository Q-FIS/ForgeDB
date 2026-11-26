# [CONN-6] Experiment: threads vs epoll, at 10, 100 and 1,000 connections
labels: comp:conn, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** CONN (with POOL) · **Size:** S · **Needs:** CONN-5, LOAD-2
**Your output:** `docs/experiments/CONN-6-threads-vs-epoll.md`

## What is this, in plain words?
You built both designs. Which is faster, and when? Measure instead of guessing.

## What you must give back
A report (template in `docs/experiments/README.md`) answering:
1. Throughput (requests/s) and latency (p50, p99) of `GET /health` and of a vector search (k = 10, 10,000 chunks of dim 384), for 10, 100 and 1,000 connections, in both `--io threads` and `--io epoll`, with the same `--threads`.
2. Memory (RSS, from `/proc/<pid>/status`) and thread count with 1,000 idle connections, in both modes.
3. Where does each design spend its time at 1,000 connections? (`perf top -p <pid>` or `perf record`; context switches from `/proc/<pid>/status` or `perf stat -e context-switches`.)

## Step by step
1. Release build only. Load 10,000 chunks with `forge load` (CLI-2), or a small script.
2. Use `forge-load` (LOAD-2) for the load; run each point 3 times; report the median.
3. Keep the load generator on different CPU cores from the server if you can (`taskset`).

## Watch out for
- With threads mode and 1,000 connections, `--threads` must be large enough to serve them all at once, or most wait in the queue. Explain what you chose and why.
- The machine's open-file limit (`ulimit -n`) must be above 1,000 for both programs.

## Done when
- [ ] The report is merged, with the commands to repeat it
- [ ] You presented it at a weekly meeting (5 minutes)

## Learn first
- What is a context switch, and what does it cost? Why would 1,000 threads cause many?
