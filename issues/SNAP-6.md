# [SNAP-6] Experiment: `read` vs `mmap`
labels: comp:snap, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** SNAP · **Size:** S · **Needs:** SNAP-5
**Your output:** `bench/snapshot_load.cpp`, `docs/experiments/SNAP-6-read-vs-mmap.md`

## What is this, in plain words?
Is `mmap` actually faster? It depends, and finding out *on what* teaches you
the page cache.

## What you must give back
For a snapshot of 100,000 chunks of dim 384 (about 160 MB):
1. Load time with `read` vs `mmap`, **cold** (page cache dropped: `sync; echo 3 | sudo tee /proc/sys/vm/drop_caches`) and **warm** (run twice).
2. Peak memory (RSS) during the load, for each.
3. Page faults (`/usr/bin/time -v`: "Minor"/"Major page faults"), and what each kind means.
4. Your conclusion: when is mmap better, and when doesn't it matter?

## Done when
- [ ] The benchmark and report are merged, and presented

## Learn first
- What is a page fault, minor vs major? What is the page cache, and why does the second run go faster?
