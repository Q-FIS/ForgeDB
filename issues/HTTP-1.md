# [HTTP-1] Parse an HTTP request
labels: comp:http, size:M
milestone: v0.1 First search

**Component:** HTTP · **Size:** M (2-3 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/http.hpp` · **Your code:** `src/http/parser.cpp` · **Tests:** `tests/contract/http/parser_test.cpp`

## What is this, in plain words?
A client sends bytes like these:
```
POST /v1/search HTTP/1.1\r\n
Host: localhost\r\n
Content-Length: 18\r\n
\r\n
{"vector": [1, 0]}
```
Your parser turns bytes into a `Request` (method, target, version, headers,
body), or says "I need more bytes", or says "this is broken: answer 400".
It never touches a socket: it's a pure function of the bytes, so it's easy to
test, and the server (CONN-2) and the fuzzer (HTTP-5) both use it. This is the
most security-sensitive code in ForgeDB: everything from the internet goes
through it first.

## What you must give back
`parse_request`, and the `Request` helpers `header()`, `path()`, `query()`,
`keep_alive()`, all as described in `include/forge/http.hpp`.

**The rules (this is what the contract tests check):**
| Part | Rule | If broken |
| --- | --- | --- |
| End of headers | the first `\r\n\r\n`. Not found yet → `need_more` | |
| Request line | exactly `METHOD SP TARGET SP VERSION` (single spaces, 3 parts) | 400 |
| Method | one or more uppercase letters `A-Z` | 400 |
| Target | starts with `/` | 400 |
| Version | exactly `HTTP/1.1` or `HTTP/1.0` | 400 |
| Header line | `Name: value`. The name is not empty and has no spaces. The value is trimmed of spaces and tabs (may be empty) | 400 |
| Folded lines | a header line starting with a space or tab is not allowed | 400 |
| `Content-Length` | digits only. Missing = no body. The body is that many bytes after `\r\n\r\n`; fewer received → `need_more` | 400 |
| `consumed` | header bytes + 4 + body length. Bytes after that belong to the **next** request | |

Limits (431/413) and `Transfer-Encoding` (501) come in HTTP-4.

## Step by step
1. Delete `src/http/stub_parser.cpp`; create `src/http/parser.cpp`.
2. Write the `Request` helpers first (easy wins): `path()` is the target up to `?`; `header()` compares names ignoring case; `keep_alive()` follows the comment in the header.
3. `parse_request`: find `\r\n\r\n`. Split the head into lines at `\r\n`. Parse the request line, then each header line. Then the body.
4. Use `std::string_view` to look at pieces of the input without copying; copy into the `Request`'s `std::string`s only at the end.
5. Make every error message short and clear (`"bad method"`, `"Content-Length is not a number"`): it goes back to the client.

## Watch out for
- `std::stoul` accepts `"  5"`, `"5abc"` and `"-5"` (!): check that every character is a digit yourself, and that the number isn't absurdly long.
- `string_view::substr` and `find` return `npos` when nothing is found: handle it before doing arithmetic with it.
- Don't read past the end: the input may stop anywhere, even in the middle of `\r\n`.

## How to test it
```bash
ctest --preset debug -R contract.http.Parser
```
The test `EveryPrefixOfARequestNeedsMore` cuts a request at every byte: it catches most out-of-bounds bugs (and AddressSanitizer catches the rest).

## Done when
- [ ] Every `contract.http.Parser.*` and `contract.http.Request.*` test passes on `debug`
- [ ] No warnings, no sanitizer reports

## Learn first
- RFC 9112 (HTTP/1.1), sections 2.1, 3 and 5: skim; notice how strict the grammar is.
- What's "request smuggling", and why do two `Content-Length` headers matter? (HTTP-4 handles it.)
