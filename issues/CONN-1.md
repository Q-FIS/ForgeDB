# [CONN-1] A Socket class that closes itself
labels: comp:conn, size:M
milestone: v0.1 First search

**Component:** CONN (connections) · **Size:** M (2-3 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/socket.hpp` · **Your code:** `src/conn/socket.cpp` · **Tests:** `tests/contract/conn/socket_test.cpp`

## What is this, in plain words?
Every network connection is a **socket**: a file descriptor (an `int`) the
kernel gives you. It must be closed exactly once. Forget, and the server leaks
one per client until it can't accept any more. Close it twice, and you may
close a *different* client's socket that got the same number in between.

C++ fixes this with **RAII**: the `int` lives inside a class whose destructor
closes it. You did this for files in Lab 1 (`Fd`); now you do it for sockets,
plus the five functions everybody in ForgeDB uses to talk TCP. The server
(CONN-2), the client (CLI-1) and the load generator (LOAD-2) are all built on
your code.

## What you must give back
Everything declared in `include/forge/socket.hpp`: the `Socket` class and
`listen_tcp`, `local_port`, `accept_client`, `connect_tcp`, `send_all`,
`recv_some`. Read the comments in the header: they are the exact rules.

## Step by step
1. Delete `src/conn/stub_socket.cpp` and create `src/conn/socket.cpp`. Build: the contract tests now run (and fail).
2. **The class.** Destructor: `if (fd_ != -1) ::close(fd_);`. Move constructor: take `other.fd_`, set `other.fd_ = -1` (`std::exchange` does both). Move assignment: if `this != &other`, close your own fd, then take the other's. Mark both moves `noexcept`.
3. **`listen_tcp`:** fill a `sockaddr_in` (`AF_INET`, `htons(port)`, and `inet_pton` for the address: if it returns 1 it worked, otherwise throw `std::invalid_argument`). Then `socket` → `setsockopt(SO_REUSEADDR)` → `bind` → `listen`. Wrap each call in `check(...)` from `forge/check.hpp`, and put the fd in a `Socket` **straight after** `socket()` returns, so it's closed if `bind` throws.
4. **`local_port`:** `getsockname`, then `ntohs`.
5. **`accept_client`:** `accept` in a loop: on `EINTR` try again, on any other error throw.
6. **`connect_tcp`:** `getaddrinfo(host, port_as_text, &hints, &result)` with `hints.ai_family = AF_INET` and `hints.ai_socktype = SOCK_STREAM`. If it fails, throw `std::runtime_error` with `gai_strerror(rc)` (it doesn't use errno!). Try each address in the list until one `connect`s. Always `freeaddrinfo`.
7. **`send_all`:** loop `send(fd, data, size, MSG_NOSIGNAL)` until every byte is out; retry on `EINTR`.
8. **`recv_some`:** one `recv`; retry on `EINTR`; return what it returns (0 = closed).

## Watch out for
- `send` may send **fewer bytes** than asked: that's why it's `send_all`. Advance by what was sent.
- Without `MSG_NOSIGNAL`, writing to a closed connection kills the whole process with `SIGPIPE`. One test checks exactly this.
- `getaddrinfo("localhost")` may return an IPv6 address first. With `AF_INET` you only get IPv4, which is what `listen_tcp` uses.
- A moved-from `Socket` must be empty, or its destructor closes the fd the new owner is using.

## How to test it
```bash
cmake --build --preset debug -j && ctest --preset debug -R contract.conn.Socket
```
Add your own tests in `tests/unit/conn/` if you want more (e.g. `listen_tcp("0.0.0.0", 0)`).

## Done when
- [ ] Every `contract.conn.Socket.*` test passes on `debug` and `tsan`
- [ ] No `close()` anywhere except in the `Socket` class
- [ ] In the PR: the `strace -f -e trace=network` output of the ConnectAcceptAndTalkBothWays test, with each line explained in a few words

## Learn first
- Beej's Guide to Network Programming, chapters 5 and 6 (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `getaddrinfo`).
- Why does `bind` fail with "Address already in use" right after a server restarts, and how does `SO_REUSEADDR` help? (Look up TCP's `TIME_WAIT` state.)
- Why must a class that owns a resource be move-only? (Lab 1, Part 3.)
