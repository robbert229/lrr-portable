#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Finding 663: is a near miss a symbol-counter phase case (finding 538 rule B)?

    python3 tools/experiments/776/phase_probe.py src/gods98/maths.c 0x479a30:ray_plane_intersection_parameter [...]
        [--range 0 256 1]

Runs finding 538's sweep.py (N inert externs after the unit's first
#include) and scores the first N of each distinct code with
match_function.py (the pads on one line in place of a blank line, so line
numbers hold). A function with one code over a full period is not a phase
case: its register choice comes from the source. Pads are an experiment
only (ADR 0005 entropy rule 3); never commit them.
"""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("unit")
    ap.add_argument("funcs", nargs="+", help="ADDR:c_name")
    ap.add_argument("--range", nargs=3, type=int, default=[0, 256, 1])
    ap.add_argument("--max-codes", type=int, default=24)
    a = ap.parse_args()
    funcs = [f.split(":") for f in a.funcs]
    lines = (ROOT / a.unit).read_text().splitlines(keepends=True)
    first = next(i for i, l in enumerate(lines) if l.startswith("#include"))
    anchor = lines[first].rstrip("\n")

    def in_comment(k: int) -> bool:
        text = "".join(lines[:k])
        return text.count("/*") > text.count("*/")

    blank = next(k for k in range(first + 1, len(lines)) if not lines[k].strip() and not in_comment(k))
    out = ROOT / "build-776/phase" / Path(a.unit).stem
    r = subprocess.run(["python3", "tools/experiments/486/sweep.py", a.unit, anchor, *["_" + n for _, n in funcs],
                        "--range", *map(str, a.range), "--out", str(out)], capture_output=True, text=True, cwd=ROOT)
    print(r.stdout[-3000:], r.stderr[-1000:])
    ns = list(range(*a.range))
    result = {}
    for addr, name in funcs:
        row = (out / f"{name[:40]}.row").read_text().strip()
        reps: dict[str, int] = {}
        for i, ch in enumerate(row):
            if ch != "!" and ch not in reps and len(reps) < a.max_codes:
                reps[ch] = ns[i]
        codes = {}
        for ch, n in reps.items():
            new = list(lines)
            new[blank] = " ".join(f"extern int pad776_{i};" for i in range(n)) + "\n"
            cand = out / f"cand-{n}{Path(a.unit).suffix}"
            cand.write_text("".join(new))
            m = subprocess.run(["python3", "tools/match_function.py", addr, "--candidate", str(cand), "--json", "--no-diff"],
                               capture_output=True, text=True, cwd=ROOT, stdin=subprocess.DEVNULL)
            f = json.loads(m.stdout)["functions"][0]
            codes[ch] = (n, f["percent"], f.get("effective"))
        result[addr] = {"name": name, "row": row, "codes": codes}
        print(addr, name, f"{len(set(row) - {'!'})} codes:", codes, flush=True)
    (out / "result.json").write_text(json.dumps(result, indent=1))


if __name__ == "__main__":
    main()
