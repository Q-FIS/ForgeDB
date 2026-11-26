# [HTTP-3] The router: which handler gets the request?
labels: comp:http, size:S
milestone: v0.1 First search

**Component:** HTTP · **Size:** S (1 day) · **Needs:** HTTP-2
**Contract:** `include/forge/http.hpp` · **Your code:** `src/http/router.cpp` · **Tests:** `tests/contract/http/router_test.cpp`

## What is this, in plain words?
The API (APP-3) registers routes: `router.add("GET", "/v1/chunks/{id}", handler)`.
For each request, the router finds the matching route, pulls `{id}` out of
the path (decoding `%3A` back to `:`), and calls the handler. No match:
404. Right path, wrong method: 405, with an `Allow` header saying which
methods would work.

## What you must give back
`Router::add` and `Router::handle`, following the comments in the header.

## Step by step
1. Delete `src/http/stub_router.cpp`; create `src/http/router.cpp`.
2. `add`: split the pattern at `/` into segments and store them.
3. `handle`: split `request.path()` (no query) the same way. A route matches if it has the same number of segments, and each segment is either equal, or a `{name}` matching a **non-empty** segment.
4. Go through the routes in order. The first one matching path and method wins. If paths match but no method does, collect the methods for `Allow` (`"GET, DELETE"`).
5. Percent-decode each parameter: `%` followed by exactly two hex digits becomes that byte; anything else after `%` → 400.
6. 404 and 405 bodies use `Response::error`.

## Watch out for
- `"/health/"` (trailing slash) is not `"/health"`: 404. Splitting `"/a/"` gives `["", "a", ""]`, so the counts differ.
- Decode **after** matching, so an encoded `%2F` (a `/` inside an id) can't change the route.
- Don't catch exceptions from handlers: APP-3 decides what they mean.

## How to test it
`ctest --preset debug -R contract.http.Router`

## Done when
- [ ] Every `contract.http.Router.*` test passes on `debug`

## Learn first
- RFC 3986 section 2.1 (percent-encoding): what it's for, and which characters need it.
