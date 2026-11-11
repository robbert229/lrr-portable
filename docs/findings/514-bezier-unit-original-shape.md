# Finding 514 — Bezier curve unit (GAME-003) rebuilt in original shape

Issue #417. This conversion follows the
[ADR 0005](../adr/0005-original-shape-c-units.md) recipe, starting from the
`tools/convert_unit.py` scaffold. It rebuilds the cubic Bezier sampler and
its arc-length helpers: original translation unit GAME-003, text
`004065f0`–`00406990`, 7 functions
([finding 505](505-original-translation-units-and-link-order.md)). It is the
second game-side conversion, after Priorities.c (finding 506).

"Before" is the *match baseline* on `main` at `0cfa8749` (#426). "After" is
this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 890 bytes) | 68.64% | **78.74%** |
| Exact functions | 3 | 3 of 7 |
| Mapped functions | 7 | 7 |
| Game-only mean (2,128 functions mapped in both builds) | 49.67% | 49.72% |
| Full mean (2,347 functions mapped in both builds) | 53.99% | 54.04% |

Byte weights are the original's code bytes, without padding.

| Address | Function (corpus name) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `004065f0` | `cubic_bezier_point_evaluate` (CubicBezierPointEvaluate) | 318 | 85.22 asm | 56.88 |
| `00406730` | `vec2_dist_alt` (Vector2DistanceVariant) | 45 | 100.00 | **100.00** |
| `00406760` | `vector2_set_length` (Vector2SetLength) | 74 | 100.00 asm | **100.00** |
| `004067b0` | `polyline_segment_lengths_total` (PolylineSegmentLengthsAndTotalCalculate) | 98 | 29.03 | 97.22 |
| `00406820` | `bezier_curve_samples_build` (BezierCurveSamplesBuild) | 149 | 75.63 | 98.08 |
| `004068c0` | `path_arc_length_interpolate` (PathArcLengthInterpolate) | 192 | 34.25 | 75.76 |
| `00406980` | `scene_object_mode_is_one` (SceneObjectModeIsOne) | 14 | 100.00 | **100.00** |

"asm" marks a function that was naked `__asm` before this change. The unit
now has no hand-written assembly. The two former asm functions (392 bytes) went
from 88.01% to 65.02% byte-weighted, and the five that were already C
(498 bytes) from 53.39% to 89.53%.

## The boundary

The spec is **confirmed as it stands** for the six curve functions; the
seventh stays where the map put it. Confidence stays **medium**.

- **Start (`004065f0`).** AITask.c (GAME-002) ends with its two unreferenced
  inline accessors (`004065d0`, `004065e0`) and its `.rdata` ends with an
  unreferenced 99999.0 at `0049e030`. GAME-003's `.rdata` block opens at
  `0049e034` with the 16-byte `{1, 2, 4, -1}` header table, which finding
  505 counted in one game object: this one.
- **`.rdata`.** After the table come an unreferenced 1.0 (`0049e044`), then
  the unit's three constants: 3.0 (`0049e048`, the evaluator), 0.0
  (`0049e04c`, the arc-length walk) and 1.0 (`0049e050`, the sample
  builder). No instruction anywhere reads `0049e030`, `0049e034`–`0049e043`
  or `0049e044`. GAME-004's first constant follows at `0049e054`.
- **No `.data` or `.bss`.** The unit has no state and no literals.
- **Padding.** All seven functions are 16-byte aligned with `0x90` fill.
- **End (`00406980`).** SceneObjectModeIsOne (14 bytes, `return
  obj->field_28 == 1`) has no data and one caller, LegoObject's
  `0044a070`, which also calls GAME-004's `00406f80` on the same objects.
  Nothing in the data places it on either side of `00406990`, where
  GAME-004 first uses its `.bss` and literals. It stays at the end of this
  unit, as the map has it. If GAME-004's conversion shows it belongs there,
  it moves without affecting the curve functions.

No neighbour needed converting: the unit calls nothing outside itself.
"BezierCurve.c" fits the alphabetical game order (between `AITask.c` and
`Bubbles.c`) but is not attested, so the spec name stays empty.

## The unit

[`src/game/bezier.c`](../../src/game/bezier.c) and its header
[`bezier.h`](../../src/game/bezier.h) hold the 7 functions in original
order. The functions keep the port's names.

- **Types.** `bezier.h` defines the original's 0x25c-byte `BezierCurve`
  record: a count, 50 `Point2F` points from `+0x004` and 50 segment lengths
  from `+0x194` (`distances[0]` is zero). `Point2F` comes from
  `gods98/images.h`. The functions take `Point2F*`/`BezierCurve*` where the
  port took `float*`/`uint32_t*`; the ABI is the same.
