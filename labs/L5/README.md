# L5 · Your own malloc
**Week 5 · C · about 5 hours** · *The test arrives before week 5.*

## What is this, in plain words?
`malloc` isn't magic: it's ordinary C code that asks the kernel for big blocks
of memory (`mmap`, or `brk`), and then hands out small pieces of them,
remembering which pieces are free. Writing one shows you how memory really
looks from inside a process.

## Build
In `my_malloc.c`, with the header `my_malloc.h`:
- `void *my_malloc(size_t size)`, `void my_free(void *ptr)`, `void *my_realloc(void *ptr, size_t size)`.
- Each block has a small **header** before it (its size, and whether it's free). Keep the free blocks in a **free list**.
- `my_malloc`: find a free block that's big enough (**first-fit**: the first one that fits). If it's much bigger than needed, **split** it. If none fits, get more memory from the kernel with `mmap` (at least 64 KB at a time).
- `my_free`: mark the block free, and **coalesce** (merge) it with a free neighbour, so free memory doesn't end up in tiny useless pieces.
- Every pointer you return must be **16-byte aligned** (`(uintptr_t)p % 16 == 0`).
- `trace_test`: a program that runs 1 million random `my_malloc`/`my_free`/`my_realloc` calls, writing a pattern into each block and checking it's intact before freeing it.

## Measure
Run the same random trace with first-fit and with **best-fit** (the smallest block that fits). Compare **fragmentation**: the memory taken from the kernel vs the memory actually in use at the peak. Count page faults with `/usr/bin/time -v ./trace_test`.

## Observe
`strace -e trace=brk,mmap,munmap ls`: when does the real `malloc` (glibc) use `brk`, and when `mmap`? (Hint: big allocations.)

## Done when
- [ ] `trace_test` passes under valgrind with no errors (`valgrind ./trace_test`)
- [ ] `NOTES.md`: first-fit vs best-fit, and the strace observation

## Extra
Build it as a shared library exporting `malloc`, `free`, `realloc` and `calloc`, and run your L2 shell with it: `LD_PRELOAD=./libmymalloc.so ./qsh`.

## Read
- *Operating Systems: Three Easy Pieces*, chapter 17 ("Free-Space Management").
- *Computer Systems: A Programmer's Perspective*, section 9.9 ("Dynamic Memory Allocation").
