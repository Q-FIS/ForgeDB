# [SNAP-4] Checkpoints: snapshot, then start a fresh log
labels: comp:snap, size:M
milestone: v0.2 Survives a crash

**Component:** SNAP (touches WAL's recovery and the Engine: both review) · **Size:** M · **Needs:** SNAP-3, WAL-4
**Your code:** `src/snap/checkpoint.cpp` + small changes in `src/wal/recovery.cpp`, `src/pool/engine.cpp`

## What is this, in plain words?
Put it together: when the log grows past 64 MiB, and when `forgedb` shuts
down cleanly, ForgeDB writes a snapshot of everything and empties the log.
Startup then loads the snapshot and replays only the (short) log. Startup
stays fast, and the disk doesn't fill up.

## What you must give back
1. `Engine::checkpoint(const std::filesystem::path& data_dir)` (agreed in this issue): with the **write lock** held (no puts during it): `write_snapshot`, then empty the log (the Journal gets a `restart()` that truncates the file to 0 and reopens it).
2. The trigger: after a put, if the log is over 64 MiB, run a checkpoint (on a pool task, not in the request). And one at clean shutdown (APP-5's sequence).
3. Recovery (`recover`, WAL-4) loads `<data_dir>/snapshot` first if it exists, then replays the log, **skipping ids already loaded** (that covers a crash after the snapshot was renamed but before the log was emptied).
4. Unit tests for each crash point: before the rename (old snapshot + full log), after the rename but before truncating (new snapshot + full log: the skips make it right), after truncating.

## Watch out for
- Holding the write lock during a big snapshot blocks searches for that long. Measure it (log the time). Later improvement: copy what you need under the lock, write outside it. Note it in the PR; don't build it yet.

## How to test it
`ctest --preset debug -R "unit.snap|unit.wal|restart|crash"` (and `tsan`).

## Done when
- [ ] Checkpoints happen; restart and crash tests still pass; your crash-point tests pass
- [ ] The WAL and POOL owners reviewed their parts

## Learn first
- OSTEP chapter 42.3: "checkpointing" in journaling file systems is the same idea.
