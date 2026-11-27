# [VEC-7] An approximate index: IVF (clusters, then search a few)
labels: comp:vec, size:M
milestone: v2 Stretch

**Component:** VEC · **Size:** M (3-4 days) · **Needs:** VEC-6 (your v1 work is done)
**Your code:** `src/vec/ivf_index.cpp` + `make_ivf_index` in `include/forge/vector_index.hpp` (agreed in this issue) · **Tests:** `tests/unit/vec/`

## What is this, in plain words?
The flat index reads **every** vector on every search. At 10 million chunks
that's 15 GB per query: no amount of SIMD saves you. Real vector databases
trade a little accuracy for a lot of speed with **approximate** nearest
neighbour (ANN) search. The simplest good one is **IVF** (inverted file):
group the vectors into clusters with k-means; for a query, find the few
nearest clusters and search only inside them.

## What you must give back
1. `std::unique_ptr<VectorIndex> make_ivf_index(std::size_t dim, std::size_t nlist, std::size_t nprobe, Executor* pool = nullptr)`, implementing the same `VectorIndex` interface:
   - until it holds `20 × nlist` vectors, it behaves like the flat index;
   - then it **trains**: k-means (k = `nlist`, 10-20 iterations, on a random sample of up to 50,000 vectors) on the normalised vectors; every vector goes into the list of its nearest centroid;
   - later `add`s go straight into the nearest centroid's list;
   - `search`: score the query against all centroids, take the `nprobe` best, score every vector in those lists, keep the top k.
2. `forgedb --vector-index flat|ivf` (with the APP owner: `Config` and `parse_args` gain a flag).
3. Unit tests: with `nprobe = nlist` it returns exactly what the flat index returns; k-means on 3 obvious, well-separated clusters finds them.
4. **The trade-off, measured** (`bench/ivf.cpp`): for 1 million vectors (or as many as your RAM allows), `nlist = 1024`, `nprobe` = 1, 4, 16, 64: queries/s, and **recall@10** (the share of the flat index's true top 10 that IVF also returns). Put the table and a recall-vs-speed chart in `docs/experiments/VEC-7-ivf.md`.

## Watch out for
- **Filters and ANN don't mix easily.** If a filter keeps 1 % of rows, the probed lists may contain almost none of them, and you return fewer than k results. Detect it (too few results) and fall back to the flat scan. Write down the rule you chose.
- k-means can leave a cluster empty: re-seed it with a random vector.
- Use cosine everywhere (normalised vectors, dot products), for the centroids too: normalise each centroid after averaging.
- The contract tests (`contract.vec.*`) test the flat index, so they still pass; your own tests cover IVF.

## Done when
- [ ] `make_ivf_index` with tests on `debug` and `tsan`; `--vector-index ivf` works in `forgedb`
- [ ] The report: at which `nprobe` is recall@10 ≥ 0.95, and how much faster than flat is it there?

## Learn first
- The FAISS wiki, "Faiss indexes" and "IndexIVFFlat": the same idea, in production.
- k-means (Lloyd's algorithm), in any intro: assignment step, update step, why it converges.
