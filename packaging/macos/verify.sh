#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Checks a macOS package: mounts the disk image CPack wrote and examines the
# app bundle inside it, then runs LegoRR once.
#
#   packaging/macos/verify.sh build/dist/lrr-portable-*-Darwin-*.dmg
#
# The run has no game data, so the game stops at once; what it shows is that
# the signed bundle starts and writes its default LegoRR.ini.
set -euo pipefail

dmg="${1:?usage: verify.sh <package.dmg>}"
mnt="$(mktemp -d)"
home="$(mktemp -d)"
cleanup() {
    hdiutil detach -quiet "$mnt" || true
    rm -rf "$mnt" "$home"
}
trap cleanup EXIT

hdiutil attach -quiet -nobrowse -readonly -mountpoint "$mnt" "$dmg"
app="$mnt/LegoRR.app"
plist="$app/Contents/Info.plist"

test -x "$app/Contents/MacOS/LegoRR"
test -f "$app/Contents/Resources/AppIcon.icns"
plutil -lint "$plist"
test "$(plutil -extract CFBundleIdentifier raw "$plist")" = "io.github.robbert229.LegoRR"
test "$(plutil -extract CFBundleExecutable raw "$plist")" = "LegoRR"
test "$(plutil -extract CFBundleIconFile raw "$plist")" = "AppIcon.icns"
codesign --verify --deep --strict --verbose=2 "$app"
lipo -archs "$app/Contents/MacOS/LegoRR"
otool -L "$app/Contents/MacOS/LegoRR"

(
    cd "$home"
    HOME="$home" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        "$app/Contents/MacOS/LegoRR" -nosound
)
test -f "$home/Library/Application Support/lrr-portable/LegoRR/LegoRR.ini"
echo "verified $dmg"
