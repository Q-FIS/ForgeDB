# [SNAP-2] A chunk as bytes, and back
labels: comp:snap, size:M
milestone: v0.2 Survives a crash

**Component:** SNAP · **Size:** M (2 days) · **Needs:** nothing, start any time
**Contract:** `include/forge/store.hpp`, `docs/FORMAT.md` · **Your code:** `src/snap/encoding.cpp` · **Tests:** `tests/contract/snap/encoding_test.cpp`

## What is this, in plain words?
To write a chunk to disk (in the log, WAL-3, and in snapshots, SNAP-3), it
must become bytes, and those bytes must turn back into **exactly** the same
chunk, years later, on another machine. `docs/FORMAT.md` defines the layout;
you write both directions. The decoder is the security-sensitive half: it
reads bytes that may be damaged, and must never crash or allocate gigabytes
because of a bad length.

## What you must give back
`encode_chunk` and `decode_chunk`, byte-exact to `docs/FORMAT.md`.

## Step by step
1. Delete `src/snap/stub_encoding.cpp`; create `src/snap/encoding.cpp`.
2. Encoder helpers: `put_u32(out, v)` (4 bytes, little-endian, byte by byte), `put_string(out, s)` (length, then bytes). Floats: `put_u32(out, std::bit_cast<std::uint32_t>(x))`.
3. Decoder: a small `Reader` struct holding the bytes and a position, with `need(n)` (throws `std::runtime_error` if fewer than n bytes are left), `u32()` and `str()`. Call `need` **before** every read and every allocation.
4. At the end, if bytes are left over, throw: the input wasn't exactly one chunk.

## Watch out for
- Before `reserve(dim)` or building a string of length `n`, check that `dim * 4` (or `n`) bytes are really left. The test feeds a length of 2 billion.
- `-0.0f`, the largest float and tiny floats must come back bit-for-bit: `bit_cast` does that; converting through `double` or text doesn't.

## How to test it
`ctest --preset debug -R contract.snap.Encoding`

## Done when
- [ ] Every `contract.snap.Encoding.*` test passes on `debug`

## Learn first
- What is endianness? Check yours: `lscpu | grep Endian`.
- cppreference: `std::bit_cast`, and why it beats `reinterpret_cast` and `memcpy` tricks.
