#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Find x87 operand-order swaps in Reccmp listings (finding 538, rule A).

Rule A: in a float product (or sum) of two named memory operands, MSVC 5.0
emits the operand whose variable was *declared later* first. A function whose
declaration order differs from the original's shows the same two x87
instructions with their memory operands exchanged:

    original            recomp
    fld  [a]            fld  [b]
    fmul [b]            fmul [a]

or two chained operations exchanged (``fmul [a]; fmul [b]`` against
``fmul [b]; fmul [a]``). This tool scans Reccmp's ``results.json`` (or
``match_function.py --json`` output) for such pairs.

A pair counts as a swap when the original listing contains the pair
``(op1 A, op2 B)`` more often than the recomp listing, and the recomp listing
contains the exchanged pair ``(op1 B, op2 A)`` more often than the original.
Only commutative pairs qualify (``fld``/``fmul``, ``fld``/``fadd``,
``fmul``/``fmul``, ``fadd``/``fadd``) plus the reversed non-commutative forms
(``fld A; fsub B`` against ``fld B; fsubr A``, and ``fdiv``/``fdivr``).

Usage:
    python3 tools/x87_operand_swaps.py build-reccmp/reccmp-report/results.json
    python3 tools/x87_operand_swaps.py results.json --json
    python3 tools/match_function.py 0x444460 --json | \\
        python3 tools/x87_operand_swaps.py -

