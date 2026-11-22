#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Render lrr-portable.svg into the macOS bundle's AppIcon.icns and the
Android launcher icons.

The Linux packages install the SVG as it is; only the macOS bundle and the
Android app need raster images, which are committed so that no build needs
ImageMagick. Run this after changing the SVG:

    python3 packaging/icons/regenerate.py

It needs ImageMagick (`magick`, or `convert`) with SVG support. The ICNS holds
PNG images at 128, 256, 512 and 1024 pixels (OSTypes ic07, ic08, ic09, ic10).
Android gets, for each screen density from mdpi to xxxhdpi, the whole tile as
the legacy launcher icon (48 dp, for Android 7.1 and earlier) and the artwork
without its tile as the adaptive icon's foreground (108 dp, the artwork in
the middle 72 dp); the adaptive background is the tile's gradient, drawn by
res/drawable/ic_launcher_background.xml.
"""
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SVG = HERE / "lrr-portable.svg"
ICNS = HERE.parent / "macos" / "AppIcon.icns"
SIZES = [("ic07", 128), ("ic08", 256), ("ic09", 512), ("ic10", 1024)]
ANDROID_RES = HERE.parent / "android" / "app" / "src" / "main" / "res"
# Pixels per dp for each density bucket.
DENSITIES = [("mdpi", 1), ("hdpi", 1.5), ("xhdpi", 2), ("xxhdpi", 3), ("xxxhdpi", 4)]
# The tile behind the artwork: the first <rect> that fills the view box.
TILE = re.compile(r'<rect width="512" height="512"[^>]*/>')


def render(png: Path, size: int, svg: Path = SVG, canvas: int = 0) -> bytes:
    """Renders `svg` at `size` pixels, centred on a transparent `canvas`."""
    magick = shutil.which("magick") or shutil.which("convert")
    if not magick:
        sys.exit("regenerate.py: ImageMagick (magick or convert) is required")
    canvas = canvas or size
    subprocess.run(
        [magick, "-background", "none", "-density", str(size * 3 // 2), str(svg),
         "-resize", f"{size}x{size}", "-gravity", "center", "-extent", f"{canvas}x{canvas}",
         "-strip", "-define", "png:exclude-chunks=date,time", f"PNG32:{png}"],
        check=True,
    )
    return png.read_bytes()


def android(tmp: Path):
    foreground = tmp / "foreground.svg"
    text = SVG.read_text()
    if not TILE.search(text):
        sys.exit("regenerate.py: the SVG's tile <rect> was not found")
    foreground.write_text(TILE.sub("", text, count=1))
    for name, scale in DENSITIES:
        out = ANDROID_RES / f"mipmap-{name}"
        out.mkdir(parents=True, exist_ok=True)
        render(out / "ic_launcher.png", round(48 * scale))
        render(out / "ic_launcher_foreground.png", round(72 * scale), foreground, round(108 * scale))
        print(f"wrote {out.relative_to(HERE.parent.parent)}/ic_launcher*.png")


def icns(images):
    body = b"".join(
        kind.encode("ascii") + struct.pack(">I", 8 + len(data)) + data
        for kind, data in images
    )
    return b"icns" + struct.pack(">I", 8 + len(body)) + body


def main():
    with tempfile.TemporaryDirectory() as tmp:
        images = [(kind, render(Path(tmp) / f"{size}.png", size)) for kind, size in SIZES]
        android(Path(tmp))
    ICNS.parent.mkdir(parents=True, exist_ok=True)
    ICNS.write_bytes(icns(images))
    print(f"wrote {ICNS.relative_to(HERE.parent.parent)} ({ICNS.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
