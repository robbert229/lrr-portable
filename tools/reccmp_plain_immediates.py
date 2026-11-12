#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Keep the original's plain immediates plain on the rebuilt side (#604).

Deviation (scoring only): ADR 0005 entropy rule 4, finding 584.

The original image ends at 0x771000, so an operand such as the entity flag
0x800000 is a plain constant there. The rebuilt image is larger, and its
.bss can cover the same value. Reccmp then names the rebuilt operand after
whatever global covers it (``s_entity_path_motion_runtime[802]...``), and
every later ``<OFFSETn>`` placeholder shifts by one. Byte-identical functions
then score below 100%.

This module changes neither Reccmp's package nor its scoring. It wraps one
FunctionComparator's two inputs at run time, per compared function:

1. after the original side is sanitised, it collects the hexadecimal values
   left in the original listing that lie outside the original image (at or
   past its end), so the original cannot have meant them as addresses;
2. while the rebuilt side of the same function is sanitised, Reccmp's
   address test and name lookup answer "not an address" for exactly those
   values, so the rebuilt operand stays the same plain constant.

Any other rebuilt operand, including a rebuilt pointer past 0x771000 whose
original counterpart is a different (in-image) address, is named as before.
Storage is untouched.

``check`` is the guard: it reads a Reccmp JSON report and fails when a rebuilt
listing still names an operand that the paired original line keeps as a
plain constant outside the original image. ``make reccmp-compare`` (and so
``make reccmp-check``) runs it.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path
from typing import Callable, Iterable

HEX_RE = re.compile(r"\b0x([0-9a-f]+)\b")
U32_MAX = 0xFFFFFFFF

# Applied to FunctionComparator classes, so repeated installs are no-ops.
_INSTALLED = "_lrr_plain_immediates_installed"


def image_end(image) -> int:
    """First virtual address past the image: ImageBase + SizeOfImage, or the
    end of its last section when the header is not available."""
    base = image.imagebase
    header = getattr(image, "optional_header", None)
    size = getattr(header, "size_of_image", None)
    if size:
        return base + size
    return max(section.virtual_range.stop for section in image.sections)


def plain_constants(listing: Iterable, end: int) -> frozenset[int]:
    """Hex values in a sanitised listing (``(addr, text)`` pairs) at or past
    ``end``: operands the original side left as plain numbers."""
    found = set()
    for _addr, text in listing:
        if not text:
            continue
        for match in HEX_RE.finditer(text):
            value = int(match.group(1), 16)
            if end <= value <= U32_MAX:
                found.add(value)
    return frozenset(found)


class _PlainImmediates:
    """Per-comparator state: the original listing's plain constants for the
    function now being compared."""

    def __init__(self, orig_end: int) -> None:
        self.orig_end = orig_end
        self.constants: frozenset[int] = frozenset()

    def wrap_orig_parse(self, parse: Callable) -> Callable:
        def parse_orig(data, start_addr):
            listing = parse(data, start_addr)
            self.constants = plain_constants(listing, self.orig_end)
            return listing

        return parse_orig

    def wrap_recomp_parse(self, parse: Callable) -> Callable:
        def parse_recomp(data, start_addr):
            try:
                return parse(data, start_addr)
            finally:
                # One function's constants never leak into the next.
                self.constants = frozenset()

        return parse_recomp

    def wrap_addr_test(self, addr_test: Callable | None) -> Callable:
        def is_addr(value: int) -> bool:
            if value in self.constants:
                return False
            return bool(addr_test(value)) if callable(addr_test) else False

        return is_addr

    def wrap_name_lookup(self, name_lookup: Callable | None) -> Callable:
        def lookup(addr: int, exact: bool = False, indirect: bool = False):
            if addr in self.constants:
                return None
            if callable(name_lookup):
                return name_lookup(addr, exact=exact, indirect=indirect)
            return None

        return lookup


def attach(comparator) -> None:
    """Wrap one constructed FunctionComparator (``orig_bin``,
    ``orig_sanitize`` and ``recomp_sanitize`` as in Reccmp)."""
    state = _PlainImmediates(image_end(comparator.orig_bin))
    orig = comparator.orig_sanitize
    recomp = comparator.recomp_sanitize
    orig.parse_asm = state.wrap_orig_parse(orig.parse_asm)
    recomp.parse_asm = state.wrap_recomp_parse(recomp.parse_asm)
    recomp.addr_test = state.wrap_addr_test(recomp.addr_test)
    recomp.name_lookup = state.wrap_name_lookup(recomp.name_lookup)
    comparator._lrr_plain_immediates = state


