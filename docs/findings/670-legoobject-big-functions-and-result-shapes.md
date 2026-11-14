# Finding 670 — LegoObject.c's big near misses, and the single-exit shapes that close smaller ones

Issue #795, part of #388 and ADR 0005. Shape rewrites toward the original
for LegoObject.c's three biggest non-exact functions, then the next ones
that had a structural (not register-choice) difference.

Measured on `main` `1f5d8a1e` with one full `make reccmp-check` before the
change and one after.

| Measure | `main` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 338,194 (52.63%) | **339,416 (52.82%)** |
| Exact / mapped, full | 2,423 / 2,833 | **2,426** / 2,833 |
| Functions that rose / fell | | 8 / **0** |
| Reccmp accuracy, full / game-only | 97.10% / 96.76% | **97.15% / 96.82%** |
| `.text` identical at original addresses | 5.34% | 5.35% |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| Function-order inversions, full / game-only (pairs) | 222 / 1 | 222 / 1 |

No function fell below the baseline, so no drop needs a label. `make
match-progress`, `make unit-data-offsets` and `make pe-audit` were run on
the final tree, and the generated files are committed (`unit-data-offsets`
changes no tracked file). `make tools-test` (no `--allow-growth`), `make
winapi-link-check` and native ctest (37 of 37) pass, and the native build
starts. Only `src/game/legoobject_unit.c` changes; no header changes. There
is no `#pragma`, no pad and no `Deviation (scoring only)` in the change.

LegoObject.c's object is 16 bytes longer, so every later object starts
0x10 later in the gate link. No function changed score from that.

## 1. Per-function results (full report)

Sizes are padded (16-byte) function sizes: the original's, then the rebuilt
one before → after.

| Address | Function | `main` | After | Original | Rebuilt | Shape |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `0043c810` | legoobject_update_callback | 89.21 | **89.35** | 9,872 | 9,856 → 9,856 | §2.5 |
| `00444680` | legoobject_step_movement_vector_resolve | 64.85 | 64.85 | 2,352 | 2,352 → 2,352 | stalled, §3 |
| `00447ea0` | legoobject_type_behavior_update | 68.05 | **69.53** | 2,336 | 2,352 → 2,320 | §2.3, §2.4 |
| `00446e40` | legoobject_nearest_cell_route_commit | 45.95 | **100** | 656 | 656 → 656 | §2.1, §2.2 |
| `0043a120` | legoobject_cell_step_command_issue_with_jitter | 81.00 | **100** | 480 | 480 → 480 | §2.2 |
| `0044b140` | legoobject_small_spider_cell_swarm_spawn | 62.82 | **100** | 272 | 256 → 272 | §2.6 |
| `004473b0` | legoobject_route_to_cell_command_issue | 60.91 | **88.79** | 320 | 304 → 304 | §2.1 |
| `004469c0` | legoobject_nearest_connected_cell_route_select | 82.72 | **87.84** | 1,152 | 1,136 → 1,152 | §2.1 |
| `00449980` | legoobject_activity_terrain_eligibility_transition | 75.52 | **77.42** | 240 | 240 → 256 | §2.1 |

`00449980` scores higher but no longer agrees in size: its `animTime <= 0`
early `return 1` gets its own epilogue, where the original jumps to the
shared `mov eax, 1` tail (forms tried in §3).

## 2. Shapes that worked

### 2.1 One `result`, one exit

The original keeps an `int result` in a stack slot and returns it at one
merge point. MSVC 5.0 does not propagate a constant into a merge point that
has several reaching definitions. It loads `result` from memory there, and
then duplicates the small return block into its predecessors. That leaves
`mov dword ptr [esp+X], 1; mov eax, dword ptr [esp+X]` or `mov eax,
dword ptr [esp+X]` before an epilogue, which the port's early `return 1;`
or `return 0;` never produces.

- `00446e40`: `result = 0` at the top, `if (...) { if (diagonals) {...}
  else {...} result = 1; } return result;`. The `0` lives in the slot that
  the unsigned-to-float conversions (`fild qword`) also use.
- `004469c0`: the found-step branch and the search branch become the two
  arms of one `if`/`else`, so the found path skips the `free` loop and both
  reach the one `return result`.
- `004473b0`: the route-found block falls through to `return result`, but
  the path-check failure keeps its own `return 0` (the original has both a
  `xor eax, eax` exit and a `mov eax, ebx` exit).
- `00449980`: the `flags1 & 0x40` test wraps the body, so the `return 0`
  path is placed last, after the main epilogue.

### 2.2 The route block offset is copied as a Point2F

`blockOffset = blockOffsets[index]` (`00446e40`) and `blockOffset = offset`
(`0043a120`) are struct assignments in the original: one `lea` of the
source and two dword moves. The two float stores became
`*(LegoObjGE_Point2F*)...blockOffset = *(LegoObjGE_Point2F*)src;`, the form
group R already uses (`LEGOOBJ_R_LAST_ROUTE`). Both functions are exact.

