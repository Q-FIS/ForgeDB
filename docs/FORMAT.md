# ForgeDB on disk

Everything ForgeDB writes to disk, byte by byte. WAL and SNAP write these
formats; the contract tests check them. Changing them needs the lead's OK.

**All integers are little-endian** (least significant byte first), whatever
the machine. Write them byte by byte (`(v >> 8*i) & 0xFF`), never by copying
the integer's memory: that's what makes the files portable.

## The data directory (`--data-dir`, default `forge-data/`)

| File | What | Written by |
| --- | --- | --- |
| `wal` | the write-ahead log: every put since the last checkpoint | WAL |
| `snapshot` | every chunk, as of the last checkpoint (missing until the first one) | SNAP |
| `snapshot.tmp` | a snapshot being written; renamed to `snapshot` when complete | SNAP |

**Starting up** (WAL-4, SNAP-4): load `snapshot` if it exists; then replay
every record of `wal`, in order; then cut off a damaged tail if there is one,
and keep appending to `wal`. A replayed put whose id is already loaded is
skipped: it can happen after a crash in the middle of a checkpoint, and
skipping it is always right.

## A chunk, as bytes (SNAP-2)

Used inside log records and snapshots.

| Bytes | What |
| --- | --- |
| u32 + that many bytes | `id` (UTF-8) |
| u32 + that many bytes | `doc_id` |
| u32 + that many bytes | `text` |
| u32 `n` | number of floats in `vector` |
| `n` × 4 bytes | the floats (IEEE 754, little-endian: `std::bit_cast<std::uint32_t>`) |
| u32 `m` | number of `meta` pairs |
| `m` × (u32 + key, u32 + value) | the pairs, in key order (the order of `std::map`) |

Example: `{id "a", doc_id "d", text "", vector [1.0], meta {"k": "v"}}` is

```
01 00 00 00 61    01 00 00 00 64    00 00 00 00    01 00 00 00 00 00 80 3f
01 00 00 00    01 00 00 00 6b 01 00 00 00 76
```

Decoding must check every length against the bytes actually left **before**
using it: a damaged length must give an error, not a 4 GB allocation.

## A log record (WAL-1)

| Bytes | What |
| --- | --- |
| u32 | `length`: the payload's size in bytes (at most 64 MiB) |
| u32 | `crc`: CRC-32 of the type byte followed by the payload |
| u8 | `type`: 1 = put (the payload is one encoded chunk) |
| `length` bytes | the payload |

The log is the records one after another, nothing else. CRC-32 is the common
one (zlib, Ethernet, PNG): polynomial `0xEDB88320` (reflected), start value
`0xFFFFFFFF`, final XOR `0xFFFFFFFF`. Check value: `crc32("123456789") = 0xCBF43926`.

**A damaged tail.** If the machine crashes in the middle of an append, the
log ends with part of a record. Reading stops at the first record that is cut
short, has a wrong CRC, or claims more than 64 MiB; everything before it is
good. Recovery cuts the file back to the end of the last good record.

## A snapshot (SNAP-3)

| Bytes | What |
| --- | --- |
| 8 | the magic bytes `FORGESNP` |
| u32 | version: 1 |
| u64 | `count`: how many chunks |
| `count` × (u32 + bytes) | each chunk's length, then the chunk (encoded as above), in row order |
| u32 | CRC-32 of every byte before it |

**Atomic replace.** Write `snapshot.tmp`, `fsync` it, `rename` it over
`snapshot`, then `fsync` the directory (so the rename itself is on disk).
After a crash, `snapshot` is either the complete old one or the complete new
one, never half of each.
