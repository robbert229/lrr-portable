#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Checks what the AppImage would run on: nothing in the image's usr/ may need
# a glibc newer than 2.39 (Ubuntu 24.04's, the build host) or a CPU above
# baseline x86-64. Adapted from isle-portable's "Check AppImage compatibility"
# CI step.
#
#   packaging/linux/appimage/check.sh <AppDir>/usr
set -euo pipefail

MAX_GLIBC=${MAX_GLIBC:-2.39}
usr="${1:?usage: check.sh <AppDir>/usr}"
cd "$usr"

files=$(find bin lib -type f 2>/dev/null || true)
# shellcheck disable=SC2086
glibc=$(echo "$files" | xargs -r objdump -T 2>/dev/null | grep -o 'GLIBC_2\.[0-9]*' | sort -uV | tail -n 1 || true)
echo "Highest glibc version required: ${glibc:-none}"
if [ -z "$glibc" ]; then
    echo "check.sh: no glibc symbol versions found; is this an AppDir's usr?" >&2
    exit 1
fi
if [ "$(printf '%s\n%s\n' "$glibc" "GLIBC_$MAX_GLIBC" | sort -V | tail -n 1)" != "GLIBC_$MAX_GLIBC" ]; then
    echo "check.sh: the AppImage must not require a glibc newer than GLIBC_$MAX_GLIBC" >&2
    exit 1
fi
# shellcheck disable=SC2086
if echo "$files" | xargs -r readelf -n 2>/dev/null | grep -q 'ISA needed:.*x86-64-v[234]'; then
    echo "check.sh: the AppImage requires a CPU newer than baseline x86-64" >&2
    exit 1
fi
echo "AppImage requirements ok (glibc <= $MAX_GLIBC, baseline x86-64)"
