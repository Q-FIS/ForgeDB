# [KW-4] Compact postings: less memory, faster search
labels: comp:kw, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** KW · **Size:** M · **Needs:** KW-3
**Your code:** `src/kw/` · **Tests:** `tests/unit/kw/`

## What is this, in plain words?
Postings lists are most of the index's memory. Store them compactly: rows
are sorted, so store the **gaps** between them (small numbers), and write
small numbers in fewer bytes (**varint**: 7 bits per byte, the top bit says
"more bytes follow"). The same trick is in Lucene, protobuf and SQLite.

## What you must give back
1. A varint encoder/decoder with unit tests (0, 127, 128, 16383, 16384, the largest `uint32_t`).
2. Postings stored as a `std::vector<std::uint8_t>` per word: (row gap, tf) pairs as varints.
3. Memory before/after for 100,000 chunks of report text (count the bytes of every postings list, or compare RSS), and search time before/after. In the PR.

## Watch out for
- Decoding must be fast: it's in the search loop. Measure it.
- The contract and BM25 tests must still pass: same results, different storage.

## Done when
- [ ] Tests pass; the PR has the memory and speed numbers

## Learn first
- *Introduction to Information Retrieval*, chapter 5, "Index compression", sections 5.3 (gap encoding, variable byte codes).
