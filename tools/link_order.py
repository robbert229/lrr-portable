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
The check command validates the list and check-map compares .text object runs.
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
# Every build but the MSVC 5.0 matching build (src/crt/'s stand-ins, #447).
PORTABLE = "portable"
MATCHING_PLATFORM = "winapi"

_ENTRY = re.compile(r"^\s*lrr_link\(([^)#]*)\)\s*(?:#.*)?$")
_LIBS = re.compile(r"^\s*set\(LRR_LINK_LIBRARIES\b([^)]*)\)", re.M)
_LATE = re.compile(r"^\s*set\(LRR_LINK_LATE_MEMBERS\b([^)]*)\)", re.M)


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
        """Whether the entry is built for ``platform`` (None: any build).

        "winapi" stands for the matching build here; a "portable" entry is
        built natively and by the MinGW reference build, never by it."""
        if platform is None or self.platform is None:
            return True
        if self.platform == PORTABLE:
            return platform != MATCHING_PLATFORM
        return self.platform == platform


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
        if platform is not None and platform not in PLATFORMS + (PORTABLE,):
            raise ValueError(f"{LIST}:{number + 1}: unknown platform {platform!r}")
        entries.append(Entry(kind, key, library, source, platform, number))
    return entries


def load(root: Path = ROOT) -> list[Entry]:
    return parse((Path(root) / LIST).read_text(encoding="utf-8"))


def libraries(text: str) -> list[str]:
    """The matching build's link libraries, in order."""
    m = _LIBS.search(text)
    return m.group(1).split() if m else []


def late_members(text: str) -> list[str]:
    """Sources the matching build links from their own libraries, after
    the libraries (``LRR_LINK_LATE_MEMBERS``; finding 657), in order."""
    m = _LATE.search(text)
    return m.group(1).split() if m else []


def late_library(source: str) -> str:
    """The library a late member is archived in: ``late_<stem>``, as
    CMakeLists.txt names it (the stem drops every extension)."""
    return "late_" + Path(source).name.split(".")[0]


