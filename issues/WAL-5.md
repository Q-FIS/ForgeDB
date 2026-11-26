# [WAL-5] The crash test: `kill -9` mid-write, lose nothing acknowledged
labels: comp:wal, size:M
milestone: v0.2 Survives a crash

**Component:** WAL (with LOAD's help) · **Size:** M · **Needs:** WAL-4
**Your code:** `tests/blackbox/crash_test.py` (replaces the placeholder)

## What is this, in plain words?
Prove it. Start `forgedb`, have several clients putting chunks as fast as
they can, and `kill -9` it at a random moment: no shutdown, no flushing,
nothing. Restart it. **Every chunk that got a 201 must be there**, and
ForgeDB must start without any manual help. Then do it again, 20 times.

## What you must give back
`crash_test.py <forgedb> [--rounds N] [--fsync POLICY]` (defaults: 20 rounds, `batch`):
1. Each round: start `forgedb` on the same data dir. 4 client threads put chunks with unique ids; each records the ids that got **201**.
2. After a random 0.2-1.5 s, `SIGKILL` the server.
3. Restart it. Check: every acknowledged id is readable with the right text; `/health` counts at least that many chunks; the server started (a damaged tail was cut, not fatal).
4. Print a summary: rounds, puts acknowledged, puts lost (must be 0).

## Watch out for
- A put that was sent but not acknowledged (the connection died) may or may not be there afterwards. Both are correct! Only acknowledged ones count.
- Random tests must be repeatable: print the random seed, and accept `--seed`.

## How to test it
`ctest --preset debug -R blackbox.crash` (and `tsan`); and once by hand with `--rounds 200`.

## Done when
- [ ] `blackbox.crash` passes on `debug` and `tsan` with 20 rounds
- [ ] 200 rounds by hand, with each `--fsync` policy, lost 0 (paste the summaries in the PR)

## Learn first
- Jepsen.io's analyses, the intro of any one: how people test databases by breaking them on purpose.
