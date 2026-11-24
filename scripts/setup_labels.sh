#!/usr/bin/env bash
# One-time: create the labels the issues use. Run from the repo root, after the
# GitHub repo exists. Safe to run again (--force updates existing labels).
set -euo pipefail
label() { gh label create "$1" --color "$2" --description "$3" --force; }
label "group:A" "1f77b4" "Group A"
label "group:B" "ff7f0e" "Group B"
for w in 1 2 3 4 5 6 7 8; do label "week:$w" "ededed" "Week $w"; done
label "size:S" "c5def5" "Up to 1 day"
label "size:M" "0e8a16" "2-3 days"
label "extra" "fbca04" "Optional, for people who finish early"
echo "Labels ready."
