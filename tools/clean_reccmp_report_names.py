#!/usr/bin/env python3
"""Remove the implementation-only namespace from Reccmp report labels."""

from pathlib import Path
import sys


PREFIX = "unported_ledger_stubs::"
REPORTS = ("index.html", "results.json", "game-only.html", "game-only.json")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: clean_reccmp_report_names.py REPORT_DIR", file=sys.stderr)
        return 2

    report_dir = Path(sys.argv[1])
    for report_name in REPORTS:
        report_path = report_dir / report_name
        if not report_path.is_file():
            continue
        content = report_path.read_text(encoding="utf-8")
        if PREFIX in content:
            report_path.write_text(content.replace(PREFIX, ""), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
