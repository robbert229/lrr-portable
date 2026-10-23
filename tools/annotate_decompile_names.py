#!/usr/bin/env python3
"""Annotate full-corpus function headings with names from the coverage ledger.

The Ghidra export remains the source for pseudocode and original symbols. This
script only adds ledger-backed human names to LEGORR.full.c section headings,
retaining the original Ghidra name in the heading when it differs.

Run tools/coverage_ledger.py first to refresh docs/coverage/ledger.csv.
"""

import csv
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parent.parent
LEDGER = ROOT / "docs/coverage/ledger.csv"
CORPUS = ROOT / "docs/decompiles/full/LEGORR.full.c"
HEADER = re.compile(
    r"^// ==== (?P<label>.+?) @ (?P<address>[0-9A-Fa-f]{8})"
    r"(?: \[Ghidra: (?P<ghidra>.*?)\])? ====$"
)


def read_names(path: pathlib.Path) -> dict[str, str]:
    names = {}
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            address = row["address"].lower().zfill(8)
            if address in names:
                raise ValueError(f"duplicate ledger address: {address}")
            names[address] = row.get("corpus_name", "")
    return names


def annotate(text: str, names: dict[str, str]) -> tuple[str, int, int, int]:
    output = []
    seen = set()
    labeled = 0
    changed = 0
    for line in text.splitlines(keepends=True):
        match = HEADER.match(line.rstrip("\r\n"))
        if match is None:
            output.append(line)
            continue

        address = match.group("address").lower().zfill(8)
        seen.add(address)
        original = match.group("ghidra") or match.group("label")
        human = names.get(address, "")
        if human and human != original:
            labeled += 1
            ending = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
            annotated = (f"// ==== {human} @ {match.group('address').upper()} "
                         f"[Ghidra: {original}] ===={ending}")
            if line != annotated:
                line = annotated
                changed += 1
        output.append(line)

    if seen != set(names):
        missing = sorted(set(names) - seen)
        extra = sorted(seen - set(names))
        raise ValueError(
            f"ledger/corpus function mismatch: {len(missing)} ledger-only, "
            f"{len(extra)} corpus-only; first ledger-only={missing[:3]}, "
            f"first corpus-only={extra[:3]}"
        )
    return "".join(output), len(seen), labeled, changed


def main() -> int:
    names = read_names(LEDGER)
    original = CORPUS.read_text(encoding="utf-8")
    updated, total, labeled, changed = annotate(original, names)
    if updated == original:
        print(f"Already annotated: {labeled} named headings across {total} functions")
        return 0

    temp = CORPUS.with_suffix(CORPUS.suffix + ".tmp")
    temp.write_text(updated, encoding="utf-8", newline="\n")
    temp.replace(CORPUS)
    print(f"Added {changed} names to function headings ({labeled} named headings; "
          f"{total} functions indexed) in {CORPUS}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
