#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# The project's miniwin patches (ADR 0009): patches/miniwin/*.patch, applied
# with `git am` on top of the upstream isle-portable commit that the
# 3rdparty/isle-portable gitlink pins.
#
#   apply         Check out the pinned commit and apply the series, unless it
#                 is applied already. Refuses to drop uncommitted changes or
#                 commits that are not in patches/miniwin.
#   check         Exit 0 when the series is applied and the checkout is
#                 clean, 1 otherwise.
#   export        Write the submodule's commits since the pin back to
#                 patches/miniwin. Run it after committing a miniwin change
#                 in the submodule.
#   rebase <rev>  Rebase the series onto upstream <rev>, repin the gitlink
#                 and export. Conflicts stop in the submodule: resolve them,
#                 `git -C 3rdparty/isle-portable rebase --continue`, then run
#                 `export` and stage the gitlink.
set -euo pipefail
cd "$(dirname "$0")/.."

SUB=3rdparty/isle-portable
PATCHES=patches/miniwin
FORMAT_OPTS=(--zero-commit --no-signature --no-numbered --no-stat)

die() {
    echo "miniwin-patches: $*" >&2
    exit 1
}

# The pinned upstream commit, from the index so a staged repin counts.
pin() {
    git ls-files -s -- "$SUB" | awk '{print $2}'
}

# SHA-256 of stdin: sha256sum where there is one, else macOS's shasum.
sha256() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum
    else
        shasum -a 256
    fi
}

