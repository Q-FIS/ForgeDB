# [HTTP-2] Build HTTP responses
labels: comp:http, size:S
milestone: v0.1 First search

**Component:** HTTP · **Size:** S (1 day) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/http.hpp` · **Your code:** `src/http/response.cpp` · **Tests:** `tests/contract/http/response_test.cpp`

## What is this, in plain words?
The other direction: turn a `Response` (status, headers, body) into the exact
bytes that go on the wire, plus helpers that every handler uses:
`Response::json`, `Response::text` and `Response::error`.

## What you must give back
`reason_phrase`, `serialize`, `Response::json`, `Response::text`, `Response::error`.

| Status | Reason |
| --- | --- |
| 200 | OK |
| 201 | Created |
| 400 | Bad Request |
| 404 | Not Found |
| 405 | Method Not Allowed |
| 409 | Conflict |
| 413 | Content Too Large |
| 431 | Request Header Fields Too Large |
| 500 | Internal Server Error |
| 501 | Not Implemented |
| 503 | Service Unavailable |
| anything else | Unknown |

`serialize` writes exactly: the status line, each of `response.headers` in
order, `Content-Length`, `Connection: keep-alive` or `Connection: close`, an
empty line, the body (see the header file).

## Step by step
1. Delete `src/http/stub_response.cpp`; create `src/http/response.cpp`.
2. `reason_phrase`: a `switch`.
3. `serialize`: build one `std::string`. Reserve its size first (`out.reserve(...)`) to avoid re-allocating.
4. `Response::error(status, message)`: the body is `{"error": "<message>"}`, and the message **must be escaped for JSON**: `"` → `\"`, `\` → `\\`, newline → `\n`, tab → `\t`, and any other byte below 0x20 → `\u00XX`. Write the escaping yourself (it's 15 lines, and you'll understand JSON strings forever), or use nlohmann/json.

## Watch out for
- Bodies can contain any bytes, including `\0`: use the string's `size()`, never `strlen`.
- A message containing a quote or a newline must still produce valid JSON: the test tries exactly that.

## How to test it
`ctest --preset debug -R contract.http.Response`

## Done when
- [ ] Every `contract.http.Response.*` test passes on `debug`

## Learn first
- json.org: the one-page picture of what a JSON string may contain.
- Why does a response need `Content-Length` when the connection stays open?
