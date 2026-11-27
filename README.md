# ForgeDB

**A small search database, built from scratch in C++20 by the Q-FIS Core CS wing.**

You store text chunks with their embeddings, and search them by meaning
(vectors), by words (BM25), or both (hybrid). It speaks HTTP/JSON, keeps every
acknowledged write safe on disk through crashes, serves many clients at once,
and powers the ML wing's question-answering system.

Every layer is built by hand, from raw system calls up: sockets, an HTTP
parser, a thread pool, an epoll event loop, a write-ahead log, snapshots, a
vector index and an inverted index. That's the point: building it is how we
learn **modern C++, operating systems and computer networks**.

```
      forge CLI · Python client · the ML wing's RAG                    CLI
                        │ HTTP/JSON
   ┌────────────────────▼─────────────────────────────────────────┐
   │ connections: sockets, epoll                                   │  CONN
   │ HTTP: parser, responses, router                               │  HTTP
   ├───────────────────────────────────────────────────────────────┤
   │ forgedb: config, API, signals, metrics                        │  APP
   │ thread pool · the Engine (locks, store + indexes in step)     │  POOL
   ├──────────────────────────────┬────────────────────────────────┤
   │ write-ahead log, recovery    │ vector search (cosine, SIMD)   │  WAL · VEC
   │ store, snapshots, mmap       │ keyword search (BM25), hybrid  │  SNAP · KW
   └──────────────────────────────┴────────────────────────────────┘
      load generator, crash and chaos tests, the numbers               LOAD
      rate limits, API keys, TLS, the sandbox, the threat model        SEC
```

Eleven components, eleven owners. See [`issues/README.md`](issues/README.md).

## New here?
1. [`issues/START_HERE.md`](issues/START_HERE.md): your first issue, step by step.
2. [`issues/README.md`](issues/README.md): how the build works, and every issue.
3. [`labs/README.md`](labs/README.md): the weekly labs, done alone in your own repo.

## Setup (once, on Ubuntu 24.04 or WSL2)

```bash
sudo apt install build-essential cmake gdb strace python3 netcat-openbsd linux-tools-generic
git clone https://github.com/Q-FIS/Finforge-CoreCS.git ~/Finforge-CoreCS && cd ~/Finforge-CoreCS
cmake --preset debug               # the first time, downloads GoogleTest and nlohmann/json
cmake --build --preset debug -j
ctest --preset debug               # everything passes or is "Skipped" (not built yet)
```

## Try it (once v0.1 is done)

```bash
./build/debug/forgedb --dim 3 &
./build/debug/forge put --id a:1 --doc report --text "net profit rose" --vector 0.1,0.9,0
./build/debug/forge search --text "profit"
```
Before that, the fake server answers the same API: `python3 tools/fake_server.py --dim 3`.

## Where things are

| Path | What |
| --- | --- |
| `include/forge/` | **The contracts**: the headers every component codes against |
| `src/<component>/` | Each component's code |
| `apps/` | The programs: `forgedb` (server), `forge` (client), `forge-load` (load generator) |
| `tests/contract/` | Tests that define each contract (written before the code) |
| `tests/unit/<component>/` | Your own tests |
| `tests/blackbox/` | Python tests that talk to the running server over real sockets |
| `tools/fake_server.py` | The whole API in Python, so clients and tests work from day 1 |
| `docs/` | The API, the file formats, experiment reports |
| `issues/` | Every task, written out step by step |
| `labs/` | Lab handouts, starter files and tests |

## Build modes

| Preset | What it's for |
| --- | --- |
| `debug` | Every day. AddressSanitizer + UndefinedBehaviorSanitizer catch memory bugs as they happen |
| `tsan` | Anything with threads. ThreadSanitizer catches data races |
| `release` | Measurements and benchmarks only |
| `fuzz` | libFuzzer (HTTP-5; needs clang) |

