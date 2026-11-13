#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Symbol-counter probe for one C unit (issue #699, finding 626).

Compiles UNIT twice with the matching build's flags, once as it is in the
working tree and once as it is at REV (default origin/main), each with a probe
static inserted before every `// FUNCTION:` annotation. The probe's `$S` number
is MSVC 5.0's symbol counter at that function (finding 538). The output lists,
per function, the counter at REV, in the working tree, and the difference, so
a change can be checked for counter neutrality before a full matching build.

    python3 tools/experiments/699/counter_probe.py src/game/lego_unit.c [REV] [--headers H...]

`--headers` names the headers whose REV copies replace the working tree's for
the REV compile (default: the unit's own `<stem>.h`, `<stem>_imports.h` and the
port alias headers next to it). Every probe costs the same in both compiles,
so the difference is the change's own cost. Needs the cached match-function
configure (`make match-function ADDR=...` once). Output goes to build-486/.
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "486"))
from msvc import ROOT, compile_many, flags_for  # noqa: E402

OUT = ROOT / "build-486" / "probe699"


def probed(text: str) -> tuple[str, list[str]]:
    out, names = [], []
    for line in text.split("\n"):
        m = re.match(r"// FUNCTION: LEGORR (0x[0-9a-f]+)", line)
        if m:
            k = len(names)
            out.append(f"static int probe699_{k}; "
                       f"int* probe699u_{k}(void) {{ return &probe699_{k}; }}")
            names.append(m.group(1))
        out.append(line)
    return "\n".join(out), names


def compile_probed(tag: str, unit: str, headers: list[str], read) -> dict[str, int | None]:
    work = OUT / tag
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    for h in headers:
        (work / Path(h).name).write_text(read(h))
    text, names = probed(read(unit))
    (work / "u.c").write_text(text)
    compile_many(work, ["u"], flags=["-I" + str(work)] + flags_for(unit))
    listing = work / "u.asm"
    if not listing.exists():
        sys.exit((work / "u.log").read_text())
    asm = listing.read_text(errors="replace")
    result = {}
    for k, addr in enumerate(names):
        m = re.search(rf"probe699_{k}\$S(\d+)", asm)
        result[addr] = int(m.group(1)) if m else None
    return result


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("unit")
    ap.add_argument("rev", nargs="?", default="origin/main")
    ap.add_argument("--headers", nargs="*")
    ap.add_argument("--changes-only", action="store_true",
                    help="print only the functions where the difference changes")
    args = ap.parse_args()
    stem = Path(args.unit).with_suffix("")
    headers = args.headers or [
        h for h in (f"{stem}.h", f"{stem}_imports.h",
                    "src/game/legoobject_port_aliases.h",
                    "src/game/lego_port_aliases.h",
                    "src/game/aitask_port_aliases.h")
        if (ROOT / h).exists()]

    def at_rev(path: str) -> str:
        return subprocess.run(["git", "-C", str(ROOT), "show", f"{args.rev}:{path}"],
                              capture_output=True, text=True, check=True).stdout

    def in_tree(path: str) -> str:
        return (ROOT / path).read_text()

    base = compile_probed("base", args.unit, headers, at_rev)
    cur = compile_probed("cur", args.unit, headers, in_tree)
    last = None
    for addr, b in base.items():
        c = cur.get(addr)
        diff = None if b is None or c is None else c - b
        if args.changes_only and diff == last:
            continue
        last = diff
        print(addr, b, c, diff)


if __name__ == "__main__":
    main()
