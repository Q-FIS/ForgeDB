# [HTTP-4] Limits and awkward requests: 413, 431, 501
labels: comp:http, size:S
milestone: v0.2 Survives a crash

**Component:** HTTP · **Size:** S · **Needs:** HTTP-1
**Your code:** `src/http/parser.cpp` · **Tests:** `tests/unit/http/test_limits.cpp` (yours)

## What is this, in plain words?
A server that accepts any request will be knocked over by one. Without
limits, a client can send headers forever (your buffer grows until the
machine runs out of memory) or claim a 50 GB body. Now the parser refuses,
early and cheaply.

## What you must give back
In `parse_request`, using `limits`:
| Case | Result |
| --- | --- |
| No `\r\n\r\n` yet, but more than `max_header_bytes` received | error **431**, immediately (don't wait for more) |
| Headers (up to and including `\r\n\r\n`) longer than `max_header_bytes` | **431** |
| `Content-Length` bigger than `max_body_bytes`, or too many digits to fit in 64 bits | **413**, as soon as the headers are in (before the body arrives) |
| Two `Content-Length` headers with different values | **400** (two with the same value: fine) |
| Any `Transfer-Encoding` header | **501** (we don't support chunked bodies) |

## Step by step
1. Write the tests first, in `tests/unit/http/test_limits.cpp`: one `TEST` per row above, plus the edges (exactly `max_header_bytes`: fine; one byte more: 431).
2. Change the parser until they pass. The contract tests must still pass.

## Watch out for
- Check the limits **before** waiting for more bytes: the point is to refuse before the memory is spent.
- `Content-Length: 99999999999999999999999` overflows `unsigned long`: don't parse it blindly.

## How to test it
`ctest --preset debug -R "unit.http|contract.http"`

## Done when
- [ ] Your limit tests and every contract test pass on `debug`
- [ ] By hand: `head -c 20000 /dev/zero | tr '\0' 'a' | nc localhost 8080` gets a 431 from `forgedb` (after APP-4)

## Learn first
- What's a slowloris attack? Which of these limits helps against it, and which doesn't? (CONN-3's idle timeout is the other half.)
- HTTP request smuggling (PortSwigger's write-up): why are two `Content-Length`s dangerous?
