#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Builds the AppImage from a configured and built tree, then checks it with
# check.sh. Adapted from isle-portable's packaging/linux/appimage/Build and
# its CI job.
#
#   packaging/linux/appimage/build.sh [build-dir [output-dir]]
#
# build-dir defaults to build, output-dir to <build-dir>/dist. It installs the
# tree into <build-dir>/AppDir with `cmake --install`, lets linuxdeploy copy
# in the libraries LegoRR needs and AppRun in place of its default, and packs
# the result with appimagetool. Both tools are pinned releases, checked against
# the SHA-256 below, and run with APPIMAGE_EXTRACT_AND_RUN so that neither
# needs FUSE. x86_64 only.
set -euo pipefail

LINUXDEPLOY_URL=https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
LINUXDEPLOY_SHA256=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
APPIMAGETOOL_URL=https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage
APPIMAGETOOL_SHA256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0

HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="$(cd "${1:-build}" && pwd)"
OUT="${2:-$BUILD/dist}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

[ "$(uname -m)" = x86_64 ] || { echo "build.sh: the AppImage is x86_64 only" >&2; exit 1; }

APP_ID="$(sed -n 's/^set(LRR_APP_ID "\(.*\)")$/\1/p' "$HERE/../../CMakeLists.txt")"
VERSION="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$BUILD/CMakeCache.txt")"
[ -n "$APP_ID" ] && [ -n "$VERSION" ] || { echo "build.sh: no app id or version found" >&2; exit 1; }

TOOLS="$BUILD/appimage-tools"
mkdir -p "$TOOLS"
fetch() { # fetch <url> <sha256> <file>
    if ! { [ -f "$3" ] && echo "$2  $3" | sha256sum -c --status; }; then
        curl -fsSL --retry 3 -o "$3" "$1"
        echo "$2  $3" | sha256sum -c --status || { echo "build.sh: $3 does not match its SHA-256" >&2; exit 1; }
    fi
    chmod +x "$3"
}
fetch "$LINUXDEPLOY_URL" "$LINUXDEPLOY_SHA256" "$TOOLS/linuxdeploy.AppImage"
fetch "$APPIMAGETOOL_URL" "$APPIMAGETOOL_SHA256" "$TOOLS/appimagetool.AppImage"
export APPIMAGE_EXTRACT_AND_RUN=1 ARCH=x86_64

APPDIR="$BUILD/AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD" --prefix /usr

# NO_STRIP: linuxdeploy's bundled strip predates the .relr.dyn sections of
# current toolchains and corrupts them.
NO_STRIP=1 "$TOOLS/linuxdeploy.AppImage" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/LegoRR" \
    --desktop-file "$APPDIR/usr/share/applications/$APP_ID.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/$APP_ID.svg" \
    --icon-filename "$APP_ID" \
    --custom-apprun "$HERE/AppRun"

"$HERE/check.sh" "$APPDIR/usr"

IMAGE="$OUT/lrr-portable-$VERSION-Linux-x86_64.AppImage"
"$TOOLS/appimagetool.AppImage" "$APPDIR" "$IMAGE"
echo "built $IMAGE"
