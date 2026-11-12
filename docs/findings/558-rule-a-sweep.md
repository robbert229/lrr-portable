# Finding 558 — Rule-A sweep: few remaining x87 swaps are caused by declaration order

Issue #534, part of #388. Finding 538's rule A says that in an x87 product
of two named variables, MSVC 5.0 emits the variable declared *later* first.
Finding 541 found one way the port breaks it: units that see `extern`s of
their own definitions. This sweep looked for every function whose remaining
mismatch contains an x87 operand swap, and asked of each one whether
declaration order explains it.

**Result:** this survey was first run on `main` at `0a90abfa`. It was rerun
on `c86c2c72` (#540, the base of this change), which adds #541's GAME-027 and
GAME-002 relocations, #540's G98-012 and G98-010 relocations, and main's
GAME-026 conversion (#537). On that base it finds 9 functions with an
arithmetic operand swap and 12 more with only comparison swaps. Only one swap is
a rule A case: `0x004075a0`. Fixing it flips the operands but does not change
Reccmp's score, so it is not applied. One other swap, `0x0045c3f0`, is
expression shape, not rule A. Its fix is one line and is applied: 93.02% to
**97.67%**. No other candidate has a declaration-order cause that a fix could
reach. The drops that earlier findings labelled as x87 swaps, such as
`0x00444460`, are rule B or scheduling (below).

## Method

[`tools/x87_operand_swaps.py`](../../tools/x87_operand_swaps.py) (tested by
`tools/tests/test_x87_operand_swaps.py`, run by `make tools-test`; usage in
`tools/README.md`) reads Reccmp's `results.json`. For each function it takes
the x87 subsequence of each listing and counts adjacent pairs. A pair counts
as swapped when the original has `(op1 A, op2 B)` more often than the recomp
listing, and the recomp listing has the exchanged pair more often than the
original:

- `fld A; fmul B` against `fld B; fmul A` (also `fadd`);
- `fmul A; fmul B` against `fmul B; fmul A`;
- `fld A; fsub B` against `fld B; fsubr A` (also `fdiv`);
- `fld st(0); fmul [x]` against `fld [x]; fmul st(1)`;
- `fld A; fcomp B` against `fld B; fcomp A`, listed separately as `compare`.

Two things would hide a swap from a plain text comparison:

- Reccmp numbers float literals per side, so the tool ignores the number.
- The original side names only annotated globals. Unannotated ones read
  `<OFFSETn>`. The tool maps each placeholder to its recomp operand through
  aligned rows that agree apart from it. This mapping found `0x004075a0`.

`--loose` also masks base registers and stack offsets. Results are ranked by
*expected gain*: two instructions per swap over the longer listing.

Each candidate was then tested:

- with source variants under `make match-function`;
- with minimal compiles using the matching flags (`tools/experiments/486/msvc.py`);
- for rule B, with `sweep.py` (0–64 inert declarations).

`sweep.py` now normalises the counter number in goto labels
(`$message_tail$51964`). Before this, every N looked like a new code.

## Survey

Scores are the match baseline at `c86c2c72`. None of the rows from the
`0a90abfa` run changed score with the relocations. "Gain" is the tool's
estimate.

| Function | Unit | Score | Gain | Swap (original / recomp) | Cause | Fix | After |
| --- | --- | ---: | ---: | --- | --- | --- | ---: |
| `0x0045c3f0` advisor_progress_bar_fill_draw | GAME-032 `panel_unit.c` | 93.02 | 4.65 | `fmul [0.01]; fmul [air]` / `fmul [air]; fmul [0.01]` | **expression shape**, not rule A: MSVC reorders a flat product `n * 0.01f * air` with the variable first, whatever the declaration or source order. A parenthesised `air * (n * 0.01f)` keeps `n * 0.01f` together | `size.x = air * ((float)len * 0.01f);` **applied** | **97.67** |
| `0x00463d20` view_blit_rect_positions_apply | GAME-039 `info_track_overlay.cpp` | 9.43 | 3.64 | `fld [p+4]; fadd [q+0x14]`, loose | structural: the recomp is a different function shape; the operands are struct fields through pointers | none | = |
| `0x00453bb0` tutorial_frame_timer_array_advance | GAME-028 `tutorial_script.cpp` | 50.00 | 2.90 | `fld [timers+i]; fadd [delta]` / reversed | not rule A: one operand is an array element through a register. The `const` local against the reused parameter, `+=` against `x = x + d` against `x = d + x`, and a 0–64 rule B sweep all leave it unchanged. A minimal C compile gives the original's order only with a call between the scale and the loop | none | = |
| `0x0046b7c0` timed_effect_runtime_update | GAME-046 `tooltip.cpp` | 21.05 | 2.72 | `fld [delta]; fadd [rec->t]` / reversed, loose | not rule A: one operand is a field through a pointer. Minimal compiles put the lvalue first in every declaration arrangement, and the source order makes no difference. The recomp is 0x1c3 bytes against 0xf0 | none | = |
| `0x004882e0` scene_node_transform_matrix_apply | G98-020 `lws.cpp` | 14.00 | 1.37 | `fld [m+4]; fmul [s]` / reversed (×2), loose | **not investigated: G98-020 is in flight** (follow-up) | — | — |
| `0x004075a0` screen_effects_tick | GAME-005 `bubbles.cpp` | 9.32 | 1.16 | `fld [pulse_timer]; fadd [delta]` / reversed | **rule A**: the original's timer was first declared after the parameter `delta`, as a function-scope `static` would be. Moving `static float s_screen_effect_pulse_timer;` into the function gives the original's order | not applied: Reccmp's score is unchanged (0.093168 before and after), and the move would rename a data symbol | = |
| `0x00450f30` terrain_map_block_vertex_normals_update | GAME-026 `terrain_map.c` (new on `c86c2c72`) | 98.86 | 1.14 | `fld [offs+i].y; fadd [(float)vy]` / reversed | not rule A: one operand is an indexed array element and the other the converted parameter, held in a stack temporary. `+= (float)vy`, `y + (float)vy` and the committed `(float)vy + y` give the same code. `match-function` already calls it an effective match (100%\*) | none | = |
| `0x0043f7d0` ledger_stub_…EnergyLowTargetRouteCommandTick | GAME-023 `legoobject.cpp` | 28.83 | 0.71 | `fmul [0.04]; fmul [delta]` / reversed, loose | expression shape, as in `0x0045c3f0` | `delta * (rate * 0.04f)` gives the original's order, but the score is unchanged because the function's other code differs. Not applied | = |
| `0x00444460` ledger_stub_…FlaggedVectorStepCommandIssue | GAME-023 `legoobject.cpp` | 22.72 | 0.69 | `fld st(0); fmul [v]` / `fld [v]; fmul st(1)` | not rule A: in the original, the two products `v[0] * scale` and `v[1] * scale` load their operands in opposite orders. No declaration order gives both. This is the "x87 swap" drop that finding 548 recorded (23.61 → 22.72): a re-roll under rule B, not a declaration change. `scale * v[0]` changes nothing | none | = |

The `compare` hits (12 functions with only comparison swaps, plus `0x0043f7d0`) are all `fld A; fcomp B` against `fld B; fcomp A`:

- `0x0040de40` GAME-013;
- `0x00419030` GAME-018;
- `0x00425ce0` and `0x00435aa0` GAME-021;
- `0x0043f7d0` GAME-023;
- `0x0044e8c0` and `0x0044ead0` GAME-026 (new on `c86c2c72`; finding 556 gives their other causes);
- `0x00465430` GAME-041;
- `0x0046a7f0` GAME-045;
- `0x0046d1c0` GAME-048;
- `0x004702b0` GAME-050;
- `0x00477150` G98-003;
- `0x00487ed0` G98-020 (in flight).

Rule A, as measured, covers products. These are comparison direction
(`a < b` against `b > a`), a source-shape question for each unit's
conversion. None was changed here.

### Drops labelled as x87 swaps in earlier findings

These functions' current listings were checked against the original.

| Function | Labelled in | Finding here |
| --- | --- | --- |
| `0x00444460` | 548 | above: the original is not consistent with any declaration order |
| `0x00446610`, `0x0046ffb0` | 548, 551 | the swap was between two port builds ("="). Against the original, the x87 code differs throughout, with no isolated swap |
| `0x00418da0`, `0x0041c1c0` | 543, 548 | the same: 35.49% and 11.49%, with the x87 sequences different throughout |
| `0x004882e0` | 543, 545 | G98-020, in flight |

## Why rule A finds little to fix

- **Finding 541 already removed the self-`extern` cause.** Each relocated
  unit now has frozen per-unit headers. The one known case, `0x0040e9c0`,
  is fixed.
- **Rule A does not cover literals.** Minimal compiles of
  `(float)n * 0.01f * air` put the variable first in every arrangement:
  - a parameter or a global;
  - with or without a named prototype;
  - with the literal also used before or after the function.

  An original with the literal first is therefore a parenthesised
  expression, not a declaration order.
- **Most swaps involve an operand through a pointer or index.** Rule A's
  ordering applies to named variables. For `x += d` with `x` reached through
  a pointer, MSVC 5.0 loaded `x` first in every arrangement tested.
- **Swaps in functions far from matching rarely move the score.** Reccmp
  aligns the listings as a whole. Correcting two instructions inside
  regions that are already different gains nothing (`0x004075a0`,
  `0x0043f7d0`).

## Follow-ups

- **G98-020 (`lws.cpp`, in flight):** `0x004882e0`, two `fld`/`fmul`
  swaps of a matrix element against a stack scale, found in loose mode;
  `0x00487ed0`, two `compare` swaps. Rerun the tool after the conversion.
- **No candidates** in GAME-002, GAME-027, GAME-037, G98-010,
  G98-012 or the creature-scene files (`creature_scene`,
  `container_native`, `lwsc_miniwin`).
- **`0x004075a0`:** if GAME-005 is converted, a function-scope
  `static float` timer matches rule A. Decide it with the unit's data
  layout (`make unit-data-offsets`), not for this function's score.
- **Conversions:** run `x87_operand_swaps.py` on the unit's functions. A
  `rule-a` hit between two *named* operands is the case this rule fixes.
  Literal operands point to parenthesisation, and `compare` hits to the
  direction of a comparison.
- **No fix needed a frozen `_for_`/`_from_` header.**

## Gate

Measured on top of #540 (`c86c2c72`); every figure is this change's delta
over that base.

- `make reccmp-check`: OK. Exactly one function differs from #540's
  baseline: `0x0045c3f0`, 93.02 → 97.67. Nothing fell, so no exact match is
  lost and there are no drops to label. The rest of that function's
  difference is the call to `image_display_scaled`, which Reccmp labels
  differently on the original side.
- The unit's other 51 functions keep their scores under single-unit
  compiles. The edit adds no declaration, so rule B's counter is unchanged.
- Means: game-only 56.27%, full 60.47%, both unchanged at two decimals.
  Exact-function code bytes are unchanged (76,390). Unit-relative byte
  identity rose from 55,718 to 55,890 bytes (10.06% to 10.09%).
- `make match-progress` regenerated the three files. Its last step
  (`binary-progress-compare`) ran on the host, because the distrobox lacks
  `pefile`.
- `make tools-test` and native ctest pass.
