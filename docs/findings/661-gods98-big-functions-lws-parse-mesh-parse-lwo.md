# Finding 661 — gods98's two largest non-exact functions: Mesh_ParseLWO and Lws_Parse

Issue #774, part of #388 and ADR 0005. A dedicated pass on gods98's two
largest non-exact functions, rather than a sweep's short budget per function:
`00480ac0` Mesh_ParseLWO (`mesh_parse_lwo`, 3,424 bytes, 38.57%) and
`004869f0` Lws_Parse (`lws_parse`, 3,280 bytes, 94.80%).

Measured on `main` `bf6335e3` with one full matching build and `make
reccmp-check` of this change.

| Measure | `main` `bf6335e3` | This change |
| --- | ---: | ---: |
| `00480ac0` mesh_parse_lwo | 38.57% | **78.43%** |
| `004869f0` lws_parse | 94.80% | 94.80% |
| Functions that rose / fell, full | | 1 / **0** |
| Exact / mapped, full | 2,408 / 2,833 | 2,408 / 2,833 |
| Exact-function code bytes | 335,096 (52.15%) | 335,096 (52.15%) |
| Accuracy, full / game-only | 97.02% / 96.68% | **97.04% / 96.70%** |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| `opt_ref` annotated removed | 0 | 0 |

No function fell below the baseline. `make match-progress`, `make
unit-data-offsets` (no tracked file changed) and `make pe-audit` were run on
the final tree and the generated files are committed. `make tools-test` (no
`--allow-growth`), `make winapi-link-check` and native ctest (37 of 37) pass,
and the native build starts.

In `binary-progress.json` the `/OPT:REF` relink's categories move two
functions (128 bytes) from `original_annotated` to `port_unit`. No annotation
changed and no function is removed (`make opt-ref-callers
OPT_REF_ARGS=--summary`: 0 removed). As in finding 654, the figure comes from
translating the gate's report rows into the relink after a code-size change,
here mesh_parse_lwo's; the cause was not identified further.

Only `src/gods98/mesh_unit.c` and `src/gods98/mesh_maths.h` change. No other
thread's file is touched.

## Source of the shapes

Earlier passes reconstructed Mesh_ParseLWO from the listing alone. This pass
compared both functions with OpenLRR's `Mesh.cpp` and `Lws.cpp`
(trigger-segfault/OpenLRR, `src/openlrr/engine/gfx/`). OpenLRR rebuilds
Gods98's own source: its comments note code the Gods98 source had commented
out, and it keeps the original's `Error_Fatal` checks, which compile away in
the shipped build. Where its Mesh_ParseLWO differs from ours, the listing
follows OpenLRR, so it is the reference for the shapes below. Lws_Parse
already had OpenLRR's shape.

## 1. `00480ac0` Mesh_ParseLWO: 38.57% to 78.43%

### Divergent regions and what fixed them

| Region | Before | Fix | Effect (single-function loop) |
| --- | --- | --- | --- |
| The four inlined Maths_Vector3DNormalise expansions | the dot product was computed into x87 registers with no stack copies; two expansions had no y/z temporaries, so the frame was `0xa0` against the original's `0xb0` | the dot product's products in parentheses, `(a->x * b->x) + (a->y * b->y) + (a->z * b->z)` (`mesh_maths.h`). MSVC 5.0 keeps a parenthesised float subexpression as written; the same expression without parentheses gives the old code | 38.57 → 61.86; frame `0xb0`; every expansion copies y and z to stack temporaries, as the original does |
| Allocations, face-normal loop, vertex loop | per-surface offsets recomputed in each subscript, `aoVerts` read as `Mesh_Vector3F`, a `surfCount` local, separate count increments | OpenLRR's shape: one `surfArraySize`/`surfFDArraySize` per face, `aoVerts` read as floats at `index * 3`, `faceDataQuad` as an initialised `const` array, post-increments inside the face-data subscripts, Maths_Vector3DAdd as `(vn, vn, faceNorm)` | 61.86 → 74.70; instruction-for-instruction the original's code up to the stack offsets |
| Group loop render flags | `alter(ADDITIVE)` then `renderFlags = ADDITIVE`; `alter(renderFlags \| 0x800000)` | `renderFlags` set before the call and accumulated with `\|=`, as the original (`push 0x2800` with the store to the flags slot; `or eax, 0x800000`) | +0.3 |
| Group loop colours | `lwSurf` reassigned before each colour call | `mesh->lightWaveSurf[loop]` directly, as OpenLRR | +1.1 |
| Loop counters | three counters (`loop`, `loopVert`, `loopSurf`), each loop its own | the vertex loop and the group loop share one counter; the face-normal loop has its own. An exhaustive search of the five counters over three names (144 assignments) gives the best score only with that sharing | +1.5 |

### The symbol counter (rule B)

The listing-faithful version is shorter. With the group loop indexed as in
OpenLRR (`&surfVertices[loop * size]`, which MSVC strength-reduces to the
same pointers), one shared `vertexIndex` and no `lwSurf`, it scores
**85.20%**. But it declares 11 fewer symbols than the old function, and every
later function in the unit moved by −11 (`tools/experiments/699/counter_probe.py`).
Two exact functions fell: `00482620` mesh_set_vertices_same_normal (100 →
88.70) and `00483420` mesh_set_group_material_values (100 → 95.15). Each is
one integer `+` whose operand order follows the phase. A `sweep.py` run after
mesh_parse_lwo (N = 0–79) gives a period of 128 for both. They have the
original's code only for N ≡ 9–64 against the 85.20% version, that is, a
counter shift of −2 to +53 against `main`.

What was tried to restore those symbols with real declarations:

