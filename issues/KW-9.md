# [KW-9] The keyword index on disk: immutable segments, merged in the background
labels: comp:kw, size:L
milestone: v2 Stretch

**Component:** KW (with SNAP: checkpoints write segments, recovery loads them) · **Size:** L (about a week) · **Needs:** KW-4, SNAP-4
**Your code:** `src/kw/`, a section in `docs/FORMAT.md` (agreed with the lead) · **Tests:** `tests/unit/kw/`

## What is this, in plain words?
Today the keyword index lives only in memory and is rebuilt at every startup
by tokenizing every chunk again. Lucene, Elasticsearch and Tantivy do it
differently: new chunks go into a small in-memory index; at a checkpoint it's
written to disk as an **immutable segment** (a sorted term dictionary plus
compressed postings) that's never changed again; searches read all segments;
and small segments are **merged** into bigger ones in the background. It's
the same idea as an LSM tree, which is how most modern databases store data.

## What you must give back
1. A segment file format in `docs/FORMAT.md`: header (magic, version, row range, doc count), the term dictionary (sorted terms, each with an offset into the postings), the postings (KW-4's encoding), chunk lengths for BM25, and a CRC.
2. Segments are opened with `mmap` (SNAP-5's RAII class), and terms found by binary search in the dictionary, without loading the whole file.
3. Search across the in-memory part and every segment, with **global** BM25 statistics (N, document frequencies and average length summed over all of them), so the scores are the same as with one big index. A test checks exactly that.
4. Checkpoints write a new segment; recovery opens the segments and indexes only the log's rows. A background task merges segments when there are more than 8 (tiered: merge similar sizes), then deletes the old ones **after** the merged one is safely renamed into place.
5. Startup time and memory before/after with 1 million chunks, in the PR.

## Watch out for
- A search may be reading a segment while a merge replaces it. Never unmap a segment a search might be using: keep segments in `std::shared_ptr`, and let the last user free it.
- Crash points again: during a merge, the old segments must stay valid until the new one is complete. Write the list of live segments to a small "manifest" file, replaced atomically.

## Done when
- [ ] Segments, merges and recovery work, with tests (including a crash during a merge); same scores as one in-memory index; the SNAP owner reviewed

## Learn first
- "Lucene's index file format" overview, and Martin Kleppmann's *Designing Data-Intensive Applications*, chapter 3 ("SSTables and LSM-Trees").
