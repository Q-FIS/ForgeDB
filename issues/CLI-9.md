# [CLI-9] Clients that survive failures: timeouts, retries, failover
labels: comp:cli, size:M
milestone: v2 Stretch

**Component:** CLI · **Size:** M (3-4 days) · **Needs:** CLI-3, CLI-8
**Your code:** `src/cli/`, `clients/python/forgedb.py` · **Tests:** `tests/unit/cli/`, `tests/blackbox/python_client_test.py`

## What is this, in plain words?
Networks drop connections, servers restart, replicas fall behind. A good
client hides the small failures and reports the real ones clearly, without
making things worse (a thousand clients retrying instantly can knock over a
server that was recovering). That's an art with well-known rules: timeouts,
retries with **exponential backoff and jitter**, respecting `Retry-After`,
knowing which requests are safe to repeat, and failing over to another server.

## What you must give back
In both `forge::Client` (C++) and `forgedb.py`:
1. **Timeouts** for connecting and for each response (defaults 2 s and 10 s), configurable.
2. **Retries**, at most 3 by default, only when it's safe: connection refused or reset, timeouts, 503. Wait `min(cap, base × 2^attempt) × random(0, 1)` ("full jitter"), or the server's `Retry-After` if it sent one.
3. **Safe retries for puts.** A put whose answer was lost may or may not have happened. On retry, a 409 might mean "it worked the first time": then `get` the id and compare. Same content → success; different → a real conflict error.
4. **Failover**: given several servers (`ForgeDB(["host1:8080", "host2:8080"])`), send searches to any healthy one, and puts only to the primary (the one whose `/health` has no `replica_of`).
5. Tests with a misbehaving server: one that resets connections, one that answers 503 twice then 200, one that never answers (timeout), and a primary killed during a load (the client keeps working against the restarted server).

## Watch out for
- Never retry a request that isn't safe to repeat without the check in point 3: double inserts are the classic bug.
- Log each retry (once, with the reason): silent retries hide real problems.

## Done when
- [ ] Both clients have the four behaviours, with tests on `debug`; the Python client's README documents them

## Learn first
- AWS Architecture Blog, "Exponential Backoff And Jitter" (Marc Brooker): read the charts.
- Google SRE book, chapter 22, "Addressing Cascading Failures": the part on retries.
