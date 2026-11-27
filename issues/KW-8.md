# [KW-8] Faster top-k: skip chunks that can't win (WAND)
labels: comp:kw, size:L
milestone: v2 Stretch

**Component:** KW · **Size:** L (about a week) · **Needs:** KW-4
**Your code:** `src/kw/` · **Tests:** `tests/unit/kw/`, `bench/keyword.cpp`

## What is this, in plain words?
A query like `the company revenue` touches huge postings lists: "the" is in
almost every chunk. Today you score every one of them, then keep 10. Search
engines skip most of that work: each word has a **maximum possible score**,
and once you have 10 good results, any chunk whose best possible score is
below the 10th can be skipped without even scoring it. That's **WAND**
(Weak AND). The results are exactly the same; only faster.

## What you must give back
1. For each term, its maximum BM25 contribution over all its postings (update it as chunks are added).
2. Document-at-a-time search with WAND: a cursor per query term, sorted by current row; the "pivot" rule; a min-heap of the current top k and its threshold.
3. **Exactly the same top k** as the exhaustive search (same rows, same order, scores within 1e-5): a randomized test that compares both on 1,000 random queries over 50,000 generated chunks.
4. `bench/keyword.cpp`: queries/s, exhaustive vs WAND, for 1-word, 3-word and 8-word queries over 1 million chunks. In the PR.
5. Optional, harder: **Block-Max WAND**: a max score per block of 128 postings, for much tighter skipping.

## Watch out for
- Filters (`RowFilter`): a skipped chunk is never checked, which is fine; but a chunk that the filter refuses must not count towards the threshold.
- Ties: the exhaustive search orders equal scores by row. Make sure WAND's heap does too, or the randomized test will (rightly) fail.
- Cursors need a fast "jump to the first row ≥ r": with varint gaps that means skip pointers every N postings. Add them.

## Done when
- [ ] The randomized equality test passes; WAND is faster on long queries (numbers in the PR); presented at a meeting

## Learn first
- Broder et al., "Efficient Query Evaluation using a Two-Level Retrieval Process" (2003): the WAND paper. Then any blog post on "Block-Max WAND".
