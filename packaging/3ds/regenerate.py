#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Render the Nintendo 3DS package's artwork and banner sound.

All three are original, made for this repository, and committed so that no
build needs ImageMagick:

- icon.png (48x48): the SMDH icon, packaging/icons/lrr-portable.svg's
  40x40 pixel art at one image pixel per art pixel, with a 4-pixel border.
- banner.png (256x128): the HOME Menu banner, banner.svg's background and
  name with the same pixel art drawn at twice its size on the left.
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
import tempfile
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
ICON_SVG = HERE.parent / "icons" / "lrr-portable.svg"
BANNER_SVG = HERE / "banner.svg"
# The icon is pixel art on a 40x40 grid. Sampling it at exactly that size,
# then scaling by whole numbers, keeps every art pixel sharp.
ICON_GRID = 40
ICON_SIZE = 48
BANNER_ART_SCALE = 2
BANNER_ART_POS = (19, 24)

RATE = 32000
# (start in seconds, frequency in Hz): E5, B5, E6.
NOTES = [(0.00, 659.26), (0.18, 987.77), (0.36, 1318.51)]
LENGTH = 2.2


def magick(*args: str) -> None:
    tool = shutil.which("magick") or shutil.which("convert")
    if not tool:
        sys.exit("regenerate.py: ImageMagick (magick or convert) is required")
    subprocess.run([tool, *args], check=True)


def render_art(png: Path) -> None:
    """Renders the icon's pixel art at one image pixel per art pixel.

    The SVG is rendered large and point-sampled at each art pixel's centre,
    so no colour bleeds across art pixels whatever the renderer's DPI.
    """
    magick("-background", "none", "-density", "288", str(ICON_SVG),
           "-filter", "point", "-resize", f"{ICON_GRID}x{ICON_GRID}!",
           "-strip", f"PNG32:{png}")


def icon(art: Path, png: Path) -> None:
    magick(str(art), "-background", "none", "-gravity", "center",
           "-extent", f"{ICON_SIZE}x{ICON_SIZE}", "-strip", f"PNG32:{png}")


def banner(art: Path, png: Path) -> None:
    x, y = BANNER_ART_POS
    magick("-background", "none", "-density", "288", str(BANNER_SVG),
           "-resize", "256x128",
           "(", str(art), "-filter", "point", "-resize", f"{BANNER_ART_SCALE * 100}%", ")",
           "-geometry", f"+{x}+{y}", "-composite", "-strip", f"PNG32:{png}")


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
    with tempfile.TemporaryDirectory() as tmp:
        art = Path(tmp) / "art.png"
        render_art(art)
        icon(art, HERE / "icon.png")
        banner(art, HERE / "banner.png")
    with wave.open(str(HERE / "banner.wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(chime())
    for name in ("icon.png", "banner.png", "banner.wav"):
        print(f"wrote packaging/3ds/{name} ({(HERE / name).stat().st_size} bytes)")


if __name__ == "__main__":
    main()
