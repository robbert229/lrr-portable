#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Insert N inert declarations into a unit and record which code each
function gets (issue #486).

    python3 tools/experiments/486/sweep.py SOURCE ANCHOR FUNC [FUNC...] \
        [--range START STOP STEP] [--decl 'extern int pad486_{i};'] [--out DIR]

SOURCE is a repo-relative unit compiled with its matching-build flags
(tools/match_function.py's cached configure). The N declarations go after the
first line that starts with ANCHOR. FUNC is the listing name (C: `_name`;
C++: the decorated name, a prefix is enough). Each distinct code is given a
letter in order of appearance; the first row is N = START. Prints one string
of letters per function, the symbol-counter edges where it changes, and the
listing's $S number of the first static it sees (the counter readout).
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from msvc import ROOT, compile_many, fingerprint, flags_for  # noqa: E402


def body(listing: Path, prefix: str) -> list[str] | None:
    if not listing.exists():
        return None
    out, inside = [], False
    for line in listing.read_text(errors="replace").splitlines():
        m = re.match(r"(\S+)\s+PROC", line)
        if m and m.group(1).startswith(prefix):
            inside = True
            continue
        if inside and re.match(r"\S+\s+ENDP", line):
            return out
        if inside:
            line = re.sub(r"\$[A-Z]\d+", "$N", line.split(";", 1)[0]).strip()
            # Goto labels carry a counter number too: $message_tail$51964.
            line = re.sub(r"(\$\w+)\$\d+", r"\1$N", line)
            if line:
                out.append(re.sub(r"\s+", " ", line))
    return None


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("anchor")
    ap.add_argument("funcs", nargs="+")
    ap.add_argument("--range", nargs=3, type=int, default=[0, 600, 1])
    ap.add_argument("--decl", default="extern int pad486_{i};")
    ap.add_argument("--out", default=None)
    a = ap.parse_args()
    src = (ROOT / a.source).read_text()
    lines = src.splitlines(keepends=True)
    at = next(i for i, l in enumerate(lines) if l.startswith(a.anchor)) + 1
    ext = Path(a.source).suffix
    out = Path(a.out or ROOT / "build-486/sweep" / Path(a.source).stem).resolve()
    out.mkdir(parents=True, exist_ok=True)
    ns = list(range(*a.range))
    names = []
    for n in ns:
        pad = "".join(a.decl.format(i=i) + "\n" for i in range(n))
        name = f"n{n:05d}"
        (out / (name + ext)).write_text("".join(lines[:at]) + pad + "".join(lines[at:]))
        names.append(name)
    compile_many(out, names, flags=flags_for(a.source), ext=ext)
    for f in a.funcs:
        letters: dict[str, str] = {}
        row = ""
        for name in names:
            fp = fingerprint(body(out / (name + ".asm"), f))
            row += "!" if fp == "ERR" else letters.setdefault(fp, chr(ord("A") + len(letters)))
        edges = [ns[i] for i in range(1, len(row)) if row[i] != row[i - 1]]
        print(f"{f}: {len(letters)} codes; edges at N = {edges}")
        for i in range(0, len(row), 64):
            print(f"  {ns[i]:5d} {row[i:i + 64]}")
        (out / f"{f.strip('?_@')[:40]}.row").write_text(row + "\n")
        (out / f"{f.strip('?_@')[:40]}.fps").write_text(
            "".join(f"{k} {v}\n" for k, v in letters.items()))


if __name__ == "__main__":
    main()
