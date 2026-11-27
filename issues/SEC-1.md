# [SEC-1] A rate limiter (token buckets) and safe secret comparison
labels: comp:sec, size:M
milestone: v0.2 Survives a crash

**Component:** SEC (the edge and security) · **Size:** M (2 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/security.hpp` · **Your code:** `src/sec/rate_limiter.cpp` · **Tests:** `tests/contract/sec/rate_limiter_test.cpp`

## What is this, in plain words?
A public server needs limits: one misbehaving script must not be able to use
all of ForgeDB's capacity. The standard tool is the **token bucket**: each
client has a bucket of tokens that refills at a steady rate; each request
takes one; an empty bucket means "429 Too Many Requests, try again in N
seconds". It allows short bursts but caps the average rate. Nginx, AWS and
Stripe all limit requests this way.

You'll also write the one comparison every secret needs: **constant-time**
equality, so an attacker can't guess an API key one byte at a time by
measuring how long the server takes to say no.

## What you must give back
`RateLimiter` (constructor, `allow`, `retry_after`, `forget_idle`, `tracked_keys`)
and `constant_time_equals`, exactly as `include/forge/security.hpp` describes.

## Step by step
1. Delete `src/sec/stub_rate_limiter.cpp`; create `src/sec/rate_limiter.cpp`. Add your members to the header.
2. Per key: `tokens` (a `double`), the time of the last refill, the time last seen. A `std::map<std::string, Bucket, std::less<>>` lets you look keys up by `string_view` (as in SNAP-1).
3. **Refill lazily**: don't run a timer. Whenever you touch a bucket, add `elapsed_seconds × rate` tokens, capped at `burst`, and move the refill time to `now`.
4. `allow`: refill; if `tokens >= 1`, take one and return true; else return false **without** changing the tokens.
5. `retry_after`: `(1 − tokens) / rate` seconds, or zero if a token is already there. Don't change anything (it's `const`).
6. One `std::mutex` around everything: requests from many threads share the limiter.
7. `constant_time_equals`: if the lengths differ, return false (the length isn't secret); otherwise OR together `a[i] ^ b[i]` over **every** byte, and only look at the result at the end. No early `return`, no `break`, no `==` on the strings.

## Watch out for
- Time is passed in: never call `steady_clock::now()` inside the class. That's what makes the tests exact (they use a fake clock).
- `std::chrono` durations: convert with `std::chrono::duration<double>(d).count()` to get seconds as a `double`.
- A refused request must not take a token, or a client that keeps retrying never gets through.

## How to test it
```bash
ctest --preset debug -R contract.sec
ctest --preset tsan  -R contract.sec
```

## Done when
- [ ] Every `contract.sec.*` test passes on `debug` and `tsan`
- [ ] In the PR: why does `memcmp` leak timing, and why doesn't your loop?

## Learn first
- The token bucket algorithm (Wikipedia is enough), and how it differs from a fixed window ("100 per minute").
- "Timing attacks" on string comparison: why `memcmp` stops at the first difference.
