# How the build works

**New? Read [`START_HERE.md`](START_HERE.md) first**: it walks you through one
issue, start to finish. [`REFERENCE.md`](REFERENCE.md) has every command, the
folder layout, and the words we use.

## One product, ten components, ten owners

We build one thing together: **ForgeDB** (see the root `README.md`). It's
split into ten components. Each member **owns** one, for the whole project:

- you **build** it: its issues are yours, in order (`WAL-1`, then `WAL-2`...);
- you **review** every change to its files (GitHub asks you automatically, through CODEOWNERS);
- you **explain** it: at a weekly meeting, 5 minutes on how your component works and what you learned.

| Component | What it is | Partner |
| --- | --- | --- |
| **CONN** | Connections: sockets, the server loop, keep-alive, epoll | HTTP |
| **HTTP** | The protocol: parser, responses, router, limits, fuzzing | CONN |
| **POOL** | Concurrency: queue, thread pool, the Engine and its locks, `parallel_for` | APP |
| **APP** | The `forgedb` program: config, JSON, the API, signals, metrics | POOL |
| **WAL** | Durability: the write-ahead log, group commit, recovery, crash tests | SNAP |
| **SNAP** | Storage: the store, the file format, snapshots, checkpoints, mmap | WAL |
| **VEC** | Vector search: cosine top-k, cache-friendly, SIMD, all cores | KW |
| **KW** | Keyword search: tokenizer, inverted index, BM25, hybrid | VEC |
| **CLI** | Clients: the `forge` command, the Python client, the ML wing's RAG | LOAD |
| **LOAD** | Testing and measurement: API tests, load generator, chaos | CLI |

Your **partner** is your default reviewer, and the first person to ask when
you're stuck. Partners work on neighbouring parts, so reviewing each other
teaches you both.

## Everyone starts on day 1

Every component's first issue needs nothing from anyone else. The contracts
(`include/forge/`, `docs/API.md`, `docs/FORMAT.md`) are already written, the
tests use stand-ins (`tests/support/fakes.hpp`), and the clients use a fake
server (`tools/fake_server.py`). Work down your list in order. An issue says
**Needs: X-N** only when it really can't be finished without X-N; until then,
start the parts you can, or go on to your next issue.

## Milestones, not deadlines

| Milestone | Means | Done when |
| --- | --- | --- |
| **v0.1 First search** | Put chunks in over HTTP, search them, in memory | `blackbox.smoke` passes against `forgedb` |
| **v0.2 Survives a crash** | Log, recovery, snapshots, BM25, limits, metrics | `blackbox.crash` passes |
| **v1.0 Fast, and powers the RAG** | epoll, SIMD, all cores, hybrid search, the ML wing on ForgeDB, the experiments | the final demo |

GitHub shows each milestone's progress bar. When one fills up, the next
weekly meeting is its demo.

## How a piece of work flows

