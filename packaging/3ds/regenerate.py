#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Render the Nintendo 3DS package's artwork and banner sound.

All three are original, made for this repository, and committed so that no
build needs ImageMagick:

- icon.png (48x48): the SMDH icon, from packaging/icons/lrr-portable.svg.
- banner.png (256x128): the HOME Menu banner, from banner.svg.
- banner.wav: the banner's sound, a short synthesized chime (three rising
  notes, like a crystal struck), 16-bit mono PCM under the three seconds
  bannertool accepts.

Run it after changing either SVG or the chime below:

    python3 packaging/3ds/regenerate.py

It needs ImageMagick (`magick`, or `convert`) with SVG support.
"""
import math
import shutil
import struct
import subprocess
import sys
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
ICON_SVG = HERE.parent / "icons" / "lrr-portable.svg"
BANNER_SVG = HERE / "banner.svg"

RATE = 32000
# (start in seconds, frequency in Hz): E5, B5, E6.
NOTES = [(0.00, 659.26), (0.18, 987.77), (0.36, 1318.51)]
LENGTH = 2.2


def render(svg: Path, png: Path, width: int, height: int) -> None:
    magick = shutil.which("magick") or shutil.which("convert")
    if not magick:
        sys.exit("regenerate.py: ImageMagick (magick or convert) is required")
    subprocess.run(
        [magick, "-background", "none", "-density", "288", str(svg),
         "-resize", f"{width}x{height}", "-strip", f"PNG32:{png}"],
        check=True,
    )


def chime() -> bytes:
    frames = []
    for n in range(int(RATE * LENGTH)):
        t = n / RATE
        sample = 0.0
        for start, freq in NOTES:
            if t < start:
                continue
            u = t - start
            attack = min(1.0, u / 0.004)
            decay = math.exp(-u * 2.6)
            # A bell-like tone: the fundamental with a soft inharmonic partial.
            sample += attack * decay * (math.sin(2 * math.pi * freq * u)
                                        + 0.3 * math.sin(2 * math.pi * freq * 2.76 * u))
        # Fade the tail to silence.
        fade = min(1.0, (LENGTH - t) / 0.25)
        frames.append(struct.pack("<h", int(max(-1.0, min(1.0, 0.28 * sample * fade)) * 32767)))
    return b"".join(frames)


def main():
    render(ICON_SVG, HERE / "icon.png", 48, 48)
    render(BANNER_SVG, HERE / "banner.png", 256, 128)
    with wave.open(str(HERE / "banner.wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(chime())
    for name in ("icon.png", "banner.png", "banner.wav"):
        print(f"wrote packaging/3ds/{name} ({(HERE / name).stat().st_size} bytes)")


if __name__ == "__main__":
    main()
