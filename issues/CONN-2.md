# [CONN-2] The server: connections in, requests to the handler, responses out
labels: comp:conn, size:M
milestone: v0.1 First search

**Component:** CONN · **Size:** M (2-3 days) · **Needs:** CONN-1, HTTP-1 (parser), HTTP-2 (responses). Start the accept loop before they're merged.
**Contract:** `include/forge/server.hpp` · **Your code:** `src/conn/server.cpp` · **Tests:** `tests/contract/conn/server_test.cpp`

## What is this, in plain words?
This is ForgeDB's front door. The `Server` accepts TCP connections and, for
each one, reads bytes, uses HTTP's parser to find complete requests, calls the
**handler** (the API), and writes the responses back. It doesn't know what a
chunk or a search is: it only moves requests and responses.

Each connection is served by one task on the **Executor** (the thread pool),
using blocking sockets: `IoMode::threads`. In CONN-5 you'll add a second way,
the epoll event loop, and in CONN-6 race the two.

## What you must give back
`Server`'s constructor, destructor, `run()`, `stop()` and `port()`, exactly as
`include/forge/server.hpp` describes. Add your private members to the header.

## Step by step
1. Delete `src/conn/stub_server.cpp`; create `src/conn/server.cpp`.
2. **Constructor:** `listen_tcp(options.host, options.port)`, and remember `local_port(...)`: that's what `port()` returns.
3. **`run()`:** loop: `accept_client`, then set a receive timeout on the new socket (`setsockopt(SO_RCVTIMEO)` with `options.idle_timeout`), then `executor.submit(...)` a task that serves the connection. If `submit` returns false (the pool is full), send a **503** response with a `Retry-After: 1` header and close.
4. **Serving one connection:** keep a `std::string buffer`. Loop:
   - `parse_request(buffer, options.limits)`.
   - `complete` → call the handler (catch `std::exception` → a **500** response), send `serialize(response, keep)`, where `keep = request.keep_alive()`. Then `buffer.erase(0, consumed)`: **the rest may already be the next request** (pipelining). If not `keep`, close.
   - `need_more` → `recv_some` and append. 0 bytes or a timeout → close.
   - `error` → send `Response::error(error_status, error_message)` with `Connection: close`, and close.
5. **`stop()`:** set an `std::atomic<bool>`, then `::shutdown(listener_fd, SHUT_RDWR)`: that wakes up a thread blocked in `accept()`, so `run()` can return.
6. **The destructor:** connection tasks use the Server's members (the handler, the options). The destructor must **wait until every connection task has finished**: keep a count (or a set of open fds) under a mutex, and wait on a `std::condition_variable` until it's empty.

## Watch out for
- `std::function` needs a **copyable** task, and a `Socket` can't be copied. Put the socket in a `std::shared_ptr<Socket>` and capture that.
- Never call the handler while holding a lock: it's slow, and it runs on many threads at once.
- A client may send half a request, wait, then send the rest. The tests do exactly that.
- `SO_RCVTIMEO` makes a silent connection's `recv` fail with `EAGAIN`: that's how idle connections get closed, so they don't hold a worker forever.

## How to test it
```bash
ctest --preset debug -R contract.conn.Server
ctest --preset tsan -R contract.conn.Server     # threads: run ThreadSanitizer too
```
By hand: write a tiny `main` in `tests/unit/conn/` or use a test, run it, and `curl -v localhost:<port>/hello`.

## Done when
- [ ] Every `contract.conn.Server.*` test passes on `debug` and `tsan`
- [ ] When APP-4 switches `forgedb` on, `blackbox.smoke` passes (pair with the APP owner on that day)

## Learn first
- TCP is a **byte stream**: two `send`s may arrive as one `recv`, and one `send` as three. Why? What does that mean for the loop above?
- What does `shutdown()` do that `close()` doesn't? (`man 2 shutdown`)
- *The Linux Programming Interface* section 61.1 (partial reads and writes), if you can get it.
