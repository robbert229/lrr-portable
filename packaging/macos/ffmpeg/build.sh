#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The FFmpeg the macOS packages bundle for the game's movies (docs/building.md,
# "Movies"). A minimal build under the LGPL, version 2.1 or later: without
# --enable-gpl, --enable-version3 or --enable-nonfree, and with nothing found
# on the build machine (--disable-autodetect), so it links only the system's
# libraries. It holds only what the movies need, and the test clip:
#
#   - the AVI demuxer and the file protocol;
#   - Indeo 5 (IV50) and Microsoft Video 1 (CRAM), the game's video;
#   - Microsoft ADPCM and 16-bit PCM, the game's sound;
#   - raw video and 8-bit PCM, src/library/testdata/synthetic.avi;
#   - libswscale, which turns the frames into the game's RGB.
#
# Usage:
#   packaging/macos/ffmpeg/build.sh fetch DIR      download and check the source
#                                                  into DIR, and print its path
#   packaging/macos/ffmpeg/build.sh build PREFIX   build and install into PREFIX
#   packaging/macos/ffmpeg/build.sh version        print the FFmpeg version
#
# build installs PREFIX/lib/lib{avformat,avcodec,avutil,swscale}.<major>.dylib,
# their headers, FFmpeg's licence as PREFIX/share/ffmpeg/COPYING.LGPLv2.1 and
# its configure line as PREFIX/share/ffmpeg/configuration.txt. Each library
# finds the others beside it (@loader_path), so the four go together into
# LegoRR.app/Contents/Frameworks. It builds for the host's architecture
# (CMAKE_OSX_ARCHITECTURES of the build that bundles it), for
# MACOSX_DEPLOYMENT_TARGET (default 12.0).
#
# The source is the release tarball, unmodified, checked against the SHA-256
# below. Every release attaches the same tarball (release.yml), as the LGPL
# asks of a program that ships the library.
set -euo pipefail

FFMPEG_VERSION="9.1"
FFMPEG_SHA256="1a4d394ae63d190a526cd052eb4415c156b5c5e2df37efab41a7de0b8fde2433"
FFMPEG_TARBALL="ffmpeg-${FFMPEG_VERSION}.tar.xz"
FFMPEG_URL="https://ffmpeg.org/releases/${FFMPEG_TARBALL}"

CONFIGURE_FLAGS=(
    --enable-shared
    --disable-static
    --disable-programs
    --disable-doc
    --disable-network
    --disable-autodetect
    --disable-everything
    --disable-avdevice
    --disable-avfilter
    --disable-swresample
    --disable-x86asm
    --enable-protocol=file
    --enable-demuxer=avi
    --enable-decoder=indeo5,msvideo1,adpcm_ms,pcm_s16le,pcm_u8,rawvideo
    --install-name-dir=@loader_path
)

fetch() {
    local dir="$1"
    mkdir -p "$dir"
    local tarball="$dir/$FFMPEG_TARBALL"
    if [ ! -f "$tarball" ]; then
        curl -fsSL --retry 3 -o "$tarball.part" "$FFMPEG_URL"
        mv "$tarball.part" "$tarball"
    fi
    if ! echo "$FFMPEG_SHA256  $tarball" | shasum -a 256 -c - >/dev/null; then
        echo "build.sh: $tarball does not match its SHA-256" >&2
        rm -f "$tarball"
        exit 1
    fi
    echo "$tarball"
}

build() {
    local prefix="$1"
    mkdir -p "$prefix"
    prefix="$(cd "$prefix" && pwd)"
    work="$(mktemp -d)"
    trap 'rm -rf "$work"' EXIT

    local tarball
    tarball="$(fetch "${LRR_FFMPEG_CACHE:-$work}")"
    tar -xJf "$tarball" -C "$work"
    local src="$work/ffmpeg-$FFMPEG_VERSION"

    local target="${MACOSX_DEPLOYMENT_TARGET:-12.0}"
    (
        cd "$src"
        ./configure --prefix="$prefix" "${CONFIGURE_FLAGS[@]}" \
            --extra-cflags="-mmacosx-version-min=$target" \
            --extra-ldflags="-mmacosx-version-min=$target"
        make -j"$(sysctl -n hw.ncpu)"
        make install
    )
    mkdir -p "$prefix/share/ffmpeg"
    cp "$src/COPYING.LGPLv2.1" "$prefix/share/ffmpeg/COPYING.LGPLv2.1"
    check "$prefix" "$work"
}

# The libraries say they are LGPL, configured as above, and link nothing
# outside the system and each other.
check() {
    local prefix="$1" work="$2"
    cat > "$work/license.c" <<'EOF'
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
int main(void)
{
    printf("%s\n%s\n%s\n%s\n%s\n", avcodec_license(), avformat_license(),
           avutil_license(), swscale_license(), avcodec_configuration());
    return 0;
}
EOF
    cc -I"$prefix/include" "$work/license.c" -L"$prefix/lib" \
        -lavcodec -lavformat -lavutil -lswscale -o "$work/license"
    # The libraries name themselves @loader_path/..., so the program finds
    # them by their leaf names on DYLD_LIBRARY_PATH.
    local report
    report="$(DYLD_LIBRARY_PATH="$prefix/lib" "$work/license")"
    local licences
    licences="$(echo "$report" | head -n 4 | sort -u)"
    if [ "$licences" != "LGPL version 2.1 or later" ]; then
        echo "build.sh: the libraries are not LGPL 2.1 or later: $licences" >&2
        exit 1
    fi
    local configuration
    configuration="$(echo "$report" | tail -n 1)"
    case "$configuration" in
        *--enable-gpl*|*--enable-version3*|*--enable-nonfree*)
            echo "build.sh: a licence-changing configure flag: $configuration" >&2
            exit 1 ;;
    esac
    echo "$configuration" > "$prefix/share/ffmpeg/configuration.txt"

    local lib
    for lib in avformat avcodec avutil swscale; do
        local dylib
        dylib="$(find "$prefix/lib" -name "lib$lib.[0-9]*.dylib" -type l | head -n 1)"
        [ -n "$dylib" ] || { echo "build.sh: no lib$lib.<major>.dylib" >&2; exit 1; }
        # The first line names the file itself; the rest are what it links.
        otool -L "$dylib" | tail -n +2 | awk '{print $1}' | while read -r dep; do
            case "$dep" in
                @loader_path/*|/usr/lib/*|/System/Library/*) ;;
                *) echo "build.sh: $dylib links $dep" >&2; exit 1 ;;
            esac
        done
    done
    echo "FFmpeg $FFMPEG_VERSION: $(echo "$report" | head -n 1), in $prefix"
}

case "${1:-}" in
    fetch) fetch "${2:?usage: build.sh fetch DIR}" ;;
    build) build "${2:?usage: build.sh build PREFIX}" ;;
    version) echo "$FFMPEG_VERSION" ;;
    *) echo "usage: build.sh fetch DIR | build PREFIX | version" >&2; exit 2 ;;
esac