- **`.rdata`.** The three constants are annotated statics in original order.
  The `{1, 2, 4, -1}` table and the unreferenced 1.0 are not reproduced, as
  in `maths.c` and `config.c`.
- **Contracts changed to the original.**
  - `vector2_set_length` returns its argument (`eax` holds it in the
    original); the port's guarded wrapper and its naked target are gone, so
    a zero vector or length now divides by zero, as in the original.
  - The evaluator, the sample builder, the length table and the walk lost
    the port's NULL guards.
  - `path_arc_length_interpolate` takes `float distance` and returns the
    segment index, as before.

### Shape notes

- **CubicBezierPointEvaluate** computes the control points
  `c1 = p0 + 3 * p1` and `c2 = p2 - 3 * p3` (`p1` and `p3` are end
  tangents), then the Graphics Gems polynomial coefficients. The original
  writes `r->x` before it reads `p0.y` and `p2.y` again, so the source
  computes both control points first, then x, stores `r->x`, then y. The
  order of the local declarations and of the terms in `r->x` changes MSVC's
  x87 register choice (finding 506). About 250 variants were scored; the
  committed one is the best, at the original's length plus 8 bytes. What
  remains is x87 stack order and which of `c1.y`/`c2.y` get stack homes.
  With a `3.0f` literal instead of the annotated static the best variant
  scored 47.71%.
- **BezierCurveSamplesBuild** divides a `1.0f` *literal* by `count - 1`.
  MSVC hoists `1.0f / (float)(count - 1)` out of the loop for a constant,
  as the original does, but not for a `const` variable, which scored
  92.31%. The one remaining difference is the constant's operand: Reccmp
  names the compiler's `1.0` instead of `0049e050`. The annotated static
  keeps the table's layout and is otherwise unused, like `maths.c`'s 2.0.
- **PolylineSegmentLengthsAndTotalCalculate** stores the distance in a local
  before adding it to the total (`fst [edi]; fadd total`). The remaining
  difference is one `fadd` stack offset.
- **PathArcLengthInterpolate** keeps the original's dead `*r = points[i - 1]`
  copy and a `delta` temporary. The original stores the fraction in the
  `distance` argument's slot; this build keeps it in a register. Inline
  vector helpers and reusing `distance` scored lower or equal.

## Callers

`edges.csv` lists five outside callers, all of which the port had
reimplemented without calling the unit:

| Caller | Original calls | Port |
| --- | --- | --- |
| `0046fc70` ObjectFlightCurveRecordInit (`effects.cpp`) | set-length ×2, samples build, length table | **now calls the unit** |
| `0046f920` (projectile tick, `effects.cpp`) | arc-length walk | **now calls the unit** (`active_effect_curve_sample`) |
| `00444680`, `004438b0` (`entity.cpp`) | set-length, samples build, length table; walk | own straight-line mover, unchanged |
| `0044a070` (`entity.cpp`) | SceneObjectModeIsOne | reads its own runner flag, unchanged |

**Why the effects callers had to move.** With no caller, the matching link
never pulled `bezier.obj` out of the `game` library, and all seven functions
dropped out of the report (`make reccmp-check`: "newly unmapped"). Routing
the flight curve through the unit fixes that and restores the original's
path:

- The port drew a Bernstein curve through `(0, 0)`, `(1, -1.3)`,
  `(distance, dz)`, `(0.3, 1)`. The original treats `(1, -1.3)` and
  `(0.3, 1)` as tangents, scaled to a fifth and a third of the straight-line
  distance, and samples 50 points. `active_effect_curve_samples_build` now
  does that, and the flight duration is the path's length, as before.
- The original embeds the `BezierCurve` at `+4` of its 0x2b0-byte effect
  record (`00503920`). The port's `ActiveEffectRecord` has its own layout,
  so the paths are a file-local `s_active_effect_paths[10]` beside the
  records. The record's `sample_count`, `curve_samples` and
  `curve_segment_lengths` fields are now unused; `effects.hpp` only gained
  comments saying so, because removing fields from a header with 13
  includers renumbers them all. The effects unit's own conversion removes
  them.

`ObjectFlightCurveRecordInit` rose from 17.14% to 17.25%.

The `entity.cpp` movers keep the port's design; converting them belongs to
LegoObject's unit.

