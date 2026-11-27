# [SNAP-5] Load snapshots with `mmap`
labels: comp:snap, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** SNAP · **Size:** M · **Needs:** SNAP-3
**Your code:** `src/snap/snapshot.cpp` · **Tests:** the snapshot contract tests (unchanged)

## What is this, in plain words?
`read_snapshot` today copies the whole file into a buffer with `read`. `mmap`
instead maps the file **into your address space**: the bytes appear in memory
without a copy, and the kernel loads each 4 KB page only when you first touch
it. Learn how virtual memory works by using it.

## What you must give back
`read_snapshot` using `open` + `fstat` + `mmap(PROT_READ, MAP_PRIVATE)` + `munmap` (in an RAII class, of course), with `madvise(MADV_SEQUENTIAL)`. The contract tests must still pass.

## Step by step
1. An RAII `MappedFile` (in your `.cpp`): constructor maps, destructor unmaps; move-only.
2. Decode straight from the mapped bytes (`std::span<const std::byte>` over them).
3. Compare with the old version in SNAP-6.

## Watch out for
- If the file is truncated while mapped, touching the missing pages gives `SIGBUS`, not an error code. We never truncate a snapshot that's being read, but say in the PR why that matters.
- An empty file can't be mapped (`mmap` of length 0 fails): handle it as "damaged".

## Done when
- [ ] The snapshot contract tests pass on `debug` with the mmap version

## Learn first
- OSTEP chapters 18-21 ("Paging", "TLBs", "Swapping"): the ideas behind `mmap`.
- `man 2 mmap`, `man 2 madvise`.
