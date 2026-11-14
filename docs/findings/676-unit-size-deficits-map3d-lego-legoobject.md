# Finding 676 — Unit-size deficits: Map3D.c closes 160 of its 288 bytes; Lego.c's tail and LegoObject.c stay where they were

Issue #807, finding 668 follow-up 5, ADR 0005 and ADR 0006. Base: `main`
`316b70f6`. Each side was measured with one full matching build
(`make reccmp-report`, then `tools/port_tail_inventory.py` and
`make match-progress`). The base's unit spans were measured on `316b70f6`
itself, not copied from finding 668. That finding's −272 (Lego.c) and −176
(LegoObject.c) date from `d6f98b83`, and #800 and #801 have changed both
units since.

## Summary

| Unit | Original bytes | `main` `316b70f6` | This change |
| --- | ---: | ---: | ---: |
| GAME-026 Map3D.c (`terrain_map.c`) | 15,376 | 15,088 (**−288**) | 15,248 (**−128**) |
| GAME-021 Lego.c (`lego_unit.c`) | 90,800 | 90,544 (−256) | 90,544 (−256) |
| GAME-023 LegoObject.c (`legoobject_unit.c`) | 88,816 | 88,656 (−160) | 88,656 (−160) |
| Entry point (original `0048f000`) | | `0048d0d0` (−7,984) | **`0048d170` (−7,824)** |
| Unit segment end (original `0048d920`) | | `0048b9e0` (−8,000) | `0048ba80` (−7,840) |

- **Map3D.c:** six functions now have the original's padded size, four of
  them exact to the byte. The unit is 160 bytes closer to the original, and
  the entry point moves the same 160 bytes. What is left is
  `terrain_map_uv_wobble_update` (−112 padded) and
  `terrain_map_block_first_vertex_get` (−16).
- **Lego.c:** three functions after the counter-sensitive span of finding
  671 now have the original's padded size. In padded bytes these were +32,
  −32 and −16 against the original. Their changes cancel out (−48, +32,
  +16), so the unit's span does not move. Its other deficits lie inside
  that span or before it, where every edit moves the counter of nine
  phase-sensitive functions (finding 671). They were not attempted.
- **LegoObject.c:** unchanged. Its near misses (§4) need structural
  rewrites that this pass did not get to.

No function moves away from its original size. The identical prefix does not
shrink (11 B raw and 528 B masked, unchanged). In the Lego.c tail, the gain
in exact bytes does not cost any identity: the unit's span stays where it
was.

## Before and after

| Measure (`binary-progress.json`) | `main` `316b70f6` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | 340,131 (52.93%) | **340,467 (52.99%)** |
| Functions that rose / fell | | **9 / 0** |
| New exact functions | | 2 (`00435940`, `00451c50`) |
| Accuracy, full / game-only | 97.20% / 96.88% | **97.30% / 96.99%** |
| Correct-size functions, full: count / original bytes | 2,600 / 445,640 | **2,604 / 447,978** |
| Correct-size functions, game-only: count / original bytes | 2,309 / 394,813 | **2,313 / 397,151** |
| Exact functions (correct-size metric), full / game-only | 2,430 / 2,142 | 2,432 / 2,144 |
| Identical prefix (raw / address-masked) | 11 / 528 B | 11 / 528 B |
| Unit size agreement (whole / correct sizes / start / end, of 227) | 0 / 166 / 2 / 1 | 0 / 166 / 2 / 1 |
| `.text` identical at original addresses | 25,975 (4.04%) | 25,901 (4.03%) |
| Fully identical functions at original address | 8 (384 B) | 8 (384 B) |
| Function-order inversions, full / game-only (pairs) | 222 / 1 | 222 / 1 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

`.text` identical at original addresses falls by 74 bytes. Everything after
GAME-026 now starts 160 bytes later than on `main`, nearer its original
address but not at it. Some runs that matched by chance at the old offset
are lost. No function's code changed for this reason.

`opt_ref.by_category` moves one 32-byte function from `port_unit` to
`original_annotated` (2,533 to 2,534). This is the `/OPT:NOREF` relink flip
that findings 654, 659, 663, 668 and 671 also saw. Its cause was not traced.

`make reccmp-check` reports no function lower than the baseline, so no drop
needs a label. `make match-progress`, `make unit-data-offsets` and `make
pe-audit` were run on the final tree. The three generated files are
committed; `unit-data-offsets` changes no tracked file. `make tools-test`
(394 tests, no `--allow-growth`), `make winapi-link-check` and native ctest
(37 of 37) pass. The native build starts with the shipped WADs.

The change is confined to `src/game/terrain_map.c`,
`src/game/terrain_map_maths.h` and `src/game/lego_unit.c`. There is no
`#pragma`, no pad, no `Deviation (scoring only)` and no header shared with
another unit.

### Rebased onto the #809–#811 stack

Measured again on #811 (`match/808-weapons-mesh-lwo`, `2ed680d3`). The base's
unit spans come from a full build of the base sources, and the branch's
from a full `make reccmp-check` and `make match-progress`. The same nine
functions rise by the same amounts, and nothing falls. Map3D.c again gains
exactly 160 bytes, and so does the entry point.

