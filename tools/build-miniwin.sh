#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Build the miniwin static archive where meson expects it.
#
# Why this exists: miniwin's CMakeLists marks its `miniwin` library
# EXCLUDE_FROM_ALL, and meson's cmake.subproject maps that nested target to an
# *empty* placeholder archive (nothing references miniwin until the frame
# loop does). This script configures + builds the miniwin CMake project for
# real and places libminiwin.a at the path meson's link rules use.
#
# Run once after `meson setup` (and after any miniwin change / clean build):
#   tools/build-miniwin.sh
# meson treats the produced archive as up to date (its rule has no inputs).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE_DIR="$ROOT/build/subprojects/miniwin/__CMake_build"
DEST="$ROOT/build/subprojects/miniwin/libminiwin.a"

if command -v cmake >/dev/null 2>&1; then
  CMAKE_BIN="$(command -v cmake)"
else
  echo "tools/build-miniwin.sh: cmake not found" >&2
  exit 1
fi

mkdir -p "$CMAKE_DIR"
"$CMAKE_BIN" -S "$ROOT/subprojects/miniwin" -B "$CMAKE_DIR" -G Ninja
"$CMAKE_BIN" --build "$CMAKE_DIR" --target miniwin

# The archive lands at <cmake>/miniwin_build/libminiwin.a; meson reads it from
# subprojects/miniwin/libminiwin.a.
SRC="$CMAKE_DIR/miniwin_build/libminiwin.a"
if [ ! -f "$SRC" ]; then
  echo "tools/build-miniwin.sh: expected archive not produced: $SRC" >&2
  exit 1
fi
cp -f "$SRC" "$DEST"
echo "tools/build-miniwin.sh: OK -> $DEST"