# Start here: read this before your first issue

About 15 minutes. It explains what we're building, how the repo works, what
each program must do, and the words we use.

---

## 1. What are we building?

**Weeks 1-4:** each group builds its own **HTTP server** in C++20, from raw
sockets up: no web framework. Same features, two designs:

- **Group A: a thread pool.** One thread accepts connections; a fixed number of worker threads handle them.
- **Group B: an event loop.** One thread watches every connection with `epoll` and handles whichever is ready.

In week 4 we race them under the same load and write down what we learn. It
turns out each design fails in a different way, and that's the lesson.

**Weeks 5-8:** we build `qfis-searchd`, a small search service. Group A builds
the engine (storage and search), Group B the service (server, API, model
client). Their contract is already written: `engine/include/qfis/engine.hpp`.
The issues for weeks 5-8 are written after the week-4 race, because what we
learn there changes them.

**Labs are separate.** You do them alone, in your **own** GitHub repo, from the
handouts in `labs/`. They aren't issues in this repo.

---

## 2. How the repo works

```
group-a/            Group A's code. Group B never edits it (and vice versa).
  src/              your shared code: Socket class, HTTP parser, ... (every .cpp here is built automatically)
  apps/             the programs: echo_server.cpp, echo_client.cpp, http_server.cpp
  tests/            your GoogleTest unit tests (every .cpp here is built automatically)
group-b/            the same, for Group B
engine/             weeks 5-8: the shared header and the stub engine
tests/              black-box tests: Python scripts that start YOUR programs and check them over real sockets
issues/             every task, written out
labs/               lab handouts and their tests (for your own repo)
```

**You never edit CMake files to add code:** put a `.cpp` in `src/` or `tests/`, and it's built.

**The placeholders:** every program in `apps/` starts as a placeholder that
exits with code 77. The black-box tests report it as **SKIPPED**. When you
write the real program, the test starts running, and must pass.

---

## 3. The commands you'll use every day

```bash
# once: install the tools (Ubuntu 24.04)
sudo apt install build-essential cmake gdb strace python3 wrk wireshark linux-tools-generic

# build and test (debug = AddressSanitizer + UBSan: use it every day)
cmake --preset debug                 # only needed once, or after adding a new file
cmake --build --preset debug -j      # compile
ctest --preset debug                 # run every test

# anything with threads: also run the ThreadSanitizer build
cmake --preset tsan && cmake --build --preset tsan -j && ctest --preset tsan

# measurements and load tests: always on the release build
cmake --preset release && cmake --build --preset release -j

# run one test, with its full output
ctest --preset debug -R a_http --output-on-failure

# run your server by hand and talk to it
./build/debug/group-a/a_http_server --port 8080 --root tests/www
curl -v http://localhost:8080/health
```

**A day's work:**
```bash
git switch main && git pull
git switch -c a/W2.1-request-parser       # <group>/<issue>-<short name>
# ... write code and tests, build, test ...
git add -A && git commit -m "W2.1: parse the request line and headers"
git push -u origin a/W2.1-request-parser
# open a PR on GitHub with "Closes #<issue number>"
```
A PR merges with **green CI + one approval from someone in your group**. That's all.

---

## 4. The contract: what our programs must do

The black-box tests check exactly this. Both groups follow it.

**echo_server** `--port <N>`
Accepts TCP connections on 127.0.0.1:N and sends back every byte it receives,
until the client closes. It must survive clients that disappear mid-message.

**echo_client** `--host <addr> --port <N> --file <path>`
Sends the file to the echo server, reads the echo, and exits with code **0** if
the echo matches the file, **1** if not.

**http_server** `--port <N> --root <dir>` (plus, from week 3: `--workers <N> --queue <N>` for Group A, and `--idle-timeout-ms <N>` for both in week 4)

| Request | Response |
| --- | --- |
| `GET /health` | `200`, `Content-Type: application/json`, body `{"status":"ok"}` |
| `POST /echo` | `200`, with the request body sent back |
| `GET /static/<file>` | `200` with the file from `--root`. Never a file outside it. |
| `GET /sleep?ms=<N>` (from week 3) | waits N milliseconds, then `200` |
| A malformed request | `400 Bad Request` |
| An unknown path | `404 Not Found` |
| A known path with the wrong method | `405 Method Not Allowed` |
| A body over 1 MiB (`Content-Length` > 1048576) | `413 Content Too Large` |
| Headers over 8 KiB | `431 Request Header Fields Too Large` |
| No free worker and a full queue (Group A) | `503 Service Unavailable` |

Every response has a `Content-Length` header. Connections stay open
(keep-alive) unless the request says `Connection: close`. On `SIGTERM`, the
server finishes the requests in progress and exits with code 0.

---

## 5. Words we use

| Word | Meaning |
| --- | --- |
| **System call** | A request from your program to the kernel: `read`, `write`, `socket`, `accept`... It returns -1 and sets `errno` on failure. |
| **File descriptor (fd)** | A small integer the kernel gives you for an open file or socket. You must `close` it, exactly once. |
| **RAII** | A C++ idea: an object acquires something in its constructor and releases it in its destructor. A `Socket` class that closes its fd when it goes out of scope can never leak it. |
| **Move-only** | A type you can move but not copy (like `std::unique_ptr`). A `Socket` must be move-only: two copies would both close the same fd. |
| **TCP is a byte stream** | TCP delivers bytes in order, but **not** in the chunks you sent. One `send` of 100 bytes can arrive as 3 `recv`s, or two sends as one. You must keep reading until you have a full message. |
| **Partial write** | `write`/`send` can send fewer bytes than you asked. You must loop until everything is sent. |
| **SIGPIPE** | The signal the kernel sends when you write to a connection the other side has closed. By default it **kills your program**. Ignore it, or use `MSG_NOSIGNAL`. |
| **Keep-alive** | Several HTTP requests on one TCP connection, one after another. |
| **Backpressure** | When overloaded, refuse new work quickly (503) instead of queueing forever. |
| **Non-blocking I/O** | A socket where `read`/`write` return immediately (with `EAGAIN`) instead of waiting. Needed by the event loop. |
| **epoll** | A Linux system call that tells you which of thousands of sockets are ready to read or write. |
| **p50 / p99** | Latency percentiles: half of the requests were faster than p50; 99% faster than p99. p99 shows the slow tail. |
| **Sanitizers** | Compiler options that add checks while your program runs. **ASan** catches memory bugs (buffer overflows, use-after-free), **UBSan** undefined behaviour, **TSan** data races between threads. |

---

## 6. Rules (the whole list)

1. **Write your own code.** AI may explain, review and help read errors. It doesn't write your code. If you can't explain a line, it doesn't merge.
2. **Check every system call** with `check(...)` (in `src/check.hpp`): it turns a -1 into an exception that says what failed.
3. **Tests first, then measure.** Make the tests pass on the debug build, then measure on the release build.
4. **Stay in your group's folder.** Need something from the other group? Talk to them; don't edit their code.
5. **Stuck for an hour?** Ask in your group's channel: what you tried, and the exact error.