| Measure | #811 `2ed680d3` | This change on it |
| --- | ---: | ---: |
| **Exact-function code bytes** | 343,425 (53.45%) | **343,761 (53.50%)** |
| Accuracy, full / game-only | 97.43% / 97.16% | **97.53% / 97.26%** |
| Correct-size functions, full | 2,789 (454,789 B) | **2,793 (457,127 B)** |
| Correct-size functions, game-only | 2,498 (403,962 B) | **2,502 (406,300 B)** |
| Identical prefix (raw / masked) | 11 / 528 B | 11 / 528 B |
| Unit size agreement (whole / correct sizes / start / end) | 0 / 167 / 2 / 1 | unchanged |
| `.text` identical at original addresses | 29,574 (4.60%) | **29,832 (4.64%)** |
| GAME-026 Map3D.c | −288 | **−128** |
| GAME-021 Lego.c | −224 | −224 |
| GAME-023 LegoObject.c | −160 | −160 |
| Entry point (original `0048f000`) | `0048eaa0` (−1,376) | **`0048eb40` (−1,216)** |
| Image end; inversions full / game-only | `0x7d8000`; 222 / 1 | unchanged |

On this base, `.text` at original addresses rises: the later units are now
close enough to their original addresses that the 160-byte move gains
runs. The `/OPT:NOREF` relink flip goes the other way here, with
`original_annotated` falling from 2,719 to 2,717 functions.

## 1. Per-function results

Sizes are `code_extent` (finding 668): the original's, then the rebuilt one
before → after. Scores are the full report's.

| Address | Function | Original | Rebuilt | `main` | After | Padded size |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `0044e0c0` | terrain_map_create | 1,033 | 998 → **1,033** | 52.38 | **92.57** | agrees (was −32) |
| `0044ead0` | terrain_map_shape_flatten | 787 | 754 → 789 | 41.81 | **93.25** | agrees (was −32) |
| `0044fb90` | terrain_map_world_edge_point_get | 732 | 683 → 730 | 57.35 | **77.33** | agrees (was −48) |
| `004515e0` | terrain_map_block_ray_intersection | 969 | 959 → **969** | 66.43 | **88.54** | agrees (was −16) |
| `00451c50` | owned_object_reference_chain_release | 114 | 109 → **114** | 73.68 | **100** | agrees (was −16) |
| `00450b60` | terrain_map_uv_wobble_update | 964 | 834 → 856 | 33.62 | **36.48** | −112 (was −128) |
| `00434710` | lego_panel_button_layout_config_load | 742 | 781 → 733 | 39.47 | **95.55** | −16 (was +32) |
| `00435300` | lego_tutorial_block_icons_update | 580 | 551 → 582 | 60.44 | **93.51** | agrees (was −32) |
| `00435940` | lego_keep_running | 222 | 208 → **222** | 73.68 | **100** | agrees (was −16) |

`owned_object_reference_chain_release` scores 98.08% in the
single-function loop. The one difference there is its call to
`Container_Remove`, which the loop sees under the port's link name, and the
full report resolves to the same address.

## 2. Map3D.c: the shapes

- **Inline `Maths_Vector2D*` helpers.** Two inline helpers are now used:
  `terrain_map_vector2d_distance`, added to `terrain_map_maths.h` in the
  teleporter unit's form, and the existing `terrain_map_vector2d_normalise`.
  The wobble update calls them on `cur`/`target` and on the random pair.
  Their struct locals stay in memory, which gives the original's
  `fst [x]; fmul [x]` squares. The header's include moves from before
  `00450f30` to before `00450b60`.
  - It cannot go any earlier. With the helpers defined before
    `terrain_map_block_hide` (`00450700`), that function's two loads swap
    registers (100 → 69.23), and `phase_probe.py` shows it has one code at
    every counter shift. So the cause is the definitions themselves, not
    the symbol count.
  - `terrain_map_world_edge_point_get` therefore writes its normalise out
    with a `scale` local.
- **Struct copies of the 2D points** (`terrain_map_world_edge_point_get`).
  The line's start and direction are `*(TerrainMapPoint2*)&v[i]`, copied
  direction-first, and the subtraction follows as its own statement. The
  original copies both with integer moves before any arithmetic. Other
  changes in this function:
  - the test is `fabs(normal.x) > fabs(normal.y)`, which is the original's
    operand order;
  - the end scales `normal2` by `distance` in place before the two
    subtractions.

  Size went from 683 to 730 bytes (original 732), and the score from 57.35%
  to 77.33%.
- **Indexing the shared block list directly**
  (`terrain_map_shape_flatten`, 41.81 → 93.25). The second loop indexes
  `s_terrain_map_flatten_blocks[i]` directly instead of through a `point`
  local. MSVC then keeps the element pointer on the stack and `&point->y`
  in `ebx`, as the original does. Other changes:
  - the flattened test has the then-branch first;
  - the comparisons are `z > highest` and `fabs(diff) > half`.
