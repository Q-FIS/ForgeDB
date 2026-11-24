# Decisions

The key decisions, and why. Change an entry when a decision changes; don't delete it.

## D1. Two groups, each owning something whole
Two groups of 5. Weeks 1-4: each builds a complete HTTP server, in a different
design (A: thread pool, B: event loop), and they're compared in week 4. Weeks
5-8: A builds the engine, B the service. Each group works in its own folder,
so there are no merge conflicts between groups and no waiting on another group's code.

## D2. Labs live in each person's own repo
Labs are individual work, so they belong on each person's own GitHub profile.
The club repo only holds the handouts, starter files and tests
(`labs/start_lab.sh` copies them in). Labs aren't GitHub issues, and aren't reviewed:
the test passing plus a 3-minute show-and-tell in the group meeting is the check.

## D3. C for labs that meet the kernel, C++20 for every build
L1, L2, L5 and L7 are in C, where you see the raw system-call interface. All
builds are C++20, which teaches the modern way to use the same calls safely:
RAII, move-only types, `std::span`, `std::jthread`, `std::system_error`.
gcc 13 (Ubuntu 24.04) supports everything we use; we avoid `std::format`.

## D4. Tests are written before the work, and skip until it exists
Every program starts as a placeholder that exits with code 77. The black-box
tests report that as SKIPPED, so CI is green from day 1. When the program is
written, its test runs and must pass. Every test was checked against a
reference implementation before the project started, so a failing test means a
bug in the code, not in the test. Later weeks' tests are commented out in
`tests/CMakeLists.txt` and switched on by the issue that needs them.

## D5. Sanitizers always on in debug; ThreadSanitizer in CI
The `debug` preset always uses AddressSanitizer + UBSan; CI also runs the
`tsan` preset. Nobody configures compiler flags by hand. (CI sets
`vm.mmap_rnd_bits=28`, because ThreadSanitizer fails to start on newer kernels without it.)

## D6. Two libraries only
GoogleTest for unit tests, and (from week 5) nlohmann/json for the service's
JSON. Both come in through CMake's FetchContent. Everything else we write
ourselves, because that's the point.

## D7. The engine contract is fixed before week 5, with a stub
`engine/include/qfis/engine.hpp` is the only thing Group B calls from Group A.
Classes use pimpl, so the header doesn't change while Group A works, and they're
movable (`VectorIndex::load` returns by value, and a class with a user-declared
destructor gets no implicit move constructor). A stub engine builds today, so
Group B starts week 5 without waiting. Changing the header needs both captains.

## D8. Weeks 5-8 issues are written after the week-4 race
What the race shows (how the thread pool and the event loop break) decides the
final server's design, so those issues are written at the end of week 4.

## D9. The search service takes vectors; it doesn't compute them
Callers send embedding vectors with chunks and queries, so the C++ service
needs no ML model and doesn't depend on the ML wing's schedule. The model
client talks to a local Ollama over plain HTTP; HTTPS is a later extra.
