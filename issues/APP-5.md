# [APP-5] Graceful shutdown: finish what you started, then exit 0
labels: comp:app, size:M
milestone: v0.2 Survives a crash

**Component:** APP · **Size:** M · **Needs:** APP-4 (and CONN-3 for a quick stop with idle connections)
**Your code:** `apps/forgedb.cpp`, `src/app/` · **Tests:** `tests/blackbox/shutdown_test.py` (yours to write)

## What is this, in plain words?
When someone stops ForgeDB (Ctrl-C, `kill`, a deploy), requests that are
in progress must finish and get their answers; new connections are refused;
the log is flushed to disk (once WAL exists); then the process exits with
code **0**. Only then can a restart be trusted.

## What you must give back
1. Signal handling with **signalfd**: block `SIGINT`/`SIGTERM`, create a `signalfd` for them, and have a thread `read()` from it. When a signal arrives, log `forgedb: stopping (SIGTERM)` and call `server.stop()`. A second signal during shutdown exits immediately with code 1.
2. The shutdown order: stop accepting → let requests in progress finish → stop the pool → flush the log (`sync()`, once WAL-3 is in) → exit 0.
3. `tests/blackbox/shutdown_test.py`, replacing the placeholder. It checks that:
   - SIGTERM with no clients → exit code 0 within 2 s;
   - SIGTERM while 20 searches are running (send them, then signal at once) → every one of them gets a complete 200 response, and then the exit code is 0;
   - after SIGTERM, new connections are refused;
   - (after CONN-3) SIGTERM with 50 idle keep-alive connections open → exits within 2 s.

## Watch out for
- `signalfd` only works if the signals are blocked in **every** thread: block them in `main` before creating any thread (threads inherit the mask).
- Exit code 0 means "I stopped cleanly". Don't call `exit()` from the signal thread while other threads are mid-request.

## How to test it
`ctest --preset debug -R blackbox.shutdown` (and `tsan`).

## Done when
- [ ] `blackbox.shutdown` passes on `debug` and `tsan`

## Learn first
- `man 2 signalfd` (read the example at the end).
- Why is very little code allowed inside a classic signal handler ("async-signal-safe")? How does signalfd avoid that problem?
