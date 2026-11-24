# L8 · Processes talking to each other
**Week 8 · C++ · about 3 hours (a light lab: week 8 is integration week)** · *The test arrives before week 8.*

## What is this, in plain words?
Two processes don't share memory, so to cooperate they must pass data through
the kernel (a pipe or a socket), or ask the kernel to share some memory
between them. You'll send 1 GB three ways, and measure what each way costs.

## Build
`ipc_bench METHOD MSG_SIZE`: the parent `fork`s a child; the parent sends 1 GB in messages of MSG_SIZE bytes, the child receives them and computes a checksum, and both print it.
- `pipe`: a `pipe()`.
- `unix`: a `socketpair(AF_UNIX, SOCK_STREAM, 0)`.
- `shm`: POSIX shared memory (`shm_open` + `mmap`) used as a ring buffer, with two semaphores (`sem_init` in the shared memory, with `pshared = 1`) for "slots full" and "slots empty".

## Measure
Throughput (GB/s) for MSG_SIZE = 64 B, 4 KB, 64 KB and 1 MB, for each method. Why does shared memory win for big messages? How many times is each byte copied in each method? What did shared memory cost you in complexity?

## Done when
- [ ] The checksums match for every method and size
- [ ] `NOTES.md`: the throughput table and your answers

## Extra
Add TCP over loopback and an `mmap`'d file as methods 4 and 5.

## Read
- *The Linux Programming Interface*, chapters 43 (IPC overview) and 54 (POSIX shared memory).
