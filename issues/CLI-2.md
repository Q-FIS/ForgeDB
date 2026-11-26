# [CLI-2] `forge load`: load thousands of chunks from a file
labels: comp:cli, size:S
milestone: v0.1 First search

**Component:** CLI · **Size:** S (1 day) · **Needs:** CLI-1
**Your code:** `apps/forge.cpp` · **Tests:** `tests/blackbox/cli_load_test.py` (replaces the placeholder)

## What is this, in plain words?
Real data comes in files: one chunk per line, as JSON ("JSON Lines"). `forge
load chunks.jsonl` sends them all, fast, over one kept-alive connection, and
reports what failed. This is how the ML wing's data gets in (CLI-4), and how
every experiment fills the database.

## What you must give back
`forge [--host H] [--port N] load FILE.jsonl`:
- each non-empty line is sent as-is as `POST /v1/chunks`;
- progress on stderr every 1,000 lines (`loaded 3000 (12500/s)`);
- at the end, on stdout, one JSON line: `{"added": 9998, "failed": 2, "errors": [{"line": 17, "status": 409, "error": "..."}]}` (at most the first 10 errors);
- exit 0 if nothing failed, 1 otherwise (and 1 if the file can't be read).

And `tests/blackbox/cli_load_test.py`, against the fake server: a file of 2,000 chunks loads (and `/health` says 2,000); a file with a duplicate id and a bad line reports exactly those lines, with exit code 1; an empty file adds 0.

## Watch out for
- Read the file line by line (`std::getline`), don't load it all into memory: files can be gigabytes.
- Line numbers start at 1, like in an editor.

## Done when
- [ ] `blackbox.cli.load` passes on `debug`; 100,000 chunks load in reasonable time (put the rate in the PR)

## Learn first
- jsonlines.org: one page.
