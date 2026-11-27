# [CLI-6] The demo: ForgeDB in five minutes
labels: comp:cli, size:S
milestone: v1.0 Fast, and powers the RAG

**Component:** CLI · **Size:** S · **Needs:** APP-5, WAL-4, CLI-2
**Your code:** `scripts/demo.sh`, `README.md` ("Try it")

## What is this, in plain words?
The project's front page: anyone can clone the repo and see ForgeDB work in
five minutes. Also the script for the final presentation.

## What you must give back
`scripts/demo.sh`, which, with pauses and a one-line explanation before each step:
1. builds the release version;
2. starts `forgedb` on a fresh data dir;
3. loads sample data (a small JSONL file you make, e.g. `tools/sample_chunks.jsonl`, 1,000 chunks; or CLI-4's real data if present);
4. runs a vector, a keyword and a hybrid search with `forge`;
5. shows `/metrics`;
6. kills it with `kill -9` and restarts it: the data is still there;
7. runs `forge-load` for 10 seconds and prints the numbers.

And a "Try it" section at the top of `README.md`.

## Done when
- [ ] Someone who has never seen the project runs it successfully from the README (ask a member of the ML wing)

## Learn first
- `set -euo pipefail` and `trap ... EXIT` in bash: so the demo cleans up after itself even when a step fails.
