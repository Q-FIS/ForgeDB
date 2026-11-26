# [KW-3] BM25: score words the way real search engines do
labels: comp:kw, size:M
milestone: v0.2 Survives a crash

**Component:** KW · **Size:** M (2 days) · **Needs:** KW-2
**Your code:** `src/kw/keyword_index.cpp` · **Tests:** `tests/unit/kw/test_bm25.cpp` (yours), plus the KW-2 contract tests

## What is this, in plain words?
Counting words has two problems. A common word like "the" or "crore" counts as
much as a rare, meaningful one like "impairment". And a long chunk wins just
by being long. **BM25** fixes both: rare words weigh more (IDF), repeating a
word helps less and less (saturation), and long chunks are scaled down. It's
the standard keyword scoring of Elasticsearch and Lucene.

## What you must give back
The score of a row for a query is the sum, over the **distinct** query words
that appear in it, of:
```
idf(w) × tf × (k1 + 1) / (tf + k1 × (1 − b + b × len / avglen))
idf(w) = ln(1 + (N − n + 0.5) / (n + 0.5))
```
with `tf` = times w appears in the row, `len` = the row's number of tokens,
`avglen` = the average over all rows, `N` = number of rows, `n` = rows
containing w, `k1 = 1.2`, `b = 0.75`. (This `idf` is never negative.)

Your tests must reproduce these numbers (computed with that formula, to 4 decimals):

| Rows | Query | Expected (row: score) |
| --- | --- | --- |
| 0 "profit rose profit", 1 "revenue rose", 2 "profit margin fell sharply this year", 3 "net loss" | `profit` | 0: 0.9742, 2: 0.5149 |
| same | `profit rose` | 0: 1.6898, 1: 0.8226, 2: 0.5149 |
| same | `sharply` | 2: 0.8944 |

## Step by step
1. Store each row's length, and keep a running total for `avglen`.
2. In `search`, compute `idf` for each query word from its postings' length.
3. Write `tests/unit/kw/test_bm25.cpp` with the three cases (`EXPECT_NEAR(score, 0.9742, 1e-4)`).
4. The KW-2 contract tests must still pass.

## Watch out for
- Compute in `double`, store the final score as `float`.
- N and avglen change as rows are added: compute them at search time (or keep running totals), not once at the start.

## Done when
- [ ] Your BM25 tests and the KW contract tests pass on `debug`
- [ ] In the PR, one query on real report text where BM25 and counting disagree, and why BM25 is right

## Learn first
- *Introduction to Information Retrieval*, sections 6.2 (tf-idf) and 11.4.3 (BM25).
