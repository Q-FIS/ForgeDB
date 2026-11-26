# [APP-3] The API: every endpoint, every error code
labels: comp:app, size:M
milestone: v0.1 First search

**Component:** APP · **Size:** M (2 days) · **Needs:** HTTP-3 (router), APP-2 (JSON), POOL-3 (Engine)
**Contract:** `include/forge/app.hpp`, `docs/API.md` · **Your code:** `src/app/api.cpp` · **Tests:** `tests/contract/app/api_test.cpp`

## What is this, in plain words?
The `Api` class is the server's handler: a request comes in, a response goes
out. It registers the four routes of `docs/API.md` on a `Router`, uses your
JSON functions and the Engine, and turns **every** error into the right
status code. `handle` never throws: one bad request must never take the
server down.

## What you must give back
`Api`'s constructor and `handle`.

| Route | Does |
| --- | --- |
| `GET /health` | `{"status": "ok", "chunks": N, "dim": D}` |
| `POST /v1/chunks` | `chunk_from_json` → `engine.put` → **201** `{"id", "row"}` |
| `GET /v1/chunks/{id}` | `engine.get` → `chunk_to_json`, or **404** |
| `POST /v1/search` | `search_from_json` → `engine.search` (timed) → `hits_to_json` |

| Exception | Status |
| --- | --- |
| `std::invalid_argument` | 400 |
| `AlreadyExists` | 409 |
| `NotImplemented` | 501 |
| any other `std::exception` | 500 |

## Step by step
1. Delete `src/app/stub_api.cpp`; create `src/app/api.cpp`. Add `Engine& engine_` and `http::Router router_` to the header.
2. In the constructor, `router_.add(...)` each route with a lambda that captures `this`.
3. `handle`: `try { return router_.handle(request); }`, then one `catch` per row of the table. **Order matters**: catch the most specific types first.
4. Time searches with `std::chrono::steady_clock`.

## Watch out for
- `NotImplemented` derives from `std::logic_error`, like `std::invalid_argument`. Make sure each one ends up with its own status code.
- Use `steady_clock`, not `system_clock`, for measuring durations: the wall clock can jump.

## How to test it
`ctest --preset debug -R contract.app.Api`

## Done when
- [ ] Every `contract.app.Api.*` test passes on `debug`

## Learn first
- cppreference: lambda captures (`[this]`, `[&]`, `[=]`), and why capturing `this` is fine here but dangerous if the `Api` were destroyed first.
