# Finding 599 — Erode.c (GAME-015) and Fallin.c (GAME-051) rebuilt in original shape, with a port tail

Issue #638, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-015 was the unit map's "lava erosion timers and pending-cell ticks",
15 functions `0040e930`-`0040f6c0`. **It is two original objects**:

- **Erode.c**, `0040e930`-`0040f0e0` (9 functions): lava erosion. It is
  rebuilt as [`src/game/erode.c`](../../src/game/erode.c) and keeps the id
  GAME-015.
- **Fallin.c**, `0040f0e0`-`0040f6c0` (6 functions): landslides and cave-ins.
  It is rebuilt as [`src/game/fallin.c`](../../src/game/fallin.c) and takes
  the new id **GAME-051**. See [Unit extent](#unit-extent).

Each unit comes with:

- a C header ([`erode.h`](../../src/game/erode.h), [`fallin.h`](../../src/game/fallin.h));
- a unit-private imports header ([`erode_imports.h`](../../src/game/erode_imports.h),
  [`fallin_imports.h`](../../src/game/fallin_imports.h));
- link names ([`erode_link_names.h`](../../src/game/erode_link_names.h),
  [`fallin_link_names.h`](../../src/game/fallin_link_names.h)).

The port's implementation, `lava_erosion.cpp` (relocated verbatim by #477),
moves unannotated to the port tail
[`lava_erosion_port.cpp`](../../src/game/lava_erosion_port.cpp).

Both units now call **Lego.c's converted terrain functions** (`lego_unit.c`,
findings 568, 571 and 577) where the original does: Level_BlockUpdateSurface
(`0042f6f0`), the `0x432b50`-`0x433090` block queries, and Map3D's
`terrain_map.c` functions. They no longer call the port's models
(`cell_init_pass`, `level_voxel`, `terrain_cell_world_center_xy_get` and the
others). Those port models were why `0040eab0` and `0040f330` showed up as
drift drops in other PRs (#583).

"Before" is the *match baseline* on `main` at `8601d8ab` (#635). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Erode.c similarity (byte-weighted, 9 functions, 1,968 bytes) | 57.93% | **99.45%** |
| Fallin.c similarity (byte-weighted, 6 functions, 1,504 bytes) | 21.37% | **100.00%** |
| All 15 addresses of issue #638 (3,472 bytes) | 42.09% | **99.69%** |
| Exact functions of the 15 | 5 (4 of them naked asm) | **14** (no asm) |
| Mapped functions of the 15 | 14 | 15 |
| Exact-function code bytes | 178,866 (27.84%) | **181,435 (28.24%)** |
| Accuracy, full / game-only | 83.30% / 81.67% | 83.51% / 81.90% |
| `opt_ref` annotated (full, by address): removed / exact removed | 498 / 239 | 500 / 241 |
| `opt_ref` annotated (game-only, by address) | 479 / 226 | 481 / 228 |
| Reccmp lint ledger (entries) | 1,267 | **1,266** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%.

The units' names follow OpenLRR's `Erode_*` and `Fallin_*` functions, in the
repository's snake case. "Link name" is the name the unit's definition takes
in the matching build (see [Link names](#link-names)).

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0040e930` | `erode_initialise` | `erosion_timing_state_init` | 96 | 100.00 (asm) | **100.00** |
| `0040e990` | `erode_get_free_activity` | `lava_erosion_stage_slot_find_free` | 48 | 100.00 (asm) | **100.00** |
| `0040e9c0` | `erode_get_block_erode_rate` | `lava_erosion_delay_from_cell` | 80 | 100.00 | **100.00** |
| `0040ea10` | `erode_set_block_erode_rate` | `lava_erosion_stage_register` | 160 | unmapped | 93.18 |
| `0040eab0` | `erode_update` | `lava_erosion_pending_cell_tick` | 848 | 40.17 | **100.00** |
| `0040ee00` | `erode_add_locked_block` | `cell_coordinate_timestamp_remember` | 80 | 94.44 (asm) | **100.00** |
| `0040ee50` | `erode_set_active_block` | `lava_erosion_pending_cell_add` | 352 | 100.00 (asm) | **100.00** |
| `0040efb0` | `erode_is_block_locked` | `cell_coordinate_timestamp_exists` | 80 | 96.43 (asm) | **100.00** |
| `0040f000` | `erode_find_adjacent_block` | `lava_erosion_neighbor_check` | 224 | 31.58 | **100.00** |
| `0040f0e0` | `fallin_update` | `kind_six_random_sweep_dispatch` | 176 | 33.33 | **100.00** |
| `0040f190` | `fallin_try_generate_land_slide` | `lava_erosion_cell_eligible_and_mutate` | 288 | 13.04 | **100.00** |
| `0040f2b0` | `fallin_can_land_slide_at_block` | `lava_erosion_neighbor_eligible_current` | 128 | 20.18 | **100.00** |
| `0040f330` | `fallin_generate_land_slide` | `lava_erosion_neighbor_mutate` | 688 | 20.19 | **100.00** |
| `0040f5e0` | `fallin_initialise` | `global_config_value_set` | 16 | 100.00 | **100.00** |
| `0040f5f0` | `fallin_generate_cave_in` | `lava_erosion_eight_neighbor_propagate` | 208 | 21.36 | **100.00** |

- **The function with no source** was `0040ea10` Erode_SetBlockErodeRate.
  `level.cpp` had an unannotated port model under its link name. It is now
  written and mapped.
- **The low-confidence function** was `0040f5f0`
  (`boundary:GAME-016`). It is confirmed as Fallin.c's last function: it
  calls `0040f190`, `0040f2b0` and `0040f330`, and nothing else in the
  original calls it except `0040f330`. GAME-016's first function, `0040f6c0`,
  starts a new `.rdata` run at `0049e14c`. The per-function confidence in
  `translation-units.csv` stays `low`, because it is computed from the
  data-step evidence. The spec note records the confirmation.

One caller outside the units rose with no source change:
`0042bf40` `lego_load_emerge_layer` (89.72%, from 88.79%). It calls
Erode_SetBlockErodeRate, which is now mapped.

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `0040ea10` Erode_SetBlockErodeRate | 93.18 | One `imul` operand order and the `push` of the call argument: the original computes `y * width` after loading `blockPos`, and this compile loads `width` first. Tried, all at 93.18% unless noted: both `imul` orders written out; `blockPos->x + …` first; `(int)` and `(unsigned)` casts on either operand; a `Lego_Block*` local; pointer arithmetic instead of indexing; the copy as a struct and as two members; both local declaration orders; an early `return`; a `const` parameter. Worse: `int erodeSpeed` (67.50%), `unsigned char erodeSpeed` (66.67%), a `float` temporary for the rate (73.12%), `level` read from the global each time (79.55%). It is not rule B (finding 538): 0-48 probe declarations before the function change nothing. `msvc_permuter.py` (240 s) found no candidate rewrites. |

`0040eab0` and `0040f330` score 99.60% and 99.06% in the single-function loop
and 100% in the full build. The loop names the calls to Effect_Spawn_Particle,
Effect_Spawn_RockFall and SFX_Random_PlaySound3DOnContainer by their link
names, and the full build compares them by address.

No naked asm, and no `Deviation (scoring only)`. Neither unit uses the flag
`0x800000`, and the plain-immediate check (finding 584) passes.

## Unit extent

The unit map's high-confidence boundary at `0040e930`-`0040f6c0` covers two
objects. The cut is at `0040f0e0`.

- **The literal pool decides it**, as in finding 597. MSVC 5.0 emits each
  float literal once per object. The `.rdata` run is:

  | Address | Value | Used by |
  | --- | --- | --- |
  | `0049e124` | `25.0f` | `0040e930`, `0040e9c0` |
  | `0049e128` | `0.2f` | `0040e930` |
  | `0049e12c` | `0.0f` | `0040eab0` (the lock countdown) |
  | `0049e130` | `4.0f` | `0040eab0` |
  | `0049e134` | `1.0f` | no code reference |
  | `0049e138` | `0.0f` | `0040f0e0` (the sweep timer) |
  | `0049e13c` | `-750.0f` | `0040f0e0` |

  `0040f0e0` loads its own `0.0f`, so it was compiled in another object
  from `0040eab0`.
- **The data agrees.** Erode.c's state is one `.bss` block,
  `004c7eb0`-`004d78c4` (`erodeGlobs`, 0xfa14 bytes). Fallin.c's is the
  `.data` word `004a1ee4` (the cave-in limit, initially 4) and the `.bss`
  float `004d78c4` (the sweep timer). No Erode.c function touches them, and
  no Fallin.c function touches `erodeGlobs`.
- **The calls agree.** Erode.c's functions call each other, Lego.c's block
  queries and the effect, Info and Map3D functions. Fallin.c's call each
  other and the same kinds of callee. Nothing in Erode.c calls Fallin.c or
  the reverse (`docs/coverage/call-graph/edges.csv`). Lego.c calls both: Erode_Initialise and
  Fallin_Initialise from the level loader (`ErodeTriggerTime`,
  `ErodeErodeTime`, `ErodeLockTime` and `NumberOfLandSlidesTillCaveIn`), and
  Erode_Update and Fallin_Update from the world update.
- **The names.** The stems follow OpenLRR (`Erode_*`, `Fallin_*`), which
  matches the original's alphabetical object order: Encyclopedia.c
  (GAME-014), Erode.c, Fallin.c, then GAME-016.

`translation-units.spec.csv` records this evidence, with `inferred` names:

- GAME-015 becomes `Erode.c`, `0040e930`-`0040f0e0`, 9 functions.
- **GAME-051** `Fallin.c`, `0040f0e0`-`0040f6c0`, 6 functions, is new.
  The next free id is used rather than renumbering every later unit. Unit
  ids are labels: link order and the unit map order units by their start
  address.

`python3 tools/translation_units.py build`: 0 conflicts. The generated
`unit_order` column shifts by one for every later unit, which accounts for
most of the `translation-units.csv` and `translation-unit-ranges.csv` diff.
`link_order.cmake` lists `game/erode.c` (GAME-015) and `game/fallin.c`
(GAME-051) in their original positions, and `game/lava_erosion_port.cpp` as a
`tail` entry.

## How the conversion is done

### State

`static Erode_Globs erodeGlobs` (`004c7eb0`), laid out as the original `.bss`:

| Offset | Address | Field |
| ---: | --- | --- |
| `0x0000` | `004c7eb0` | `activeBlocks[2000]`: lava blocks that can erode their neighbours |
| `0x3e80` | `004cbd30` | `activeCount` |
| `0x3e84` | `004cbd34` | `erodeBlocks[2000]`: blocks that are eroding |
| `0x7d04` | `004cfbb4` | `erodeUsed[2000]` |
| `0x9c44` | `004d1af4` | `erodeTimes[2000]` |
| `0xbb84` | `004d3a34` | `lockedBlocks[1000]` |
| `0xdac4` | `004d5974` | `lockedTimes[1000]` |
| `0xea64` | `004d6914` | `lockedUsed[1000]` |
| `0xfa04` | `004d78b4` | `elapsedTimer` |
| `0xfa08` | `004d78b8` | `triggerTime` (ErodeTriggerTime × 25) |
| `0xfa0c` | `004d78bc` | `erodeTime` (ErodeErodeTime × 25 × 0.2) |
| `0xfa10` | `004d78c0` | `lockTime` (ErodeLockTime × 25) |

Fallin.c has `static unsigned int fallinCaveInLimit = 4` (`004a1ee4`) and
`static float fallinTimer` (`004d78c4`). The port had `004a1ee4` as
`s_config_word = 0`; the original's initial value is 4.

The `.rdata` floats are literals in the code. The 17 `// GLOBAL` annotations
of the port file are removed: the units own those addresses.

`make unit-data-offsets`:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-015 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |
| GAME-051 | n/a (1, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 6, partial 26, n/a 194.

The image's `.bss` grows by 64,016 bytes (`binary-progress.json`
`opt_ref.image.bss_bytes`, 3,125,408 to 3,189,424). That is `erodeGlobs`,
which now exists beside the port's own arrays in the port tail. The
plain-immediate check (finding 584) still passes. Shrinking port-only `.bss`
is #641's.

### The port tail

The port's lava erosion is another contract:

- its own state, with an explicit-level sidecar;
- bounds and NULL guards;
- the edge, work-clear, cell-change, sound and event hooks that the native
  renderer installs;
- the peeks and queries that `lego_port.cpp`, `level.cpp` and the tests use.

As in findings 590 and 592-597, `lava_erosion.cpp` is renamed
`lava_erosion_port.cpp` and becomes a `tail` entry. Each `// FUNCTION`
becomes a `/* Port model of 0x… */` comment.

### Link names

In the matching build, `erode_link_names.h` and `fallin_link_names.h` give
**all 15** unit functions the port's names (table above). All 15 port names
have C linkage (`level.hpp`'s `extern "C"` block, `cmd.hpp`, `crt.hpp`), and
every caller passes the original's arguments: a block position as two ints,
a direction mask, the cave-in flag, the elapsed time or the Lego.cfg values.
So `lego_unit.c` and the port's C++ callers reach the units with **no caller
edit**.

- `lava_erosion_port.cpp` compiles its definitions of the 15 names out of
  the matching build behind `LRR_LAVA_EROSION_PORT_NAMES`, set by an explicit
  `#if` (finding 590).
- `level.cpp`'s port model of `0040ea10` (`lava_erosion_stage_register`) is
  compiled out the same way, with a plain `#if`.

The return types differ in two places: Erode_SetBlockErodeRate and
Fallin_Update return nothing in the original, where the port returned a
value. No caller in the matching build reads those values.

Natively the units keep their own names and nothing calls them. The port
tail serves every port name, so native behaviour is unchanged.

### Callees

All the units' callees are converted, exact functions:

| Original | Called as | Unit |
| --- | --- | --- |
| Level_BlockUpdateSurface `0042f6f0` | `lego_cell_init_pass` | Lego.c (`lego_unit.c`) |
| `0x4302b0`, `0x430320`, `0x431570`, `0x432b50`-`0x433090` | their `lego_unit.c` names | Lego.c |
| Map3D_BlockToWorldPos `0044f640`, Map3D_GetWorldZ `0044f940`, `004502c0` | `terrain_map.c` names | GAME-026 |
| Effect_Spawn_Particle `0040cce0`, Effect_Spawn_RockFall `0040c230` | `rock_fall_styles.c`'s link names | GAME-012 |
| Info_Send `00419b80` | `info_send` (`info_unit.h`) | GAME-019 |
| SFX_Random_PlaySound3DOnContainer `00465050` | `sfx.c`'s link name | GAME-040 |
| Maths_Rand `004798a0`, Maths_Vector3DRandom `00479500` | `maths.c` | G98-007 |

Lego_GetMap (`00431570`) takes no arguments. As in `rock_fall_styles_imports.h`,
the C matching build calls `lego_unit.c`'s definition through an unprototyped
function type, and the native C++ compile goes through `lego_port.cpp`'s
adapter.

No `extern "C"` was added to any existing declaration. The imports headers
repeat the existing C declarations exactly, and `test_c_boundaries.cpp`
includes all four new headers.

### Caller switches

None. Every original caller is in Lego.c (`lego_unit.c`): the level loader
`00429890`, the EmergeMap loader `0042bf40`, Level_BlockUpdateSurface
`0042f6f0`, `004288e0`, `0042cb70`, `00432600`, `00435230` and the world
update. Each already called the port name that the unit now takes. No Lego.c
or LegoObject.c file is edited.

No file owned by #637 (`bubbles.cpp`), #639 (`gods98/text_window.cpp`), #640
(`gods98/lwo.cpp`) or #641 (port-only `.bss` in `entity.cpp`, `frontend.cpp`
and other port files) is edited. Outside the units and the port tail, this
change edits only `level.cpp` (the `#if` around its `0040ea10` model) and
`test_c_boundaries.cpp`.

### Source shapes that matched

- **Erode_Initialise** (`0040e930`, asm before): `(erodeTime * 25.0f) * 0.2f`
  with explicit parentheses. Without them MSVC stores each product as it
  goes (30.00%). With them it schedules all three products before the
  stores, as the original does.
- **Erode_IsBlockLocked** (`0040efb0`, asm before): the decompile's pointer
  loop, `while (!*used || … ) { used++; locked++; if (used >= end) return 0; }`,
  with `locked` declared before `used`. An indexed `for` gives 60.38%.
- **Erode_SetActiveBlock** (`0040ee50`, asm before): the two removal scans
  as `do … while` loops over a pointer, the first guarded by
  `if (i < activeCount)` (`jbe`, not `je`); `i` and the pointer declared
  before `level`; and the append as a struct copy,
  `activeBlocks[activeCount] = *blockPos`. Indexed `for` loops give 64.55%.
- **Erode_Update** (`0040eab0`): the stage loop takes
  `ErodePoint2I* pos = &erodeGlobs.erodeBlocks[i]` in the loop body. MSVC then
  strides the block pointer, as the original does, and keeps `level` in
  `edi` (85.14% without it, for all 120 orders of the locals).
- **Erode_GetBlockErodeRate** (`0040e9c0`): `level` as a local.
- **Fallin_TryGenerateLandSlide** (`0040f190`): the locals initialised in
  the original's order, with the map from Lego_GetMap in an unused local
  before the direction table. The original fetches the map and never reads
  it, and initialises the table after the call.
- **Fallin_GenerateLandSlide** (`0040f330`): the switch's cases in the
  original's code order (3, 6, 12, 9, 7/8, 2/10/13, 1/14, 4/5/11), and a
  `FallinPoint2F* dir = &DIRECTIONS[direction]` taken before the first call
  (69.79% to 99.06%).

**Rule B (finding 538).** `0040e9c0`, `0040ee50` and `0040f000` each match in
a different window of the symbol counter. Measured with probe declarations
at the top of `erode.c` (a throwaway experiment, not kept), every Erode.c
function except `0040ea10` is exact at +36 to +38, +68 to +70 and +100 to
+102. The committed include set lands there with no filler: `erode.c`
includes `<stdio.h>` and `crt/vc5names.h`, as `tooltip.c` and `weapons.c`
do, and `erode_imports.h` takes `info_send` from Info.c's own header,
`info_unit.h`, rather than repeating its declaration.

## Entropy and drops

`make reccmp-check` against the baseline at `8601d8ab`:

- 10 improved: 9 of the 15 addresses and `0042bf40`;
- 1 newly mapped: `0040ea10`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at the units' addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0040ea10` | — | 44 |
| `0040eab0` | 232 | 248 |
| `0040f000` | 86 | 84 |
| `0040f0e0` | 102 | 54 |
| `0040f190` | 97 | 101 |
| `0040f2b0` | 52 | 46 |
| `0040f330` | 234 | 212 |
| `0040f5f0` | 57 | 73 |

The other seven addresses keep their listings.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `8601d8ab`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 498 / 239 | 500 / 241 |
| `opt_ref.annotated.game_only` (by address) | 479 / 226 | 481 / 228 |

`make opt-ref-callers` lists no removed function in `erode.c.obj` or
`fallin.c.obj`. All 15 functions are kept.

The net change of +2 is four functions, measured by building `8601d8ab` and
this change in the same worktree and comparing the removed `.text` symbols:

- **Kept now:** `00432e90` `terrain_cell_edge_wall_current` (Lego.c, exact).
  Fallin_CanLandSlideAtBlock calls it, where the port model called its own
  copy.
- **Removed now** (all exact, labelled *`opt_ref` swap*):
  - `00429160` `lego_terrain_cell_palette_resource_at_grid_position` (`lego_unit.c`);
  - `00438ed0` `legoobject_flags_contain_requested_single_bit` (`legoobject_unit.c`);
  - `0043baf0` `legoobject_flags_mask107c33de_or_1c00203_any_test` (`legoobject_unit.c`).

  Each has a port twin at the same original address: `lego_port.cpp`'s
  `terrain_cell_palette_resource_at_grid_position_current` and
  `legoobject_port.cpp`'s `entity_flags_contain_requested_single_bit` and
  `entity_flags_mask107c33de_or_1c00203_any_test`. On `main` the relink kept
  the unit's definition and dropped the port twin. After this change it is
  the other way round. The same swap moves two port-only symbols
  (`frontend.cpp`'s `0041baa0` stub and `interface_port.cpp`'s
  `interface_handle_mouse_adapter`) and one LIBC member (`_strncnt`, from
  `aw_map.obj` to `aw_cmp.obj`).

  The old port erosion code was live in the matching build and called the
  port's `message_dispatch`, `cell_init_pass` and hook paths. The units call
  Info.c's `info_send` and Lego.c's functions instead. That changes which of
  each twin pair keeps a live reference. The functions' code and scores are
  unchanged. Their remaining callers are in the Lego.c and LegoObject.c
  roots, which finding 579 attributes to #587 and #588. **Flagged for
  review**, not owner-reviewed.

## Reccmp lint ratchet

1,267 → 1,266 entries (−1, no growth). The port's static helper
`lava_erosion_current_level_get` is now in a tail file, which the lint
exempts. `make reccmp-lint-update` removed its entry.

## Native behaviour

Unchanged:

- the port tail serves all 15 port names natively, and `level.cpp` keeps its
  `0040ea10` model;
- the units compile natively as C++ under their own names, and nothing
  native calls them.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The units' state and the
port's are separate. In the matching build, callers of the 15 link names use
`erodeGlobs`, `fallinCaveInLimit` and `fallinTimer`. The port's remaining
queries (`lava_erosion_pending_cell_count_get`, `lava_erosion_stage_get` and
the peeks) read the port's own arrays. This is the same split that findings
590 and 592 accept until the callers' units are converted.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding
  584), `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`. `tools/binary_progress_inputs.txt` is
  regenerated (`binary_progress_hash.py --write-manifest`).
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`.
- `erode.c` and `fallin.c` pass `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **`0040ea10`**: the `imul` operand order above.
- **The port's erosion queries** (`lava_erosion_stage_get`, the peeks, the
  explicit-level adapters): they retire with the native renderer's hooks,
  once Lego.c's world update runs natively (#552).
- **GAME-016**: `0040f6c0`-`004102b0` follows Fallin.c alphabetically
  (OpenLRR's Flocks.c). Its conversion can confirm that stem.

## Rebased onto stack #647

This section gives the figures after rebasing onto #642 (`d2e18b60`,
port-only `.bss`). That base contains #643 (lwo.c), #644 (Bubbles.c) and
#645 (TextWindow.c). None of them touches the units, `lava_erosion.cpp` or
`level.cpp`, and #642 moved no buffer out of the lava erosion port file. The
spec split is re-applied on #642's unit map, and `translation_units.py build`
reports 0 conflicts. Apart from `unit_order`, only the 15 rows of this change
and the GAME-015 and GAME-051 ranges differ from #642's map. All the
per-function scores above are unchanged on the rebase.

`make reccmp-check` against #642's baseline: the same 10 improved and 1 newly
mapped, **none below baseline, no exact match lost**.

| | #642 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 181,261 (28.21%) | **183,830 (28.61%)** |
| Exact functions, full / game-only | 1,573 / 1,371 | 1,582 / 1,380 |
| Accuracy, full / game-only | 84.05% / 82.49% | 84.26% / 82.73% |
| `opt_ref` full: removed / exact removed | 502 / 245 | **500 / 243** |
| `opt_ref` game-only | 483 / 232 | 481 / 230 |
| Lint ledger | 1,240 | **1,239** |

The `opt_ref` change is the same unit/port twin mechanism as on `main`,
measured the same way (#642's head and this change built in one worktree):

- **Newly kept, all exact:** `00429160`, `00432200`
  (`lego_terrain_cell_previous_state_track`), `00432e90` and `00484c90`
  (`flic_opcode27_fallback_noop`).
- **Newly removed, both exact, labelled *`opt_ref` swap*:**
  - `00437440` `legoobject_grid_record_count_tables_clear`;
  - `00438ed0` `legoobject_flags_contain_requested_single_bit`.

  Their port twins in `legoobject_port.cpp` (`grid_record_count_tables_clear`,
  `entity_flags_contain_requested_single_bit`) are kept instead. **Flagged
  for review**, not owner-reviewed.

`.bss` grows by 64,000 bytes on #642's image (1,543,024 to 1,607,024): the
units' `erodeGlobs` beside the port's arrays, which stay in the port tail.

## Reproducing

    git mv src/game/lava_erosion.cpp src/game/lava_erosion_port.cpp   # and make it a tail entry
    # split the spec row at 0040f0e0 (GAME-051 Fallin.c), then:
    python3 tools/translation_units.py build
    python3 tools/convert_unit.py GAME-015 --stem erode
    python3 tools/convert_unit.py GAME-051 --stem fallin
    make match-function SOURCE=src/game/erode.c
    make match-function SOURCE=src/game/fallin.c
    make reccmp-check
    make opt-ref-link opt-ref-callers OPT_REF_ARGS="--min 1"
    make match-progress
    make unit-data-offsets