### 2.3 The boulder's step uses the inline Maths helpers

In `00447ea0`'s boulder branch the original stores `1.0f / len` into the
dead `elapsed` slot and copies `speed` into the dead `obj` slot before
multiplying. Those are the materialised `f` parameters of two inlined
`Vector3_Scale` calls. `len = legoobject_o_vector3_modulus(&dir)` and two
`legoobject_o_vector3_scale` calls give the same pattern (+0.6 points in
the single-function loop).

### 2.4 Two calls in a ternary, not a ternary argument

`00447ea0`'s type-12 update calls `Container_MoveAnimation` twice: once
with `0` and once with `elapsed`, with the call tail-merged. A ternary
argument stores `0` into `elapsed`. `(cond ? f(o, 0.0f) : f(o, elapsed)) !=
0` gives the original's branch layout (+0.85).

### 2.5 Operand order of the upgrade-cost comparisons

`0043c810` compares `cmp eax, ebx; jb` (resource count first). Writing
`lego_upgrade_resource_check(n) >= cost` keeps the call order and swaps the
compare (two sites, +0.14).

### 2.6 A struct local is not packed into a dead parameter slot

`0044b140` keeps the spawn position in two frame slots (`sub esp, 0x44`),
while the heading reuses the dead `count` slot. With scalar `x`/`y`, MSVC
5.0 packs them into the dead `bx`/`by` slots (`sub esp, 0x3c`, 72.73). As
a `Point2F` they stay in the frame (87.27). The original also computes the
spawn count before `Map3D_GetBlockVertexPositions`, which frees `count`'s
slot for the heading: `count = (count & 3) + 2;` as a statement and
`for (i = 0; i < count; i++)` make it exact.

## 3. What stalled, and what was tried

- **`00444680`** (unchanged). The original's frame is `0x74`; the port's
  is `0x80`. The original packs `facing` into the slots that `adjusted` and
  the jitter use earlier, and `dot1`/`dot2` into the block-cell slots. The
  original also makes two `Map3D_BlockToWorldPos` calls per branch
  (`sx, sy` / `bx, by`, tail-merged), not one call after `sx = bx`. That
  rewrite reproduces the original's branch layout exactly but scores 52.95,
  because every slot offset and register after it moves. The jitter as one
  `Point2F` with `Vector2_Add` brings the frame to `0x78` but scores 46.71
  with it, 55.10 without. 2D instead of 3D normalisation, and a separate
  `Vector2_Scale` into `facing`, compile to identical code. The switch's
  `movzx ax, al; and eax, 0xffff` (an `unsigned short` switch value) is not
  reproduced by a cast; the cast is folded away. None was kept.
- **Declaration order does not move frame slots.** Random declaration
  permutations (150 for `00444680`, 120 for `004469c0`, each scored with
  `make match-function`) changed no instruction, except one +0.3 on a
  layout already discarded. This agrees with finding 581: MSVC 5.0 orders
  packed stack slots by use weight.
- **`0043c810`** (rest). Most of the remaining difference is stack-slot
  assignment and callee names (`make match-function` reads 86.9 against
  89.35 in the full report). Tried without effect: `>=` and `!(<)` forms of
  the upgrade-time compare (the original loads the spilled time and uses
  `fcomp st(1)`), and four casts of `objectGlobs_f_eadcc & 3` (the
  original's `test dl, dl` after `and edx, 3`). Not tried: the `fld`/`fstp`
  argument pushes of `field_2c8`/`field_2cc` (the port pushes them with
  `mov`), and the fused `fstp [esp+8]; add esp, 8` push of
  `Map3D_GetWorldZ`'s result (also in `004401b0`).
- **`00447ea0`** (rest). Register choice, and the electric fence's block
  position: the original uses two scalars, one of them in the dead `obj`
  slot. Function-scope or block-scope scalars both grow the frame (`0x54`,
  `0x50`), so the array stays.
- **`00449980`.** `else if (animTime > 0) {...} else return 1;` and the
  early `return 1` compile alike; neither reaches the original's shared
  `mov eax, 1` tail.
- **`0043aea0`** has one more frame dword than the port, which the original
  never reads (`[esp+0x14]`). It was not reproduced, because it could only
  be a guessed unused local.

## Follow-up

- `00444680` needs its frame first. The candidates are which values the
  original keeps live across the turn block, and whether `facing` is
  written only after the move vector's reciprocal is consumed.
- The single-exit `result` shape (§2.1) and the dead-parameter-slot
  packing rule (§2.6) are worth checking in other units' near misses. Look
  for `mov eax, dword ptr [esp+X]` before an epilogue, or a frame 4–8 bytes
  larger than the port's.
