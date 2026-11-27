# [APP-4] Switch on `forgedb`: everything joined up
labels: comp:app, size:M
milestone: v0.1 First search

**Component:** APP · **Size:** M (1-2 days, done together with the owners whose parts it joins) · **Needs:** APP-1, APP-3, CONN-2, POOL-2, SNAP-1, VEC-1, KW-2
**Your code:** `apps/forgedb.cpp` · **Tests:** `blackbox.smoke` (already written)

## What is this, in plain words?
The moment ForgeDB exists. `apps/forgedb.cpp` is still the placeholder (it
exits with 77, so its black-box tests are skipped). Replace it with the real
`main`: read the config, build the parts, start the server. When
`blackbox.smoke` turns from **Skipped** to **Passed**, milestone v0.1 is
done. Celebrate at the next meeting: run `forge` against it live.

## What you must give back
`main` in `apps/forgedb.cpp`:
1. `--help` prints `usage()` and exits 0. A `UsageError` prints the message and `usage()` to stderr, and exits **2**.
2. Block `SIGINT` and `SIGTERM` in every thread (`pthread_sigmask`, **before** creating any thread), and ignore `SIGPIPE`.
3. Create the data directory (`std::filesystem::create_directories`).
4. `Engine engine(make_memory_store(), make_flat_index(cfg.dim), make_keyword_index());`, `Api api(engine);`, `ThreadPool pool(cfg.threads, cfg.queue);`, and a `Server` whose handler calls `api.handle`.
5. A thread that waits for a signal (`sigwait` for now; APP-5 improves it) and calls `server.stop()`.
6. `server.run()`; then `pool.stop()`; then return 0.
7. Print one line to stderr when ready: `forgedb listening on 127.0.0.1:8080`.

## Watch out for
- **Order of destruction.** Objects are destroyed in reverse order of creation. The pool's tasks use the Api and the Engine, so the pool must be stopped before they're destroyed. Write a comment in `main` explaining the order you chose.
- A signal can arrive in any thread unless it's blocked everywhere: that's why you block it before starting threads, and let one thread wait for it.

## How to test it
```bash
cmake --build --preset debug -j && ctest --preset debug -R blackbox.smoke
./build/debug/forgedb --dim 4 --port 8080 &        # by hand
./build/debug/forge health                         # once CLI-1 is merged (or: curl localhost:8080/health)
kill %1
```

## Done when
- [ ] `blackbox.smoke` passes on `debug` and `tsan` (and `blackbox.cli` if CLI-1 is merged)
- [ ] `forgedb --port abc` exits with code 2 and a clear message
- [ ] Demoed at the weekly meeting

## Learn first
- `man 7 signal` (the "Signal mask and pending signals" part) and `man 3 sigwait`.
- What happens to a process's threads when `main` returns?
