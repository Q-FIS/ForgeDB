# [CLI-7] An async client: thousands of requests in flight from one thread
labels: comp:cli, size:L
milestone: v2 Stretch

**Component:** CLI · **Size:** L (about a week) · **Needs:** CLI-6 (your v1 work is done), CONN-4 (`Epoll`)
**Your code:** `include/forge/async_client.hpp` (new, agreed in this issue), `src/cli/async_client.cpp` · **Tests:** `tests/unit/cli/`, `bench/async_client.cpp`

## What is this, in plain words?
`forge::Client` sends one request, waits, sends the next. To load 10 million
chunks, or to generate real load, you need many requests in flight at once,
without a thread for each. Build the client side of what CONN-5 does on the
server: one thread, non-blocking sockets, epoll, a pool of connections, and
**pipelining** (several requests sent on one connection before the answers come back).

## What you must give back
```cpp
class AsyncClient {
public:
    AsyncClient(std::string host, std::uint16_t port, std::size_t connections = 8, std::size_t pipeline_depth = 16);
    ~AsyncClient();  // finishes what's in flight, then stops its thread
    /// Queues a request; `done` is called (on the client's thread) with the reply,
    /// or with an error if the connection failed.
    void send(std::string method, std::string path, std::string body,
              std::function<void(std::optional<HttpReply>, std::string error)> done);
    void wait_idle();  // blocks until nothing is in flight
};
```
- Responses on one connection come back in request order: match them with a FIFO per connection.
- A connection that breaks fails its in-flight requests (their callbacks get an error) and is reopened.
- `forge load` gets a `--async` flag that uses it. Report the load rate, sync vs async, for 100,000 chunks, in the PR.
- `bench/async_client.cpp`: requests/s against `forgedb` for `GET /health` with 1, 4, 16 connections × pipeline depth 1, 4, 16.

## Watch out for
- Callbacks run on the client's own thread: a slow callback stalls everything. Say so in the header's comments.
- With pipelining, a server that closes the connection after a response (`Connection: close`) leaves the requests behind it unanswered: resend them (they were never processed) on a new connection.
- `send` is called from other threads: the queue from callers to the event loop needs a lock, and an `EventFd` to wake the loop (as in CONN-5).

## Done when
- [ ] Tests (including a server that drops connections) pass on `debug` and `tsan`; `forge load --async` works; the numbers are in the PR

## Learn first
- CONN-5's issue: you're building its mirror image. Pair with the CONN owner for an hour before you start.
