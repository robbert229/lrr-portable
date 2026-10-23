#!/usr/bin/env python3
"""Unpack Lego Rock Raiders .wad archives (LegoRR0.wad / LegoRR1.wad).

Format (see docs/findings/01-binary-and-loader-map.md):
    "WWAD" | u32 count | count*NUL names | count*NUL dev-paths | count*16B TOC | data
    TOC entry: flags u32 | orig_len u32 | stored_len u32 | file_offset u32

Usage: python3 tools/unwad.py reference/LegoRR0.wad [output_dir]
       python3 tools/unwad.py reference/*.wad --stats   # inventory only, no extraction
"""
import argparse
import os
import struct
import sys
from collections import Counter


def parse(path):
    def read_string_table(f, n):
        items = []
        buf = b""
        while len(items) < n:
            chunk = f.read(1 << 16)
            if not chunk:
                raise ValueError("short read in string table")
            buf += chunk
            while True:
                z = buf.find(b"\x00")
                if z < 0:
                    break
                items.append(buf[:z].decode("latin1"))
                buf = buf[z + 1:]
                if len(items) == n:
                    break
        f.seek(-len(buf), 1)  # rewind past the table terminator
        return items

    with open(path, "rb") as f:
        header = f.read(8)
        if header[:4] != b"WWAD":
            raise ValueError(f"{path}: bad magic {header[:4]!r}")
        count = struct.unpack("<I", header[4:8])[0]
        names = read_string_table(f, count)
        read_string_table(f, count)  # developer build paths, unused at runtime
        off = f.tell()

        entries = []
        for i in range(count):
            f.seek(off)
            flags, orig, stored, data_off = struct.unpack("<IIII", f.read(16))
            entries.append((names[i], flags, orig, stored, data_off))
            off += 16
    return entries


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wads", nargs="+")
    ap.add_argument("--stats", action="store_true", help="print inventory only")
    ap.add_argument("--out", default="unpacked", help="output root directory (default: unpacked)")
    args = ap.parse_args()

    for path in args.wads:
        entries = parse(path)
        print(f"== {path}: {len(entries)} entries")
        if args.stats:
            tags = Counter()
            total = 0
            with open(path, "rb") as f:
                for name, flags, orig, stored, off in entries:
                    f.seek(off)
                    tag = f.read(4)
                    tags[tag] += 1
                    total += stored
            for tag, n in tags.most_common(15):
                print(f"   {tag!r:12} {n}")
            print(f"   payload bytes: {total}")
            continue

        outroot = os.path.join(args.out, os.path.splitext(os.path.basename(path))[0])
        with open(path, "rb") as f:
            for name, flags, orig, stored, off in entries:
                dest = os.path.join(outroot, name)
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                f.seek(off)
                data = f.read(stored)
                with open(dest, "wb") as out:
                    out.write(data)
        print(f"   wrote {len(entries)} files to {outroot}/")


if __name__ == "__main__":
    main()
