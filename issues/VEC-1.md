# [VEC-1] Vector search: find the nearest vectors
labels: comp:vec, size:M
milestone: v0.1 First search

**Component:** VEC (vector search) · **Size:** M (2 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/vector_index.hpp` · **Your code:** `src/vec/flat_index.cpp` · **Tests:** `tests/contract/vec/flat_index_test.cpp`

## What is this, in plain words?
Each chunk has an **embedding**: a list of numbers (384 of them for the ML
wing's model) that captures its meaning. Chunks with similar meaning have
vectors pointing in similar directions. A search sends a query vector, and
you return the k chunks whose vectors point most nearly the same way:
**cosine similarity**, from 1 (same direction) to -1 (opposite).

The "flat" index is the honest, brute-force version: compare the query with
every vector. It's exact, and with good C++ it's surprisingly fast. VEC-3,
VEC-4 and VEC-5 make it much faster; this issue makes it correct.

## What you must give back
`make_flat_index` and a class implementing `VectorIndex`, as the header describes.

## Step by step
1. Delete `src/vec/stub_flat_index.cpp`; create `src/vec/flat_index.cpp`.
2. **Normalise once, when adding:** divide each vector by its length (`sqrt(sum of squares)`). Then cosine(q, v) is just the **dot product** of the normalised vectors: no square roots during search.
3. Store all vectors in **one** `std::vector<float>`, row after row (row r starts at `r * dim`). Not a `std::vector<std::vector<float>>`: VEC-3 explains why.
4. `search`: normalise the query; compute the dot product with each row the filter keeps; keep the best k.
5. **Keeping the best k:** sorting all n scores is O(n log n). Better: `std::partial_sort`, or a min-heap of size k (`std::priority_queue` with the comparison reversed): O(n log k).
6. Order the result with `better` (from `types.hpp`): higher score first, smaller row first on ties.

## Watch out for
- Compute the length in `double`, then store floats: a sum of 384 squares in `float` loses precision.
- k may be larger than the number of rows; k may be 0.
- Check the dimension and the all-zero case in **both** `add` and `search`.

## How to test it
`ctest --preset debug -R contract.vec.FlatIndex`. The last test compares you with a double-precision brute force on 3,000 random vectors.

## Done when
- [ ] Every `contract.vec.FlatIndex.*` test passes on `debug`

## Learn first
- What is cosine similarity, and why is it the dot product of normalised vectors? (Any linear algebra intro; or the ML wing's RET-04 issue.)
- cppreference: `std::partial_sort`, `std::priority_queue`.
