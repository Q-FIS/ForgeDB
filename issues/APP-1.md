# [APP-1] The command line: `forgedb --dim 384 --port 8080 ...`
labels: comp:app, size:S
milestone: v0.1 First search

**Component:** APP (the forgedb program) · **Size:** S (1 day) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/app.hpp` · **Your code:** `src/app/config.cpp` · **Tests:** `tests/contract/app/config_test.cpp`

## What is this, in plain words?
`forgedb` is started like `forgedb --dim 384 --port 9000 --data-dir /srv/forge`.
`parse_args` turns that into a `Config`, and refuses anything wrong with a
message that says exactly what and why. Every test, script and teammate
starts the server this way, so it must be strict and clear.

## What you must give back
`parse_args` and `usage`.

| Flag | Value | Default |
| --- | --- | --- |
| `--dim` | 1 to 4096 | **required** |
| `--host` | not empty | `127.0.0.1` |
| `--port` | 0 to 65535 (0 = any free port) | `8080` |
| `--data-dir` | not empty | `forge-data` |
| `--threads` | 1 to 256 | `4` |
| `--queue` | 1 to 100000 | `256` |
| `--io` | `threads` or `epoll` | `threads` |
| `--fsync` | `always`, `batch` or `never` | `batch` |
| `--idle-timeout-ms` | 1 to 3600000 | `10000` |

A `UsageError` whose message **names the flag**: for a value that isn't a
whole number in range, an unknown flag, a flag with no value after it, a
flag given twice, anything that isn't a flag, or a missing `--dim`.

## Step by step
1. Delete `src/app/stub_config.cpp`; create `src/app/config.cpp`.
2. Walk the arguments two at a time: flag, value.
3. One helper for numbers: `std::from_chars`, then check it used **every** character and the range. Use it for every numeric flag.
4. Keep a `std::set` of flags already seen, for "given twice".
5. `usage()`: a short help text listing every flag, its range and default.

## Watch out for
- `std::stoi("80x")` returns 80 and `std::atoi("abc")` returns 0, silently. That's why we use `std::from_chars` and check the end pointer.
- `"-1"` parsed into an unsigned type wraps around to a huge number with some functions: make sure it's an error.

## How to test it
`ctest --preset debug -R contract.app.Config`

## Done when
- [ ] Every `contract.app.Config.*` test passes on `debug`

## Learn first
- cppreference: `std::from_chars` (its return value: `ptr` and `ec`).
