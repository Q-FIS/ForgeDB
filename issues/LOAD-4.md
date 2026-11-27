# [LOAD-4] The concurrency test: many clients at once, nothing lost
labels: comp:load, size:M
milestone: v0.2 Survives a crash

**Component:** LOAD · **Size:** M · **Needs:** APP-4
**Your code:** `tests/blackbox/concurrency_test.py` (replaces the placeholder)

## What is this, in plain words?
Unit tests check pieces; this checks the whole server under real
concurrency, and ctest runs it on the **tsan** build too, so ThreadSanitizer
watches every thread of `forgedb` while your clients hammer it.

## What you must give back
`concurrency_test.py <forgedb>`:
1. 32 client threads, each putting 50 chunks with unique ids and a unique word in the text (`token-<thread>-<n>`), searching (vector and keyword) between puts.
2. Afterwards: `/health` counts exactly 1,600; every chunk reads back exactly; a keyword search for each unique word finds exactly its chunk, first; no response was ever malformed or a 500.
3. The same again with `--threads 2 --queue 4`, so the server refuses work (503): clients wait (`Retry-After`) and retry. Everything must still arrive exactly once: a 503 must mean "not done", never "half done".
4. The server's stderr has no sanitizer report (`harness.py` checks this for you).

## Watch out for
- Python threads are fine here: they spend their time waiting for the network.
- Keep it under ~60 s on the tsan build (which is slow): measure, and scale the numbers down if needed.

## Done when
- [ ] `blackbox.concurrency` passes on `debug` and `tsan`

## Learn first
- What does ThreadSanitizer actually detect, and what can't it see? (The Clang docs page for ThreadSanitizer.)
