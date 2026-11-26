# [KW-5] Hybrid search: merge keyword and vector results
labels: comp:kw, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** KW (changes POOL's Engine: POOL reviews) · **Size:** M · **Needs:** KW-2, VEC-1, POOL-3
**Contract:** `rrf_merge` in `include/forge/keyword_index.hpp` · **Your code:** `src/kw/rrf.cpp`, `src/pool/engine.cpp` · **Tests:** `tests/contract/kw/rrf_test.cpp`

## What is this, in plain words?
Vector search finds meaning ("company earnings" finds "net profit"); keyword
search finds exact terms ("1,45,000", "FY2025"). Hybrid search runs both and
merges the rankings with **Reciprocal Rank Fusion**: each chunk scores
`1/(60 + rank)` in each list it appears in, summed. It only uses positions,
so it doesn't care that the two kinds of scores aren't comparable. Simple,
and it works remarkably well.

## What you must give back
1. `rrf_merge`, as the header describes.
2. In `Engine::search`, `mode == hybrid`: take the best `max(k, 100)` rows from each index (with the filter), `rrf_merge` them with `c = 60`, keep the best k. Remove the `NotImplemented`.
3. A unit test in `tests/unit/pool/` (or `kw/`) for hybrid through the Engine, using the fakes.

## Step by step
1. Delete `src/kw/stub_rrf.cpp`; create `src/kw/rrf.cpp`; pass the contract tests.
2. Then the Engine change, with the POOL owner as reviewer.
3. Check with `blackbox.smoke` and by hand with `forge search --text ... --vector ...`.

## Done when
- [ ] `contract.kw.Rrf.*` passes; hybrid works end to end; the POOL owner approved

## Learn first
- The RRF paper's abstract (Cormack, Clarke, Büttcher, 2009): what they compared it with, and what they found.
