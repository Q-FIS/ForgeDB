# [SEC-3] TLS: encrypted connections with OpenSSL
labels: comp:sec, size:L
milestone: v1.0 Fast, and powers the RAG

**Component:** SEC (with CONN: CONN reviews) · **Size:** L (about a week) · **Needs:** CONN-3, SEC-2
**Your code:** `include/forge/tls.hpp` (new, agreed in this issue), `src/sec/tls.cpp`, changes in `src/conn/server.cpp` and `src/cli/client.cpp` · **Tests:** `tests/unit/sec/`, `tests/blackbox/security_test.py`

## What is this, in plain words?
Today every byte, API keys included, crosses the network in plain text:
anyone on the path can read it (try it: Wireshark on the loopback). **TLS**
(the S in HTTPS) encrypts the connection and proves the server is who it
says it is, with a certificate. You won't write the cryptography (never do
that); you'll put OpenSSL between ForgeDB's sockets and its HTTP code, and
learn what actually happens in a handshake.

## What you must give back
1. `forgedb --tls-cert FILE --tls-key FILE`: every connection does a TLS handshake (`SSL_accept`), then reads and writes through `SSL_read` / `SSL_write`, in `--io threads` mode. (epoll mode is SEC-7.)
2. A small RAII layer in `tls.hpp`: a `TlsContext` (`SSL_CTX`, loads the certificate and key; TLS 1.2 minimum) and a `TlsStream` (one `SSL*` over a `Socket`) with `read_some` / `write_all`, so the server's loop uses either a plain socket or TLS without caring which.
3. `forge --tls --ca FILE ...`: the client side, which **checks the certificate** (`SSL_CTX_load_verify_locations`, `SSL_set1_host`).
4. A test certificate made with `openssl req -x509 ...` in a script (`scripts/make_test_cert.sh`), not committed keys.
5. Tests: `curl --cacert test-ca.pem https://localhost:PORT/health` works; a plain-HTTP client to the TLS port fails cleanly (the server logs it and carries on); a client with the wrong CA refuses to connect.
6. In the PR: a Wireshark screenshot (or `tshark` output) of the same request with and without TLS. Can you still see the API key?

## Watch out for
- `SSL_read` can return less than asked, and can fail with `SSL_ERROR_WANT_READ` even on a blocking socket during renegotiation: read the man page's RETURN VALUES carefully.
- OpenSSL errors are a queue: print them with `ERR_print_errors_fp(stderr)`, and clear them, or the next call reports an old error.
- `SSL_free` and `SSL_CTX_free` in destructors: RAII again, with a C library.
- Add `find_package(OpenSSL REQUIRED)` and link `OpenSSL::SSL` in the top-level `CMakeLists.txt`: the one CMake edit in this project, agreed with the lead (`sudo apt install libssl-dev`; add it to CI too).

## Done when
- [ ] TLS works in `forgedb` and `forge`; tests pass on `debug` and `tsan`; CONN reviewed; CI installs `libssl-dev`

## Learn first
- "The Illustrated TLS 1.3 Connection" (tls13.xargs.org): every byte of a handshake, explained.
- `man SSL_read`, `man SSL_get_error`.
