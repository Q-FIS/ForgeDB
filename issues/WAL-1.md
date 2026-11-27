# [WAL-1] The write-ahead log: append records safely
labels: comp:wal, size:M
milestone: v0.2 Survives a crash

**Component:** WAL (durability) · **Size:** M (2-3 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/wal.hpp`, `docs/FORMAT.md` · **Your code:** `src/wal/writer.cpp` · **Tests:** `tests/contract/wal/writer_test.cpp`

## What is this, in plain words?
Before ForgeDB answers "saved" (201), the chunk must be safe on disk, even if
the power goes out a millisecond later. Databases do this with a
**write-ahead log**: a file you only ever add to at the end. Every change is
written there first; after a crash, reading the log back rebuilds everything
(WAL-4).

You're writing the heart of ForgeDB's durability: the record format, the
checksum that detects damage, and the class that appends records.

## What you must give back
`crc32` and the `WalWriter` class, exactly as `include/forge/wal.hpp` and
`docs/FORMAT.md` describe. A record is:
```
| length: u32 | crc: u32 | type: u8 | payload: length bytes |
```
all little-endian; the CRC covers the type byte and the payload.

## Step by step
1. Delete `src/wal/stub_writer.cpp`; create `src/wal/writer.cpp`.
2. **`crc32`:** the table version. Build a 256-entry table once (a `constexpr` function can build it at compile time), then for each byte: `c = table[(c ^ byte) & 0xFF] ^ (c >> 8)`, starting from `0xFFFFFFFF` and XOR-ing with `0xFFFFFFFF` at the end. Check: `crc32("123456789") == 0xCBF43926`.
3. **Constructor:** `open(path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644)` with `check(...)`. Find the current size with `fstat` (old records stay; new ones go after them).
4. **Move-only, RAII:** destructor closes; moves take the fd and leave the other side at -1 (as in Lab 1 and CONN-1).
5. **`append`:** refuse payloads over 64 MiB. Build the 9-byte header in a small array, writing each `u32` byte by byte (`(v >> 8*i) & 0xFF`). Write header and payload, looping on short writes (or use `writev` to write both in one call; it can also write less than asked).
6. **`sync`:** `fsync(fd_)`. Don't call it inside `append`: WAL-3 decides when to sync.

## Watch out for
- Writing a `u32` with `memcpy(&v, ...)` depends on the machine's byte order. Byte by byte doesn't, and the format says little-endian.
- The CRC must include the **type** byte: the contract test damages it on purpose.
- Opening a file that's already there must not erase it (no `O_TRUNC`!).

## How to test it
```bash
ctest --preset debug -R contract.wal.Crc32
ctest --preset debug -R contract.wal.WalWriter
xxd /tmp/some-log | head        # look at your bytes: can you read the header by eye?
```

## Done when
- [ ] Every `contract.wal.Crc32.*` and `contract.wal.WalWriter.*` test passes on `debug`
- [ ] In the PR: one record dumped with `xxd`, each byte explained

## Learn first
- OSTEP chapter 42, "Crash Consistency: FSCK and Journaling": the introduction and section 42.3 (journaling). This is exactly what you're building.
- What does `write()` returning mean, and what does `fsync()` add? (LWN, "Ensuring data reaches disk".)
- What is a CRC, and which kinds of damage does it catch?
