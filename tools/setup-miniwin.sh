#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Bootstrap the sparse miniwin submodule for the native build (ADR 0004, Q-F).
#
# A fresh clone has only the .gitmodules pointer. This fetches isle-portable
# with a blob-none filter AND a cone sparse-checkout of miniwin/ ONLY — the
# sibling game's decompiled source is never written to the working tree, not
# even transiently. The checkout is pinned to the exact commit recorded in the
# superproject's gitlink, so the build stays reproducible.
set -euo pipefail
cd "$(dirname "$0")/.."

SUB=3rdparty/isle-portable
MK="$(git ls-tree HEAD "$SUB" | awk '{print $3}')"   # pinned commit (gitlink SHA)

if [ -d "$SUB/.git" ]; then
    echo "$SUB already present (commit ${MK:-?})"
    exit 0
fi

if ! git ls-remote --exit-code "$(git config -f .gitmodules --get submodule.$SUB.url)" >/dev/null 2>&1; then
    echo "cannot reach isle-portable remote — network required" >&2
    exit 1
fi

UR=$(git config -f .gitmodules --get submodule.$SUB.url || echo "https://github.com/isledecomp/isle-portable.git")

# Sparse-aware partial clone: commits+trees only, no blobs, no worktree files.
# Sparse patterns are established BEFORE any checkout, so non-miniwin content
# never lands.
git clone --filter=blob:none --no-checkout "$UR" "$SUB"
git -C "$SUB" sparse-checkout init --cone
git -C "$SUB" sparse-checkout set miniwin
git -C "$SUB" -c advice.detachedHead=false checkout "$MK"

# Move the nested .git into the superproject's module store so this is a real
# git submodule again (fresh `git submodule status` works, .gitmodules intact).
git submodule absorbgitdirs "$SUB"

# Gate: only miniwin/ plus root metadata may exist here.
echo "--- $SUB working tree ---"
git -C "$SUB" sparse-checkout list
find "$SUB" -mindepth 1 -maxdepth 1 -not -path "$SUB/.git*" | sort

echo "miniwin ready (sparse, miniwin/ only, commit ${MK})"