def late_object(source: str) -> str:
    """A late member's object name as the link map writes it."""
    return f"{late_library(source)}:{Path(source).name}.obj"


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
                starts: dict[str, int], crt: int | None = None) -> str:
    """Add a unit's entry in its original position; no-op if it is listed.

    The entry goes after the last unit or legacy entry before the port tail
    whose address is not above the unit's start, so it lands among the game
    and gods98 segments, never in the tail or the CRT-range legacy files.
    Given ``crt`` (the CRT segment's start), a unit that starts at or above
    it is a late unit and goes among the CRT-range entries after the tail.
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
    if crt is not None and start >= crt:
        # A late unit (after the CRT segment's start) sits among the
        # CRT-range entries that follow the port tail.
        tails = [e for e in entries if e.kind == "tail"]
        for e in entries:
            if tails and e.line <= tails[-1].line:
                continue
            address = entry_address(e, starts)
            if address is not None and address <= start:
                after = e
        if after is None:
            after = tails[-1] if tails else None
    else:
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



MARKER = re.compile(r"^\s*//\s*(?:FUNCTION|LIBRARY):\s*LEGORR\s+0x([0-9a-f]+)", re.I | re.M)


def check(root: Path = ROOT) -> list[str]:
    """Validate the list against source annotations and the unit specification."""
    root = Path(root)
    text = (root / LIST).read_text(encoding="utf-8")
    entries = parse(text)
    with (root / "docs/coverage/translation-units.spec.csv").open(newline="", encoding="utf-8") as f:
        spec = {r["unit_id"]: r for r in csv.DictReader(f)}
    with (root / RANGES).open(newline="", encoding="utf-8") as f:
        ranges = {r["unit_id"]: (int(r["text_start"], 16), int(r["text_end"], 16)) for r in csv.DictReader(f)}
    g98 = min(int(r["start"], 16) for r in spec.values() if r["kind"] == "gods98")
    crt = min(int(r["start"], 16) for r in spec.values() if r["kind"] == "crt")
    errors, seen_paths, seen_units = [], set(), set()
    phase, previous = 0, -1
    for e in entries:
        where = f"{LIST}:{e.line + 1} ({e.source})"
        if e.source in seen_paths:
            errors.append(f"{where}: source is listed twice; remove the duplicate")
        seen_paths.add(e.source)
        path = root / e.path
        if not path.is_file():
            errors.append(f"{where}: source file does not exist")
            continue
        addresses = sorted(int(a, 16) for a in MARKER.findall(path.read_text(encoding="utf-8")))
        address = None
        if e.kind == "unit":
            if e.key in seen_units:
                errors.append(f"{where}: unit {e.key} is listed twice; remove the duplicate")
            seen_units.add(e.key)
            if e.key not in spec or spec[e.key]["kind"] not in ("game", "gods98", "late") or e.key not in ranges:
                errors.append(f"{where}: {e.key} is not a game, gods98 or late unit in the spec and ranges")
            else:
                lo, hi = ranges[e.key]
                address = lo
                for a in addresses:
                    if not lo <= a < hi:
                        errors.append(f"{where}: marker 0x{a:08x} is outside {e.key} range 0x{lo:08x}..0x{hi:08x}; move it to its unit")
        elif e.kind == "legacy":
            if not addresses:
                errors.append(f"{where}: legacy file has no annotated functions; move it to the tail")
            else:
                median = addresses[(len(addresses) - 1) // 2]
                try:
                    address = int(e.key, 16)
                except ValueError:
                    errors.append(f"{where}: invalid median {e.key!r}; set it to {median:08x}")
                if address is not None and address != median:
                    errors.append(f"{where}: stale median {e.key}; set it to {median:08x} and move the entry into order")
        elif e.key != "-":
            errors.append(f"{where}: tail key must be -")
        part = 2 if e.kind == "tail" else (3 if address is not None and address >= crt else 1 if address is not None and address >= g98 else 0)
        if part < phase:
            errors.append(f"{where}: entry is in the wrong segment; order game, gods98, tail, CRT")
        if part != phase:
            previous = -1
        phase = max(phase, part)
        if address is not None:
            if address < previous:
                errors.append(f"{where}: address {address:08x} is out of order; move entry after the correct predecessor")
            previous = address
    by_source = {e.source: e for e in entries}
    for source in late_members(text):
        e = by_source.get(source)
        if e is None or e.kind != "unit" or not e.applies_to(MATCHING_PLATFORM):
            errors.append(f"{LIST}: late member {source} is not a unit entry of the matching build")
        elif spec.get(e.key, {}).get("kind") != "late":
            errors.append(f"{LIST}: late member {source} ({e.key}) is not a late unit in the spec")
    for directory in ("game", "gods98"):
        for path in (root / "src" / directory).rglob("*"):
            if path.suffix.lower() in (".c", ".cpp") and not path.name.startswith("test_"):
                relative = path.relative_to(root / "src").as_posix()
                if relative not in seen_paths:
                    errors.append(f"{relative}: source is missing from {LIST}; add an entry")
    return errors


def check_map(entries: list[Entry], text: str) -> list[str]:
    """Compare .text symbol contribution runs to explicit objects in list order.

    Maps omit empty objects and identify objects by basename only. The
    subsequence comparison handles both omissions and adjacent name collisions.
    """
    sections = set(re.findall(r"^\s*([0-9a-fA-F]{4}):[0-9a-fA-F]+\s+[0-9a-fA-F]+H\s+\.text\s", text, re.M))
    if not sections:
        return ["map has no .text section table entry"]
    symbols = []
    pattern = re.compile(r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+\S+\s+[0-9a-fA-F]{8}\s+(?:[fi]\s+)*(\S+\.obj)\s*$", re.I | re.M)
    for m in pattern.finditer(text):
        if m.group(1) in sections:
            symbols.append((int(m.group(2), 16), m.group(3).lower()))
    if not symbols:
        return ["map has no .text object symbols; check the map format"]
    runs = []
    for _, obj in sorted(symbols):
        if not runs or runs[-1] != obj:
            runs.append(obj)
    expected = [Path(e.source).name.lower() + ".obj" for e in entries if e.applies_to(MATCHING_PLATFORM)]
    index, library_seen, errors = 0, False, []
    for obj in runs:
        if ":" in obj:
            library_seen = True
            continue
        if library_seen:
            errors.append(f"map: explicit object {obj} contributes after a library; check link order")
        try:
            index = expected.index(obj, index) + 1
        except ValueError:
            errors.append(f"map: unexpected or out-of-order .text contribution {obj}; compare with {LIST}")
    return errors
def lower_median(text: str) -> int | None:
    """The key ``check`` expects for a legacy file: the lower median of its
    FUNCTION and LIBRARY marker addresses, or None when it has none."""
    addresses = sorted(int(a, 16) for a in MARKER.findall(text))
    return addresses[(len(addresses) - 1) // 2] if addresses else None


def crt_start(root: Path = ROOT) -> int:
    """First address of the CRT segment (translation-units.spec.csv)."""
    with (Path(root) / "docs/coverage/translation-units.spec.csv").open(newline="", encoding="utf-8") as f:
        return min(int(r["start"], 16) for r in csv.DictReader(f) if r["kind"] == "crt")


def place_legacy(text: str, source: str, median: int, starts: dict[str, int], crt: int) -> str:
    """Give a legacy entry the key ``median`` and move it into address order.

    Entries below the CRT segment sit among the game and gods98 entries,
    before the port tail; CRT-range entries sit after the tail. A file that
    is not a legacy entry is left alone.
    """
    entries = parse(text)
    entry = next((e for e in entries if e.source == source), None)
    if entry is None or entry.kind != "legacy":
        return text
    lines = text.splitlines(keepends=True)
    del lines[entry.line]
    rest = parse("".join(lines))
    new = format_entry("legacy", f"{median:08x}", entry.library, entry.source, entry.platform) + "\n"
    tail = [e for e in rest if e.kind == "tail"]
    if median >= crt:
        region = [e for e in rest if e.kind != "tail" and tail and e.line > tail[-1].line]
        floor = tail[-1].line + 1 if tail else len(lines)
    else:
        region = [e for e in rest if e.kind != "tail" and (not tail or e.line < tail[0].line)]
        floor = region[0].line if region else 0
    at = floor
    for e in region:
        address = entry_address(e, starts)
        if address is not None and address <= median:
            at = e.line + 1
    lines.insert(at, new)
    return "".join(lines)


def retire_legacy(text: str, source: str) -> str:
    """Turn a drained legacy file into a port-tail entry (ADR 0006 decision 4).

    A legacy file whose original functions have all been relocated holds only
    port-only code. Its entry becomes ``tail -`` and moves after the last tail
    entry. A file that is not a legacy entry is left alone.
    """
    entries = parse(text)
    entry = next((e for e in entries if e.source == source), None)
    if entry is None or entry.kind != "legacy":
        return text
    tails = [e for e in entries if e.kind == "tail"]
    if not tails:
        raise ValueError("the list has no port tail to move a drained legacy file into")
    lines = text.splitlines(keepends=True)
    new = format_entry("tail", "-", entry.library, entry.source, entry.platform) + "\n"
    at = tails[-1].line + 1
    lines.insert(at, new)
    del lines[entry.line if entry.line < at else entry.line + 1]
    return "".join(lines)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", type=Path, default=ROOT)
    ap.add_argument("--platform", choices=PLATFORMS,
                    help="only the entries this backend builds")
    ap.add_argument("--sources", action="store_true",
                    help="print repository-relative sources only")
    ap.add_argument("command", nargs="?", choices=("check", "check-map"))
    ap.add_argument("mapfile", nargs="?", type=Path)
    args = ap.parse_args(argv)
    if args.command:
        try:
            if args.command == "check":
                errors = check(args.root)
            elif args.mapfile:
                errors = check_map(load(args.root), args.mapfile.read_text(encoding="utf-8", errors="replace"))
            else:
                ap.error("check-map requires a map file")
        except (OSError, ValueError) as exc:
            print(exc, file=sys.stderr)
            return 1
        for error in errors:
            print(error, file=sys.stderr)
        if not errors:
            print(f"link order {args.command}: OK")
        return bool(errors)
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
