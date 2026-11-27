# [WAL-4] Recovery: rebuild everything from the log at startup
labels: comp:wal, size:M
milestone: v0.2 Survives a crash

**Component:** WAL (touches `apps/forgedb.cpp`: APP reviews) · **Size:** M · **Needs:** WAL-2, WAL-3
**Your code:** `src/wal/recovery.cpp`, `include/forge/wal.hpp` (additions) · **Tests:** `blackbox.restart` (uncomment it), `tests/unit/wal/`

## What is this, in plain words?
The payoff. When `forgedb` starts, it reads the log and replays every put, so
after a restart, or a crash, every acknowledged chunk is back. If the last
record was cut off by a crash, recovery cuts it off the file and carries on.

## What you must give back
Added to `include/forge/wal.hpp`:
```cpp
struct Recovered {
    std::unique_ptr<Store> store;
    std::unique_ptr<VectorIndex> vectors;
    std::unique_ptr<KeywordIndex> keywords;
    std::size_t replayed = 0;   // records replayed from the log
    bool cut_tail = false;      // a damaged tail was cut off
};
/// Rebuilds the store and both indexes from <data_dir>/wal (and, after
/// SNAP-4, <data_dir>/snapshot). Throws std::runtime_error if the data was
/// written with a different dim.
Recovered recover(const std::filesystem::path& data_dir, std::size_t dim);
```
`forgedb` calls it at startup, logs `recovered N chunks in X ms`, builds the
Engine from the three parts (the Engine accepts parts that already hold
rows), and then opens the Journal (which appends after the good part).

## Step by step
1. `read_wal`; if `damaged_tail`, `std::filesystem::resize_file(wal, valid_bytes)` and log a warning.
2. For each record: type `put` → `decode_chunk` → if its vector's length isn't `dim`, throw (`--dim` must not change between runs) → if the id is already in the store, **skip** it (see `docs/FORMAT.md`) → add to the store and both indexes, with the same row.
3. Uncomment `restart` in `tests/blackbox/CMakeLists.txt`.
4. Unit tests: recover from an empty dir; from a log of 1,000 puts; from a log with a cut tail (the tail is gone afterwards, and new appends work); a dim mismatch throws.

## Watch out for
- An unknown record type means a newer program wrote the log, or damage: refuse to start (throw) rather than guess.
- Time it: recovering 1 million chunks should take seconds, not minutes. Log the time.

## How to test it
`ctest --preset debug -R "restart|unit.wal"` (and `tsan`).

## Done when
- [ ] `blackbox.restart` is switched on and passes against `forgedb`
- [ ] The APP owner reviewed the change to `forgedb.cpp`

## Learn first
- What does "idempotent" mean, and why must replaying the log be idempotent?
