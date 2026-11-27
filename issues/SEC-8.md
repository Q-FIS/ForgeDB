# [SEC-8] Memory limits: refuse gracefully instead of being killed
labels: comp:sec, size:M
milestone: v2 Stretch

**Component:** SEC (touches the Engine and the API: POOL and APP review) · **Size:** M · **Needs:** SEC-4, APP-6
**Your code:** `src/sec/`, small changes in `src/pool/engine.cpp` and `src/app/` · **Tests:** `tests/blackbox/security_test.py`

## What is this, in plain words?
Keep adding chunks and ForgeDB uses more and more memory, until the kernel's
**OOM killer** ends it with `SIGKILL`, mid-request, without a word. A good
server knows its budget and refuses new data politely before that happens.
This issue is about how Linux really accounts for memory.

## What you must give back
1. `--memory-limit SIZE` (e.g. `2G`): when the process's memory use (resident set, from `/proc/self/statm`, or the cgroup's `memory.current` when running in a cgroup) is above 90 % of the limit, puts get **507 Insufficient Storage**; searches still work.
2. `/metrics` shows memory used and the limit. `reason_phrase` (HTTP reviews) learns 507 "Insufficient Storage", and `docs/API.md` lists it.
3. An experiment, written in the PR: run `forgedb` inside a cgroup with a hard limit (`systemd-run --user --scope -p MemoryMax=500M ./build/release/forgedb ...`), and load chunks until the end, once with `--memory-limit` and once without. What happens in each case? What did `dmesg` say the second time?
4. A test: with a small `--memory-limit`, loading keeps going until 507s appear, and the server stays up and keeps answering searches.

## Watch out for
- RSS is not "the memory my data needs": the allocator keeps freed memory, and the page cache isn't counted. Explain which number you used and why.
- Check memory cheaply: not on every put (reading `/proc` is a system call); every 100 puts, or once a second, is enough.

## Done when
- [ ] Puts are refused with 507 near the limit; the test passes; the experiment is in the PR

## Learn first
- The kernel docs on cgroup v2, the "Memory" section (`memory.current`, `memory.max`), and what the OOM killer does.
- OSTEP chapter 22 ("Swapping: Policies"), for what happens when memory runs out.
