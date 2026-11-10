#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Check tools/match_function.py against a full Reccmp report.

Runs match_function.py once per translation unit (every annotated function in
it) and compares each score with the `matching` value of the same address in
a full `make reccmp-report` results.json built from the same source tree:

    python3 tools/match_function_parity.py [--report results.json] [--jobs 4]
        [--source src/gods98/block.cpp ...] [--json]

Exits 1 when any score differs, 0 when all agree, 2 on setup errors.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOL = ROOT / "tools" / "match_function.py"


def run_source(source: Path, build_dir: str | None) -> tuple[Path, dict]:
    cmd = [sys.executable, str(TOOL), "--source", str(source), "--json", "--no-diff"]
    if build_dir:
        cmd += ["--build-dir", build_dir]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    try:
        return source, json.loads(proc.stdout)
    except json.JSONDecodeError:
        return source, {"error": (proc.stdout + proc.stderr)[-2000:]}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    default_report = Path(os.environ.get("RECCMP_BUILD_DIR", ROOT / "build-reccmp")) / "reccmp-report/results.json"
    parser.add_argument("--report", type=Path, default=default_report, help="full Reccmp results.json (default: %(default)s)")
    parser.add_argument("--source", type=Path, nargs="*", help="limit to these source files")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--build-dir", help="passed through to match_function.py")
    parser.add_argument("--json", action="store_true", help="print a JSON summary")
    args = parser.parse_args(argv)

    if not args.report.exists():
        print(f"report not found: {args.report} (run make reccmp-report)", file=sys.stderr)
        return 2
    full = {int(f["address"], 16): f for f in json.loads(args.report.read_text())["data"]}

    if args.source:
        sources = [p.resolve() for p in args.source]
    else:
        sys.path.insert(0, str(ROOT / "tools"))
        import match_function  # pylint: disable=import-outside-toplevel

        env = match_function.Environment.detect(argparse.Namespace(build_dir=args.build_dir))
        commands = match_function.load_compile_commands(env, {})
        sources = sorted({c.source for c in commands})

    exact, differ, only_here, only_full, errors, failed_files = [], [], [], [], [], []
    seen: set[int] = set()
    with concurrent.futures.ThreadPoolExecutor(max(1, args.jobs)) as pool:
        for source, doc in pool.map(lambda s: run_source(s, args.build_dir), sources):
            rel = os.path.relpath(source, ROOT)
            if "functions" not in doc:
                if "no annotated functions" not in doc.get("error", ""):
                    failed_files.append((rel, doc.get("error", "")))
                continue
            for fn in doc["functions"]:
                addr = int(fn["address"], 16)
                seen.add(addr)
                ref = full.get(addr)
                row = {"address": fn["address"], "name": fn.get("name") or (ref or {}).get("name"), "source": rel,
                       "single": fn.get("matching"), "full": None if ref is None else ref["matching"]}
                if ref is None:
                    (errors if fn.get("error") else only_here).append(row | {"error": fn.get("error")})
                elif fn.get("error"):
                    only_full.append(row | {"error": fn["error"]})
                elif abs(fn["matching"] - ref["matching"]) < 1e-9:
                    exact.append(row)
                else:
                    differ.append(row)
    not_swept = [{"address": f"0x{a:x}", "name": full[a]["name"]} for a in sorted(set(full) - seen)]

    def area(rel: str) -> str:
        parts = Path(rel).parts
        return parts[1] if len(parts) > 2 and parts[0] == "src" else parts[0]

    areas: dict[str, dict[str, int]] = {}
    for rows, key in ((exact, "identical"), (differ, "different"), (only_full, "missing_here")):
        for row in rows:
            counts = areas.setdefault(area(row["source"]), {"identical": 0, "different": 0, "missing_here": 0})
            counts[key] += 1

    summary = {
        "report": str(args.report),
        "in_full_report": len(full),
        "identical": len(exact),
        "areas": areas,
        "different": differ,
        "missing_here": only_full,
        "not_linked_in_full": only_here,
        "not_in_any_source": not_swept if not args.source else [],
        "file_errors": failed_files,
    }
    if args.json:
        print(json.dumps(summary, indent=2))
    else:
        compared = len(exact) + len(differ) + len(only_full)
        print(f"{len(exact)}/{compared} functions score identically to {args.report}")
        for name, counts in sorted(areas.items()):
            total = sum(counts.values())
            print(f"  {name:<10} {counts['identical']}/{total}")
        for row in differ:
            print(f"  differs  {row['address']} {row['name']}: single {row['single']:.4f} vs full {row['full']:.4f} ({row['source']})")
        for row in only_full:
            print(f"  missing  {row['address']} {row['name']}: {row['error']} ({row['source']})")
        if only_here:
            print(f"  {len(only_here)} scored here but absent from the full report (not pulled into the link)")
        if summary["not_in_any_source"]:
            print(f"  {len(not_swept)} full-report functions not reached from any compiled source")
        for rel, err in failed_files:
            print(f"  error    {rel}: {err.strip().splitlines()[-1] if err.strip() else '?'}")
    return 1 if differ or only_full or failed_files else 0


if __name__ == "__main__":
    sys.exit(main())
