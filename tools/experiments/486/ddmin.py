#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Delta-debug which top-level declarations of an inserted context flip a
function's code (issue #486).

    python3 tools/experiments/486/ddmin.py BASE.c ANCHOR INSERT.txt FUNC FLIPPED_FP OUTDIR

BASE.c is compiled with INSERT.txt's text placed after the line ANCHOR. A
subset is "interesting" when FUNC compiles to fingerprint FLIPPED_FP. Prints
the minimal set of top-level chunks (1-minimal, Zeller's ddmin) and writes it
to OUTDIR/minimal.txt.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from msvc import compile_many, fingerprint, function_body  # noqa: E402


def chunks(lines: list[str]) -> list[str]:
    """Split preprocessed text into top-level declarations and pragmas."""
    out, cur, depth = [], [], 0
    for line in lines:
        if depth == 0 and not cur and line.lstrip().startswith("#"):
            out.append(line)
            continue
        cur.append(line)
        depth += line.count("{") - line.count("}") + line.count("(") - line.count(")")
        s = line.rstrip()
        if depth == 0 and (s.endswith(";") or s.endswith("}")):
            out.append("\n".join(cur))
            cur = []
    if cur:
        out.append("\n".join(cur))
    return out


class Tester:
    def __init__(self, base: str, anchor: str, func: str, flipped: str, out: Path, prefix: str = ""):
        self.base, self.anchor, self.func, self.flipped, self.out = base, anchor, func, flipped, out
        self.prefix, self.n, self.cache = prefix, 0, {}

    def source(self, parts: list[str]) -> str:
        ins = self.prefix + "".join(p + "\n" for p in parts)
        return self.base.replace(self.anchor, self.anchor + ins, 1)

    def run(self, sets: list[tuple[int, ...]], all_chunks: list[str]) -> list[str]:
        todo = [s for s in sets if s not in self.cache]
        names = []
        for s in todo:
            self.n += 1
            name = f"t{self.n:05d}"
            (self.out / (name + ".c")).write_text(self.source([all_chunks[i] for i in s]))
            names.append(name)
        if names:
            compile_many(self.out, names)
            for s, name in zip(todo, names):
                self.cache[s] = fingerprint(function_body(self.out / (name + ".asm"), self.func))
                for ext in (".c", ".asm", ".obj", ".log"):
                    (self.out / (name + ext)).unlink(missing_ok=True)
        return [self.cache[s] for s in sets]


def ddmin(t: Tester, items: list[str]) -> tuple[int, ...]:
    cur = tuple(range(len(items)))
    n = 2
    while len(cur) >= 2:
        size = len(cur) // n
        subsets = [cur[i * size:(i + 1) * size] if i < n - 1 else cur[i * size:] for i in range(n)]
        comps = [tuple(x for x in cur if x not in set(s)) for s in subsets]
        fps = t.run(subsets + comps, items)
        hit = None
        for i, s in enumerate(subsets):
            if fps[i] == t.flipped:
                hit, n = s, 2
                break
        if hit is None:
            for i, c in enumerate(comps):
                if fps[len(subsets) + i] == t.flipped:
                    hit, n = c, max(n - 1, 2)
                    break
        if hit is not None:
            cur = hit
            print(f"  -> {len(cur)} chunks", flush=True)
            continue
        if n >= len(cur):
            break
        n = min(len(cur), n * 2)
    return cur


def main() -> None:
    base, anchor, insert, func, flipped, out = sys.argv[1:7]
    prefix = sys.argv[7].encode().decode("unicode_escape") if len(sys.argv) > 7 else ""
    outp = Path(out).resolve()
    outp.mkdir(parents=True, exist_ok=True)
    items = chunks(Path(insert).read_text().splitlines())
    print(f"{len(items)} chunks")
    t = Tester(Path(base).read_text(), anchor.encode().decode("unicode_escape"), func, flipped, outp, prefix)
    full = t.run([tuple(range(len(items))), ()], items)
    print("full:", full[0], "empty:", full[1])
    if full[0] != flipped:
        sys.exit("the full insert does not give the flipped fingerprint")
    keep = ddmin(t, items)
    text = "\n".join(items[i] for i in keep)
    (outp / "minimal.txt").write_text(text + "\n")
    print(f"minimal: {len(keep)} chunks after {t.n} compiles")
    print(text)


if __name__ == "__main__":
    main()
