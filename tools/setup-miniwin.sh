#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Bootstrap the sparse miniwin submodule for the native build (ADR 0004, Q-F)
# and apply the project's miniwin patches to it (ADR 0009).
#
# A fresh clone has only the .gitmodules pointer. This fetches upstream
# isle-portable with a blob-none filter AND a cone sparse-checkout of miniwin/
# ONLY — the sibling game's decompiled source is never written to the working
# tree, not even transiently. The checkout is pinned to the exact commit
# recorded in the superproject's gitlink, so the build stays reproducible, and
# tools/miniwin-patches.sh then applies patches/miniwin on top. Safe to rerun:
# an existing checkout is moved to the upstream URL, made sparse, and
# re-patched when patches/miniwin or the pin changed.
set -euo pipefail
cd "$(dirname "$0")/.."

SUB=3rdparty/isle-portable
MK="$(git ls-files -s -- "$SUB" | awk '{print $2}')" # pinned commit (gitlink SHA)
UR="$(git config -f .gitmodules --get submodule.$SUB.url)"

if [ ! -e "$SUB/.git" ]; then # -e: a file after `git submodule absorbgitdirs`
    if ! git ls-remote --exit-code "$UR" >/dev/null 2>&1; then
        echo "cannot reach isle-portable remote — network required" >&2
        exit 1
    fi
    # Sparse-aware partial clone: commits+trees only, no blobs, no worktree
    # files. Sparse patterns are established BEFORE any checkout, so
    # non-miniwin content never lands.
    rm -rf "$SUB"
    git clone -q --filter=blob:none --no-checkout "$UR" "$SUB"
    git -C "$SUB" sparse-checkout init --cone
    git -C "$SUB" sparse-checkout set miniwin
    git -C "$SUB" cat-file -e "$MK^{commit}" 2>/dev/null || git -C "$SUB" fetch -q --filter=blob:none origin "$MK"
    git -C "$SUB" -c advice.detachedHead=false checkout -q "$MK"

    # Move the nested .git into the superproject's module store so this is a
    # real git submodule again (fresh `git submodule status` works,
    # .gitmodules intact).
    git submodule absorbgitdirs "$SUB"
else
    # Checkouts made before ADR 0009 point at the project's old fork, and
    # some are not sparse.
    if [ "$(git -C "$SUB" remote get-url origin)" != "$UR" ]; then
        git -C "$SUB" remote set-url origin "$UR"
    fi
    if [ "$(git -C "$SUB" config --bool core.sparseCheckout || echo false)" != true ]; then
        git -C "$SUB" sparse-checkout init --cone
        git -C "$SUB" sparse-checkout set miniwin
    fi
fi

tools/miniwin-patches.sh apply

# Gate: only miniwin/ plus root metadata may exist here.
echo "--- $SUB working tree ---"
git -C "$SUB" sparse-checkout list
find "$SUB" -mindepth 1 -maxdepth 1 -not -path "$SUB/.git*" | sort

echo "miniwin ready (sparse, miniwin/ only, upstream ${MK} + patches/miniwin)"
