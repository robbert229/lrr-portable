#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Regenerates src/library/testdata/synthetic.avi, the video library test's
# clip: 8 frames of 32x24 at 15 fps, each a solid colour (red, green, blue,
# white, black, yellow, cyan, magenta), in uncompressed BGR24, with a mono
# 8 kHz unsigned 8-bit tone. Needs ffmpeg and python3.
set -euo pipefail
out="${1:-$(dirname "$0")/../src/library/testdata/synthetic.avi}"
python3 -c '
import sys
colours = [(255,0,0),(0,255,0),(0,0,255),(255,255,255),(0,0,0),(255,255,0),(0,255,255),(255,0,255)]
for r, g, b in colours:
    sys.stdout.buffer.write(bytes([b, g, r]) * (32 * 24))
' | ffmpeg -v error -y -f rawvideo -pix_fmt bgr24 -s 32x24 -r 15 -i - \
    -f lavfi -i "sine=frequency=440:sample_rate=8000:duration=0.5333" \
    -map 0:v -map 1:a -c:v rawvideo -pix_fmt bgr24 -c:a pcm_u8 -ac 1 \
    -fflags +bitexact -flags:v +bitexact -flags:a +bitexact "$out"
