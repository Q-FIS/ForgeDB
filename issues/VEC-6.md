# [VEC-6] Experiment: where does a search spend its time?
labels: comp:vec, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** VEC · **Size:** S · **Needs:** VEC-3, VEC-4, VEC-5
**Your output:** `docs/experiments/VEC-6-search-speed.md`

## What is this, in plain words?
Tell the whole story of making one search fast, step by step, with numbers.

## What you must give back
A report with one row per version, for n = 1,000,000 (or the most your RAM holds), dim 384, k = 10:
naive (sort everything) → top-k heap → contiguous layout → `-O3` auto-vectorised → AVX2 → all cores.
For each: median search time, speed-up over naive, and GB/s of vectors read.
Then answer: what is the final limit, compute or memory bandwidth? How do you know? How far are you from that limit?

## Done when
- [ ] The report is merged and presented

## Learn first
- The "roofline model", in its simplest form: arithmetic intensity, and what it tells you here.
