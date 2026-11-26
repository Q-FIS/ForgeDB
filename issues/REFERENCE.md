# Reference: how everything works

You don't need to read this all at once. Start with `START_HERE.md`, and come
back here when you need something: the repo layout, a command, how the tests
work, or a word you don't know.

---

## 1. What are we building?

**ForgeDB**, a small search database, in C++20, from raw system calls up. It
stores text chunks with their embeddings and searches them by vector, by
keyword, or both; it speaks HTTP/JSON; it survives crashes; it serves many
clients at once; and the ML wing's RAG system uses it. The root `README.md`
has the picture; `issues/README.md` has the ten components and who owns what.

**Labs are separate.** You do them alone, in your **own** GitHub repo, from the
handouts in `labs/`. They aren't issues in this repo.

---

## 2. How the repo works

```
include/forge/      THE CONTRACTS: the headers every component codes against (the lead owns them)
src/<component>/    each component's code: conn, http, pool, app, wal, snap, vec, kw, cli, load
apps/               the programs: forgedb.cpp (server), forge.cpp (client), forge_load.cpp
tests/contract/     tests that define each contract, written before the code
tests/unit/         your own tests, one folder per component
tests/blackbox/     Python tests that start forgedb and talk to it over real sockets
tests/support/      stand-ins for tests (fakes.hpp) and a raw HTTP client
tools/              fake_server.py (the whole API in Python), and your scripts
bench/              benchmark programs (each .cpp becomes bench_<name>)
fuzz/               fuzz targets (HTTP-5)
docs/               API.md, FORMAT.md, experiments/
clients/python/     the Python client (CLI-3)
issues/             every task, written out
labs/               lab handouts and their tests (for your own repo)
```

**You never edit CMake files to add code.** Any `.cpp` under `src/` is part of
the library; any `.cpp` in `tests/unit/<component>/` is a test; any `.cpp` in
`bench/` is a benchmark.

**Stubs and contract tests.** Each unfinished part has a placeholder
`src/<component>/stub_<part>.cpp`, and contract tests
`tests/contract/<component>/<part>_test.cpp`. While the stub exists, ctest shows
`contract.<component>.<part>` as **Skipped**. Delete the stub when you start:
the tests switch on, and must pass before you merge.

**Placeholder programs.** `forgedb`, `forge` and `forge-load` start as
placeholders that exit with code 77; the black-box tests report that as
**Skipped**. APP-4, CLI-1 and LOAD-2 replace them.

**The fake server.** `python3 tools/fake_server.py --port 8080 --dim 4` answers
the whole API of `docs/API.md`. Use it to try clients and tests before
`forgedb` works, and to check what the right answer is.

---

## 3. The commands you'll use every day

```bash
# build and test (debug = AddressSanitizer + UBSan: use it every day)
cmake --preset debug                 # once (it re-runs by itself when files are added)
cmake --build --preset debug -j      # compile
ctest --preset debug                 # run every test
ctest --preset debug -R contract.wal --output-on-failure   # only some tests, with details

# anything with threads: also the ThreadSanitizer build
cmake --preset tsan && cmake --build --preset tsan -j && ctest --preset tsan

# measurements and benchmarks: always the release build
cmake --preset release && cmake --build --preset release -j

# the server and the client, by hand
./build/debug/forgedb --dim 4 --port 8080
./build/debug/forge health
curl -v http://localhost:8080/health

# watch the system calls of a running server
strace -f -p $(pgrep forgedb) -e trace=network
```

**A piece of work:**
```bash
git switch main && git pull
git switch -c wal-2-reader               # <issue>-<short name>
# ... write code and tests, build, test ...
git add -A && git commit -m "WAL-2: read the log, stop at a damaged tail"
git push -u origin wal-2-reader
# open a PR on GitHub with "Closes #<issue number>"
```
A PR merges with **green CI + one approval** (your partner, or the owner of the files you changed).

---

## 4. The contracts

| What | Where |
| --- | --- |
| Shared types (`Chunk`, `SearchRequest`, `SearchHit`, errors) | `include/forge/types.hpp` |
| Each component's functions and classes | `include/forge/<name>.hpp` (each says which component and issue) |
| The HTTP API: endpoints, fields, status codes | `docs/API.md` |
| The files on disk: log records, chunks, snapshots | `docs/FORMAT.md` |

You may add **private** members and helpers to your own component's classes.
Anything public needs the lead's OK first, in the issue: other people code against it.

---

## 5. Words we use

| Word | Meaning |
| --- | --- |
| **System call** | A request from your program to the kernel: `read`, `write`, `socket`, `fsync`... It returns -1 and sets `errno` on failure. |
| **File descriptor (fd)** | A small integer the kernel gives you for an open file or socket. You must `close` it, exactly once. |
| **RAII** | An object acquires something in its constructor and releases it in its destructor, so it can't leak. |
| **Move-only** | A type you can move but not copy (like `std::unique_ptr`, `Socket`, `WalWriter`): two copies would both release the same thing. |
| **Contract** | A header (or doc) that says exactly what a component promises. Others code against it. |
| **Stub** | A placeholder file. Deleting it switches on that part's tests. |
| **Embedding / vector** | A list of numbers representing a text's meaning. Similar texts have vectors pointing the same way. |
| **Cosine similarity** | How closely two vectors point the same way: 1 = same, 0 = unrelated, -1 = opposite. |
| **Inverted index** | For each word, the list of chunks that contain it. |
| **BM25** | The standard keyword relevance score: rare words count more; repeats and long chunks count less. |
| **Hybrid / RRF** | Running vector and keyword search and merging the two rankings (Reciprocal Rank Fusion). |
| **TCP is a byte stream** | TCP delivers bytes in order, but **not** in the pieces you sent. Keep reading until you have a whole message. |
| **Keep-alive / pipelining** | Several HTTP requests on one connection, one after another / sent all at once. |
| **Backpressure** | When overloaded, refuse new work quickly (503) instead of queueing forever. |
| **epoll** | A Linux system call that tells one thread which of thousands of sockets are ready. |
| **Write-ahead log (WAL)** | A file where every change is appended before it's applied, so it can be replayed after a crash. |
| **fsync / durable** | `write` puts data in the kernel's memory; `fsync` waits until it's on the disk itself. Durable = it survives a power cut. |
| **Group commit** | One `fsync` for many writes at once, instead of one each. |
| **Snapshot / checkpoint** | A file with the whole database; writing one lets the log start over. |
| **mmap** | Mapping a file into memory, so its bytes appear in your address space without copying. |
| **SIMD** | One instruction working on several numbers at once (AVX2: 8 floats). |
| **p50 / p99** | Latency percentiles: half the requests were faster than p50, 99% faster than p99. |
| **Sanitizers** | Compiler options that add checks while the program runs: **ASan** (memory bugs), **UBSan** (undefined behaviour), **TSan** (data races). |
| **Fuzzing** | Feeding a program millions of generated inputs to find crashes. |

---

## 6. Rules (the whole list)

1. **Write your own code.** AI may explain, review and help read errors. It doesn't write your code. If you can't explain a line, it doesn't merge.
2. **Check every system call** with `check(...)` (in `forge/check.hpp`): it turns a -1 into an exception that says what failed.
3. **Tests first, then measure.** Tests pass on `debug` (and `tsan` for threads), then measure on `release`.
4. **Work in your component's files.** Changing someone else's? Their review is required (GitHub asks them).
5. **Contracts change only with the lead's OK.**
6. **Stuck for an hour?** Ask your partner or the channel: what you tried, and the exact error.
