# [SNAP-3] Snapshots: save the whole store, atomically
labels: comp:snap, size:M
milestone: v0.2 Survives a crash

**Component:** SNAP · **Size:** M (2 days) · **Needs:** SNAP-1, SNAP-2, WAL-1 (for `crc32`)
**Contract:** `include/forge/store.hpp`, `docs/FORMAT.md` · **Your code:** `src/snap/snapshot.cpp` · **Tests:** `tests/contract/snap/snapshot_test.cpp`

## What is this, in plain words?
Replaying a huge log at every startup gets slow, and the log grows forever.
A **snapshot** saves every chunk in one file; then the log can start over
(SNAP-4). The hard part: if the machine dies halfway through writing the
snapshot, you must still have a complete one: the old one. That's done with a
classic trick: write a temporary file, fsync it, then `rename` it over the
old one, because a rename is atomic.

## What you must give back
`write_snapshot` and `read_snapshot`, in the format of `docs/FORMAT.md`
(magic `FORGESNP`, version, count, the chunks, a CRC at the end).

## Step by step
1. Delete `src/snap/stub_snapshot.cpp`; create `src/snap/snapshot.cpp`.
2. Write: build the bytes (or stream them, keeping a running CRC); write to `<file>.tmp`; `fsync` it; close; `std::filesystem::rename(tmp, file)`; then open the **directory** and `fsync` it, so the rename itself is on disk.
3. Read: read the file; check the size, the magic, the version, the CRC; then decode each chunk into a new memory store. Any problem → `std::runtime_error`.

## Watch out for
- Without the directory fsync, after a power cut the rename may not have happened, even though the file's contents are on disk.
- Never write the real `snapshot` file in place: a crash would leave half a file.

## How to test it
`ctest --preset debug -R contract.snap.Snapshot`
Bonus: write a test that kills a child process (`fork` + `kill`) in the middle of `write_snapshot`, and check the old snapshot is intact.

## Done when
- [ ] Every `contract.snap.Snapshot.*` test passes on `debug`

## Learn first
- `man 2 rename`: the sentence about atomic replacement.
- "Files are hard" (Dan Luu's article): the surprising ways saving a file goes wrong.
