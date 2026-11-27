# [VEC-9] Save the vector index, so startup doesn't rebuild it
labels: comp:vec, size:M
milestone: v2 Stretch

**Component:** VEC (with SNAP: checkpoints write it, recovery loads it) · **Size:** M · **Needs:** VEC-8, SNAP-4
**Your code:** `src/vec/`, a section in `docs/FORMAT.md` (agreed with the lead), small changes in `src/snap/checkpoint.cpp` and `src/wal/recovery.cpp`

## What is this, in plain words?
Today recovery rebuilds the vector index from scratch, by adding every
vector again. For the flat index that's fast; for HNSW with a million vectors
it takes minutes. Save the built index next to the snapshot, and load it at
startup in seconds, ideally with `mmap`, so it's usable before it's all read.

## What you must give back
1. `void save(const std::filesystem::path&) const` and a matching `load` for the IVF and HNSW indexes. A file format with a magic, a version, the parameters (dim, M, ...), the row count, the data, and a CRC (like the snapshot). Write it into `docs/FORMAT.md`.
2. Checkpoints (SNAP-4) also write `<data_dir>/vectors.idx`, with the same row count as the snapshot, atomically (temp file + fsync + rename).
3. Recovery loads `vectors.idx` if its row count matches the snapshot, then adds only the rows replayed from the log. If it's missing or damaged: rebuild, and log a warning. Never fail to start because of it: it's only a cache.
4. Startup time before/after with 1 million vectors, in the PR.

## Watch out for
- The index and the snapshot must describe the same rows. Write the index **before** the snapshot's rename, check the counts on load, and think through each crash point (as in SNAP-4's tests).
- If you `mmap` the HNSW links, they're read-only: new links after startup need somewhere else to go. A simple answer: load with `mmap`, then copy into your normal arrays in the background. Explain your choice.

## Done when
- [ ] Save/load with tests (including a damaged file → rebuild); startup is faster; the SNAP owner reviewed

## Learn first
- SNAP-3 and SNAP-4's issues again: the same atomic-replace and crash-point thinking applies here.
