#!/usr/bin/env bash
# Create the labels and milestones on GitHub. Run once, from the repo root.
#   scripts/setup_github.sh
# Safe to run again: existing labels are updated, existing milestones are kept.
set -euo pipefail

label() { gh label create "$1" --color "$2" --description "$3" --force; }

for c in conn http pool app wal snap vec kw cli load sec; do
  label "comp:$c" "1d76db" "Component $(echo $c | tr a-z A-Z)"
done
label "size:S" "c2e0c6" "About 1 day"
label "size:M" "fbca04" "About 2-3 days"
label "size:L" "e99695" "About a week (stretch work)"
label "experiment" "d93f0b" "Measure something and write a report in docs/experiments/"

# Milestones show progress (a bar on GitHub), not deadlines: no due dates.
existing="$(gh api "repos/{owner}/{repo}/milestones?state=all&per_page=100" --jq '.[].title')"
milestone() {
  if grep -qxF "$1" <<< "$existing"; then echo "milestone exists: $1"; return; fi
  gh api "repos/{owner}/{repo}/milestones" -f title="$1" -f description="$2" > /dev/null
  echo "milestone created: $1"
}
milestone "v0.1 First search" "ForgeDB answers requests end to end, in memory. Done when blackbox.smoke passes against forgedb."
milestone "v0.2 Survives a crash" "Write-ahead log, recovery, snapshots, BM25, limits, metrics. Done when blackbox.crash passes."
milestone "v1.0 Fast, and powers the RAG" "epoll, SIMD, all cores, hybrid search, the ML wing's RAG on ForgeDB, and the experiments."
milestone "v2 Stretch" "Harder extras for whoever finishes their v1 work: approximate vector indexes, on-disk segments, WAND, replication, resilient clients."
