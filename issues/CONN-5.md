# [CONN-5] The event loop: `--io epoll`
labels: comp:conn, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** CONN · **Size:** M (3 days) · **Needs:** CONN-3, CONN-4
**Your code:** `src/conn/` · **Tests:** `smoke.epoll` (uncomment it), `tests/unit/conn/`

## What is this, in plain words?
The second way to run the server: **one thread** owns every connection. It
waits in `epoll_wait`, reads whatever bytes are ready, parses, and hands each
complete request to the pool. When a worker finishes, it puts the response in
a queue and wakes the loop with the `EventFd`; the loop writes it out. No
thread ever blocks on a slow client.

## What you must give back
`Server` with `options.io == IoMode::epoll` behaves exactly like the threads
mode, from the outside: same responses, keep-alive, pipelining, limits,
idle timeout, 503s, `stop()`.

## Step by step
1. A `Connection` struct per fd: the socket, an input buffer, an output buffer, the last-activity time, and a state (reading, waiting for the handler, writing).
2. The loop: `epoll.wait(timeout)`. Listener readable → `accept` until `EAGAIN`, `set_nonblocking`, add with `EPOLLIN`. Connection readable → read until `EAGAIN`, then `parse_request`; when complete, `executor.submit` a task that calls the handler and pushes `{fd, response}` to a mutex-protected "done" queue, then `eventfd.notify()`.
3. EventFd readable → `drain()`, move done responses into their connections' output buffers, and switch them to `EPOLLOUT`.
4. Connection writable → `send` as much as it takes; when the buffer is empty, go back to reading (or close if not keep-alive).
5. Every loop: close connections idle longer than `idle_timeout`.
6. One request at a time per connection: don't parse the next pipelined request until the current response is written. This keeps responses in order.
7. Uncomment `smoke.epoll` in `tests/blackbox/CMakeLists.txt`.

## Watch out for
- A connection can close while its request is in the pool. When the response arrives, the fd may belong to a **new** connection. Use a connection id (a counter), not the fd, to match them.
- `send` on a non-blocking socket may send part of the buffer: keep the rest and wait for `EPOLLOUT`.
- Never call the handler on the loop thread: one slow search would freeze every client.

## How to test it
```bash
ctest --preset debug -R "smoke|keepalive|contract.conn"
ctest --preset tsan -R "smoke|keepalive|contract.conn"
```
Also run `keepalive_test.py` and `concurrency_test.py` (LOAD-4) by hand with `--io epoll` added.

## Done when
- [ ] `blackbox.smoke.epoll` is switched on and passes on `debug` and `tsan`
- [ ] The CONN-2 contract tests still pass (threads mode unchanged)
- [ ] 1,000 idle connections are held with one thread (check `ls /proc/<pid>/task | wc -l`)

## Learn first
- Read a small event loop's source: e.g. Redis's `ae.c` (about 500 lines).
- What is the "thundering herd", and why doesn't it matter with one loop thread?
