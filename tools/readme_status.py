#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Print the README "Decompilation status" figures from the committed reports.

Reads docs/coverage/binary-progress.json and docs/coverage/match-baseline.txt;
needs no build. Usage: python3 tools/readme_status.py
"""

import json
import pathlib

COV = pathlib.Path(__file__).resolve().parent.parent / "docs/coverage"
p = json.loads((COV / "binary-progress.json").read_text())
counts, scope = {}, None
for line in (COV / "match-baseline.txt").read_text().splitlines():
    if line.startswith("["):
        scope = line[1:line.index("]")]
        counts[scope] = [0, 0]
    elif scope and line.strip() and not line.startswith("#"):
        counts[scope][0] += 1
        counts[scope][1] += line.split()[1] == "100.00"

e, a = p["exact_function_code_bytes"], p["accuracy_percent"]
print(f"Exact-function code bytes: {e['bytes']:,} / {e['original_text_bytes']:,} ({e['percent']}%)")
print(f"Accuracy: {a['game_only']}% game-only, {a['full']}% full")
for s, (m, x) in counts.items():
    print(f"Mapped / exact ({s}): {m:,} / {x:,}")
f = p["fully_identical_functions_at_original_address"]
print(f"Fully identical at original address: {f['count']} ({f['bytes']} bytes)")
for name, s in p["sections"].items():
    if name.startswith("."):
        print(f"{name}: {s['rebuilt_bytes']:,} rebuilt vs {s['original_bytes']:,} original; "
              f"{s['identical_percent']}% identical (chance {s['chance_percent']}%)")
o = p["opt_ref"]
print(f"/OPT:REF link removes {o['removed_bytes']:,} bytes in {o['removed_functions']:,} functions; "
      f".text then {o['image']['section_bytes']['.text']['rebuilt']:,}")
u = p["layout"]["unit_contiguity"]
print(f"Contiguous units: {u['units']} / {u['eligible_units']} ({u['percent']}%)")
