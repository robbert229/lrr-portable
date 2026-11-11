#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Match-baseline regression gate for the matching build.

The match baseline is a committed, address-sorted text file holding every
function's Reccmp similarity score for two scopes: ``game-only`` (Reccmp's
``--nolib`` report) and ``full``. This tool compares a Reccmp JSON report
against it:

    match_baseline.py check   [--report-dir DIR] [--baseline FILE]
    match_baseline.py update  [--report-dir DIR] [--baseline FILE]

``check`` exits 1 when any function's score fell below its baseline, listing
each with ``old -> new``. Improvements, newly mapped functions and newly
unmapped functions (Reccmp only lists mapped ones, so these are functions that
appeared in or vanished from the report) are reported but do not fail. ``update`` rewrites the
baseline from the report (the explicit "accept this change" step).

Scores are stored as percentages rounded to two decimals so that float noise
never churns the file. Only the standard library is used.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from binary_progress_hash import input_hash

SCOPES = (
    ("game-only", "game-only.json"),
    ("full", "results.json"),
)
HEADER = (
    "# Match baseline: per-function Reccmp similarity (percent) from the last\n"
    "# accepted matching build. Do not edit by hand; refresh with\n"
    "# `make reccmp-baseline-update` (see GLOSSARY.md, docs/building.md).\n"
    "# Format: <address> <score> <name>; sections sorted by address.\n"
    "# Reccmp reports only mapped functions, so a function absent from a\n"
    "# section is unmapped.\n"
)

DEFAULT_REPORT_DIR = "build-reccmp/reccmp-report"
DEFAULT_BASELINE = "docs/coverage/match-baseline.txt"


def parse_address(value) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 16)


def format_score(entry) -> str:
    """Percent score with two decimals."""
    # Round via integer hundredths so the text form is stable.
    hundredths = int(round(float(entry.get("matching", 0.0)) * 10000))
    return "%d.%02d" % divmod(hundredths, 100)


def load_report(path: Path) -> dict:
    """Return {address: (score_text, name)} from a Reccmp JSON report."""
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)
    entries = data["data"] if isinstance(data, dict) else data
    result = {}
    for entry in entries:
        address = parse_address(entry["address"])
        result[address] = (format_score(entry), entry.get("name", ""))
    return result


def render(scopes: dict) -> str:
    lines = [HEADER]
    for scope, _ in SCOPES:
        entries = scopes[scope]
        lines.append("\n[%s] %d mapped functions\n" % (scope, len(entries)))
        for address in sorted(entries):
            score, name = entries[address]
            lines.append("0x%08x %s %s\n" % (address, score, name))
    return "".join(lines)


def load_baseline(path: Path) -> dict:
    scopes = {scope: {} for scope, _ in SCOPES}
    current = None
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("["):
            current = line[1:line.index("]")]
            if current not in scopes:
                raise ValueError("%s:%d: unknown scope %r" % (path, number, current))
            continue
        parts = line.split(None, 2)
        if current is None or len(parts) < 2:
            raise ValueError("%s:%d: malformed line" % (path, number))
        name = parts[2] if len(parts) > 2 else ""
        scopes[current][parse_address(parts[0])] = (parts[1], name)
    return scopes


def load_reports(report_dir: Path) -> dict:
    scopes = {}
    for scope, filename in SCOPES:
        path = report_dir / filename
        if not path.is_file():
            raise FileNotFoundError(
                "%s not found; run `make reccmp-report` first (or pass "
                "--report-dir)" % path
            )
        scopes[scope] = load_report(path)
    return scopes


def compare(baseline: dict, current: dict):
    """Return (regressions, improvements, newly_mapped, newly_unmapped) for one
    scope; each item is (address, name, old, new) with None for "absent"."""
    regressions, improvements = [], []
    newly_mapped, newly_unmapped = [], []
    for address in sorted(set(baseline) | set(current)):
        if address not in baseline:
            score, name = current[address]
            newly_mapped.append((address, name, None, score))
        elif address not in current:
            score, name = baseline[address]
            newly_unmapped.append((address, name, score, None))
        else:
            old, old_name = baseline[address]
            new, name = current[address]
            if float(new) < float(old):
                regressions.append((address, name or old_name, old, new))
            elif float(new) > float(old):
                improvements.append((address, name or old_name, old, new))
    return regressions, improvements, newly_mapped, newly_unmapped


def show(item) -> str:
    address, name, old, new = item
    old_text = "unmapped" if old is None else old + "%"
    new_text = "unmapped" if new is None else new + "%"
    return "  0x%08x %-40s %s -> %s" % (address, name, old_text, new_text)


def summarize(scopes: dict) -> str:
    out = []
    for scope, _ in SCOPES:
        entries = scopes[scope]
        mean = (sum(float(s) for s, _ in entries.values()) / len(entries)
                if entries else 0.0)
        out.append("%s: %d mapped functions, mean score %.2f%%"
                   % (scope, len(entries), mean))
    return "\n".join(out)


def command_check(args) -> int:
    baseline_path = Path(args.baseline)
    if not baseline_path.is_file():
        print("Baseline %s not found; create it with "
              "`make reccmp-baseline-update`." % baseline_path, file=sys.stderr)
        return 2
    try:
        baseline = load_baseline(baseline_path)
        current = load_reports(Path(args.report_dir))
    except (OSError, ValueError, KeyError) as error:
        print("error: %s" % error, file=sys.stderr)
        return 2

    failed = False
    sections = (
        ("REGRESSED (below baseline)", 0),
        ("improved", 1),
        ("newly mapped (not in baseline)", 2),
        ("newly unmapped (absent from report)", 3),
    )
    for scope, _ in SCOPES:
        results = compare(baseline[scope], current[scope])
        if not any(results):
            continue
        print("[%s]" % scope)
        for title, index in sections:
            items = results[index]
            if items:
                print(" %s: %d" % (title, len(items)))
                for item in items:
                    print(show(item))
        if results[0]:
            failed = True

    if failed:
        print("\nFAIL: functions fell below the match baseline. Fix the "
              "regression; if the drop is intended, refresh the baseline with "
              "`make reccmp-baseline-update` in the same change.",
              file=sys.stderr)
        return 1
    print("OK: no function fell below the match baseline.")
    print(summarize(current))
    return 0


def command_update(args) -> int:
    try:
        current = load_reports(Path(args.report_dir))
    except (OSError, ValueError, KeyError) as error:
        print("error: %s" % error, file=sys.stderr)
        return 2
    baseline_path = Path(args.baseline)
    baseline_path.parent.mkdir(parents=True, exist_ok=True)
    baseline_path.write_text("# input_sha256: %s\n" % input_hash() + render(current), encoding="utf-8", newline="\n")
    print("Wrote %s" % baseline_path)
    print(summarize(current))
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name, handler in (("check", command_check), ("update", command_update)):
        p = sub.add_parser(name)
        p.add_argument("--report-dir", default=DEFAULT_REPORT_DIR,
                       help="directory holding results.json and game-only.json")
        p.add_argument("--baseline", default=DEFAULT_BASELINE)
        p.set_defaults(func=handler)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
