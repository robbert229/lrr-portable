#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Render lrr-portable.svg into the macOS bundle's AppIcon.icns.

The Linux packages install the SVG as it is; only the macOS bundle needs the
raster ICNS, which is committed so that no build needs ImageMagick. Run this
after changing the SVG:

    python3 packaging/icons/regenerate.py

It needs ImageMagick (`magick`, or `convert`) with SVG support. The ICNS holds
PNG images at 128, 256, 512 and 1024 pixels (OSTypes ic07, ic08, ic09, ic10).
"""
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


def render(png: Path, size: int) -> bytes:
    magick = shutil.which("magick") or shutil.which("convert")
    if not magick:
        sys.exit("regenerate.py: ImageMagick (magick or convert) is required")
    subprocess.run(
        [magick, "-background", "none", "-density", str(size * 3 // 2), str(SVG),
         "-resize", f"{size}x{size}", "-strip", f"PNG32:{png}"],
        check=True,
    )
    return png.read_bytes()


def icns(images):
    body = b"".join(
        kind.encode("ascii") + struct.pack(">I", 8 + len(data)) + data
        for kind, data in images
    )
    return b"icns" + struct.pack(">I", 8 + len(body)) + body


def main():
    with tempfile.TemporaryDirectory() as tmp:
        images = [(kind, render(Path(tmp) / f"{size}.png", size)) for kind, size in SIZES]
    ICNS.parent.mkdir(parents=True, exist_ok=True)
    ICNS.write_bytes(icns(images))
    print(f"wrote {ICNS.relative_to(HERE.parent.parent)} ({ICNS.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
