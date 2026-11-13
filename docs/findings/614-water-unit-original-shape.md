# Finding 614 — Water.c (GAME-049) rebuilt in original shape, with a port tail

Issue #671, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-049 is the level's water pools. OpenLRR names the object `Water.c`, and
the unit map now uses that name (`inferred`). The unit map called it "staged
cell flood/run passes" and listed 12 functions, 11 of them low-confidence
and 4 with no source. **The extent holds** (`0046db90`-`0046eb80`), and it
has a 13th function: the `qsort` comparator `0046e9a0`, which Ghidra left as
a label (see [Unit extent](#unit-extent)).

What the unit does:

- **Water_Initialise** (`0046db90`) gathers each row's runs of lake blocks
  (Lego_Block terrain 7) into pools. A run that touches a pool in the row
  above joins it; two pools that one run touches are merged afterwards. The
  pools are sorted largest first, and the empty ones are dropped.
- Each pool gets its **entries** (`0046dd10`): the edges where the
  neighbouring block is neither lake (7) nor solid rock (1). It also gets a
  **mesh** of one quad per block (`0046de80`) at its start level: the
  highest corner of a wall block around it. The mesh is hidden unless a
  block beside the pool is open.
- When the wall beside an entry is destroyed (`0046e220`, from
  Level_DestroyWall), the entry opens at the wall's height. When the block
  is sealed again (`0046e330`), it closes.
- **Water_Update** (`0046e390`, once a frame from Lego_MainLoop) raises a
  pool towards its highest open entry, or lowers it back, and fades the
  mesh's alpha with the depth. `0046e610` marks the blocks that each
  entry's flow front and tail reach (`0042f2e0`).

This change rebuilds the unit as the C unit
[`src/game/water.c`](../../src/game/water.c), with:

- the C header [`water.h`](../../src/game/water.h);
- the unit-private imports header [`water_imports.h`](../../src/game/water_imports.h);
- the matching-build link names [`water_link_names.h`](../../src/game/water_link_names.h);
- the inline helper header [`water_maths.h`](../../src/game/water_maths.h).

The port's model moves, unannotated, to the port tail
[`cell_flood_passes_port.cpp`](../../src/game/cell_flood_passes_port.cpp),
which is `cell_flood_passes.cpp` under `git mv`.

"Before" is the *match baseline* on `main` (`8abe8b6b`). "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (13 functions, weighted by span, 4,080 bytes) | 10.56% | **98.94%** |
| Exact functions in the unit | 0 | **11** (no asm) |
| Mapped functions in the unit | 8 | **13** |
| Exact-function code bytes (`binary-progress.json`) | 196,138 | **198,558** |
| `opt_ref` annotated (full): removed / exact removed | 428 / 207 | **423 / 206** |
| `opt_ref` annotated (game-only) | 421 / 200 | 416 / 199 |
| Gate image end (`make pe-audit`) | `0x8ea000` | **`0x8df000`** |
| `/OPT:REF` relink `.bss` (`binary-progress.json`) | 1,668,960 | **1,628,256** |
| Reccmp lint ledger (entries) | 1,080 | **1,072** |

The weights are each function's span up to the next function, padding
included. An unmapped function counts as 0%. The full run scores the
effective match `0046e610` as 95.58% (effective matches are not promoted);
`make match-function` reports it as `100.00%*`.

The names are the unit's own, after OpenLRR's `Water_*` functions where they
are known. They are `inferred`: the unit has no strings. "Link name" is the
name the matching build gives the definition (see [Link names](#link-names)).

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046db90` | `water_initialise` | `terrain_cell_quad_run_stage_level` | 384 | 11.11 | **100.00** |
| `0046dd10` | `water_init_entries` | (its own) | 368 | 18.69 | **100.00** |
| `0046de80` | `water_init_vertices` | (its own) | 832 | unmapped | 99.08 |
| `0046e1c0` | `water_change_view_mode` | (its own; Lego.c's name in `lego_unit_imports.h`) | 96 | unmapped | **100.00** |
| `0046e220` | `water_destroy_wall_complete` | `terrain_cell_surface_quad_height_fill_current` | 272 | 11.32 | **100.00** |
| `0046e330` | `water_seal_block` | (its own; Lego.c's name in `lego_unit_imports.h`) | 96 | 31.71 | **100.00** |
| `0046e390` | `water_update` | (its own; Lego.c's name in `lego_unit_imports.h`) | 640 | 29.27 | 98.99 |
| `0046e610` | `water_update_flow_blocks` | (its own) | 656 | unmapped | 95.58 (effective) |
| `0046e8a0` | `water_find_pool` | (its own; Lego.c's name in `lego_unit_imports.h`) | 256 | unmapped | **100.00** |
| `0046e9a0` | `water_compare_pools` | (its own) | 48 | not annotated | **100.00** |
| `0046e9d0` | `water_find_joining_pool` | (its own) | 256 | 13.33 | **100.00** |
| `0046ead0` | `water_add_pool_points` | (its own) | 96 | 16.22 | **100.00** |
| `0046eb30` | `water_create_pool` | (its own) | 80 | 26.42 | **100.00** |

**The four functions with no source** (`0046de80`, `0046e1c0`, `0046e610`,
`0046e8a0`) are written and mapped, and so is the comparator `0046e9a0`, as
finding 577 did for `00434450`. The port had unannotated models with other
contracts for three of them: `gods98/view.cpp`'s
`global_entity_render_parameter_mode_set` and
`world_spatial_record_find_at_grid_coordinate`, and `level.cpp`'s
`terrain_edge_flag_midpoint_update`.

Two Lego.c callers became exact with no source change. They now call the
unit's functions:

- `004295f0` `lego_sound_minimum_distance_update_mode` (Lego_SetViewMode),
  from 98.37% to **100.00%**;
- `00430470` `lego_terrain_neighbor_flag_set`, from 96.88% to **100.00%**.

`004232e0` `lego_engine_tick` rose from 97.59% to 97.66%.

### What is left

- **Container.c adapters** (`0046de80`, `0046e390`): every instruction
  matches except the call targets of Container_MakeMesh2 (`00473560`) and
  Container_Mesh_SetVertices (`00474d30`). The port's annotated entries have
  C++ linkage and another contract (a ledger stub in a namespace, and a
  four-argument dispatcher), so the unit calls `gods98/mesh.h`'s unannotated
  adapters, as Map3D.c (`terrain_map.c`) does. Converting Container.c
  (G98-002) closes them.
- **`0046e390`** (98.99%) also has one scheduling difference: the original
  loads `level` before `fsub startLevel`, the rebuild after. The statement
  forms, operand orders and declaration orders tried all give the same code;
  `tools/msvc_permuter.py` made 6 attempts and found nothing.
- **`0046e610`** (effective match): in the second half, the rebuild loads
  the tail distance before `add esp` and swaps the operands of one `fadd`.
  It is the symbol counter's phase (finding 538): the first half, the same
  inline expansion, matches. Every operand order of the helper gives the
  same code.

No naked asm. The four `view.hpp` declarations kept below are the only
`Deviation (scoring only)`.

## Unit extent

Findings 599 and 603 apply: MSVC 5.0 emits each float literal once per
object, so a function's literal pool shows which object it was compiled in.

- **The data.** The unit's state is one common block, `00549520`-`0054bf0c`
  (`waterGlobs`, 0x29ec bytes: ten 0x428-byte pools, the pool count
  `0054beb0`, ten join pairs `0054beb4`, the join count `0054bf04` and the
  level's dig depth `0054bf08`). It opens the common region. Every
  reference to it is in `0046db90`-`0046eb30`, and all 12 listed functions
  but `0046ead0` reference it (`0046ead0` is given a pool by its two
  callers).
- **The literal pool.** The `.rdata` run is:

  | Address | Value | Used by |
  | --- | --- | --- |
  | `0049e7e4`-`0049e7f0` | `0.0f 0.5f 1.0f 0.4f` | GAME-048 (`0046cc30`-`0046d6f0`) |
  | `0049e7f4` | `0.0f` | no code reference |
  | `0049e7f8` | `-1.0f` | `0046e4d7` |
  | `0049e7fc` | `1.0f` | no code reference |
  | `0049e800` | `1.5f` | `0046e3dd` |
  | `0049e804` | `0.5f` | `0046e449` (`0046e390`), `0046e740`, `0046e756`, `0046e7fa`, `0046e810` (`0046e610`) |
  | `0049e808` | `240.0f` | `0046e455`, `0046e46c` |
  | `0049e80c`-`0049e818` | `0.3f 0.05f 0.40000004f 0.6f` | `0046e390` |
  | `0049e81c` | `0.0f` | no code reference |
  | `0049e820`- | `1.0f -1.0f 0.0f 25.0f` … | GAME-050 (`0046f1e8` onwards) |

  `0046e390` and `0046e610` load the same `0.5f`, so they are one object.
  The pool sits between GAME-048's `0.4f` and GAME-050's `1.0f`, and no
  function outside `0046db90`-`0046eb80` reads it.
  - `-1.0f` and `1.0f`: the source writes `ratio + 1.0f`, and MSVC emits
    `fsub -1.0f`. The rebuild does the same, so the `1.0f` keeps its pool
    entry with no code reference. This is the "`- -1.0`" in the
    pseudocode.
  - `0.40000004f` is `0.6f - 0.2f` folded in float.
  - The two zeros are both unreferenced: `0046e1c0` and `0046e390` push
    `0.0f` as an immediate. Why the pool has two is not explained; neither
    neighbour can own one, because both have their own `0.0f`.
  - `0046e1c0`'s `0.12f` is `0x3df5c290`, which is `0.3f * 0.4f` folded,
    not the literal `0.12f`. It and `0.32f` are pushed as immediates with
    no pool entry.
- **The calls.** The unit's functions call each other, Map3D.c (`004500d0`,
  `0044f6d0`), Container.c, Lego.c's `0042f2e0`, the empty function the
  linker folded to `00484c90`, and the C library's `qsort`. Their callers
  are all Lego.c:
  - MapLoadPipeline `0042b510` → `0046db90`;
  - `004295f0` → `0046e1c0`;
  - `00430470` → `0046e8a0`, `0046e330`;
  - `00430530` → `0046e220`;
  - the main loop → `0046e390`.
- **The edges.** `0046db70` (GAME-048) is a Vehicle accessor with no
  reference to the block. `0046eb80` is Weapons.c's WeaponTypeConfigLoad
  (`'WeaponRange'`, `.bss` from `00503870`). `0046eb30` ends at `0046eb73`.

`translation-units.spec.csv`: GAME-049 becomes `Water.c` (`inferred`) with
this evidence. `python3 tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

### State

| Offset | Field |
| ---: | --- |
| `0x000` | `points[100]`: the block positions, `Point2F` |
| `0x320` | `pointCount` |
| `0x324` | `entries[10]`, 0x18 bytes each: point index, direction (0 up, 1 right, 2 down, 3 left), level, flow front, flow tail, open |
| `0x414` | `entryCount` |
| `0x418` | `startLevel` (from -10000.0f, the highest wall corner) |
| `0x41c` | `level` |
| `0x420` | `mesh` (Container) |
| `0x424` | `flags`: 1 visible, 2 drained, 4 filled |

`Water_Globs waterGlobs` is a plain file-scope definition, so C places it
in the common region at `00549520`. `make unit-data-offsets`, GAME-049 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-049 | n/a (0, 0 unknown) | n/a (0, 0 unknown) | n/a (0, 0 unknown) | 00549520:_waterGlobs(size=10732) |

### `s_map_row_span_records` (findings 602, 609)

`gods98/view.cpp`'s port-only `s_map_row_span_records` (64 records of
`0x324` bytes, 51,456 bytes) and its count were the port's stand-in for the
pools' points. Their one writer was the port's model of `0046eb30`, and
their readers were `view.cpp`'s `map_row_span_record_count` and
`map_row_span_record_get`. **Nothing called any of the four natively**: the
port's water model (`TerrainWorldSpatialRecords`) keeps its own runs on the
level. So:

- the buffer, its count and the two readers are deleted from `view.cpp`;
- the port's writers `map_row_span_append` and `map_row_span_record_start`
  are deleted from the port tail;
- the original's own state, `waterGlobs` (10,732 bytes), holds the pools in
  the matching build.

Native behaviour is unchanged: the deleted code was unreachable. The frozen
`view_for_*.hpp` and `view_shared.hpp` headers still declare the buffer;
nothing references it. **`view.hpp` keeps the four function declarations**,
tagged `Deviation (scoring only)`: removing them dropped `osf_read`
(`00459470`, `object_state_file.cpp`) from 20.28% to 9.71% (11 fewer
instructions), finding 538's drift. They declare nothing that exists.

`.bss`:

- **The gate build**: the image end moves from `0x8ea000` to `0x8df000`
  (45,056 bytes lower). `make pe-audit` still reports the image over
  `0x800000` (margin -913,408), and the plain-immediate check passes.
- **The `/OPT:REF` relink**: `.bss` falls from 1,668,960 to 1,628,256
  bytes (-40,704: the 51,460-byte buffer and count, less `waterGlobs`).

### The port tail

As in findings 597-612, `cell_flood_passes.cpp` becomes
`cell_flood_passes_port.cpp`, a `tail` entry, in Meson too:

- its `// FUNCTION` annotations are removed;
- its definitions of the two names `water_link_names.h` gives the unit are
  compiled out of the matching build (`LRR_WATER_PORT_NAMES`, an explicit
  `#if`, finding 590);
- the row-span writers are deleted (above).

Its eight static helpers leave the lint ledger (1,080 to 1,072 entries).

### Link names

- `water_link_names.h` renames `water_initialise` and
  `water_destroy_wall_complete` to the port's names in the matching build.
  Their port definitions are in the tail, so the tail compiles them out
  there.
- Lego.c's other four callees in the unit have port definitions outside the
  tail (`gods98/view.cpp`, `terrain_grid.cpp`, the adapter
  `simtick_fx_bank_tick_original` in `lego_port.cpp`). So
  `lego_unit_imports.h` points Lego.c at the unit's names in the matching
  build instead, with four `#define`s beside Camera's and Viewports' (no
  declaration is added, so `lego_unit.c`'s numbering does not move).

Every matching-build caller passes the original's arguments. Natively the
port serves every name and nothing calls the unit.

### Source shapes worth noting

- `0046e1c0`'s callee is the empty function at `00484c90`, called with the
  mesh and three floats. In the matching build `water_imports.h` declares
  `flic_opcode27_fallback_noop` with that prototype (an unprototyped
  declaration would promote the floats to doubles). Elsewhere the call is a
  `PORT-ONLY` no-op macro.
- `0046e610`'s flow point is an inline helper with pointer arguments
  (`water_maths.h`). Only that form keeps the result in memory and the
  distance on the x87 stack, as the original does; written in place, MSVC
  forwards the values to the call. The helper is in a header so that it adds
  no lint debt, and the include moves the symbol numbering:
  `0046e8a0` then needs its point sum in a `Point2F` local through pointer
  temporaries.
- `0046e9d0` decrements its `by` parameter once (`by--`, "the row above"),
  as the original's hoisted `by - 1` shows.
- `0046e610` declares its direction table before its corner pairs, and
  `0046de80` declares its block offsets and face data before the normal and
  UVs. The order of the zero stores (register or immediate) follows it.
- Container_Mesh_AddGroup's port contract takes 16-bit indices; the original
  passes 32-bit face data, so `0046de80` casts it (no code change).

## Checks

- `make reccmp-check`: OK. No function fell below the baseline; the drop
  above is fixed by the deviation, not accepted.
- The plain-immediate check, `winapi-link-check` and `check-map`: OK.
- `make match-progress`: baseline, listing digests and `binary-progress.json`
  regenerated. Accuracy 87.16% to 87.43% (full), 85.90% to 86.20%
  (game-only).
- `make unit-data-offsets`: the row above.
- `make tools-test` (no `--allow-growth`), the native build and `ctest`
  (37 of 37), and `gcc -std=gnu89 -Werror=declaration-after-statement` on
  the unit: pass.

## `/OPT:REF` (finding 579)

`make opt-ref-callers`: `water.c.obj` loses no function, and before the
change `cell_flood_passes.cpp.obj` lost none either. The per-object caller
lists are unchanged except one line: the live caller of the removed
Container_MakeMesh2 (`00473560`) is now `0046de80`, which calls the
`container_make_mesh2` adapter instead (see [What is left](#what-is-left)).
The totals fall from 428 / 207 to 423 / 206 (full) as the unit's 13
functions are mapped and kept; the lists do not attribute that change to an
object.
