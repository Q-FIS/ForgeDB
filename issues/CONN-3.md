# [CONN-3] Keep-alive done right: limits, idle connections, quick shutdown
labels: comp:conn, size:M
milestone: v0.2 Survives a crash

**Component:** CONN · **Size:** M · **Needs:** CONN-2
**Your code:** `src/conn/server.cpp` · **Tests:** `tests/blackbox/keepalive_test.py` (yours to write), `tests/unit/conn/`

## What is this, in plain words?
CONN-2 serves connections. Now make it behave well when there are many, or
when they misbehave: a hard cap on open connections, idle connections that
don't slow down shutdown, and proof (a black-box test) that keep-alive and
pipelining work through the whole `forgedb` program.

## What you must give back
1. **The connection limit:** with `options.max_connections` connections already open, a new one gets a **503** (`Retry-After: 1`) and is closed.
2. **A quick stop:** `stop()` also wakes every connection that's waiting in `recv` (call `::shutdown(fd, SHUT_RD)` on each open connection's fd), so the server stops in well under a second, even with `idle_timeout` = 10 s.
3. **`tests/blackbox/keepalive_test.py`**, replacing the placeholder. Using `harness.py`, start `forgedb` with `--idle-timeout-ms 500` and check:
   - 100 requests on one connection all succeed;
   - 3 pipelined requests in one `send` get 3 responses, in order;
   - a silent connection is closed by the server after about 0.5 s (and not before 0.4 s);
   - `Connection: close` → closed after the response; an HTTP/1.0 request without `Connection: keep-alive` → closed too.

## Step by step
1. Keep a `std::set<int>` of open connection fds (or a counter plus the fds) under the mutex you added in CONN-2.
2. In `run()`, check the limit before submitting; in `stop()`, `shutdown(SHUT_RD)` every fd in the set.
3. Careful with fd reuse: remove a connection's fd from the set and close it **while holding the mutex**. Otherwise `stop()` might `shutdown` a number the kernel has already given to a new socket.
4. Write unit tests in `tests/unit/conn/`: max_connections = 2 → the third gets 503; stop with an idle connection open returns in < 1 s.
5. Write the black-box test (copy the shape of `smoke_test.py`).

## Watch out for
- `shutdown(SHUT_RD)` on a socket makes a blocked `recv` return 0: your serving loop then closes it normally.
- Timing tests are flaky if they're too tight: allow generous margins.

## How to test it
```bash
ctest --preset debug -R "keepalive|contract.conn|unit.conn"
```

## Done when
- [ ] The new unit tests and `blackbox.keepalive` pass on `debug` and `tsan`
- [ ] `forgedb` stops within 1 s of SIGTERM even with idle connections open (after APP-5)

## Learn first
- What's the difference between `shutdown(SHUT_RD)`, `shutdown(SHUT_WR)` and `close()`?
- How many file descriptors may one process open? (`ulimit -n`, `getrlimit`.) What happens at the limit?
