#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Rerun issue #486's experiments (finding 538). Needs the MSVC 5.0 toolchain
(Wine on PATH or the `developer` distrobox) and a cached match-function
configure (`make match-function ADDR=0x004869a0` once). No game data.

    python3 tools/experiments/486/run.py [counter|order|isolate|phase|all]

Output goes to build-486/. The sweeps over real units take a few minutes;
`sweep.py` and `ddmin.py` run single experiments.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from msvc import ROOT, compile_many, function_body  # noqa: E402

OUT = ROOT / "build-486" / "run"


def pad(n: int, prefix: str = "pad") -> str:
    return "".join(f"extern int {prefix}{i};\n" for i in range(n))


def counter() -> None:
    """Symbol-counter cost of each construct: $S of a static after 10 copies."""
    w = OUT / "counter"
    w.mkdir(parents=True, exist_ok=True)
    probe = "static int probe486;\nint* use486(void) { return &probe486; }\n"
    kinds = {
        "(none)": lambda i: "",
        "extern int v;": lambda i: f"extern int v{i};",
        "int v; (definition)": lambda i: f"int v{i};",
        "static const float v = 1.0f;": lambda i: f"static const float v{i} = 1.0f;",
        "typedef int T;": lambda i: f"typedef int t{i};",
        "struct S { 4 fields };": lambda i: f"struct s{i} {{ int a, b, c, d; }};",
        "typedef struct S {..} T, *PT;": lambda i: f"typedef struct s{i} {{ int a; }} T{i}, *PT{i};",
        "enum with 3 members": lambda i: f"enum e{i} {{ E{i}a, E{i}b, E{i}c }};",
        "void f(void);": lambda i: f"void f{i}(void);",
        "int f(int a, int b);": lambda i: f"int f{i}(int a, int b);",
        "redeclaration of extern int": lambda i: "extern int v0;",
        "#define": lambda i: f"#define M{i} 1",
        "int f(int x) { return x; }": lambda i: f"int f{i}(int x) {{ return x; }}",
    }
    names = []
    for j, (k, f) in enumerate(kinds.items()):
        pre = "extern int v0;\n" if k.startswith("redecl") else ""
        (w / f"k{j}.c").write_text(pre + "".join(f(i) + "\n" for i in range(10)) + probe)
        names.append(f"k{j}")
    compile_many(w, names)
    base = None
    print("| Construct | Counter cost |\n| --- | ---: |")
    for j, k in enumerate(kinds):
        s = int(re.search(r"probe486\$S(\d+)", (w / f"k{j}.asm").read_text(errors="replace")).group(1))
        base = s if base is None else base
        if j:
            print(f"| `{k}` | {(s - base) / 10:g} |")


def order() -> None:
    """Mechanism 1: memory operands of a float product, by declaration order."""
    w = OUT / "order"
    w.mkdir(parents=True, exist_ok=True)
    fab = "float f(const int* xy) { int s = 6 - xy[0]; return (float)s * a * b; }\n"
    fba = "float f(const int* xy) { int s = 6 - xy[0]; return (float)s * b * a; }\n"
    v = {}
    for src, fn in (("s*a*b", fab), ("s*b*a", fba)):
        for first, second in (("extern float a;", "static const float b = 25.0f;"),
                              ("static const float b = 25.0f;", "extern float a;"),
                              ("extern float a;", "extern float b;"),
                              ("extern float b;", "extern float a;")):
            for gap in (0, 255, 300):
                key = f"v{len(v)}"
                v[key] = (src, first, second, gap,
                          first + "\n" + pad(gap) + second + "\n" + fn)
    for k, row in v.items():
        (w / f"{k}.c").write_text(row[4])
    compile_many(w, list(v))
    print("| Source | First declared | Then | Gap | fmul order |\n| --- | --- | --- | ---: | --- |")
    for k, (src, first, second, gap, _) in v.items():
        body = function_body(w / f"{k}.asm", "_f") or []
        muls = [l.split()[-1].lstrip("_").split("$")[0] for l in body if l.startswith("fmul")]
        print(f"| `{src}` | `{first}` | `{second}` | {gap} | {', '.join(muls)} |")


def isolate() -> None:
    """Mechanism 2 in a minimal TU: Draw_Pixel24 against N file-scope pads."""
    subprocess.run([sys.executable, str(HERE / "sweep.py"), "tools/experiments/486/pixel24.c",
                    "/* PADS */", "_Draw_Pixel24", "--range", "0", "1100", "1",
                    "--out", str(OUT / "pixel24")], check=True)


def phase() -> None:
    """Mechanism 2 in the real units (matching-build flags)."""
    runs = [
        ("src/gods98/draw.c", '#include "crt/crt.h"',
         ["_viewport_subrectangle_set", "_Draw_Pixel16HalfTrans", "_Draw_Pixel24"], "0 1024 1"),
        ("src/game/lava_erosion.cpp", '#include "cmd_shared.hpp"',
         ["_lava_erosion_delay_from_cell"], "0 128 1"),
        ("src/game/steering.cpp", '#include "gods98/record_shared.hpp"',
         ["?vector_scale_components_in_place@"], "0 1024 2"),
        ("src/game/cmd.cpp", '#include "cmd_shared.hpp"',
         ["?cmd_place_ray_candidate_check@"], "0 512 1"),
    ]
    for src, anchor, funcs, rng in runs:
        print(f"## {src}")
        subprocess.run([sys.executable, str(HERE / "sweep.py"), src, anchor, *funcs,
                        "--range", *rng.split(), "--out", str(OUT / Path(src).stem)], check=True)


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    for name, fn in (("counter", counter), ("order", order), ("isolate", isolate), ("phase", phase)):
        if what in (name, "all"):
            print(f"# {name}")
            fn()
