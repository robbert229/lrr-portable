#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Finding 663: score every annotated function of each unit under alternative
cl option sets (appended after the matching build's /O2 /Ob1).

    python3 tools/experiments/776/flags.py src/gods98/lwo.c,src/game/roof.c [--sets '/Oa|/G6|/Oy-']

Writes build-776/flags.json ({unit: {flags: {address: score}}}) and prints,
per option set, how many functions rose and fell against the unit's default.
"""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SETS = ["/Ox", "/Oa", "/Ow", "/G6", "/G5", "/Op", "/Ob2", "/Oy-", "/Os", "/Oi-", "/GB",
        "/Oa /G6", "/Ow /G6", "/Og-", "/Ob0"]


def scores(unit: str, flags: str) -> dict[str, float]:
    r = subprocess.run(["python3", "tools/match_function.py", "--source", unit, "--json", "--no-diff",
                        "--cflags", flags], capture_output=True, text=True, cwd=ROOT, stdin=subprocess.DEVNULL)
    return {f["address"]: f["matching"] for f in json.loads(r.stdout)["functions"] if not f.get("error")}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("units")
    ap.add_argument("--sets", default="|".join(SETS))
    a = ap.parse_args()
    out_path = ROOT / "build-776/flags.json"
    out_path.parent.mkdir(exist_ok=True)
    out = json.loads(out_path.read_text()) if out_path.exists() else {}
    totals: dict[str, list[int]] = {}
    for unit in a.units.split(","):
        base = scores(unit, "")
        out[unit] = {"": base}
        for fl in a.sets.split("|"):
            sc = out[unit][fl] = scores(unit, fl)
            up = [k for k in sc if k in base and sc[k] > base[k] + 1e-9]
            dn = [k for k in sc if k in base and sc[k] < base[k] - 1e-9]
            t = totals.setdefault(fl, [0, 0, 0])
            t[0] += len(up); t[1] += len(dn); t[2] += 1
            if up and not dn:
                print(f"clean gain: {unit} {fl}: {up}")
        out_path.write_text(json.dumps(out, indent=0))
    for fl, (u, d, n) in totals.items():
        print(f"{fl:10} units {n:3} up {u:4} down {d:4}")


if __name__ == "__main__":
    main()
