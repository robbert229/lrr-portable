#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Read and edit the link-order list, ``src/link_order.cmake`` (ADR 0006).

The list is CMake code, but each entry is one line of blank-separated
fields, so this module reads it with the standard library only:

    lrr_link(<kind> <key> <library> <source> [<platform>])

    python3 tools/link_order.py                 # print the entries
    python3 tools/link_order.py --platform winapi --sources
                                                # the matching build's sources

``insert_unit`` is how ``convert_unit.py`` wires a new unit into the list.
The static and map checks of the list are issue #438.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIST = "src/link_order.cmake"
RANGES = "docs/coverage/translation-unit-ranges.csv"
KINDS = ("unit", "legacy", "tail")
LIBRARIES = ("game", "gods98", "platform_glue", "program")
PLATFORMS = ("native", "winapi")
MATCHING_PLATFORM = "winapi"

_ENTRY = re.compile(r"^\s*lrr_link\(([^)#]*)\)\s*(?:#.*)?$")
_LIBS = re.compile(r"^\s*set\(LRR_LINK_LIBRARIES\b([^)]*)\)", re.M)


@dataclass(frozen=True)
class Entry:
    kind: str
    key: str            # unit id, median address (hex) or "-"
    library: str
    source: str         # relative to src/
    platform: str | None
    line: int           # 0-based line number in the list

    @property
    def path(self) -> str:
        """The source's path relative to the repository root."""
        return f"src/{self.source}"

    def applies_to(self, platform: str | None) -> bool:
        return platform is None or self.platform in (None, platform)


def parse(text: str) -> list[Entry]:
    entries = []
    for number, line in enumerate(text.splitlines()):
        m = _ENTRY.match(line)
        if not m:
            if re.match(r"^\s*lrr_link\b", line):
                raise ValueError(f"{LIST}:{number + 1}: malformed entry: {line.strip()}")
            continue
        fields = m.group(1).split()
        if len(fields) not in (4, 5):
            raise ValueError(f"{LIST}:{number + 1}: expected 4 or 5 fields, got {len(fields)}")
        kind, key, library, source = fields[:4]
        platform = fields[4] if len(fields) == 5 else None
        if kind not in KINDS:
            raise ValueError(f"{LIST}:{number + 1}: unknown kind {kind!r}")
        if library not in LIBRARIES:
            raise ValueError(f"{LIST}:{number + 1}: unknown library {library!r}")
        if platform is not None and platform not in PLATFORMS:
            raise ValueError(f"{LIST}:{number + 1}: unknown platform {platform!r}")
        entries.append(Entry(kind, key, library, source, platform, number))
    return entries


def load(root: Path = ROOT) -> list[Entry]:
    return parse((Path(root) / LIST).read_text(encoding="utf-8"))


def libraries(text: str) -> list[str]:
    """The matching build's link libraries, in order."""
    m = _LIBS.search(text)
    return m.group(1).split() if m else []


def sources(entries: list[Entry], platform: str | None = MATCHING_PLATFORM) -> list[str]:
    """Repository-relative sources in link order (all platforms if None)."""
    return [e.path for e in entries if e.applies_to(platform)]


def unit_starts(root: Path = ROOT) -> dict[str, int]:
    with (Path(root) / RANGES).open(newline="", encoding="utf-8") as fh:
        return {r["unit_id"]: int(r["text_start"], 16) for r in csv.DictReader(fh)}


def entry_address(entry: Entry, starts: dict[str, int]) -> int | None:
    """The original address that places an entry: its unit start or median."""
    if entry.kind == "unit":
        return starts.get(entry.key)
    if entry.kind == "legacy":
        return int(entry.key, 16)
    return None


def format_entry(kind: str, key: str, library: str, source: str,
                 platform: str | None = None) -> str:
    fields = f"{kind:<6} {key:<8} {library:<13} {source}"
    return f"lrr_link({fields}{' ' + platform if platform else ''})"


def insert_unit(text: str, unit_id: str, library: str, source: str,
                starts: dict[str, int]) -> str:
    """Add a unit's entry in its original position; no-op if it is listed.

    The entry goes after the last unit or legacy entry before the port tail
    whose address is not above the unit's start, so it lands among the game
    and gods98 segments, never in the tail or the CRT-range legacy files.
    """
    entries = parse(text)
    for e in entries:
        if e.source == source:
            if e.kind == "unit" and e.key == unit_id and e.library == library:
                return text
            raise ValueError(f"{source} is already listed as {e.kind} {e.key}")
        if e.kind == "unit" and e.key == unit_id:
            raise ValueError(f"{unit_id} is already listed as {e.source}")
    if unit_id not in starts:
        raise ValueError(f"{unit_id} is not in {RANGES}")
    start = starts[unit_id]
    after = None
    for e in entries:
        if e.kind == "tail":
            break
        address = entry_address(e, starts)
        if address is not None and address <= start:
            after = e
    lines = text.splitlines(keepends=True)
    new = format_entry("unit", unit_id, library, source) + "\n"
    if after is None:
        first = next(e for e in entries if e.kind != "tail")
        lines.insert(first.line, new)
    else:
        lines.insert(after.line + 1, new)
    return "".join(lines)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", type=Path, default=ROOT)
    ap.add_argument("--platform", choices=PLATFORMS,
                    help="only the entries this backend builds")
    ap.add_argument("--sources", action="store_true",
                    help="print repository-relative sources only")
    args = ap.parse_args(argv)
    entries = [e for e in load(args.root) if e.applies_to(args.platform)]
    for e in entries:
        if args.sources:
            print(e.path)
        else:
            print(f"{e.kind:<6} {e.key:<8} {e.library:<13} {e.source}"
                  f"{' ' + e.platform if e.platform else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
