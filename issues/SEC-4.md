# [SEC-4] Lock it down: no root, resource limits, and a seccomp sandbox
labels: comp:sec, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** SEC (changes `apps/forgedb.cpp`: APP reviews) · **Size:** M (3-4 days) · **Needs:** APP-5, WAL-4
**Your code:** `src/sec/sandbox.cpp`, `include/forge/security.hpp` (additions, agreed in this issue) · **Tests:** `tests/unit/sec/`, `tests/blackbox/security_test.py`

## What is this, in plain words?
If an attacker ever finds a bug in ForgeDB (say, in the parser), what can
they do with it? Today: anything the process can do, which is a lot. Linux
lets a program **give up powers it doesn't need**, so a bug can't become a
disaster. This is how browsers, Docker and OpenSSH protect themselves, and
it's a tour of the kernel's security features.

## What you must give back
1. **Never run as root:** if `geteuid() == 0` and `--user NAME` is given, drop to that user after binding the port (`setgroups`, `setgid`, then `setuid`, in that order, each checked). If root and no `--user`: refuse to start.
2. **Resource limits** (`setrlimit`): raise `RLIMIT_NOFILE` to its hard limit (for many connections), set `RLIMIT_CORE` to 0 (no core files full of data).
3. **`prctl(PR_SET_NO_NEW_PRIVS, 1)`**, then a **seccomp-bpf filter** that allows only the system calls ForgeDB really uses and kills the process on anything else. Find the list with `strace -f -c` under the full black-box test suite. Use libseccomp (`seccomp_init`, `seccomp_rule_add`, `seccomp_load`) and add it like OpenSSL in SEC-3.
4. `--no-sandbox` to switch the filter off (for debugging).
5. Tests: a unit test that forks a child, installs a filter without `execve`, and checks that the child dies (`SIGSYS`) when it calls `execve`; and the whole black-box suite still passes with the sandbox on.
6. In the PR: your allowed list, with one line on why each call is needed.

## Watch out for
- The sandbox goes on **after** everything that needs extra calls (binding, opening the data dir, starting threads) but before serving the first request.
- Sanitizer builds make extra system calls (ASan, TSan): you may need a slightly bigger list there. Explain the difference.
- A missing call shows up as a sudden death by `SIGSYS`. `dmesg`, or `strace`, tells you which one.

## Done when
- [ ] The sandbox is on by default; every black-box test passes with it; your unit tests pass

## Learn first
- `man 2 seccomp` (the overview), and the libseccomp examples.
- `man 7 capabilities`, the first section: why "root" is really many separate powers.
