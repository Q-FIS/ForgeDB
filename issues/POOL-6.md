# [POOL-6] Experiment: how many threads?
labels: comp:pool, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** POOL · **Size:** S · **Needs:** VEC-5, LOAD-2
**Your output:** `docs/experiments/POOL-6-how-many-threads.md`

## What is this, in plain words?
More threads should mean more speed, until it doesn't. Find the curve, and
explain where it bends and why.

## What you must give back
A report answering:
1. **One search, split.** With 100,000 vectors of dim 384, how long does one vector search take with `parallel_for` using 1, 2, 4, 8, 16 pieces? (A small benchmark in `bench/`.) Plot speed-up vs pieces.
2. **Many searches, one each.** Throughput of `forgedb --threads N` for N = 1, 2, 4, 8, 16, 32 under `forge-load` with 64 connections doing searches.
3. Why each curve flattens: number of cores (`nproc`), memory bandwidth (100,000 × 384 floats = 150 MB read per search!), queueing.

## Watch out for
- Release build only. Close other programs. Report the median of 5 runs.
- Hyper-threading: `nproc` may show twice the real cores. Check `lscpu`.

## Done when
- [ ] The benchmark and the report are merged, and presented at a weekly meeting

## Learn first
- Amdahl's law: the one formula, and what it predicts here.
- What limits a loop that reads 150 MB: the CPU, or the memory? (A first look at "memory-bound".)
