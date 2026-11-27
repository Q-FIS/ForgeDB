# [SEC-2] API keys and rate limits in `forgedb`: 401 and 429
labels: comp:sec, size:M
milestone: v0.2 Survives a crash

**Component:** SEC (changes APP's code: APP reviews) · **Size:** M · **Needs:** SEC-1, APP-4
**Your code:** `src/sec/`, small changes in `src/app/` and `apps/forgedb.cpp`, `docs/API.md` (agreed with the lead) · **Tests:** `tests/blackbox/security_test.py` (replaces the placeholder)

## What is this, in plain words?
Put SEC-1 to work. With `--api-keys FILE`, every request must carry a valid
key (`Authorization: Bearer <key>`), and each key gets its own rate limit.
Without the flag, ForgeDB behaves exactly as before, so nothing else breaks.

## What you must give back
1. New flags (with the APP owner): `--api-keys FILE` (one key per line; `#` comments) and `--rate-limit N` (requests per second per key, default 100; burst = 2 × N).
2. A check that runs **before** the router: no `Authorization` header, or a key not in the file (compared with `constant_time_equals`) → **401** `{"error": "missing or invalid API key"}` with a `WWW-Authenticate: Bearer` header. `GET /health` stays open (load balancers need it).
3. Rate limits per key: over the limit → **429** with `Retry-After: <seconds, rounded up>`. Clean up idle keys now and then (`forget_idle`).
4. `SIGHUP` reloads the key file without a restart (the APP owner's signal thread reads it; swap the key set safely between threads).
5. `docs/API.md`: the `Authorization` header, 401 and 429 in the status table. And `reason_phrase` (HTTP's code, HTTP reviews) learns 401 "Unauthorized" and 429 "Too Many Requests".
6. `security_test.py`: no key → 401; a wrong key → 401; a good key → 200; `/health` works without a key; 300 fast requests with `--rate-limit 50` → some 429s, each with a `Retry-After`; after waiting, requests pass again; editing the key file and sending SIGHUP adds and removes keys.

## Watch out for
- Never log the keys, not even in error messages. Log the first 4 characters at most.
- Comparing the key against each key in the file one by one, stopping at the first match, leaks *which* key matched. Hash both sides (`std::hash` is not good enough for real security; say so in the PR) or compare against all of them.

## Done when
- [ ] `blackbox.security` passes on `debug` and `tsan`; every other black-box test still passes (they run without `--api-keys`)

## Learn first
- RFC 6750 sections 2.1 and 3 (Bearer tokens, `WWW-Authenticate`), and RFC 6585 section 4 (429).