**Tests.** The curve tests moved from `test_view.cpp` and `test_record.cpp`
to a new game test, [`test_bezier.cpp`](../../src/game/test_bezier.cpp),
because the gods98 tests do not link `game`. It checks the record layout,
the evaluator's ends, the distance and set-length helpers, the sample
builder's count clamp, the length table and the arc-length walk. Two old
checks went away with the port's deviations: the NULL guards and the
zero-length guard.

## Entropy

This change refreshes the *match baseline* under ADR 0005 entropy rule 2.
`make reccmp-check` on a clean checkout of `0cfa8749` passes, so every
change below is this change's.

**Declarations kept for the count (rule 1).** Deleting the unit's six
superseded declarations from `view.hpp` (recipe step 7) renumbered every
file that includes it. Four functions in unchanged files fell, among them
`lava_erosion_delay_from_cell` (`0040e9c0`), from 100.00% to 84.21% with the
same 16 instructions in another order. Deleting SceneObjectModeIsOne's
declaration from `record.hpp` moved `terrain_cell_normal_sample`
(`004512d0`) from 11.22% to 10.31% in the same way. Restoring each header
alone brought its functions back exactly. So, following rule 1's preference
for zero-drift techniques:

- `view.hpp` keeps the six old C++ declarations (`vec2_dist_alt`,
  `vector2_set_length`, `cubic_bezier_point_evaluate`,
  `bezier_curve_samples_build`, `polyline_segment_lengths_total`,
  `path_arc_length_interpolate`) with their old `float*`/`uint32_t*`
  signatures. Nothing defines or calls them. A comment says they are kept
  only for the declaration count (finding 506) until the #388 relocation.
  They overload the `extern "C"` versions in `bezier.h` without
  conflicting, because the parameter types differ.
- `record.hpp` keeps `scene_object_mode_is_one`. Its signature is the same
  as `bezier.h`'s, so a C++-linkage copy would conflict in `effects.cpp`,
  which includes both. It is declared `extern "C"` in place instead, so it
  declares the real function, and `test_c_boundaries.cpp` checks it against
  `bezier.h`.

With both, every one of those functions is back at its `main` score, and
the drift rises that the deletions caused are gone too.

**What remains.** Outside the unit, one function rose and one fell. Inside
it, 3 rose and 1 fell (the evaluator, asm to C, above).

| Address | Function | File | before | after | Listing difference | Class |
| --- | --- | --- | ---: | ---: | --- | --- |
| `0046fc70` | `object_flight_curve_record_init` | `effects.cpp` | 17.14 | 17.25 | now calls the unit, as the original does | callee contract |
| `004882e0` | `scene_node_transform_matrix_apply` | `view.cpp` | 14.00 | 13.97 | x87 stack order, `fxch` placement and spill order; one extra instruction net | drift, rule 2 (see below) |

`scene_node_transform_matrix_apply`'s source did not change, but its file
did: `view.cpp` lost the unit's definitions and its `0049e050` static,
which the conversion must delete. Single-function listings from `0cfa8749`
and from this change differ only in x87 scheduling.

**For the owner.** That listing is one instruction longer (an `fxch`).
Findings 510 and 512 classified x87 stack exchanges as rule 2 drift, and it
is classified the same way. The drop is 0.03 points.

**Net effect, both scopes:** 4 functions rose and 2 fell, a net of +103.89
percentage points. No function was newly mapped or lost.

**Whole binary** (`make match-progress`, `docs/coverage/binary-progress.json`):

| Metric | before | after |
| --- | ---: | ---: |
| Accuracy, game-only | 49.74% | 49.79% |
| Accuracy, full | 54.07% | 54.12% |
| Exact-function code bytes | 40,883 | 40,883 |
| `.text` identical at original addresses | 18,260 (2.84%) | 18,239 (2.84%) |
| Adjacent order inversions, full | 908 | 907 |

## Native port

`cmake -S . -B build && cmake --build build && ctest --test-dir build
--output-on-failure` builds with no errors, and **all 33 tests pass**,
including `c_boundaries` and the new `bezier`. `bezier.c` passes
`gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only -Wall`.

Behaviour changes in the native port:

- Flight-curve projectiles follow the original's tangent-scaled path
  instead of the port's Bernstein approximation.
- The evaluator now computes in single precision (the port's fallback used
  double), and the curve helpers no longer guard NULL or zero lengths.

## Reproducing

    python3 tools/translation_units.py dossier 4065f0 406990
    python3 tools/convert_unit.py GAME-003 --stem bezier --dry-run
    make match-function SOURCE=src/game/bezier.c
    make reccmp-check
    python3 tools/translation_units.py build
