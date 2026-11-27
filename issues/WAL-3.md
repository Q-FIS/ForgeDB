# [WAL-3] Every put goes through the log, and `--fsync` decides when it's on disk
labels: comp:wal, size:M
milestone: v0.2 Survives a crash

**Component:** WAL (changes POOL's Engine and APP's `forgedb.cpp`: POOL and APP review) · **Size:** M (3 days) · **Needs:** WAL-1, SNAP-2, POOL-3, APP-4
**Your code:** `include/forge/wal.hpp` (additions), `src/wal/journal.cpp`, a small change in `src/pool/engine.cpp` and `apps/forgedb.cpp`

## What is this, in plain words?
Now the log gets used: every put is appended **before** it's applied, and
the client gets its 201 only once the record is durable. "Durable" depends on
`--fsync`:

| Policy | When a put returns | Can lose, on a power cut |
| --- | --- | --- |
| `always` | after its own `fsync` | nothing |
| `batch` | after the next **group fsync** (one fsync for all puts of the last ~2 ms) | nothing that was acknowledged |
| `never` | at once; the kernel writes to disk when it likes | the last ~30 s of puts |

`batch` (**group commit**) is the clever one: one `fsync` takes milliseconds,
so syncing each put alone limits you to a few hundred puts a second.
Waiting a moment and syncing many at once gives nearly `never`'s speed with
`always`'s safety. Real databases do exactly this.

## What you must give back
Added to `include/forge/wal.hpp` (agreed in this issue):
```cpp
enum class FsyncPolicy { always, batch, never };
FsyncPolicy parse_fsync_policy(std::string_view text);  // "always" / "batch" / "never"

class Journal {
public:
    Journal(const std::filesystem::path& file, FsyncPolicy policy);
    ~Journal();  // syncs, and stops the flusher thread
    /// Appends one record; returns its number (1, 2, 3...). Call it while
    /// holding the Engine's write lock, so the log's order is the real order.
    std::uint64_t append(RecordType type, std::span<const std::byte> payload);
    /// Returns once record `seq` is on disk, as the policy defines it.
    /// Call it AFTER releasing the Engine's lock.
    void wait_durable(std::uint64_t seq);
    void sync();  // flush everything now (shutdown)
};
```
And in the Engine: a constructor parameter `Journal* journal = nullptr`. In
`put`: validate → lock → check the id → `seq = journal->append(RecordType::put, encode_chunk(chunk))` →
apply to the store and indexes → **unlock** → `journal->wait_durable(seq)` → return.
`forgedb` creates the Journal at `<data-dir>/wal` and passes it in.

## Step by step
1. `Journal` holds a `WalWriter`. For `batch`: a flusher thread loops: wait (condition variable) until there are unsynced records, sleep ~2 ms, take note of the last appended number, `sync()`, publish "durable up to N", `notify_all`. `wait_durable(seq)` waits until "durable up to" ≥ `seq`.
2. For `always`: `append` syncs before returning; `wait_durable` returns at once. For `never`: neither syncs.
3. Unit tests in `tests/unit/wal/`: every policy returns; with `batch`, 8 threads × 1,000 appends all become durable, and the number of fsyncs (count them) is far below 8,000.
4. Run the contract tests of POOL and APP: they must still pass (they don't use a Journal).

## Watch out for
- **Never** wait for the disk while holding the Engine's lock: every search would stall behind the fsync.
- A chunk is visible to searches a moment before it's durable. Is that a problem? Write your answer in the PR (hint: what has the client been told?).
- `kill -9` does NOT lose data with `never`: the data is in the kernel's page cache, which survives the process. Only a power cut or a kernel crash loses it. WAL-5 can only test the first kind.

## How to test it
`ctest --preset tsan -R "unit.wal|contract.pool|contract.app|blackbox.smoke"` (and `debug`).

## Done when
- [ ] Puts go through the Journal in `forgedb`; all three policies work; your unit tests pass on `debug` and `tsan`
- [ ] The POOL owner reviewed the Engine change, and the APP owner the change to `forgedb.cpp`

## Learn first
- "Group commit" (look it up in PostgreSQL's or MySQL's documentation).
- OSTEP chapter 42 again: what "durable" really requires (the disk's own cache too!).