# A fingerprint of the pin and the patch files, recorded in the submodule's
# git directory after an apply or export.
fingerprint() {
    { pin; cat "$PATCHES"/*.patch 2>/dev/null; } | sha256 | awk '{print $1}'
}

stamp_file() {
    echo "$(git -C "$SUB" rev-parse --absolute-git-dir)/lrr-miniwin-patches"
}

# The stamp holds "<fingerprint> <head after apply>".
read_stamp() {
    local f
    f="$(stamp_file)"
    [ -f "$f" ] && cat "$f" || echo "- -"
}

write_stamp() {
    echo "$(fingerprint) $(git -C "$SUB" rev-parse HEAD)" >"$(stamp_file)"
}

require_checkout() {
    [ -e "$SUB/.git" ] || die "$SUB is missing; run tools/setup-miniwin.sh"
}

dirty() {
    [ -n "$(git -C "$SUB" status --porcelain --untracked-files=no)" ]
}

# Applied and clean: the stamp matches the patch files and HEAD.
is_applied() {
    local s h
    read -r s h <<<"$(read_stamp)"
    [ "$s" = "$(fingerprint)" ] && [ "$h" = "$(git -C "$SUB" rev-parse HEAD)" ] && ! dirty
}

# True when HEAD is on an upstream branch, so it holds no local work: the
# pin of another branch (main pins plain upstream) or a newer upstream commit.
on_upstream() {
    [ -n "$(git -C "$SUB" branch -r --contains HEAD 2>/dev/null)" ]
}

# True when HEAD descends from the pin and each commit since it has the
# patch id of one of the patch files.
only_series_commits() {
    local p="$1" known c
    git -C "$SUB" merge-base --is-ancestor "$p" HEAD 2>/dev/null || return 1
    known="$(for f in "$PATCHES"/*.patch; do git patch-id --stable <"$f"; done | awk '{print $1}')"
    for c in $(git -C "$SUB" rev-list "$p..HEAD"); do
        local id
        id="$(git -C "$SUB" show "$c" | git patch-id --stable | awk '{print $1}')"
        grep -qx "$id" <<<"$known" || return 1
    done
}

cmd_check() {
    require_checkout
    if is_applied; then
        return 0
    fi
    echo "miniwin-patches: $SUB does not match $PATCHES; run tools/setup-miniwin.sh" >&2
    return 1
}

cmd_apply() {
    require_checkout
    if is_applied; then
        echo "miniwin patches up to date ($(ls "$PATCHES"/*.patch | wc -l) on $(pin | cut -c1-9))"
        return 0
    fi
    local p head s h
    p="$(pin)"
    head="$(git -C "$SUB" rev-parse HEAD)"
    read -r s h <<<"$(read_stamp)"
    if dirty; then
        die "$SUB has uncommitted changes; commit them and run '$0 export', or discard them"
    fi
    # HEAD may be the pin (fresh clone, or `git submodule update`), the last
    # applied series, an upstream commit (left by a checkout of another
    # branch, whose pin differs), or any commits that are all in the series
    # (a checkout of the old fork). Anything else holds commits we would lose.
    if [ "$head" != "$p" ] && [ "$head" != "$h" ] && ! on_upstream && ! only_series_commits "$p"; then
        if git -C "$SUB" merge-base --is-ancestor "$p" HEAD 2>/dev/null; then
            die "$SUB HEAD $head has commits not in $PATCHES; run '$0 export' to keep them, or discard them with 'git -C $SUB checkout --detach $p'"
        fi
        die "$SUB HEAD $head has local commits and does not descend from the pin $p; move them onto the pin and run '$0 export' to keep them, or discard them with 'git -C $SUB checkout --detach $p'"
    fi
    git -C "$SUB" cat-file -e "$p^{commit}" 2>/dev/null || git -C "$SUB" fetch -q --filter=blob:none origin "$p"
    git -C "$SUB" -c advice.detachedHead=false checkout -q --detach "$p"
    shopt -s nullglob
    local series=("$PWD/$PATCHES"/*.patch)
    if [ ${#series[@]} -gt 0 ]; then
        GIT_COMMITTER_NAME="miniwin patches" GIT_COMMITTER_EMAIL="miniwin-patches@localhost" \
            git -C "$SUB" am -q --whitespace=nowarn --committer-date-is-author-date "${series[@]}" ||
            die "a patch did not apply; see 'git -C $SUB am --show-current-patch', then 'git -C $SUB am --abort'"
    fi
    write_stamp
    echo "miniwin patches applied (${#series[@]} on $(echo "$p" | cut -c1-9))"
}

cmd_export() {
    require_checkout
    dirty && die "$SUB has uncommitted changes; commit them first"
    local p
    p="$(pin)"
    git -C "$SUB" merge-base --is-ancestor "$p" HEAD || die "$SUB HEAD does not descend from the pin $p"
    rm -f "$PATCHES"/*.patch
    mkdir -p "$PATCHES"
    git -C "$SUB" format-patch -q "${FORMAT_OPTS[@]}" -o "$PWD/$PATCHES" "$p..HEAD"
    write_stamp
    echo "exported $(ls "$PATCHES"/*.patch 2>/dev/null | wc -l) patches to $PATCHES"
}

cmd_rebase() {
    local rev="${1:-}"
    [ -n "$rev" ] || die "usage: $0 rebase <upstream commit>"
    cmd_apply
    git -C "$SUB" fetch -q --filter=blob:none origin "$rev"
    local new
    new="$(git -C "$SUB" rev-parse "$rev^{commit}" 2>/dev/null || git -C "$SUB" rev-parse FETCH_HEAD)"
    git -C "$SUB" rebase -q --onto "$new" "$(pin)" ||
        die "rebase stopped; resolve it in $SUB, then: git update-index --cacheinfo 160000,$new,$SUB && $0 export"
    git update-index --cacheinfo "160000,$new,$SUB"
    cmd_export
    echo "repinned $SUB to $new; commit the gitlink and $PATCHES"
}

case "${1:-}" in
apply) cmd_apply ;;
check) cmd_check ;;
export) cmd_export ;;
rebase) cmd_rebase "${2:-}" ;;
*) die "usage: $0 {apply|check|export|rebase <upstream commit>}" ;;
esac
