#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Finding 663: classify near misses: is the whole diff a register renaming?

Reads match_function --json output (stdin) and prints, per function:
addr score kind n_diff_lines source name
kind: REG   = diff vanishes once register names are normalised
      REGOP = vanishes once registers AND commutative operand order are normalised
      OTHER = something else differs
"""
import json, re, sys

REG = r"\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b"

def norm(line, ops=False, raw=False):
    line = line.split("\t")[0].strip()
    line = re.sub(r"\S+ \((FUNCTION|OFFSET|STRING|DATA|IMPORT|THUNK|FLOAT|VTABLE|POINTER)\)", "SYM", line)
    line = re.sub(r"\b(j\w+|call|jmp) 0x[0-9a-f]+", r"\1 T", line)
    if not raw:
        line = re.sub(REG, "R", line)
    if ops:
        # [R + R*2 + 4] vs [R*2 + R + 4] etc: sort the address terms
        def srt(m):
            return "[" + " + ".join(sorted(t.strip() for t in m.group(1).split("+"))) + "]"
        line = re.sub(r"\[([^\]]*)\]", srt, line)
        m = re.match(r"(add|imul|and|or|xor|test|cmp) (.*), (.*)$", line)
        if m and m.group(1) in ("test",):
            line = m.group(1) + " " + ",".join(sorted([m.group(2), m.group(3)]))
    return line

def sides(diff):
    o, r = [], []
    for hdr, chunks in diff:
        for ch in chunks:
            if "both" in ch:
                for x in ch["both"]:
                    o.append(x[1]); r.append(x[1])
            else:
                o += [x[1] for x in ch.get("orig", [])]
                r += [x[1] for x in ch.get("recomp", [])]
    return o, r

d = json.load(sys.stdin)
for f in d["functions"]:
    if f.get("error"):
        print(f["address"], "ERR", f["error"][:60]); continue
    diff = f.get("diff") or []
    o, r = sides(diff)
    nd = sum(len(ch.get("orig", [])) + len(ch.get("recomp", [])) for _, chs in diff for ch in chs if "both" not in ch)
    if [norm(x, raw=True) for x in o] == [norm(x, raw=True) for x in r]:
        kind = "NAME"
    elif [norm(x) for x in o] == [norm(x) for x in r]:
        kind = "REG"
    elif sorted(norm(x, True) for x in o) == sorted(norm(x, True) for x in r):
        kind = "REGOP"
    else:
        kind = "OTHER"
    print(f["address"], f.get("baseline"), f["percent"], kind, nd, f["orig_size"], f["source"], f["name"], sep="\t")
