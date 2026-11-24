# The build issues

**New? Read [`START_HERE.md`](START_HERE.md) first.**

Each `W*.md` file here is one task. Weeks 1-2 are the same for both groups,
so they become two GitHub issues each (one per group, titled `[A · W1.2] ...`
and `[B · W1.2] ...`). From week 3 the groups' tasks differ.

## How work is shared out

- **Each group has about 5 issues a week, and 5 people.** At the weekly group
  meeting, everyone picks one (the captain settles clashes). Bigger issues (M)
  suit a pair; small ones (S) suit one person.
- **Assign yourself on GitHub** when you start, so nobody does the same thing twice.
- **Finished early?** Help whoever is on the hardest issue, or start the next
  week's issue that doesn't depend on anything.
- **Stuck on an issue for a day?** Say so in the group meeting or channel. Pairing up is normal.
- **Labs aren't issues.** They're done alone, in your own repo (see `labs/README.md`).

## Weeks 1-4

| Week | Issue | Size | Groups | What |
| --- | --- | --- | --- | --- |
| 1 | W1.1 | S | A, B | A Socket class that closes itself (RAII) |
| 1 | W1.2 | M | A, B | The echo server |
| 1 | W1.3 | S | A, B | The echo client |
| 1 | W1.4 | S | A, B | Robustness: SIGPIPE, EINTR, restarts, bad arguments |
| 2 | W2.1 | M | A, B | Parse an HTTP request (no copies) |
| 2 | W2.2 | S | A, B | Build HTTP responses |
| 2 | W2.3 | S | A, B | Routes: /health, /echo, static files (no path traversal) |
| 2 | W2.4 | M | A, B | The HTTP server: whole requests, keep-alive, limits |
| 2 | W2.5 | S | A, B | Fuzz it harder (libFuzzer), and write down what broke |
| 3 | W3.A1 | S | A | A bounded queue (template, condition variables) |
| 3 | W3.A2 | M | A | The worker pool (`std::jthread`) |
| 3 | W3.A3 | S | A | Acceptor, 503 backpressure, /sleep |
| 3 | W3.A4 | M | A | Graceful shutdown on SIGTERM |
| 3 | W3.A5 | S | A | Load test: how many workers? |
| 3 | W3.B1 | S | B | An Epoll class, and non-blocking sockets |
| 3 | W3.B2 | M | B | The connection state machine |
| 3 | W3.B3 | M | B | The event loop |
| 3 | W3.B4 | S | B | Partial writes and big responses |
| 3 | W3.B5 | S | B | /sleep, 1,000 connections, the concurrency tests |
| 4 | W4.A1 | S | A | Idle timeout for keep-alive connections |
| 4 | W4.A2 | S | A | Timing every request |
| 4 | W4.A3 | M | A | The race: Group A's measurements |
| 4 | W4.B1 | M | B | Timers: close idle connections on time |
| 4 | W4.B2 | S | B | Graceful shutdown with signalfd |
| 4 | W4.B3 | M | B | The race: Group B's measurements |
| 4 | W4.C | S | A, B | Write the comparison, and present it |

## Weeks 5-8

Written at the end of week 4, after the race: what we learn there decides the
final server's design. The contract between the groups is already fixed in
`engine/include/qfis/engine.hpp`, and a stub engine already builds.

## Creating them on GitHub

```bash
scripts/setup_labels.sh              # once
scripts/create_issues.sh --dry-run   # check
scripts/create_issues.sh             # create (titles that exist are skipped)
```