- **The original divides `diff` by itself.** At `0044ed4f` the original
  computes `fld st(0); fdiv st(1)` on a stack that holds only `diff`
  (Ghidra: `fVar4 / fVar4`). The quotient is always 1, so a vertex outside
  the tolerance always moves to `average + half`, whichever side it was on.
  The port had `diff / fabs(diff)`. It now has `diff / diff`, which is the
  original's behaviour, and this changes native behaviour.
- **`terrain_map_create`** (52.38 → 92.57, exact size):
  - the block pointer is taken before the height call, so it lives across
    the call as in the original;
  - the rotation test uses the four height reads as plain expressions, not
    `block`/`below` pointers, which gives the original's two base
    registers;
  - the normalise sums `r * r + uv->x * uv->x`, in that order.
- **`terrain_map_block_ray_intersection`** (66.43 → 88.54, exact size):
  - the two found flags are cleared after the vertex call;
  - the selection is `if (!A && !B) return 0; if (A && B) {...} else if (A)
    {...}`. This is the original's branch tree, with its re-test of `A`.
- **`owned_object_reference_chain_release`** (exact): the free loop uses its
  own variable (`for (node = root; node; node = next)`) instead of advancing
  `root`.
- **Tried and dropped:**
  - in `terrain_map_block_first_vertex_get`, a flags local and `?1:0`. The
    code is unchanged; the original's `xor ebx, ebx; mov bl, ...` still
    comes from an unknown form;
  - in the wobble update, named `dx`/`dy` locals (the compiler keeps them
    in registers), a nested inline subtract, and `int j`.

## 3. Lego.c: the tail after `00434530`

All three functions are after the last counter-sensitive function of
finding 671. The other Lego.c functions keep their scores, so the counter
phase held.

- **`lego_panel_button_layout_config_load`** (39.47 → 95.55). The original's
  frame is `0x2dec` bytes, against the port's `0x2b74`. Its offsets fix the
  locals:
  - `buff[128]` and `argv[10]`;
  - one `names[3][12][24]` array, looped with
    `stricmp("NULL", argv[1 + i])`, as Ghidra's `(panel + i*12)*24`
    indexing shows;
  - `copies[12 * 24]`;
  - `buttonCount[12] = { 0 }` (one store, then `rep stosd` of 11);
  - a `toolTip` local tested `if (lookup) = toolTip; else = 0`.

  The panel loop reuses `panelType`, which the original keeps in memory,
  so no induction pointers are made. Its `jb` makes it unsigned; the lookup
  takes `(int*)&panelType`. The copies are freed in a forward loop. The
  function is 9 bytes short, in the `copyCount = 0` store (an immediate in
  the original) and the free loop's count register.
- **`lego_tutorial_block_icons_update`** (60.44 → 93.51, padded size agrees).
  - The icon's width and height are signed in the original's arithmetic
    (`cdq; sar` and `fiadd`), so they are read as `(int)`.
  - The phase step is `-0.013333334f * (elapsed * 25.0f)`, two multiplies
    as in the original.
  - Each highlight arm sets the block's flag itself: the original does not
    merge the two stores.
  - `by` is declared before `bx`.
- **`lego_keep_running`** (exact). `levelName == NULL` and a failed
  `lego_level_load` are separate early returns, each with its own epilogue
  as in the original.
- **`lego_menu_option_panel_texture_update` was reverted** (65.42, size
  −32 padded). The original reads the text windows' `+4`, `+8` and `+0xc`
  as floats (`fld dword ptr [ecx + 4]`). These are `TextWindow.windowSize`,
  which `textwindow.c` declares as floats. The port reads them as ints and
  converts, so in the native build the OK/Cancel hover test runs on the bit
  patterns. With float reads, the width-first call order and the
  original's strict-inside test, the size lands in the original's padded
  slot. The score falls below the baseline, though (57.55 to 59.68 in the
  loop, against 60.19 for `main`). What is left: the original keeps the
  three zero stores of its rect, and subtracts `textWidth >> 1` with
  `fisub`. **Follow-up:** this is a native behaviour bug, to be filed with
  the `port` label.

## 4. Left, with what was seen

- **`00450b60` terrain_map_uv_wobble_update** (−112 padded). The original
  frame is 12 bytes larger, and the inner loop is strength-reduced into
  three stack pointers (`uvOffsets`, `blockOffsets` and the `j * 8` byte
  offset); the port indexes `[esp + j*8 + ...]`. No source form tried made
  MSVC strength-reduce.
- **LegoObject.c:**
  - `00440c70` terrain_grid_path_search (−32, 51.91%) needs a rewrite of its
    breadth-first search;
  - `0043eea0` simulation_queue_flush (−16, 89.26%) keeps a `(i == 0)` flag
    through `sete`/`cmp 1`, and pushes immediates where the port has a zero
    register;
  - `0044a200` type_component_get (−16) reloads the parameter for its last
    `return obj`.
- **Lego.c before `00434530`.** The interaction step `00426520` (−96), the
  cell pass `0042f6f0` (−48) and nine smaller deficits sit before or inside
  finding 671's counter-sensitive span. A fix there has to keep the counter
  modulo 64 at `004311d0`.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json   # unit_spans, entry_point
make match-function ADDR=0x44ead0
```
