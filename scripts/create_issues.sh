#!/usr/bin/env bash
# Create the GitHub issues from the files in issues/ (W1.1.md, W3.A2.md, ...).
#
# Each issue file starts with:
#   line 1:  # [W1.1] Title
#   line 2:  labels: week:1, size:S
#   line 3:  groups: A, B          <- the issue is created once for each group listed
#   line 4+: the body
#
# Usage, from the repo root, after scripts/setup_labels.sh:
#   scripts/create_issues.sh --dry-run        # show what would be created
#   scripts/create_issues.sh                  # create everything not created yet
#   scripts/create_issues.sh W1               # only files starting with W1
# Titles that already exist on GitHub are skipped, so running it twice is safe.
set -euo pipefail

DRY_RUN=""; PREFIX=""
for arg in "$@"; do
  if [[ "$arg" == "--dry-run" ]]; then DRY_RUN=1; else PREFIX="$arg"; fi
done

existing="$(gh issue list --state all --limit 1000 --json title --jq '.[].title' 2>/dev/null || true)"
for file in $(ls issues/W*.md | sort -V); do
  base="$(basename "$file" .md)"
  [[ -n "$PREFIX" && "$base" != "$PREFIX"* ]] && continue
  title_rest="$(head -n 1 "$file" | sed 's/^# \[[^]]*\] //')"
  labels="$(sed -n '2p' "$file" | sed 's/^labels: //; s/, /,/g')"
  groups="$(sed -n '3p' "$file" | sed 's/^groups: //; s/,/ /g')"
  body="$(tail -n +4 "$file")"
  for g in $groups; do
    title="[$g · $base] $title_rest"
    if grep -qxF "$title" <<< "$existing"; then echo "exists: $title"; continue; fi
    if [[ -n "$DRY_RUN" ]]; then
      echo "would create: $title   [labels: $labels,group:$g]"
    else
      gh issue create --title "$title" --label "$labels,group:$g" --body "$body" >/dev/null
      echo "created: $title"
    fi
  done
done
