#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Checks a macOS package: mounts the disk image CPack wrote and examines the
# app bundle inside it, then runs LegoRR once. The bundle must carry the
# FFmpeg of packaging/macos/ffmpeg (a build with LRR_FFMPEG_DIR): its four
# libraries, linking only the system and each other, built without a
# licence-changing configure flag, and its licence.
#
#   packaging/macos/verify.sh build/dist/lrr-portable-*-Darwin-*.dmg
#
# The run has no game data, so the game stops at once; what it shows is that
# the signed bundle starts and writes its default LegoRR.ini. SDL takes the
# preference directory from the account, not from $HOME, so that file is the
# real ~/Library/Application Support/lrr-portable/LegoRR/LegoRR.ini, which any
# launch of the game writes.
set -euo pipefail

dmg="${1:?usage: verify.sh <package.dmg>}"
mnt="$(mktemp -d)"
work="$(mktemp -d)"
cleanup() {
    hdiutil detach -quiet "$mnt" || true
    rm -rf "$mnt" "$work"
}
trap cleanup EXIT

hdiutil attach -nobrowse -readonly -mountpoint "$mnt" "$dmg"
app="$mnt/LegoRR.app"
plist="$app/Contents/Info.plist"

test -x "$app/Contents/MacOS/LegoRR"
test -f "$app/Contents/Resources/AppIcon.icns"
test -f "$app/Contents/Resources/LICENSE"
test -f "$app/Contents/Resources/THIRD-PARTY-NOTICES.txt"
python3 "$(dirname "$0")/../../tools/check_package.py" "$mnt"
plutil -lint "$plist"
test "$(plutil -extract CFBundleIdentifier raw "$plist")" = "io.github.robbert229.LegoRR"
test "$(plutil -extract CFBundleExecutable raw "$plist")" = "LegoRR"
test "$(plutil -extract CFBundleIconFile raw "$plist")" = "AppIcon.icns"
frameworks="$app/Contents/Frameworks"
test -f "$app/Contents/Resources/FFmpeg-COPYING.LGPLv2.1.txt"
for lib in avformat avcodec avutil swscale; do
    dylib="$(find "$frameworks" -name "lib$lib.[0-9]*.dylib" | head -n 1)"
    test -n "$dylib" || { echo "verify.sh: no lib$lib in $frameworks" >&2; exit 1; }
    otool -L "$dylib" | tail -n +2 | awk '{print $1}' | while read -r dep; do
        case "$dep" in
            @loader_path/*|/usr/lib/*|/System/Library/*) ;;
            *) echo "verify.sh: $dylib links $dep" >&2; exit 1 ;;
        esac
    done
done
# libavcodec keeps its configure line (avcodec_configuration()).
configuration="$(strings "$frameworks"/libavcodec.*.dylib | grep -e '--disable-everything' | head -n 1)"
test -n "$configuration" || { echo "verify.sh: libavcodec has no configure line" >&2; exit 1; }
case "$configuration" in
    *--enable-gpl*|*--enable-version3*|*--enable-nonfree*)
        echo "verify.sh: the bundled FFmpeg is not LGPL 2.1: $configuration" >&2; exit 1 ;;
esac
codesign --verify --deep --strict --verbose=2 "$app"
lipo -archs "$app/Contents/MacOS/LegoRR"
otool -L "$app/Contents/MacOS/LegoRR"

(
    cd "$work"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        "$app/Contents/MacOS/LegoRR" -nosound || echo "LegoRR exited with status $?"
)
test -f "$HOME/Library/Application Support/lrr-portable/LegoRR/LegoRR.ini"
echo "verified $dmg"
