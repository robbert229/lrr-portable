#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Line coverage of src/ from a build configured with -DLRR_COVERAGE=ON
(docs/building.md, "Coverage").

    tools/coverage.py BUILD_DIR OUT_DIR [--compare-branch REF]
                      [--base-summary FILE] [--run-url URL]

Run it after ctest. It reads the gcov counts in BUILD_DIR with gcovr and
writes to OUT_DIR:
  html/index.html  the line-by-line report;
  coverage.xml     Cobertura XML;
  summary.json     gcovr's per-file summary;
  diff.json        with --compare-branch: the lines this branch changes
                   against REF, and which of them the tests ran (diff-cover);
  comment.md       a Markdown summary: changed-line coverage, then coverage
                   by area of src/, against --base-summary (a summary.json
                   from main) when given. CI posts it on the pull request.

The report covers src/ less the tests themselves (src/test/ and test_*
sources). gcovr and diff-cover must be on PATH.
"""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MARKER = "<!-- lrr-coverage -->"
# A gcovr regex, matched against paths relative to the repository root.
EXCLUDES = [r"src/test/", r"(.*/)?test_[^/]*$"]
MAX_FILES = 25
MAX_RANGES = 12


def run_gcovr(build_dir, out_dir):
    (out_dir / "html").mkdir(parents=True, exist_ok=True)
    cmd = ["gcovr", "--root", str(ROOT), "--object-directory", str(build_dir),
           "--filter", "src/", "-j", str(os.cpu_count() or 1),
           # Some tests compile a unit again with other defines, so one
           # function can have two line numbers; keep each.
           "--merge-mode-functions", "separate",
           "--html-details", str(out_dir / "html" / "index.html"),
           "--html-title", "lrr-portable coverage",
           "--cobertura", str(out_dir / "coverage.xml"),
           "--json-summary-pretty", "--json-summary", str(out_dir / "summary.json"),
           "--print-summary"]
    for pattern in EXCLUDES:
        cmd += ["--exclude", pattern]
    # gcovr reads --filter and --exclude against the working directory.
    subprocess.run(cmd, check=True, cwd=ROOT)


def run_diff_cover(out_dir, compare_branch):
    subprocess.run(["diff-cover", str(out_dir / "coverage.xml"),
                    "--compare-branch", compare_branch,
                    "--json-report", str(out_dir / "diff.json")],
                   check=True, cwd=ROOT)


def area(filename):
    """The directory under src/ a file counts toward: src/game for
    src/game/Lego.cpp, src/platform for src/platform/native/entry.cpp."""
    parts = PurePosixPath(filename).parts
    return "/".join(parts[:2]) if len(parts) > 2 else parts[0]


def by_area(summary):
    """{area: (covered, total)} from a gcovr JSON summary."""
    areas = {}
    for entry in summary.get("files", []):
        name = area(entry["filename"])
        covered, total = areas.get(name, (0, 0))
        areas[name] = (covered + entry["line_covered"],
                       total + entry["line_total"])
    return areas


def percent(covered, total):
    return 100.0 * covered / total if total else 100.0


def ranges(lines):
    """[3, 4, 5, 9] -> "3-5, 9", at most MAX_RANGES ranges."""
    spans = []
    for line in sorted(set(lines)):
        if spans and line == spans[-1][1] + 1:
            spans[-1][1] = line
        else:
            spans.append([line, line])
    text = [f"{a}-{b}" if a != b else f"{a}" for a, b in spans[:MAX_RANGES]]
    if len(spans) > MAX_RANGES:
        text.append(f"and {len(spans) - MAX_RANGES} more")
    return ", ".join(text)


def change(now, base):
    delta = now - base
    if abs(delta) < 0.05:
        return "±0.0"
    return f"{delta:+.1f}"


def render_diff(diff, base_name):
    out = []
    stats = diff.get("src_stats", {})
    changed = {path: s for path, s in stats.items()
               if s.get("covered_lines") or s.get("violation_lines")}
    if not changed:
        out.append(f"**Changed lines:** this branch changes no instrumented "
                   f"lines in `src/` against `{base_name}`.")
        return out
    total = sum(len(s["covered_lines"]) + len(s["violation_lines"])
                for s in changed.values())
    missed = sum(len(s["violation_lines"]) for s in changed.values())
    covered = total - missed
    out.append(f"**Changed lines:** {percent(covered, total):.1f}% covered "
               f"({covered} of {total} instrumented lines changed against "
               f"`{base_name}`).")
    out.append("")
    out.append("| File | Covered | Lines the tests miss |")
    out.append("| --- | ---: | --- |")
    rows = sorted(changed.items(),
                  key=lambda item: (-len(item[1]["violation_lines"]), item[0]))
    for path, s in rows[:MAX_FILES]:
        n_cov = len(s["covered_lines"])
        n_all = n_cov + len(s["violation_lines"])
        missed_text = ranges(s["violation_lines"]) or "none"
        out.append(f"| `{path}` | {n_cov}/{n_all} | {missed_text} |")
    if len(rows) > MAX_FILES:
        out.append(f"| {len(rows) - MAX_FILES} more files | | |")
    return out


def render_total(summary, base, base_name):
    out = []
    now = by_area(summary)
    then = by_area(base) if base else {}
    covered = sum(c for c, _ in now.values())
    total = sum(t for _, t in now.values())
    line = (f"**All of `src/`:** {percent(covered, total):.1f}% "
            f"({covered:,} of {total:,} lines)")
    if base:
        b_cov = sum(c for c, _ in then.values())
        b_tot = sum(t for _, t in then.values())
        line += (f", {change(percent(covered, total), percent(b_cov, b_tot))}"
                 f" points against `{base_name}`")
    out.append(line + ".")
    out.append("")
    if base:
        out.append(f"| Area | Lines | Covered | Against `{base_name}` |")
        out.append("| --- | ---: | ---: | ---: |")
    else:
        out.append("| Area | Lines | Covered |")
        out.append("| --- | ---: | ---: |")
    for name in sorted(now):
        c, t = now[name]
        row = f"| `{name}` | {t:,} | {percent(c, t):.1f}% |"
        if base:
            if name in then:
                row += f" {change(percent(c, t), percent(*then[name]))} |"
            else:
                row += " new |"
        out.append(row)
    return out


def render(summary, diff=None, base=None, base_name="main", run_url=None):
    """The pull request comment, as Markdown."""
    out = [MARKER, "## Coverage", ""]
    if diff is not None:
        out += render_diff(diff, base_name)
        out.append("")
    out += render_total(summary, base, base_name)
    out.append("")
    note = ("Line coverage from the synthetic-fixture tests on the Linux GCC "
            "build; the real-data tests do not run in CI. Tests themselves "
            "are not counted.")
    if run_url:
        note += f" Line-by-line report: the `coverage-report` artifact of [this run]({run_url})."
    out.append(f"<sub>{note}</sub>")
    return "\n".join(out) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("out_dir", type=Path)
    parser.add_argument("--compare-branch",
                        help="report the lines changed against this git ref")
    parser.add_argument("--base-summary", type=Path,
                        help="summary.json of the base branch, to compare totals")
    parser.add_argument("--run-url", help="link to the CI run in the comment")
    args = parser.parse_args(argv)

    out_dir = args.out_dir.resolve()
    run_gcovr(args.build_dir.resolve(), out_dir)
    summary = json.loads((out_dir / "summary.json").read_text())

    diff = None
    base_name = "main"
    if args.compare_branch:
        run_diff_cover(out_dir, args.compare_branch)
        diff = json.loads((out_dir / "diff.json").read_text())
        base_name = args.compare_branch.removeprefix("origin/")

    base = None
    if args.base_summary and args.base_summary.is_file():
        base = json.loads(args.base_summary.read_text())

    comment = render(summary, diff, base, base_name, args.run_url)
    (out_dir / "comment.md").write_text(comment)
    print(f"Wrote {out_dir / 'html' / 'index.html'} and {out_dir / 'comment.md'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
