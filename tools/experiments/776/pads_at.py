#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Finding 663: score a unit's functions with pads at chosen lines.
usage: padat.py UNIT 'L1:N1,L2:N2' ['L1:N1,...' ...]   (pads inserted before line L, C units only)
prints per spec: up/down vs no pads, and the listed functions"""
import json, re, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
unit = sys.argv[1]
lines = Path(unit).read_text().splitlines(keepends=True)
addrs = re.findall(r"// FUNCTION: LEGORR (0x[0-9a-fA-F]+)", "".join(lines))
def score(spec):
    pads = {}
    for item in filter(None, spec.split(",")):
        l, n = item.split(":"); pads[int(l)] = int(n)
    new = []
    k = 0
    for i, line in enumerate(lines, 1):
        if i in pads and pads[i]:
            new.append(" ".join(f"extern int pad776_{k + j};" for j in range(pads[i])) + "\n"); k += pads[i]
        new.append(line)
    cand = ROOT / "build-776" / f"padat-{Path(unit).stem}{Path(unit).suffix}"
    cand.parent.mkdir(exist_ok=True); cand.write_text("".join(new))
    m = subprocess.run(["python3", "tools/match_function.py", *addrs, "--candidate", str(cand), "--json", "--no-diff"],
                       capture_output=True, text=True, stdin=subprocess.DEVNULL)
    return {f["address"]: f["matching"] for f in json.loads(m.stdout)["functions"] if not f.get("error")}
base = score("")
for spec in sys.argv[2:]:
    r = score(spec)
    up = {k: (round(base[k]*100,2), round(v*100,2)) for k, v in r.items() if v > base[k] + 1e-9}
    dn = {k: (round(base[k]*100,2), round(v*100,2)) for k, v in r.items() if v < base[k] - 1e-9}
    print(spec, "UP", up, "DOWN", dn, flush=True)
