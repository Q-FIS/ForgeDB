# [POOL-3] The Engine: the store and both indexes, safe for many threads
labels: comp:pool, size:M
milestone: v0.1 First search

**Component:** POOL · **Size:** M (2-3 days) · **Needs:** nothing, start on day 1 (the tests use fakes)
**Contract:** `include/forge/engine.hpp` · **Your code:** `src/pool/engine.cpp` · **Tests:** `tests/contract/pool/engine_test.cpp`

## What is this, in plain words?
The Engine is ForgeDB's heart. The API calls only the Engine; the Engine
checks every request, then keeps three things in step: the **Store** (SNAP),
the **vector index** (VEC) and the **keyword index** (KW). A put adds the
chunk to all three; a search asks an index for rows, then turns them into
hits using the store.

And it's where concurrency gets real: the server runs many requests at once.
Searches only read, so many may run together; a put changes things, so it
must run alone. That's exactly what a **reader-writer lock**
(`std::shared_mutex`) gives you.

You don't wait for SNAP, VEC or KW: the tests use simple stand-ins from
`tests/support/fakes.hpp`.

## What you must give back
`Engine`'s constructor, `put`, `get`, `search`, `size` and `dim`, following
the header's comments exactly (which errors, when).

## Step by step
1. Delete `src/pool/stub_engine.cpp`; create `src/pool/engine.cpp`. Add your members to the header: the three `std::unique_ptr`s and a `mutable std::shared_mutex mutex_`.
2. `put`: **check everything first** (empty id, length, all zeros), then take a `std::unique_lock`, check the id isn't taken (`store->find`), and only then change things: `store->add`, `vectors->add(row, ...)`, `keywords->add(row, ...)`. If you change one part and then throw, the three disagree forever.
3. `get` and `search`: `std::shared_lock`. Many threads may hold it at once.
4. `search`: check `k` and the mode's inputs; turn the `Filter` into a `RowFilter` (an empty `Filter` → an empty `RowFilter`); call the right index; map each `ScoredRow` to a `SearchHit` with the store.
5. Hybrid: throw `NotImplemented` for now. KW-5 adds it.

## Watch out for
- `put` moves the chunk into the store: copy what you still need (the vector, the text) **before** you `std::move` it.
- The RowFilter lambda reads the store: it's called while you hold the lock, so that's safe. Don't keep it after.
- **Writer starvation:** with `std::shared_mutex` on Linux, if readers keep arriving back to back, a waiting writer may wait a very long time. The concurrency test adds small pauses for this reason. Write two sentences about it in the PR.

## How to test it
```bash
ctest --preset debug -R contract.pool.Engine
ctest --preset tsan  -R contract.pool.Engine     # catches any access without the lock
```

## Done when
- [ ] Every `contract.pool.Engine.*` test passes on `debug` and `tsan`

## Learn first
- cppreference: `std::shared_mutex`, `std::shared_lock`, `std::unique_lock`.
- OSTEP chapter 31, "Semaphores", section 31.5, "Reader-Writer Locks", including its note on starvation.
