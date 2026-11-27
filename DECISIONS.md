# Decisions

The key decisions, and why. Change an entry when a decision changes; don't delete it.

## D1. One product, eleven owners
All eleven members build one thing together, ForgeDB, instead of two groups building
the same thing twice. Each person owns one component for the whole project:
they build it, review changes to it, and explain it to everyone. Ownership
gives responsibility and depth; the weekly "how my component works" talks and
the labs give breadth.

## D2. Contracts first, so everyone starts on day 1
The headers in `include/forge/`, `docs/API.md` and `docs/FORMAT.md` were written
before any code, by the lead. Every component codes against them, and only the
lead changes public signatures. Components that depend on others are tested
with stand-ins (`tests/support/fakes.hpp`) and a fake server
(`tools/fake_server.py`), so nobody waits for anybody to start.
(The same idea as the ML wing's `schema.py`.)

## D3. Contract tests, switched on by deleting a stub
Each part of a contract has a placeholder `src/<component>/stub_<part>.cpp` and
contract tests `tests/contract/<component>/<part>_test.cpp`. While the stub
exists, its tests show as Skipped; the owner deletes it when they start, and the
tests switch on. No CMake edits, and CI is green from day 1.

## D4. Every test was checked against a reference implementation
A complete reference ForgeDB was written and every contract and black-box test
passed against it (on debug with ASan/UBSan and on tsan), and typical bugs were
planted to check that the tests catch them. The black-box tests also pass
against the fake server. So a failing test means a bug in the code under test,
not in the test. (The reference is kept by the lead, outside this repo.)

## D5. Milestones, not a schedule
Issues have an order within each component and "Needs" between components,
but no dates. Three GitHub milestones (v0.1 First search, v0.2 Survives a crash,
v1.0 Fast, and powers the RAG) show progress; a milestone's demo happens when
its bar is full. A fourth, v2 Stretch, holds harder extras for the components
whose v1 work is lighter (VEC, KW, CLI), so every owner has a similar amount
of hard work.

## D6. Labs stay separate
Labs (L1-L8, one a week for 8 weeks) are done alone, in each person's own
GitHub repo, and checked by their tests. They give everyone the same base in
OS and networks, whatever component they own.

## D7. C++20 on gcc 13 (Ubuntu 24.04)
Every build is C++20: RAII, move-only types, `std::span`, `std::jthread`,
`std::shared_mutex`, `std::bit_cast`, `std::from_chars`. Nothing newer than C++20
(no `std::print`, `std::move_only_function`, `std::ranges::to`), so gcc 13 builds it all.
nlohmann/json is the one JSON library; GoogleTest the one test framework.

## D8. The caller sends vectors
ForgeDB stores and searches embeddings; it doesn't compute them. The ML wing
embeds text with its model and sends the vectors. That keeps ML models out of
the C++ code and keeps ForgeDB about systems.

## D9. No deletes or updates in v1
A chunk id can be added once. Deletes and updates complicate every component
(tombstones, index rebuilds, compaction) and aren't needed by the RAG. They're a
good "v2" project.

## D10. Durability through a write-ahead log with group commit
A put is acknowledged only once its log record is durable by the chosen
`--fsync` policy (always, batch, never). `batch` (group commit) is the default:
nearly as fast as `never`, as safe as `always`. Snapshots plus checkpoints keep
the log short.
