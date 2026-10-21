#!/usr/bin/env bash
# Bootstrap the sparse miniwin submodule for the native build (ADR 0004, Q-F).
#
# A fresh clone has only the .gitmodules pointer. This fetches isle-portable
# shallowly and sparse-checks-out ONLY miniwin/, so the sibling game's
# decompiled source never lands in our working tree.
set -euo pipefail
cd "$(dirname "$0")/.."

if [ -d 3rdparty/isle-portable/.git ]; then
    echo "3rdparty/isle-portable already present"
    exit 0
fi

git submodule update --init --depth 1 3rdparty/isle-portable

git -C 3rdparty/isle-portable sparse-checkout set --cone miniwin

echo "miniwin ready (sparse, miniwin/ only):"
git -C 3rdparty/isle-portable sparse-checkout list