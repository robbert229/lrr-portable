#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Copy a known old portable .sav into the retail numeric-header format.

Some early portable builds wrote music at +0x10 without a GOPS marker.
Those files cannot be distinguished from retail saves with muted music.
Use this only for a file known to come from that writer. Brightness was
not stored, so supply its desired value explicitly. The input is never
rewritten, the destination must not exist, and all other bytes are retained.
GOPS-marked saves are migrated by the game loader and need no conversion.
"""
import argparse
from pathlib import Path
import struct


def convert(data, brightness):
    if not 0 <= brightness <= 10:
        raise ValueError("brightness must be between 0 and 10")
    if len(data) < 0xb0:
        raise ValueError("truncated save header")
    music_word, sfx, speed, old_music = struct.unpack_from("<4i", data, 4)
    count = struct.unpack_from("<i", data, 0x18)[0]
    if not 1 <= count <= 0x1000 or len(data) < 0xb0 + count * 0x3190:
        raise ValueError("invalid or truncated record payload")
    if data[0x20:0x24] == b"GOPS":
        raise ValueError("GOPS saves already have an automatic loader migration")
    if music_word != 0 or not 0 <= old_music <= 10 or not 0 <= sfx <= 10 or not 0 <= speed <= 5:
        raise ValueError("header does not match the old portable writer")
    result = bytearray(data)
    struct.pack_into("<i", result, 4, old_music)
    struct.pack_into("<i", result, 0x10, brightness)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-music-header", action="store_true", required=True,
                        help="confirm that the input came from the old portable writer")
    parser.add_argument("--brightness", type=int, required=True)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    try:
        converted = convert(args.source.read_bytes(), args.brightness)
        with args.destination.open("xb") as output:
            output.write(converted)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
