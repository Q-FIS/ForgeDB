# [CONN-4] Non-blocking sockets and an Epoll class
labels: comp:conn, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** CONN · **Size:** M · **Needs:** CONN-1
**Your code:** `include/forge/epoll.hpp` + `src/conn/epoll.cpp` (new) · **Tests:** `tests/unit/conn/`

## What is this, in plain words?
With blocking sockets, every connection needs its own thread, even when it's
doing nothing. 1,000 idle keep-alive clients = 1,000 threads. **epoll** lets
one thread watch thousands of sockets and ask the kernel "which of these are
ready?". This issue builds the tools; CONN-5 builds the event loop with them.

## What you must give back
- `void set_nonblocking(const Socket&)` (in `socket.hpp`): `fcntl(F_GETFL)`, add `O_NONBLOCK`, `fcntl(F_SETFL)`.
- An RAII `Epoll` class in a new header `include/forge/epoll.hpp`:
  ```cpp
  class Epoll {
  public:
      Epoll();                                              // epoll_create1(EPOLL_CLOEXEC)
      void add(int fd, std::uint32_t events, std::uint64_t tag);
      void modify(int fd, std::uint32_t events, std::uint64_t tag);
      void remove(int fd);
      // Waits up to `timeout` (-1 ms = forever); returns the ready (events, tag) pairs.
      std::vector<std::pair<std::uint32_t, std::uint64_t>> wait(std::chrono::milliseconds timeout);
  };
  ```
  Move-only, closes its fd in the destructor.
- An `EventFd` RAII class (`eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)`) with `notify()` and `drain()`: CONN-5 uses it so worker threads can wake the event loop.

## Step by step
1. Add the declarations; write the code; unit-test each piece:
   - a non-blocking socket's `recv` with no data fails with `EAGAIN` (instead of waiting);
   - `Epoll::wait` reports a listening socket as readable once a client connects, and returns nothing after a 50 ms timeout when nothing happens;
   - `EventFd::notify()` from another thread wakes a `wait`.
2. `recv_some` on a non-blocking socket throws on `EAGAIN` today. Decide (and write in the PR) how CONN-5 will tell "no data yet" apart from a real error, e.g. a `try_recv` that returns `std::optional<std::size_t>`.

## Watch out for
- Level-triggered (the default) vs edge-triggered (`EPOLLET`): pick level-triggered; it's much easier to get right.
- The `tag` (`epoll_data.u64`) is how you find your connection when its fd is ready: e.g. the fd itself, or an index.

## How to test it
`ctest --preset debug -R unit.conn` and on `tsan`.

## Done when
- [ ] `Epoll`, `EventFd` and `set_nonblocking` exist, with unit tests passing on `debug` and `tsan`
- [ ] The lead approved the new public declarations (`epoll.hpp` is a new contract CONN-5 builds on)

## Learn first
- `man 7 epoll`: read all of it, especially "Level-triggered and edge-triggered" and the Q&A at the end.
- The C10K problem (Dan Kegel's page): why thread-per-connection stops scaling.
