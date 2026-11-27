# [LOAD-5] The chaos test: clients that misbehave
labels: comp:load, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** LOAD · **Size:** M · **Needs:** APP-4, HTTP-4, CONN-3
**Your code:** `tests/blackbox/chaos_test.py` (replaces the placeholder)

## What is this, in plain words?
The internet is rude. Clients send one byte a minute, vanish halfway through,
send garbage, or open thousands of connections and do nothing. A server
must survive all of it and keep serving everyone else.

## What you must give back
`chaos_test.py <forgedb>`, starting `forgedb --idle-timeout-ms 1000`, then:
1. **Slowloris:** 20 connections sending one header byte every 200 ms: they're closed by the idle timeout or the header limit; meanwhile normal requests still succeed.
2. **Vanishing clients:** close right after sending half a request; close with a TCP reset (`SO_LINGER` with timeout 0) in the middle of a large search response.
3. **Garbage:** 1,000 random byte strings (fixed seed), each on a new connection: every one gets a 4xx or a closed connection, never a crash.
4. **Too big:** a 2 MiB body → 413; 16 KiB of headers → 431.
5. **Many idle connections:** open 500 and leave them idle; then check whether a new client is served. In `--io threads` mode with `--threads 4`, explain what you see (it's the C10K problem in action); with `--io epoll` (after CONN-5), it must be served.
6. After everything: `/health` is 200, a smoke-style put and search work, and stderr has no sanitizer reports.

## Done when
- [ ] `blackbox.chaos` passes on `debug` and `tsan`; the PR explains what part 5 showed

## Learn first
- What a TCP reset (RST) is, compared with a normal close (FIN).
