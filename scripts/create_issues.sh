#!/usr/bin/env bash
# Create the GitHub issues from the files in issues/ (CONN-1.md, WAL-3.md, ...).
#
# Each issue file starts with:
#   line 1:  # [WAL-3] Title
#   line 2:  labels: comp:wal, size:M
#   line 3:  milestone: v0.2 Survives a crash
#   line 4+: the body
#
# Each issue is assigned to its component's owner from scripts/owners.txt
# (if filled in). Run scripts/setup_github.sh first.
#
#   scripts/create_issues.sh --dry-run     # show what would be created
#   scripts/create_issues.sh               # create everything not created yet
#   scripts/create_issues.sh WAL           # only files starting with WAL
# Titles that already exist on GitHub are skipped, so running it twice is safe.
set -euo pipefail
cd "$(dirname "$0")/.."

DRY_RUN=""; PREFIX=""
for arg in "$@"; do
  if [[ "$arg" == "--dry-run" ]]; then DRY_RUN=1; else PREFIX="$arg"; fi
done

owner_of() {  # owner_of CONN -> the username in owners.txt, or nothing
  awk -v c="$1" '$1 == c && NF >= 2 { print $2 }' scripts/owners.txt
}

existing=""
if [[ -z "$DRY_RUN" ]]; then
  existing="$(gh issue list --state all --limit 1000 --json title --jq '.[].title')"
fi
for file in $(ls issues/*-[0-9]*.md | sort -V); do
  id="$(basename "$file" .md)"
  [[ -n "$PREFIX" && "$id" != "$PREFIX"* ]] && continue
  title="$(head -n 1 "$file" | sed 's/^# //')"
  labels="$(sed -n '2p' "$file" | sed 's/^labels: //; s/, /,/g')"
  milestone="$(sed -n '3p' "$file" | sed 's/^milestone: //')"
  body="$(tail -n +4 "$file")"
  owner="$(owner_of "${id%%-*}")"
  if grep -qxF "$title" <<< "$existing"; then echo "exists:  $title"; continue; fi
  if [[ -n "$DRY_RUN" ]]; then
    echo "would create: $title  [labels: $labels] [milestone: $milestone] [owner: ${owner:-none}]"
    continue
  fi
  args=(--title "$title" --label "$labels" --milestone "$milestone" --body "$body")
  [[ -n "$owner" ]] && args+=(--assignee "$owner")
  gh issue create "${args[@]}" > /dev/null
  echo "created: $title"
done