Output is ranked by an *expected gain*: the share of the function's
instructions that the swaps account for (2 per swap, over the larger of the
two listing lengths). That is an estimate, not Reccmp's exact delta.
"""
from __future__ import annotations

import argparse
import collections
import csv
import difflib
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
UNITS_CSV = REPO / "docs" / "coverage" / "translation-units.csv"

_SOURCE_REF = re.compile(r"\s*\t\(.*\)\s*$")
# Reccmp numbers unnamed operands (float literals) per side, so the same
# constant is <OFFSET7> in one listing and <OFFSET11> in the other.
_OFFSET = re.compile(r"<OFFSET\d+>")
_COMMUTATIVE = {"fmul", "fadd"}
_REVERSED = {"fsub": "fsubr", "fsubr": "fsub", "fdiv": "fdivr", "fdivr": "fdiv"}
_LOADS = {"fld", "fild"}


def _strip(text: str) -> tuple[str, str]:
    text = _SOURCE_REF.sub("", text).strip()
    mnemonic, _, operands = text.partition(" ")
    return mnemonic, " ".join(operands.split())


def normalize(text: str) -> tuple[str, str]:
    """Return (mnemonic, operands) with source references removed."""
    mnemonic, operands = _strip(text)
    return mnemonic, _OFFSET.sub("<OFFSET>", operands)


# An operand Reccmp names: a literal placeholder, or a symbol with its kind.
_TOKEN = re.compile(r"<OFFSET\d+>|[^\s\[\],]+ \((?:DATA|OFFSET|GLOBAL)\)")


def _correspondence(pairs) -> dict[str, str]:
    """Map the original side's ``<OFFSETn>`` placeholders to recomp operands.

    The original side names only annotated addresses; an unannotated global
    is ``<OFFSETn>`` there and a symbol name in the recomp listing. Aligned
    instructions that agree apart from such tokens tell which is which, so a
    swap of a named global against a literal stays visible."""
    votes: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    for orig_line, recomp_line in pairs:
        (mo, oo), (mr, orr) = orig_line, recomp_line
        if mo != mr:
            continue
        to, tr = _TOKEN.findall(oo), _TOKEN.findall(orr)
        if len(to) != len(tr) or _TOKEN.sub("@", oo) != _TOKEN.sub("@", orr):
            continue
        for x, y in zip(to, tr):
            if x.startswith("<OFFSET"):
                votes[x][y] += 1
    mapping = {}
    for x, counter in votes.items():
        (y, n), = counter.most_common(1)
        if n * 2 > sum(counter.values()):
            mapping[x] = y
    return mapping


def listings(diff) -> tuple[list[tuple[str, str]], list[tuple[str, str]]]:
    """Linear original and recomp listings from a Reccmp ``diff`` value."""
    orig: list[tuple[str, str]] = []
    recomp: list[tuple[str, str]] = []
    aligned = []
    for _header, blocks in diff or []:
        for block in blocks:
            for row in block.get("both", []):
                orig.append(_strip(row[1]))
                recomp.append(_strip(row[1]))
            o = [_strip(row[1]) for row in block.get("orig", [])]
            r = [_strip(row[1]) for row in block.get("recomp", [])]
            if o and r:
                masked = difflib.SequenceMatcher(
                    a=[(m, _TOKEN.sub("@", x)) for m, x in o],
                    b=[(m, _TOKEN.sub("@", x)) for m, x in r], autojunk=False)
                for i, j, n in masked.get_matching_blocks():
                    aligned.extend(zip(o[i:i + n], r[j:j + n]))
            orig.extend(o)
            recomp.extend(r)
    mapping = _correspondence(aligned)

    def rename(line):
        m, ops = line
        ops = _TOKEN.sub(lambda t: mapping.get(t.group(0), t.group(0)), ops)
        return m, _OFFSET.sub("<OFFSET>", ops)
    recomp_names = [(m, _OFFSET.sub("<OFFSET>", x)) for m, x in recomp]
    return [rename(line) for line in orig], recomp_names


_STACK_REG = re.compile(r"^st\((\d)\)$")
_COMPARES = {"fcom", "fcomp"}


def _is_memory(operand: str) -> bool:
    return "[" in operand


def _x87(listing):
    """The x87 subsequence: integer code between two x87 operations is
    scheduling, not operand order."""
    return [item for item in listing if item[0].startswith("f")]


def _pairs(listing: list[tuple[str, str]]) -> collections.Counter:
    pairs: collections.Counter = collections.Counter()
    x87 = _x87(listing)
    for first, second in zip(x87, x87[1:]):
        m1, a = first
        m2, b = second
        if a == b or not (_is_memory(a) or _STACK_REG.match(a)):
            continue
        if not (_is_memory(b) or _STACK_REG.match(b)):
            continue
        pairs[(m1, a, m2, b)] += 1
    return pairs


def _swapped(pair):
    """The pairs that compute the same value with the operands exchanged,
    with the kind of swap: ``rule-a`` for arithmetic, ``compare`` for a
    comparison (a source-shape difference, not rule A)."""
    m1, a, m2, b = pair
    sa, sb = _STACK_REG.match(a), _STACK_REG.match(b)
    if m1 == "fld" and _is_memory(a) and _is_memory(b):
        if m2 in _COMMUTATIVE:
            return [((m1, b, m2, a), "rule-a")]
        if m2 in _REVERSED:
            return [((m1, b, _REVERSED[m2], a), "rule-a")]
        if m2 in _COMPARES:
            return [((m1, b, m2, a), "compare")]
    if m1 == m2 and m1 in _COMMUTATIVE and _is_memory(a) and _is_memory(b):
        return [((m1, b, m2, a), "rule-a")]
    # One operand already on the x87 stack (a constant or value reused):
    # ``fld st(k); fmul [x]`` against ``fld [x]; fmul st(k+1)``.
    if m1 == "fld" and sa and _is_memory(b) and m2 in _COMMUTATIVE:
        return [((m1, b, m2, f"st({int(sa.group(1)) + 1})"), "rule-a")]
    if m1 == "fld" and _is_memory(a) and sb and int(sb.group(1)) > 0 and m2 in _COMMUTATIVE:
        return [((m1, f"st({int(sb.group(1)) - 1})", m2, a), "rule-a")]
    return []


def find_swaps(orig, recomp) -> list[dict]:
    """Every rule-A operand swap between two listings."""
    po, pr = _pairs(orig), _pairs(recomp)
    found = []
    seen = set()
    for pair, count in po.items():
        surplus = count - pr.get(pair, 0)
        if surplus <= 0:
            continue
        for other, kind in _swapped(pair):
            other_surplus = pr.get(other, 0) - po.get(other, 0)
            if other_surplus <= 0:
                continue
            key = (pair, other)
            if key in seen:
                continue
            seen.add(key)
            n = min(surplus, other_surplus)
            found.append({
                "orig": f"{pair[0]} {pair[1]}; {pair[2]} {pair[3]}",
                "recomp": f"{other[0]} {other[1]}; {other[2]} {other[3]}",
                "operands": [pair[1], pair[3]],
                "count": n,
                "kind": kind,
            })
    return found


def _covers(exact, loose_swap) -> bool:
    for orig, recomp in exact:
        if loosen([tuple(p.strip().partition(" ")[::2]) for p in orig.split(";")]) == \
                [tuple(p.strip().partition(" ")[::2]) for p in loose_swap["orig"].split(";")]:
            return True
    return False


def fxch_delta(orig, recomp) -> int:
    count = lambda listing: sum(1 for m, _ in listing if m == "fxch")
    return count(recomp) - count(orig)


def load_units() -> dict[int, tuple[str, str]]:
    units: dict[int, tuple[str, str]] = {}
    if not UNITS_CSV.exists():
        return units
    with UNITS_CSV.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            units[int(row["address"], 16)] = (row["unit_id"], row["current_source"])
    return units


_REGISTER = re.compile(r"\b(e[abcd]x|e[sd]i|ebp)\b")
_ESP_SLOT = re.compile(r"\[esp(?: \+ (?:0x[0-9a-f]+|\d+))?\]")


def loosen(listing):
    """Mask base registers and stack offsets. Register allocation and frame
    layout often differ between the two listings, which hides a swap from the
    exact comparison; masked operands can still be told apart by class (a
    stack slot, a named global, a literal, a field displacement)."""
    return [(m, _ESP_SLOT.sub("[esp+?]", _REGISTER.sub("R", o))) for m, o in listing]


def survey(entries, units=None, loose=False) -> list[dict]:
    units = units if units is not None else {}
    results = []
    for entry in entries:
        diff = entry.get("diff")
        if not diff:
            continue
        orig, recomp = listings(diff)
        swaps = find_swaps(orig, recomp)
        if loose:
            exact = {(s["orig"], s["recomp"]) for s in swaps}
            for swap in find_swaps(loosen(orig), loosen(recomp)):
                swap["loose"] = True
                swaps.append(swap)
            # Keep a loose duplicate of an exact swap out of the count.
            if exact:
                swaps = [s for s in swaps if not s.get("loose")] + [
                    s for s in swaps if s.get("loose") and not _covers(exact, s)]
        if not swaps:
            continue
        address = int(str(entry["address"]), 16)
        unit, source = units.get(address, ("", entry.get("source", "")))
        swapped = sum(s["count"] for s in swaps if s["kind"] == "rule-a") * 2
        size = max(len(orig), len(recomp)) or 1
        matching = entry.get("matching")
        results.append({
            "address": f"0x{address:08x}",
            "name": entry.get("name", ""),
            "unit": unit,
            "source": source,
            "matching": round(100 * matching, 2) if isinstance(matching, (int, float)) else None,
            "instructions": [len(orig), len(recomp)],
            "swaps": swaps,
            "fxch_delta": fxch_delta(orig, recomp),
            "expected_gain": round(100 * swapped / size, 2),
        })
    results.sort(key=lambda r: (-r["expected_gain"], r["address"]))
    return results


def _entries(document) -> list[dict]:
    if isinstance(document, dict):
        if "data" in document:
            return document["data"]
        if "functions" in document:
            return document["functions"]
    return list(document)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("report", help="results.json, match_function --json output, or - for stdin")
    parser.add_argument("--json", action="store_true", help="print JSON")
    parser.add_argument("--loose", action="store_true",
                        help="also match with base registers and stack offsets masked")
    args = parser.parse_args(argv)
    text = sys.stdin.read() if args.report == "-" else Path(args.report).read_text(encoding="utf-8")
    results = survey(_entries(json.loads(text)), load_units(), loose=args.loose)
    if args.json:
        json.dump(results, sys.stdout, indent=1)
        print()
        return 0
    print(f"{'address':<11} {'score':>6} {'gain':>5} {'fxch':>4}  {'unit':<9} function / swaps")
    for r in results:
        print(f"{r['address']:<11} {r['matching'] if r['matching'] is not None else '':>6} "
              f"{r['expected_gain']:>5} {r['fxch_delta']:>+4}  {r['unit']:<9} {r['name']} ({r['source']})")
        for s in r["swaps"]:
            mark = "~" if s.get("loose") else " "
            print(f"{'':<11}{mark}{s['kind']:<8} orig: {s['orig']}  |  recomp: {s['recomp']}  x{s['count']}")
    print(f"{len(results)} functions with x87 operand swaps")
    return 0


if __name__ == "__main__":
    sys.exit(main())
