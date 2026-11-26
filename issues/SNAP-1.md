# [SNAP-1] The in-memory store
labels: comp:snap, size:S
milestone: v0.1 First search

**Component:** SNAP (storage) · **Size:** S (1 day) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/store.hpp` · **Your code:** `src/snap/memory_store.cpp` · **Tests:** `tests/contract/snap/memory_store_test.cpp`

## What is this, in plain words?
The Store keeps every chunk, and finds it two ways: by **row** (its position:
0, 1, 2...; that's what the indexes return) and by **id** (the client's name
for it). `Store` is an *interface* (a class with only virtual functions), and
`make_memory_store()` returns your implementation of it. Later you'll save it
to disk (SNAP-3) and load it fast (SNAP-5).

## What you must give back
A class `MemoryStore final : public Store` (hidden inside your `.cpp`, in an
anonymous namespace), and `make_memory_store()`, following the header.

## Step by step
1. Delete `src/snap/stub_memory_store.cpp`; create `src/snap/memory_store.cpp`.
2. Two members: `std::vector<Chunk> chunks_` (row → chunk) and a map from id to row.
3. `add`: if the id exists, throw `AlreadyExists` **before** changing anything; else push back and record the row.
4. `at`: `chunks_.at(row)` already throws `std::out_of_range`.
5. `find(std::string_view id)`: with `std::map<std::string, Row, std::less<>>`, you can look up a `string_view` without building a `std::string` (that's what `std::less<>` enables: "heterogeneous lookup").

## Watch out for
- `override` on every function: the compiler then checks the signatures match the interface.
- `find` gets a `string_view` that may point into a longer string: never assume it ends with `\0` (no `.data()` into a C function).

## How to test it
`ctest --preset debug -R contract.snap.MemoryStore`

## Done when
- [ ] Every `contract.snap.MemoryStore.*` test passes on `debug`
- [ ] In the PR: one sentence on why `std::less<>` lets `find` skip a copy

## Learn first
- learncpp chapter 25, "Virtual functions" (virtual, override, final, and why the base class has a virtual destructor).
- cppreference: `std::map::find`, the "transparent comparator" overload.
