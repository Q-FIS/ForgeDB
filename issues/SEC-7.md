# [SEC-7] TLS in the event loop
labels: comp:sec, size:L
milestone: v2 Stretch

**Component:** SEC (with CONN) · **Size:** L (about a week) · **Needs:** SEC-3, CONN-5
**Your code:** `src/sec/tls.cpp`, `src/conn/` · **Tests:** `smoke` and `security_test.py` with `--io epoll --tls-cert ...`

## What is this, in plain words?
With blocking sockets, TLS is "call `SSL_read` and wait". In the event loop
nothing may wait, and TLS makes that much harder: a TLS **read** may need to
**write** (part of the handshake), and a write may need to read. OpenSSL tells
you with `SSL_ERROR_WANT_READ` / `SSL_ERROR_WANT_WRITE`, and your connection
state machine has to follow it. Every serious server (Nginx, HAProxy) solves
exactly this.

## What you must give back
1. `--io epoll` with TLS: non-blocking handshake (`SSL_accept` retried as the socket becomes ready), then non-blocking `SSL_read` / `SSL_write`, with the connection's epoll interest (`EPOLLIN`, `EPOLLOUT`) driven by what OpenSSL asks for, not by what HTTP wants.
2. Remember that OpenSSL may already hold decrypted bytes inside it (`SSL_pending`): epoll won't tell you about those.
3. The same tests as SEC-3, plus `chaos_test.py` against a TLS + epoll server, plus 1,000 idle TLS connections held by one thread.

## Watch out for
- After a `WANT_WRITE` on `SSL_write`, you must retry with the **same** buffer and length (or set `SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER`).

## Done when
- [ ] TLS works in both I/O modes; the tests pass on `debug` and `tsan`; CONN reviewed

## Learn first
- `man SSL_get_error` again, slowly, and OpenSSL's notes on non-blocking I/O.
