# [LOAD-1] The full API test suite
labels: comp:load, size:M
milestone: v0.1 First search

**Component:** LOAD (testing and measurement) · **Size:** M (2-3 days) · **Needs:** nothing, start on day 1 (against the fake server)
**Contract:** `docs/API.md` · **Your code:** `tests/blackbox/api_test.py` (replaces the placeholder) · **Helpers:** `tests/blackbox/harness.py`

## What is this, in plain words?
`smoke_test.py` checks that the main paths work. Your suite checks
**everything** `docs/API.md` promises: every field rule, every status code,
every HTTP detail. It runs against the fake server today (which proves your
test is right), and against the real `forgedb` the moment APP-4 switches it
on, so it becomes the referee for every component's work.

## What you must give back
`api_test.py <server>` (where `<server>` is `fake` or the path to `forgedb`), using `harness.py`, with sections:
1. **Chunks:** every rule of the `POST /v1/chunks` table: each required field missing, each wrong type, empty id, unknown field, wrong length, all zeros, `meta` with a non-string value, `meta` missing (→ `{}` when read back); 409; the `row` numbers; reading back exactly what was put (unicode, quotes, a 10,000-character text).
2. **Reading:** ids with `:`, `/` (as `%2F`), spaces, unicode; unknown id → 404.
3. **Search:** each rule of the search table; default `k` is 10 (put 15 chunks); `k` = 1 and 1000; fewer results than k; filters by `doc_id`, by one and by two `meta` pairs, and both; equal scores ordered by row (identical vectors); scores best first; vector scores are cosines (a chunk's own vector scores ≈ 1.0); hybrid gives 200 with hits, **or** 501 on a `forgedb` built before KW-5.
4. **HTTP:** 404 and 405 (with `Allow`) on each path; keep-alive (50 requests on one connection); `Connection: close`; HTTP/1.0; a malformed request line → 400; pipelining (3 requests in one `send`).
5. **Limits, `forgedb` only** (the fake server can't do these: skip them when `server == "fake"`): a body over 1 MiB → 413; headers over 8 KiB → 431; `Transfer-Encoding: chunked` → 501.

Group the output by section, like `smoke_test.py`, and make every failure message say what was expected and what came back.

## Step by step
1. Read `docs/API.md` and `smoke_test.py` closely. Copy smoke's structure.
2. Write one section at a time; run it against the fake server after each: `python3 tests/blackbox/api_test.py fake`.
3. If a check fails against the fake, either the fake or your test is wrong: work out which (with the lead if it's the fake).

## Watch out for
- Tests must not depend on each other's leftovers: start a fresh server (`with forgedb(...)`) per section, or use unique ids.
- Floating-point scores: compare with a tolerance (`abs(a - b) < 1e-5`), never `==`.

## How to test it
`ctest --preset debug -R blackbox.api` (runs it against both the fake and `forgedb`).

## Done when
- [ ] `blackbox.api.fake` passes, and `blackbox.api` passes or is skipped (before APP-4)
- [ ] When `forgedb` is switched on, you triage every failure: a bug in which component? Open an issue for each, assigned to its owner

## Learn first
- Python's `socket` module basics, and `harness.py` line by line: you'll use it all the time.
