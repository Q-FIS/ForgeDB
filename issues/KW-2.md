# [KW-2] The inverted index: find chunks by their words
labels: comp:kw, size:M
milestone: v0.1 First search

**Component:** KW · **Size:** M (2 days) · **Needs:** KW-1
**Contract:** `include/forge/keyword_index.hpp` · **Your code:** `src/kw/keyword_index.cpp` · **Tests:** `tests/contract/kw/keyword_index_test.cpp`

## What is this, in plain words?
Searching every chunk's text for a word is slow. An **inverted index** turns
it around: for each word, keep the list of rows that contain it (its
"postings"), and how many times. A query then only touches the lists of its
own words. Every search engine is built on this.

For now, the score is simple: **the number of times the query's words appear
in the chunk** (each distinct query word counted once). KW-3 replaces it with
BM25; the contract tests are written so they pass with both.

## What you must give back
`make_keyword_index` and a class implementing `KeywordIndex`, as the header describes.

## Step by step
1. Delete `src/kw/stub_keyword_index.cpp`; create `src/kw/keyword_index.cpp`.
2. `add(row, text)`: tokenize, count each word (a `std::map<std::string, int>` or `unordered_map`), then append `{row, count}` to each word's postings.
3. `search(query, k, keep)`: tokenize the query, remove duplicate words; for each word, walk its postings and add `count` to that row's score (a map from row to score); drop rows `keep` refuses; return the best k by `better`.
4. Remember the number of rows (`size()`), and refuse a row that isn't the next one.

## Watch out for
- Rows arrive in order, so each postings list is **sorted by row** for free. KW-4 relies on that.
- Ties (equal scores) must come out smallest row first: use `better`.

## How to test it
`ctest --preset debug -R contract.kw.KeywordIndex`

## Done when
- [ ] Every `contract.kw.KeywordIndex.*` test passes on `debug`

## Learn first
- *Introduction to Information Retrieval* (Manning, Raghavan, Schütze; free online), chapter 1, "Boolean retrieval": the inverted index.
