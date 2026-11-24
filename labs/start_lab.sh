#!/usr/bin/env bash
# Copy a lab's handout, starter files and test into your own labs repo.
#   labs/start_lab.sh L1 ~/qfis-labs
set -euo pipefail
lab="${1:?usage: start_lab.sh <L1..L8> <your labs repo>}"
repo="${2:?usage: start_lab.sh <L1..L8> <your labs repo>}"
here="$(cd "$(dirname "$0")" && pwd)"
src="$here/$lab"
[[ -d "$src" ]] || { echo "no such lab: $lab"; exit 1; }
dest="$repo/$lab"
if [[ -e "$dest" ]]; then echo "$dest already exists: not overwriting your work"; exit 1; fi
mkdir -p "$dest"
cp -r "$src/starter/." "$dest/"
cp "$src/README.md" "$dest/README.md"
cp "$src"/test_*.py "$dest/" 2>/dev/null || echo "(this lab's test isn't ready yet: it will be before the lab's week)"
echo "Started $lab in $dest. Read $dest/README.md."
