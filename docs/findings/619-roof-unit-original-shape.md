# Finding 619 — Roof.c (GAME-038) rebuilt in original shape, with a port tail

Issue #683, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-038 is the level's roof: one mesh group per block over the blocks the
player cannot see into. OpenLRR names the object `Roof.c`, and the unit map
now uses that name (`inferred`). The unit map called it "terrain overlay
quads" and listed 9 functions, all low-confidence and 5 with no source.
**The extent holds** (`00462da0`-`004636b0`, 9 functions). See
[Unit extent](#unit-extent).

What the unit does:

- **Roof_Initialise** (`00462da0`, from Lego.c's MapLoadPipeline `0042b510`)
  makes one mesh container under the root and a grid of 8-byte blocks, none
  with a group yet.
- **Roof_SetTexture** (`00462e60`, from TextureSetConfigLoad `0042bb60`)
  loads the roof texture and sets it on every group.
- **Roof_SetBlockRoofVertices** (`00462f20`, from `00433380`) gives a block
  its quad. The block's group is made the first time. The quad is split
  along the diagonal through its lowest corner, so the corners are stored in
  one of two orders (flag 4). The block is marked for update (flag 2).
- **Roof_Update** (`00463230`, once a frame from the main loop) gives each
  corner of a marked block the normalised sum of the plane normals of the
  four blocks around that corner (`00463590`).
- **Roof_Hide** (`004634b0`) hides the whole roof outside the top-down view.
  **Roof_HideBlock** (`004634f0`, from `00433e80`) hides the blocks around
  the cursor until **Roof_ShowHiddenBlocks** (`00463540`) shows them again.
- **Roof_Shutdown** (`00462ed0`, from Lego_LevelFree `0042f0c0`).

This change rebuilds the unit as the C unit
[`src/game/roof.c`](../../src/game/roof.c), with:

- the C header [`roof.h`](../../src/game/roof.h);
- the unit-private imports header [`roof_imports.h`](../../src/game/roof_imports.h);
- the inline helper header [`roof_maths.h`](../../src/game/roof_maths.h).

The relocated `terrain_overlay_quads.cpp` becomes the port tail
[`terrain_overlay_quads_port.cpp`](../../src/game/terrain_overlay_quads_port.cpp),
under `git mv`, unannotated.

"Before" is the *match baseline* at `main` `9043af89`. "After" is this
change, from the full matching build (`make reccmp-check`). The per-function
scores are the same on `main` and on the stack; the
[stacked figures](#stacked-on-693) follow.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (9 functions, weighted by span, 2,320 bytes) | 9.24% | **90.89%** |
| Exact functions in the unit | 0 | **1** (no asm) |
| Mapped functions in the unit | 4 | **9** |
| Exact-function code bytes (`binary-progress.json`) | 205,613 | **205,981** |
| `opt_ref` annotated (full): removed / exact removed | 412 / 223 | **410 / 222** |
| `opt_ref` annotated (game-only) | 405 / 216 | 403 / 215 |
| Gate image end (`make pe-audit`) | `0x8bd000` | `0x8bf000` |
| `/OPT:REF` relink `.bss` (`binary-progress.json`) | 1,486,864 | 1,488,912 |
| Reccmp lint ledger (entries) | 1,052 | 1,052 |

The weights are each function's span up to the next function, padding
included. An unmapped function counts as 0%. The full run does not promote
effective matches: it scores `00463590` as 98.90%, and `make
match-function` reports it as `100.00%*`.

The names are the unit's own, after OpenLRR's `Roof_*` functions. They are
`inferred`: the unit has no strings. "Lego.c's name" is the name
`lego_unit.c` calls (see [Callers](#callers)).

| Address | Function | Lego.c's name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00462da0` | `roof_initialise` | `indexed_surface_resource_allocate` | 192 | unmapped | 98.15 |
| `00462e60` | `roof_set_texture` | `animated_texture_replace` | 112 | unmapped | 95.45 |
| `00462ed0` | `roof_shutdown` | `graphics_overlay_resources_release` | 80 | unmapped | 94.74 |
| `00462f20` | `roof_set_block_roof_vertices` | `lego_roof_block_vertices_set_adapter` | 784 | unmapped | 98.51 |
| `00463230` | `roof_update` | `terrain_dirty_quad_normal_emit_pass_original` | 640 | unmapped | 72.43 |
| `004634b0` | `roof_hide` | `graphics_overlay_visibility_set` | 64 | 89.47 | **100.00** |
| `004634f0` | `roof_hide_block` | `indexed_surface_entry_hide_and_record` | 80 | 16.95 | 95.24 |
| `00463540` | `roof_show_hidden_blocks` | `indexed_surface_entries_restore` | 80 | 47.62 | 95.83 |
| `00463590` | `roof_get_block_plane_normal` | (none: only `00463230` calls it) | 288 | 36.60 | 98.90 (effective) |

**The five functions with no source** (`00462da0`, `00462e60`, `00462ed0`,
`00462f20`, `00463230`) are written and mapped. The port had models with
other contracts for three of them, in `gods98/resource.cpp` (behind
`/* FUNCTION */` block comments, which Reccmp does not read; they now say
what they model), and no-op adapters for the other two in `lego_port.cpp`.

Lego.c callers that rose, with no source change:

- `00433380` `terrain_cell_mutation_progress`, from 99.09% to **100.00%**;
- `0042bb60` `lego_texture_set_config_load`, 98.08% to 98.72%;
- `0042b510` `lego_map_load_pipeline`, 97.84% to 98.20%;
- `0042f0c0` `lego_level_free`, 84.62% to 85.31%;
- `004232e0` `lego_engine_tick`, 97.66% to 97.73%.

No other function changed score or size.

### What is left

- **Container.c adapters.** Six functions differ only in call targets:
  - Container_MakeMesh2 (`00473560`) in `00462da0`;
  - Container_LoadTexture2 and Container_FreeTexture (`004744f0`,
    `00474710`) in `00462e60` and `00462ed0`;
  - Container_Mesh_HideGroup (`00474b30`) in `00462f20`, `004634f0` and
    `00463540`;
  - Container_Mesh_SetPerspectiveCorrection (`00474e30`) in `00462f20`;
  - Container_Mesh_SetVertices (`00474d30`) in `00462f20` and `00463230`.

  The port's annotated entries have C++ linkage or another contract, so the
  unit calls `gods98/mesh.h`'s adapters and Detail.c's texture adapters
  (`tile_variants_port.cpp`), as Map3D.c and Water.c do. Converting
  Container.c (G98-002) closes them.
- **`00463590`** (effective match): the rebuild sets `eax` to 1 a few
  instructions earlier. The three forms of the two guards tried (nested,
  combined, early returns) give the same code.
- **`00463230`** (72.43%). The structure, the corner table and the calls
  match. Three differences remain, and every variant tried keeps them:
  - In the accumulation, the original loads the neighbour's normal first
    (`fld normal.x`, `fadd v.x`) and hoists the loads of `normal.y` and
    `normal.z` above the store to `v.x`. The rebuild loads `v.x` first and
    does not hoist.
  - In the normalisation, the rebuild preloads the three components before
    `fsqrt` and spills the scale to the stack. The original keeps the scale
    on the x87 stack.
  - The frame slots of `bx`, `by`, the block pointer, the inner loop's
    count-down and the two inlined `dot` temporaries are in another order.

  Tried and unchanged: every declaration order of the eight locals (24
  permutations); the vertices, the normal or both declared in the block or
  the inner loop; a `const` corner table; an `int` corner; `add(v, n, v)`,
  `add(v, v, n)`, `+=` and explicit statements; a pointer local for the
  vertex normal; five forms of the `dot`, `modulus`, `scale` and
  `normalise` helpers (one of them `set_length(v, 1.0f)`); and the include
  and declaration counts before the function (removing `maths.h` or
  `mesh.h`, other include orders). A sweep of 1 to 256 inert declarations
  before the function, compiled to test finding 538's phase and not kept,
  changed nothing either. So this is not the symbol counter's phase.
  Writing the normalisation out in place, without the nested `normalise`
  inline, drops it to 49.72%. `tools/msvc_permuter.py` made 4 attempts in
  150 s and found nothing.

No naked asm and no `Deviation (scoring only)`.

## Unit extent

Findings 599 and 603 apply: MSVC 5.0 emits each float literal once per
object, so a function's literal pool shows which object it was compiled in.

- **The data.** The unit's state is one common block, `00552180`-`00552970`
  (`roofGlobs`, 0x7f0 bytes): the mesh `00552180`, the grid size
  `00552184`/`00552188`, Roof_Hide's last value `0055218c`, the update flag
  `00552190`, the texture `00552194`, the block grid `00552198`, 500
  hidden-block pointers `0055219c` and their count `0055296c`. Every
  reference to it is in `00462db3`-`004635da`, and all 9 functions
  reference it. No code references `00552970` or `00552974`; Reward.c's
  commons start at `00552980`.
  ScrollInfo.c's commons (`00552140`-`00552164`) end before it.
- **The literal pool.** The `.rdata` run is:

  | Address | Value | Used by |
  | --- | --- | --- |
  | `0049e6f0`-`0049e6f8` | `-0.01f 0.01f 0.1f` | GAME-037 (`00462d29`, `00462d3e`, `00462d77`: Reward.c's `00462ad0`) |
  | `0049e6fc` | `0.0f` | no code reference |
  | `0049e700` | `1.0f` | `00463428` (`00463230`), `0046366c` (`00463590`) |
  | `0049e704`-`0049e70c` | `0.0f 1.0f 0.0f` | no code reference |
  | `0049e710` | `0.5` (double) | GAME-039 (`00463e65`) |

  `00463230` and `00463590` load the same `1.0f`, so they are one object.
  The rebuilt `roof.c.obj`'s `.rdata` is exactly `0.0f 1.0f`. The zero
  comes from `00462f20`'s zero normal and texture coordinates, which MSVC
  stores as immediates while still pooling the literal. So the pool
  `0049e6fc`-`0049e700` is Roof.c's, right after Reward.c's `0.1f`.
  The three entries `0049e704`-`0049e70c` are not explained: the rebuilt
  ScrollInfo.c pools only its double.

  Finding 611's table gives `0049e6f8` (`0.1f`, `00462d77`) to GAME-038.
  `00462d77` is in Reward.c's `00462ad0` (`reward.c`'s
  `reward_scroll_update`, finding 560), so that `0.1f` is GAME-037's.
- **The calls.** The unit calls Container.c, Maths.c's Maths_PlaneNormal
  (`00479530`) and the C library's `malloc` and `free`. Its callers are all
  Lego.c:
  - MapLoadPipeline `0042b510` → `00462da0`, `004634b0`;
  - TextureSetConfigLoad `0042bb60` → `00462e60`;
  - Lego_LevelFree `0042f0c0` → `00462ed0`;
  - `00433380` → `00462f20`;
  - the main loop `004232e0` → `004634b0`, `00463230`, `00463540`;
  - `00433e80` → `004634f0`.

  `00463590` is called only by `00463230`.
- **The edges.** `00462da0` follows Reward.c's `00462ad0`, which has its
  own pool and data. `004636b0` (GAME-039, ScrollInfo.c) writes
  `00552140` onwards and loads only its own double.

`translation-units.spec.csv`: GAME-038 becomes `Roof.c` (`inferred`) with
this evidence. `python3 tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

### State

`RoofBlock` (8 bytes) is the group index and a flags byte: 1 the block has
no group yet, 2 its normals need updating, 4 its quad is split along the
other diagonal. `Roof_Globs roofGlobs` is a plain file-scope definition,
so C places it in the common region at `00552180`. `make
unit-data-offsets`, GAME-038 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-038 | n/a (0, 0 unknown) | n/a (0, 0 unknown) | n/a (0, 0 unknown) | 00552180:_roofGlobs(size=2032) |

The port's statics for the same state stay where they are
(`gods98/record.cpp`'s `s_overlay_word` and `s_overlay_cache`, which lose
their `// GLOBAL` annotations for `00552180` and `0055218c`, and
`resource.cpp`'s indexed-surface statics): native code uses them.
`roofGlobs` adds 2,032 bytes to the gate build (image end `0x8bd000` to
`0x8bf000`) and to the relink's `.bss`. The plain-immediate check passes.

### The port tail

As in findings 597-617, `terrain_overlay_quads.cpp` becomes
`terrain_overlay_quads_port.cpp`, a `tail` entry, in Meson too. Its four
`// FUNCTION` annotations are removed. Its definitions stay in every build,
because native code calls them; in the matching build nothing does. The
relocation's frozen private headers (`record_for_terrain_overlay_quads.hpp`,
`level_for_terrain_overlay_quads.hpp`) are unchanged. The file had no lint
debt, so the ledger stays at 1,052.

### Callers

Every port definition of Lego.c's eight callees is outside a unit file
(`gods98/resource.cpp`, the port tail, and `lego_port.cpp`'s two no-op
adapters), so `lego_unit_imports.h` points Lego.c at the unit's names in
the matching build, with eight `#define`s beside Water.c's. No declaration
is added, so `lego_unit.c`'s numbering does not move. Every call passes
the original's arguments. Natively the port serves every name and nothing
calls the unit; native callers are not switched (#552). Natively the
`00462f20` and `00463230` adapters still do nothing: #596 tracks the first.

### Source shapes worth noting

- `00462f20` writes the four vertices in the order 3, 2, 1, 0, in both
  branches and for the normals, colours and texture coordinates. The
  normal is a zero-initialised `Roof_Vector3F` copied into each vertex: the
  original stores its three zeros at the top of the function. Written in
  order 0 to 3 with scalar zeros, the function scored 52.53%.
- `00463230` declares the corner table inside the `if` block, where the
  original initialises it, and writes the corner as an `if`/`else`.
- `00463590` adds the two plane normals straight into the output and
  normalises it there (`roof_vector3d_add(normal, &second, &first)`, then
  `roof_vector3d_normalise(normal)`). Adding into a local first scored
  80.90%.
- `00463230` and `00463590` use gods98 Maths.h's inline helpers as
  `static __inline` functions in `roof_maths.h` (finding 544), so they add
  no lint debt.
- Container_Mesh_AddGroup's port contract takes 16-bit indices. The original
  passes 32-bit face data, so `00462f20` casts it (no code change).

## Checks

- `make reccmp-check`: OK. No function fell below the baseline.
- The plain-immediate check, `winapi-link-check` and `check-map`: OK.
- `make match-progress`: baseline, listing digests and `binary-progress.json`
  regenerated. Accuracy 88.24% to 88.33% (full), 87.09% to 87.19%
  (game-only).
- `make unit-data-offsets`: the row above.
- `make tools-test` (no `--allow-growth`), the native build and `ctest`
  (37 of 37), and `gcc -std=gnu89 -Werror=declaration-after-statement` on
  the unit: pass.

## `/OPT:REF` (finding 579)

`make opt-ref-callers`: `roof.c.obj` loses no function. The list names
`00462da0` as a live caller of the removed Container_MakeMesh2
(`00473560`), which it calls through the `container_make_mesh2` adapter
(see [What is left](#what-is-left)). The totals move from 412 / 223 to
410 / 222 (full) as the unit's 9 functions are mapped and kept.

## Stacked on #693

Rebased as the top layer over #693 (`030e322b`, over #691, #690 and
#689). The unit's scores, and the four Lego.c callers' gains, are
unchanged (`004232e0` is 97.80% to 97.87% there).

| | #693 | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 208,597 | **208,965** |
| `opt_ref` annotated (full): removed / exact removed | 399 / 220 | **398 / 220** |
| `opt_ref` annotated (game-only) | 392 / 213 | 391 / 213 |
| Accuracy, full / game-only | 88.95 / 87.87 | 89.03 / 87.97 |
| Gate image end (`make pe-audit`) | `0x891000` | `0x892000` |
| Relink `.bss` | 1,302,048 | 1,304,096 |

The gate image grows by one page, in `.data`. `roofGlobs` adds 2,032
bytes of commons to `.data`'s virtual size (now 3,457,520, `0x34c1f0`),
which carries it past a page boundary. The relink's `.bss` grows by the
same 2,048 bytes, with alignment. `.text` grows by 2,336 bytes (the five
newly compiled functions) and stays inside its last page. `.rdata` grows
by 16 bytes.
