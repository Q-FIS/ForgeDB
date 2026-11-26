# finforge-coreCS

The Core CS wing's 8-week project: **modern C and C++, operating systems and
computer networks**, learned by building real things and measuring them.

- **Builds** (this repo): in groups. Weeks 1-4: each group builds an HTTP server
  from raw sockets, Group A with a thread pool, Group B with an event loop, and
  we race them. Weeks 5-8: together we build `qfis-searchd`, a small search service.
- **Labs** (your own GitHub repo): alone, one a week: a shell, threads, a chat
  server, `malloc`, CPU caches, DNS, IPC. Handouts and tests are in `labs/`.

**New here? Read [`issues/START_HERE.md`](issues/START_HERE.md) first**, then [`labs/README.md`](labs/README.md).

## Setup (once, on Ubuntu 24.04)

```bash
sudo apt install build-essential cmake gdb strace python3 wrk wireshark linux-tools-generic
git clone <this repo> && cd finforge-coreCS
cmake --preset debug                  # first time: downloads GoogleTest
cmake --build --preset debug -j
ctest --preset debug                  # everything passes or is SKIPPED (not written yet)
```

## Build modes

| Preset | What it's for |
| --- | --- |
| `debug` | Every day. AddressSanitizer + UndefinedBehaviorSanitizer catch memory bugs as they happen. |
| `tsan` | Anything with threads. ThreadSanitizer catches data races. |
| `release` | Measurements and load tests only. Optimised, no sanitizers. |

## Where things are

| Path | What |
| --- | --- |
| `group-a/`, `group-b/` | Each group's code: `src/` (shared code), `apps/` (programs), `tests/` (unit tests) |
| `engine/` | Weeks 5-8: the contract `include/qfis/engine.hpp` and a stub engine |
| `tests/` | Black-box tests (Python) that check both groups' programs over real sockets |
| `issues/` | Every build task, written out step by step |
| `labs/` | Lab handouts, starter files and tests, for your own labs repo |
| `docs/` | Notes, measurements and the week-4 server comparison |
| `DECISIONS.md` | Why things are the way they are |

