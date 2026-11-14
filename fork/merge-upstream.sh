#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Merges Dolphin's master into a new branch made from the current one.
#
#   fork/merge-upstream.sh [branch]      the branch defaults to upstream-sync/<today>
#
# Two kinds of conflict always go the fork's way and are resolved here:
#   - a file that the fork deleted stays deleted, whatever Dolphin changed in it;
#   - the handbook in doc/ is the fork's own.
# Every other conflict is left for a person (or Claude) to resolve. See fork/README.md.
#
# Exit status: 0 merged and committed, 3 nothing new in Dolphin, 2 conflicts are left.
set -euo pipefail

UPSTREAM_URL="${UPSTREAM_URL:-https://github.com/KDE/dolphin.git}"
cd "$(git rev-parse --show-toplevel)"

if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
    echo "Commit or stash your changes first." >&2
    exit 1
fi

git fetch --quiet "$UPSTREAM_URL" master
upstream="$(git rev-parse FETCH_HEAD)"
count="$(git rev-list --count "HEAD..$upstream")"
if [[ "$count" == 0 ]]; then
    echo "Already up to date with Dolphin master."
    exit 3
fi
echo "Dolphin master has $count commits that are not merged yet."

branch="${1:-upstream-sync/$(date +%F)}"
git switch -c "$branch"
# rerere remembers how a conflict was resolved and resolves it the same way when it comes again.
git config rerere.enabled true
git config rerere.autoupdate true

git merge --no-ff --no-commit "$upstream" || true

# Deleted by the fork, changed by Dolphin: stays deleted.
git status --porcelain | awk '$1 == "DU" { sub(/^DU /, ""); print }' | while IFS= read -r path; do
    git rm --quiet -- "$path"
    echo "kept deleted: $path"
done
# The fork's handbook.
git diff --name-only --diff-filter=U -- doc | while IFS= read -r path; do
    git checkout --ours -- "$path"
    git add -- "$path"
    echo "kept the fork's: $path"
done

remaining="$(git diff --name-only --diff-filter=U)"
if [[ -n "$remaining" ]]; then
    echo
    echo "These files have conflicts to resolve on $branch:"
    echo "$remaining" | sed 's/^/  /'
    exit 2
fi

git commit --quiet -m "Merge Dolphin master ($(git rev-parse --short "$upstream"), $count commits) into dolphinCommander"
echo "Merged into $branch."
