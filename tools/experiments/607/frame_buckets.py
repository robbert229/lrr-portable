# SPDX-License-Identifier: LGPL-3.0-or-later
"""MSVC 5.0 /Od frame layout of locals by name (issue #607, finding 587).

Without optimisation MSVC 5.0 gives each local of a block a frame slot in
descending order of a 16-way hash bucket of its name, lowest address first;
locals in the same bucket keep their declaration order. The bucket function
is not identified (for one-character names it is ord(c) % 16), so this
measures it: a probe function declares a "ladder" of 16 one-letter locals,
one per bucket, then the candidates, and each candidate's slot follows its
bucket's ladder letter.

    python3 tools/experiments/607/frame_buckets.py NAME [NAME ...]
    python3 tools/experiments/607/frame_buckets.py --order a,b,c

The first form prints each name's bucket; the second compiles one block with
the locals declared in the given order and prints their frame order, lowest
address first. Compiles through ../486/msvc.py (wine, or distrobox
`developer`) with the C-unit flags plus /Od.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "486"))
import msvc  # noqa: E402

LADDER = list("pabcdefghijklmno")  # ord(c) % 16 == 0 .. 15


def _probe(path: Path, names: list[str]) -> None:
    lines = ["void sink(void*);", "void probe(void)", "{"]
    lines += ["    int %s;" % n for n in names]
    lines += ["    sink(&%s);" % n for n in names]
    lines += ["}", ""]
    path.write_text("\n".join(lines))


def frame_order(names: list[str]) -> list[str]:
    """The locals' frame order, lowest address first."""
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        _probe(work / "probe.c", names)
        msvc.compile_many(work, ["probe"], extra=["/Od", "/Faprobe.asm"])
        listing = work / "probe.asm"
        if not listing.exists():
            raise SystemExit((work / "probe.log").read_text(errors="replace"))
        rows = []
        for line in listing.read_text(errors="replace").splitlines():
            m = re.match(r"_(\w+)\$\d* = (-?\d+)", line)
            if m:
                rows.append((int(m.group(2)), m.group(1)))
    return [n for _, n in sorted(rows)]


def buckets(names: list[str]) -> dict[str, int]:
    cands = [n for n in dict.fromkeys(names) if n not in LADDER]
    result: dict[str, int] = {}
    current = None
    for n in frame_order(LADDER + cands):
        if n in LADDER:
            current = ord(n) % 16
        else:
            result[n] = current
    for n in names:
        if n in LADDER:
            result[n] = ord(n) % 16
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("names", nargs="*")
    ap.add_argument("--order", help="comma-separated declaration order")
    args = ap.parse_args()
    if args.order:
        print(" ".join(frame_order(args.order.split(","))))
        return 0
    for n, b in buckets(args.names).items():
        print(f"{b:2d} {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
