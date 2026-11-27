# [WAL-2] Read the log back, and spot a damaged tail
labels: comp:wal, size:S
milestone: v0.2 Survives a crash

**Component:** WAL · **Size:** S (1 day) · **Needs:** WAL-1
**Contract:** `include/forge/wal.hpp` · **Your code:** `src/wal/reader.cpp` · **Tests:** `tests/contract/wal/reader_test.cpp`

## What is this, in plain words?
After a crash, the log may end in the middle of a record: the machine died
halfway through an `append`. `read_wal` returns every good record, says where
the good part ends, and says whether there was junk after it. Recovery
(WAL-4) then cuts the junk off.

## What you must give back
`read_wal`, as the header describes. Stop at the first record that:
- has fewer than 9 bytes left for its header;
- claims a length over 64 MiB;
- has fewer bytes left than its length;
- has a CRC that doesn't match.

`valid_bytes` = where the last good record ends; `damaged_tail` = there are bytes after it.

## Step by step
1. Delete `src/wal/stub_reader.cpp`; create `src/wal/reader.cpp`.
2. A missing file → empty contents (check `std::filesystem::exists` first).
3. Read the whole file into memory (fine for now: checkpoints, SNAP-4, keep the log small).
4. Loop over the records, checking the four conditions **before** reading each part.

## Watch out for
- Check the length **before** allocating or reading the payload: a damaged length may say 4 GB.
- `pos + 9 > size` can overflow if you're not careful with types; `size - pos < 9` (when `pos <= size`) can't.

## How to test it
`ctest --preset debug -R contract.wal.ReadWal`: it cuts a log at every byte of its last record, and flips bytes.

## Done when
- [ ] Every `contract.wal.ReadWal.*` test passes on `debug`

## Learn first
- Why is "stop at the first bad record" safe, and "skip a bad record and keep going" dangerous?
