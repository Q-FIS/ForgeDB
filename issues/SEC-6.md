# [SEC-6] Experiment: what does security cost?
labels: comp:sec, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** SEC · **Size:** S · **Needs:** SEC-3, SEC-4, LOAD-2
**Your output:** `docs/experiments/SEC-6-cost-of-security.md`

## What is this, in plain words?
Encryption, key checks and sandboxes aren't free. Measure how much each one
costs, so the club can decide with numbers instead of fear.

## What you must give back
On a release build, with `forge-load` (it needs a `--tls` option: add it, with the LOAD owner):
1. **TLS handshake cost:** new connections per second, plain vs TLS (one request per connection, `Connection: close`). Then with **TLS session resumption** on.
2. **Steady state:** requests/s and p99 for `GET /health` and a vector search on kept-alive connections, plain vs TLS. Why is the difference small here and big in point 1?
3. API keys plus rate limiting: requests/s with and without `--api-keys`.
4. The seccomp filter: requests/s with and without `--no-sandbox`. (Hint: a BPF filter runs on every system call.)
5. A conclusion: which protections should always be on?

## Done when
- [ ] The report is merged and presented

## Learn first
- What happens in a TLS 1.3 handshake, in round trips, and why keep-alive makes it matter less.
