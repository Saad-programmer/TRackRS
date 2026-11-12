#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Print the tag commands for every gate in docs/GATES.md not yet on the remote.
#
# Read-only: it creates and pushes nothing. Build sessions cannot push tags,
# so the maintainer runs the printed commands by hand. Each gate row's commit
# is resolved to its full SHA, so a short SHA that has become ambiguous fails
# here instead of tagging the wrong commit.
#
# Usage: tools/make_tags.sh [remote]      (default remote: origin)
set -euo pipefail

remote="${1:-origin}"
root="$(git rev-parse --show-toplevel)"
gates="$root/docs/GATES.md"
[[ -f "$gates" ]] || { echo "missing $gates" >&2; exit 1; }

# Tags already on the remote (annotated tags appear twice, with and without ^{}).
remote_tags="$(git ls-remote --tags "$remote" | sed -e 's#.*refs/tags/##' -e 's#\^{}$##' | sort -u)"

missing=0
# Gate rows look like: | `m1-done` | `381848a` | [run](url) ... | date | on remote |
while IFS= read -r line; do
  gate="$(sed -E 's/^\| `([^`]+)` \| `([0-9a-f]+)`.*/\1/' <<<"$line")"
  short="$(sed -E 's/^\| `([^`]+)` \| `([0-9a-f]+)`.*/\2/' <<<"$line")"
  if grep -qxF "$gate" <<<"$remote_tags"; then
    continue
  fi
  if ! full="$(git rev-parse --verify --quiet "${short}^{commit}")"; then
    echo "# $gate: commit $short is not in this clone; run 'git fetch $remote' first" >&2
    missing=1
    continue
  fi
  echo "git tag -a $gate $full -m \"$gate: see docs/GATES.md\""
  echo "git push $remote $gate"
done < <(grep -E '^\| `[^`]+` \| `[0-9a-f]{7,40}`' "$gates")

exit "$missing"