- **More locals in mesh_parse_lwo.** mesh_parse_lwo has one code at every
  phase (N = 0–129 before its annotation), but each extra local changes its
  register allocation. Combinations of a `lwSurf` local, per-block
  `vertexIndex` and a separate group-loop counter cost 2.5 to 18.8 points.
- **The two dropped functions.** The `operands`, `decls` and `stores`
  searches find nothing: 28 and 89 operand orders, and no decl or store
  groups.
- **The whole of Gods98's Maths.h.** Restoring Maths.h's other declarations
  at `mesh_maths.h`'s include would be well founded, since the original
  Mesh.c included Maths.h. But a sweep there (N = 0–139) shows twelve
  phase-sensitive functions with windows that conflict (edges at 7, 10, 12,
  33, 34, 38, 42, 47, 50, 53 and 63), so no single count is safe without the
  real header's exact cost.

What is committed is counter-safe at −2:

- The group loop keeps the old pointer-increment form (`faceDataPtr`,
  `surfVerticesPtr`, `surfNormalsPtr`, `textCoordsPtr`,
  `surfVertexCountPtr`) and its `lwSurf`/`lwo` locals. It is equivalent, and
  it carries over the render-flag and colour fixes above.
- Two declarations from the Gods98 source that emit no code:
  - mesh_parse_lwo's unused `textCoords[][2]` table, which OpenLRR keeps as
    "UNUSED";
  - mesh_get_surf_info's `tempNum = texNum`, used only by a search the Gods98
    source had commented out.

  Neither is a filler: both are in the original source as OpenLRR records
  it. Neither function's code changes (97.98% stays).

That leaves mesh_parse_lwo at **78.43%**, 6.8 points under the shorter
version. The counter probe reports −3 at `00481820` and −2 from `00481ac0`
on, and every other function in the unit scores as on `main`.

### What remains

- **Stack slot order.** After the fixes the instructions match. The
  differences are stack offsets (the original's `[esp+0x28]` maxVertices,
  `[esp+0x5c]` fDSize, `[esp+0x14]` shared temporary, and so on), plus the
  register choices that follow from them. Declaration order does not affect
  them: the `decls` search tried 203 orders of the 85.20% version, all equal.
  They change with the set of variables and their uses.
- **The fourth normalise (quad branch, reference found).** The original
  stores the modulus and then the factor to `[esp+0x14]` (`fst`), reloads x,
  and pops a dead copy later (`fstp st(0)`). Expansions 1–3 do not do this.
  The same shape is in `00482500` (77.17%) and was finding 625's "fst"
  note. Three local-variable forms of the helper change nothing (the original
  `fst` shapes elsewhere, e.g. `004425f0`, come from a float local that is
  read again later, which is not the case here).
- **The 85.20% version.** It becomes committable once the unit's real
  declaration count is known. For example, a faithful Maths.h whose measured
  cost lands every phase-sensitive function in its window would do it. The
  steps are recorded above.

## 2. `004869f0` Lws_Parse: 94.80%, unchanged

### Divergent regions

| Region | Original | Rebuild |
| --- | --- | --- |
| Loop head (`lws.c:305`) | `mov [esp+0x14], eax` (parentList) then `mov eax, [esp+0x20]` (the file) | the file load hoisted above `add esp, 4` into `ecx` |
| Node-list growth (`lws.c:322–324`) | `nodeList`'s `realloc` result stored at once; the size and `parentList` reloaded into `eax` | the store sunk below the second call's argument set-up (`ecx`, `edx`) |
| Dissolve clamps (`lws.c:395–398`) | `eax`, `ecx`, `edx`, `ecx` for the successive temporaries | the same rotated by one register, and one `add ecx, esi; fld [ecx]` against `fld [eax+esi]` |
| Calls | File_Open, File_GetLine, File_Close, `_strnicmp` | the same functions (the single-function loop shows the port names; the full report maps them) |

All three code regions have one cause. The original gives the next temporary
`eax` as soon as a call's result is stored, and the rebuild keeps `eax`
busy. It is register allocation (cause D/C), not data.

### What was tried

- **Rule B.** `sweep.py` over N = 0–1023 before the unit's functions gives
  one code for lws_parse, so the counter phase plays no part. Finding 561's
  note that `sweep.py` cannot judge this function was a normalisation gap:
  the `$SG` string labels were not normalised. With them normalised there is
  one code.
- **`make match-search`**: `decls` (59 orders), `stores` (3) and `operands`
  (1,378, with pairs). No improvement.
- **OpenLRR's shape.** These change nothing (94.07% in the single-function
  loop, 94.80% in the report):
  - `argc` assigned from the tokeniser;
  - `argc`, `loop`, `parent` and the keys declared in the outer block, in
    OpenLRR's order;
  - the outer declarations as `ifp`, `scene`, `path`;
  - `sizeof(LwsNode) * scene->nodeListSize` for the first allocation.
- Finding 650 already tried the `realloc` sizes and `+=` spelled out.

### What remains

The register rotation, from the loop head on. Nothing found shifts it.

## Reproducing

    make match-function ADDR=0x00480ac0
    python3 tools/experiments/699/counter_probe.py src/gods98/mesh_unit.c origin/main --headers src/gods98/mesh_maths.h
    python3 tools/experiments/486/sweep.py src/gods98/mesh_unit.c '// FUNCTION: LEGORR 0x00481820' \
        _mesh_set_vertices_same_normal _mesh_set_group_material_values --range 0 80 1
    make match-search ADDR=0x004869f0 MODE=operands SECONDS=600

`sweep.py`'s fingerprint keeps `$SG` (string-literal) labels, so functions
with string literals show a new code at every N. Normalising
`\$[A-Z]*[0-9]+` gives the true count; this pass did so by hand.