1. Take the next issue on your list (it's already assigned to you).
2. Branch, code in your component's folder, test, open a PR with `Closes #N`: see [`START_HERE.md`](START_HERE.md).
3. CI must be green. Your partner reviews (and the owner of any other files you touched).
4. Merge, and tell the channel what's now possible ("Socket is merged: CLI and LOAD, you can use it").

**Changing a contract.** The headers in `include/forge/` are shared promises.
You may add **private** members to your own component's classes. Anything
else (a new public function, a changed signature) needs the lead's OK in the
issue first, because other people code against it.

**Stuck for an hour?** Ask your partner or the channel: what you tried, and the exact error.
**Found a bug in someone else's component?** Open an issue with the component label: its owner fixes it.

## Meetings

- **Weekly meeting (60 min, everyone).** Each person: 1 minute on what they merged and what's next. Then two component talks (5 min each, rotating, so everyone talks about once every five weeks). Then a demo if a milestone filled up, or a lab show-and-tell.
- **The channel**, the rest of the time. No stand-ups.

## Labs are separate

The labs (`labs/`, one a week for 8 weeks) are done alone, in your own repo.
They aren't issues, nobody reviews them, and they cover everything: whatever
you own in ForgeDB, the labs make sure you also know the rest of OS and networking.

## The 60 issues

Sizes: **S** ≈ 1 day, **M** ≈ 2-3 days. "Needs" lists the issues that must be merged first.

| Issue | Title | Size | Milestone | Needs |
| --- | --- | --- | --- | --- |
| [CONN-1](CONN-1.md) | A Socket class that closes itself | M | v0.1 | **day 1** |
| [CONN-2](CONN-2.md) | The server: connections in, requests to the handler, responses out | M | v0.1 | CONN-1, HTTP-1, HTTP-2 |
| [CONN-3](CONN-3.md) | Keep-alive done right: limits, idle connections, quick shutdown | M | v0.2 | CONN-2 |
| [CONN-4](CONN-4.md) | Non-blocking sockets and an Epoll class | M | v1.0 | CONN-1 |
| [CONN-5](CONN-5.md) | The event loop: `--io epoll` | M | v1.0 | CONN-3, CONN-4 |
| [CONN-6](CONN-6.md) | Experiment: threads vs epoll, at 10, 100 and 1,000 connections | S | v1.0 | CONN-5, LOAD-2 |
| [HTTP-1](HTTP-1.md) | Parse an HTTP request | M | v0.1 | **day 1** |
| [HTTP-2](HTTP-2.md) | Build HTTP responses | S | v0.1 | **day 1** |
| [HTTP-3](HTTP-3.md) | The router: which handler gets the request? | S | v0.1 | HTTP-2 |
| [HTTP-4](HTTP-4.md) | Limits and awkward requests: 413, 431, 501 | S | v0.2 | HTTP-1 |
| [HTTP-5](HTTP-5.md) | Fuzz the parser with libFuzzer | M | v0.2 | HTTP-1, HTTP-4 |
| [HTTP-6](HTTP-6.md) | Experiment: how fast is our parser, and can we make it faster? | S | v1.0 | HTTP-4 |
| [POOL-1](POOL-1.md) | A thread-safe queue with a size limit: `BoundedQueue<T>` | M | v0.1 | **day 1** |
| [POOL-2](POOL-2.md) | The thread pool | M | v0.1 | POOL-1 |
| [POOL-3](POOL-3.md) | The Engine: the store and both indexes, safe for many threads | M | v0.1 | **day 1** |
| [POOL-4](POOL-4.md) | A tougher pool: failing tasks, statistics, and a stress test | S | v0.2 | POOL-2 |
| [POOL-5](POOL-5.md) | `parallel_for`: split a loop across the pool | M | v1.0 | POOL-2 |
| [POOL-6](POOL-6.md) | Experiment: how many threads? | S | v1.0 | VEC-5, LOAD-2 |
| [APP-1](APP-1.md) | The command line: `forgedb --dim 384 --port 8080 ...` | S | v0.1 | **day 1** |
| [APP-2](APP-2.md) | The API's JSON: chunks, searches, hits | M | v0.1 | **day 1** |
| [APP-3](APP-3.md) | The API: every endpoint, every error code | M | v0.1 | HTTP-3, APP-2, POOL-3 |
| [APP-4](APP-4.md) | Switch on `forgedb`: everything joined up | M | v0.1 | APP-1, APP-3, CONN-2, POOL-2, SNAP-1, VEC-1, KW-2 |
| [APP-5](APP-5.md) | Graceful shutdown: finish what you started, then exit 0 | M | v0.2 | APP-4, CONN-3 |
| [APP-6](APP-6.md) | `/metrics`: what is the server doing? | M | v0.2 | APP-4, POOL-4 |
| [WAL-1](WAL-1.md) | The write-ahead log: append records safely | M | v0.2 | **day 1** |
| [WAL-2](WAL-2.md) | Read the log back, and spot a damaged tail | S | v0.2 | WAL-1 |
| [WAL-3](WAL-3.md) | Every put goes through the log, and `--fsync` decides when it's on disk | M | v0.2 | WAL-1, SNAP-2, POOL-3, APP-4 |
| [WAL-4](WAL-4.md) | Recovery: rebuild everything from the log at startup | M | v0.2 | WAL-2, WAL-3 |
| [WAL-5](WAL-5.md) | The crash test: `kill -9` mid-write, lose nothing acknowledged | M | v0.2 | WAL-4 |
| [WAL-6](WAL-6.md) | Experiment: fsync policies, speed vs safety | S | v1.0 | WAL-3, LOAD-2 |
| [SNAP-1](SNAP-1.md) | The in-memory store | S | v0.1 | **day 1** |
| [SNAP-2](SNAP-2.md) | A chunk as bytes, and back | M | v0.2 | nothing |
| [SNAP-3](SNAP-3.md) | Snapshots: save the whole store, atomically | M | v0.2 | SNAP-1, SNAP-2, WAL-1 |
| [SNAP-4](SNAP-4.md) | Checkpoints: snapshot, then start a fresh log | M | v0.2 | SNAP-3, WAL-4 |
| [SNAP-5](SNAP-5.md) | Load snapshots with `mmap` | M | v1.0 | SNAP-3 |
| [SNAP-6](SNAP-6.md) | Experiment: `read` vs `mmap` | S | v1.0 | SNAP-5 |
| [VEC-1](VEC-1.md) | Vector search: find the nearest vectors | M | v0.1 | **day 1** |
| [VEC-2](VEC-2.md) | Measure the flat index | S | v0.2 | VEC-1 |
| [VEC-3](VEC-3.md) | Cache-friendly search | M | v1.0 | VEC-2 |
| [VEC-4](VEC-4.md) | SIMD: 8 multiplications per instruction | M | v1.0 | VEC-2 |
| [VEC-5](VEC-5.md) | Search on all cores | S | v1.0 | POOL-5, VEC-1 |
| [VEC-6](VEC-6.md) | Experiment: where does a search spend its time? | S | v1.0 | VEC-3, VEC-4, VEC-5 |
| [KW-1](KW-1.md) | The tokenizer: text into words | S | v0.1 | **day 1** |
| [KW-2](KW-2.md) | The inverted index: find chunks by their words | M | v0.1 | KW-1 |
| [KW-3](KW-3.md) | BM25: score words the way real search engines do | M | v0.2 | KW-2 |
| [KW-4](KW-4.md) | Compact postings: less memory, faster search | M | v1.0 | KW-3 |
| [KW-5](KW-5.md) | Hybrid search: merge keyword and vector results | M | v1.0 | KW-2, VEC-1, POOL-3 |
| [KW-6](KW-6.md) | Experiment: keyword vs vector vs hybrid, on real questions | M | v1.0 | KW-5, CLI-4 |
| [CLI-1](CLI-1.md) | The `forge` command and a C++ client | M | v0.1 | CONN-1 |
| [CLI-2](CLI-2.md) | `forge load`: load thousands of chunks from a file | S | v0.1 | CLI-1 |
| [CLI-3](CLI-3.md) | A Python client, for the ML wing | M | v0.2 | nothing |
| [CLI-4](CLI-4.md) | Bring the ML wing's data into ForgeDB | S | v0.2 | CLI-2 |
| [CLI-5](CLI-5.md) | The RAG uses ForgeDB | M | v1.0 | CLI-3, CLI-4, APP-4, WAL-4 |
| [CLI-6](CLI-6.md) | The demo: ForgeDB in five minutes | S | v1.0 | APP-5, WAL-4, CLI-2 |
| [LOAD-1](LOAD-1.md) | The full API test suite | M | v0.1 | **day 1** |
| [LOAD-2](LOAD-2.md) | `forge-load`: a load generator | M | v0.2 | CONN-1 |
| [LOAD-3](LOAD-3.md) | Open-loop load, and the coordinated omission trap | M | v1.0 | LOAD-2 |
| [LOAD-4](LOAD-4.md) | The concurrency test: many clients at once, nothing lost | M | v0.2 | APP-4 |
| [LOAD-5](LOAD-5.md) | The chaos test: clients that misbehave | M | v1.0 | APP-4, HTTP-4, CONN-3 |
| [LOAD-6](LOAD-6.md) | The final numbers | S | v1.0 | LOAD-3 |

## For the lead: putting this on GitHub

```bash
scripts/setup_github.sh                # labels and milestones (once)
# fill in scripts/owners.txt with each component's GitHub username, then:
scripts/create_issues.sh --dry-run     # check
scripts/create_issues.sh               # creates the 60 issues, assigned to their owners
scripts/make_codeowners.sh             # writes .github/CODEOWNERS; commit it
```
Then, in the repo's settings, protect `main`: require a pull request, 1
approval, review from code owners, and the CI checks.
