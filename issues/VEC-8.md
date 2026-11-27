# [VEC-8] HNSW: the graph index behind most vector databases
labels: comp:vec, size:L
milestone: v2 Stretch

**Component:** VEC · **Size:** L (about a week) · **Needs:** VEC-7
**Your code:** `src/vec/hnsw_index.cpp` + `make_hnsw_index` in `include/forge/vector_index.hpp` (agreed in this issue) · **Tests:** `tests/unit/vec/`

## What is this, in plain words?
**HNSW** (Hierarchical Navigable Small World) links every vector to a few of
its neighbours, in several layers: the top layers have few nodes and long
links (like motorways), the bottom layer has every node and short links
(like streets). A search starts at the top, walks greedily towards the
query, drops a layer, and repeats. It touches a few thousand vectors instead
of millions, with recall around 0.99. Qdrant, Weaviate, pgvector and Lucene
all use it. You'll build one from the original paper.

## What you must give back
1. `make_hnsw_index(dim, M = 16, ef_construction = 200, ef_search = 64, Executor* pool = nullptr)` implementing `VectorIndex`, following the paper (Malkov & Yashunin, 2016): Algorithm 1 (insert), 2 (search a layer), 4 (choose neighbours with the heuristic), 5 (k-NN search). Each new node gets a random level `floor(-ln(uniform) × mL)` with `mL = 1/ln(M)`.
2. Memory you can explain: store neighbour lists as fixed-size arrays per layer (`M` links, `2M` on layer 0), not `std::vector` per node. Report bytes per vector.
3. `forgedb --vector-index hnsw`.
4. Unit tests: tiny graphs you check by hand; on random data, recall@10 ≥ 0.95 with the defaults; every node is reachable from the entry point (no orphans after thousands of inserts).
5. `docs/experiments/VEC-8-hnsw.md`: flat vs IVF vs HNSW at 1 million vectors: build time, memory, queries/s and recall@10, with `ef_search` = 16, 32, 64, 128, 256.

## Watch out for
- Concurrency is already handled: the Engine runs searches under a shared lock and adds under an exclusive one. Don't add your own locks, but **do not** cache anything mutable in `search` (it's `const`, and runs on many threads at once). Use a per-call "visited" structure, not a shared one.
- The "visited" set is the hot spot: a `std::unordered_set` per search is slow. Try a `std::vector<std::uint32_t>` of visit "generations".
- Filters: same problem as IVF, worse. Search with a larger `ef` when a filter is set, and fall back to flat if too few results survive.
- Randomness: a fixed seed in tests, so failures can be repeated.

## Done when
- [ ] HNSW with tests on `debug` and `tsan`; works in `forgedb`; the report is merged and presented

## Learn first
- The HNSW paper (arXiv 1603.09320): sections 3 and 4, and the algorithms. Read them twice.
- Pinecone's "Hierarchical Navigable Small Worlds" explainer, for the pictures.