def install(comparator_cls=None) -> None:
    """Attach to every FunctionComparator constructed from now on."""
    if comparator_cls is None:
        from reccmp.compare.functions import FunctionComparator as comparator_cls
    if getattr(comparator_cls, _INSTALLED, False):
        return
    post_init = comparator_cls.__post_init__

    def __post_init__(self):
        post_init(self)
        attach(self)

    comparator_cls.__post_init__ = __post_init__
    setattr(comparator_cls, _INSTALLED, True)


def pe_image_range(path: Path) -> tuple[int, int]:
    """(ImageBase, ImageBase + SizeOfImage) from a PE32 header."""
    data = Path(path).read_bytes()[:4096]
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    optional = pe + 24
    base = struct.unpack_from("<I", data, optional + 28)[0]
    size = struct.unpack_from("<I", data, optional + 56)[0]
    return base, base + size


# What Reccmp puts in place of an address: ``name (OFFSET)``, ``name (DATA)``,
# ``"text" (STRING)`` or a ``<OFFSETn>`` placeholder.
_RECCMP_NAME = re.compile(r"(<OFFSET\d*>|\([A-Z_]+\))$")
_SOURCE_REF = re.compile(r"\s*\t\(.*\)\s*$")


def _instruction(text: str) -> str:
    return _SOURCE_REF.sub("", text).strip()


def symbolized_constants(report: dict, orig_end: int) -> list[dict]:
    """Paired diff lines where the original keeps a hex value at or past
    ``orig_end`` and the rebuilt line is the same instruction with that value
    replaced by a Reccmp name or placeholder."""
    found = []
    for entity in report.get("data", []):
        for hunk in entity.get("diff") or []:
            for block in hunk[1]:
                orig_rows = block.get("orig") or []
                recomp_rows = block.get("recomp") or []
                for orig_row, recomp_row in zip(orig_rows, recomp_rows):
                    orig_text = _instruction(orig_row[1])
                    recomp_text = _instruction(recomp_row[1])
                    for match in HEX_RE.finditer(orig_text):
                        value = int(match.group(1), 16)
                        if not orig_end <= value <= U32_MAX:
                            continue
                        pattern = (re.escape(orig_text[:match.start()]) + "(.+)" +
                                   re.escape(orig_text[match.end():]))
                        hit = re.fullmatch(pattern, recomp_text)
                        if hit and _RECCMP_NAME.search(hit.group(1)):
                            found.append({"address": entity.get("address"),
                                          "name": entity.get("name"),
                                          "orig": orig_text, "recomp": recomp_text})
    return found


def check(report_dir: Path, original: Path) -> int:
    _base, orig_end = pe_image_range(original)
    status = 0
    for name in ("results.json", "game-only.json"):
        path = Path(report_dir) / name
        if not path.exists():
            continue
        hits = symbolized_constants(json.loads(path.read_text(encoding="utf-8")), orig_end)
        for hit in hits:
            print(f"{name}: {hit['address']} {hit['name']}: original `{hit['orig']}` "
                  f"is named on the rebuilt side: `{hit['recomp']}`", file=sys.stderr)
        if hits:
            status = 1
    if status:
        print("plain-immediate check FAILED: a rebuilt operand is symbolized where the "
              "original has a constant outside its image (finding 584); is "
              "reccmp_plain_immediates installed in reccmp_full_diff.py?", file=sys.stderr)
    else:
        print(f"plain-immediate check: OK (original image ends at {orig_end:#x})")
    return status


def main(argv: list[str] | None = None) -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    cmd = sub.add_parser("check", help="fail on rebuilt names for original plain constants")
    cmd.add_argument("--report-dir", type=Path, default=root / "build-reccmp/reccmp-report")
    cmd.add_argument("--original", type=Path, default=root / "reference/LegoRR.exe")
    args = parser.parse_args(argv)
    if not args.original.exists():
        print(f"{args.original} does not exist; skipping the plain-immediate check")
        return 0
    return check(args.report_dir, args.original)


if __name__ == "__main__":
    sys.exit(main())
