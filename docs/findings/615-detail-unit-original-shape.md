# Finding 615 — Detail.c (GAME-011) rebuilt in original shape, with a port tail

Issue #672, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-011 had no name. Its extent is text `0040b310`-`0040bdc0`, 11
functions. It is the block detail meshes, named here after OpenLRR's
Detail.c:

- **Texture sets.** Detail_LoadTextureSet reads a width x height grid of
  `<base><x><y>.bmp` textures. Detail_GetTexture picks one by a texture
  byte `0xXY` (x in the high nibble).
- **Detail meshes.** Detail_LoadMeshes loads two LightWave meshes (the
  uncovered and the surface mesh) at 1% of the block size, hidden, and flips
  the second one's UVs. For each visible block, `0040b880` clones both,
  places each on three of the block's corners (the second turned by two
  corners), textures it and moves it into place. Detail_ResetAll removes
  the clones.
- **`0040b470`** weighs a block's projected screen area. Each corner off
  screen, or outside the depth range, costs a quarter.

#496 relocated the unit verbatim into `src/game/tile_variants.cpp`. This
change rebuilds it as the C unit [`src/game/detail.c`](../../src/game/detail.c),
with these headers:

- [`detail.h`](../../src/game/detail.h): the unit's types and functions;
- [`detail_imports.h`](../../src/game/detail_imports.h): the callees, by
  their matching-build names;
- [`detail_link_names.h`](../../src/game/detail_link_names.h);
- [`detail_maths.h`](../../src/game/detail_maths.h): the inline Maths.h
  vector helpers (dot, modulus, subtract, cross product).

The port's code moves by `git mv` to the port tail
[`tile_variants_port.cpp`](../../src/game/tile_variants_port.cpp), a `tail`
entry and a Meson source.

"Before" is the *match baseline* on `main` at `8abe8b6b` (#669). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (11 functions, 2,736 bytes, span-weighted) | 32.60% | **88.08%** |
| Exact functions in the unit | 1 | **8** (no asm) |
| Exact-function code bytes (`binary-progress.json`) | 196,138 | **197,965** |
| Accuracy, full / game-only | 87.16% / 85.90% | 87.39% / 86.15% |
| Annotated exact functions (full) | 1,686 | 1,695 |
| `opt_ref` annotated (full): removed / exact removed | 428 / 207 | **421 / 207** |
| `opt_ref` annotated (game-only) | 421 / 200 | 414 / 200 |
| Reccmp lint ledger (entries) | 1,080 | **1,078** |

The names are `inferred`. Detail_LoadTextureSet, Detail_FreeTextureSet,
Detail_LoadMeshes, Detail_FreeMesh, Detail_ResetAll and Detail_GetTexture
follow OpenLRR. The other four are descriptive. "Link name" is the name a
unit definition takes in the matching build (see [Link names](#link-names)).

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0040b310` | `detail_load_texture_set` | `terrain_cell_resource_grid_load` | 256 | 15.56 | 98.81 |
| `0040b410` | `detail_free_texture_set` | `terrain_cell_resource_grid_dispose` | 96 | 35.79 | 97.44 |
| `0040b470` | `detail_get_block_screen_area` | `lego_projected_quad_area_adapter` | 384 | 51.88 | **100.00** |
| `0040b5f0` | `detail_load_meshes` | `tile_model_entity_register_original` | 480 | 17.46 | 89.29 |
| `0040b7d0` | `detail_free_mesh` | `cell_visual_channel_table_dispose_original` | 64 | 65.67 | **100.00** |
| `0040b810` | `detail_reset_all` | `lego_detail_mesh_resources_clear_adapter` | 64 | 51.72 | **100.00** |
| `0040b850` | `detail_get_texture` | `packed_grid_cell_value_get` | 48 | 18.52 | **100.00** |
| `0040b880` | `detail_add_block` | `scene_cell_visual_channels_apply` | 384 | 16.59 | **100.00** |
| `0040ba00` | `detail_place_mesh` | (its own) | 400 | 49.19 | **100.00** |
| `0040bb90` | `detail_set_mesh_frame` | (its own) | 464 | 20.28 | 41.98 |
| `0040bd60` | `detail_matrix_setup` | (its own) | 96 | 100.00 | **100.00** |

These Lego.c functions rose with no change to their code. Each now calls a
unit function instead of a `PORT-ONLY` stub:

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0042b330` | `lego_load_tile_meshes` | 99.08 | **100.00** |
| `0042b480` | `lego_rectangular_resource_table_destroy` | 97.73 | **100.00** |
| `00433e80` | `lego_terrain_projected_cell_state_refresh` | 94.29 | 94.51 |
| `00434530` | `terrain_grid_entry_resources_release_all_current` | 95.65 | **100.00** |

### What is left

- **`0040b310`, `0040b410`** differ only in one callee operand each:
  Container_LoadTexture2 (`004744f0`) and Container_FreeTexture
  (`00474710`). Both have C++ linkage in the port, so the unit calls
  `PORT-ONLY` adapters (see [Callees](#callees)).
- **`0040b5f0`** (89.29%): five callee operands (three Container_Load,
  Container_Mesh_GetGroup and the `00474e90` scale stub), and two dead
  parameter slots used the other way round. The original keeps the scale
  and the memory-held swap flag in `surfaceName`'s slot, and the vertex
  count in `blockSize`'s. MSVC here keeps the scale in `root`'s slot, and
  swaps the other two. Tried:
  - six declaration orders;
  - the scale as a local, and as `blockSize * 0.01f` at the call;
  - five forms of the two swap flags (chained assignments both ways round,
    braced pairs, `?:`). `if (loop == 0) swapV = swapU = 0; else swapV =
    swapU = 1;` gives the original's register for the first flag (88.10% to
    89.29%). `?:` gives `setcc` (56.79%).

  `tools/msvc_permuter.py` (240 s) found no candidate, and the inert
  declaration sweep (finding 538, rule B) leaves its code unchanged at
  every N from 0 to 127.
- **`0040bb90`** (41.98%) is two vector differences, a cross product, two
  normalisations, and four Container calls. Its frame now matches (`sub
  esp, 0x58`): two vectors and the matrix, with the two InverseTransform
  outputs sharing the vectors' slots. Its x87 code does not match. The
  original:
  - keeps `dir.x` in a register and never stores it;
  - stores `dir.y` and `dir.z` before it loads `c`;
  - takes the reciprocal lengths into two parameter slots and multiplies at
    each push.

  MSVC here loads `c` first and keeps `dir.z` instead. Tried:
  - the helpers or literal expressions;
  - scalar locals (33.33%);
  - a cross product through a temporary vector, through scalars, and in
    both handednesses;
  - in-place normalisation (43.02%, but more stores);
  - the reciprocal before or after the up vector;
  - two block scopes (the first form that matched the frame);
  - the InverseTransform outputs as new vectors or as `dir` and `up`;
  - the dot product in reverse order;
  - 0-40 inert declarations (no change in score).

  The `fld [edi+4]`/`fstp [esp]` argument copy of SetPosition's `y` and `z`
  is the one finding 589 found only here, in `0040be10` and in `0040cce0`.
  It did not reproduce there either. The permuter (240 s) found no
  candidate.

No naked asm, and no `Deviation (scoring only)`. The unit has no `.bss`, so
the plain-immediate check (finding 584) passes unchanged.

## Unit extent

The map gave seven functions (`0040b7d0`-`0040bd60`) low confidence, with
the boundary to GAME-012 unproven. All seven belong to the unit, and the
unit is one object:

- **Call graph.** The unit's internal calls form a closed graph:
  - `0040b7d0` calls `0040b810`;
  - `0040b880` calls `0040b850` and `0040ba00` (twice);
  - `0040ba00` calls `0040bb90`;
  - `0040bb90` calls `0040bd60`.

  `0040ba00`, `0040bb90` and `0040bd60` have no other caller. Every outside
  caller is in Lego.c (GAME-021: `0042b330`, `0042b480`, `0042bb60`,
  `0042f0c0`, `00433e80`, `00434480`, `00434530`, `004288e0`, `00429160`)
  or in GAME-026 (`0044e6d0`, `0044f230`, which call Detail_GetTexture).
- **The next unit.** `0040bdc0` (`SetTimeoutTo10000`) calls `0040c3a0`, the
  effects update. It is GAME-012's first function (finding 589).
- **Data.** The unit has no `.bss` or `.data`. Its `.rdata` is
  `0049e0d0`-`0049e0ec`:
  - `0.0f` (`0049e0d0`), `1.0f` (`0049e0d4`) and `0.25f` (`0049e0d8`);
  - padding at `0049e0dc`, then `0.25` as a double at `0049e0e0`;
  - `0.01f` (`0049e0e8`).

  Only `0040b470`, `0040b5f0` and `0040bb90` read them. GAME-012's `.rdata`
  starts at `0049e0ec` (`-2.0f`). The two words before `0049e0d0`
  (`1.0f`, `0.0f` at `0049e0c8`/`0049e0cc`) have no reference. The
  literals `"%s%0.1x%0.1x.bmp"` (`004a1da8`) and `"LWO:NOTEXTURE"`
  (`004a1dbc`) are the unit's own; GAME-012's `"RockFallStyles"` follows at
  `004a1dcc`.
- **The double.** `0040b470` subtracts `0.25f` four times and `0.25` once.
  The last test (`z > w || z < 0.0f`) shares one `fsub qword`. The unit
  writes that one `mult -= 0.25;`. Plain literals match all six constants;
  no annotated static is needed.

`translation-units.spec.csv` now names GAME-011 `Detail.c` (`inferred`) and
records this evidence. `python3 tools/translation_units.py build`: 0
conflicts.

`make unit-data-offsets`, GAME-011 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-011 | gaps-only (2, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 11, partial 22, n/a 194.

## How the conversion is done

### Step 0

`tools/convert_unit.py GAME-011 --stem detail` scaffolded the unit after
`tile_variants.cpp` became the tail `tile_variants_port.cpp`. Before that
it refused, because the link-order list already named GAME-011. The
scaffold's link-order entry, `.rdata` statics and stubs were then replaced
by the unit.

### Link names

In the matching build `detail_link_names.h` gives eight unit functions the
port's names (table above). Each has C linkage, and every caller passes the
original's arguments. The port's records have the original's layout on x86:
`TerrainCellResourceGrid` is Detail_TextureSet, and
`SceneCellVisualRelayRecord` is Detail_Mesh, 0x340 bytes. So these callers
reach the unit with no caller edit:

- `lego_unit.c` (all eight) and `terrain_map.c` (Detail_GetTexture);
- `terrain_grid.cpp` (load and free the texture set);
- `util.cpp` and `lego_port.cpp` (`0040b880`).

Compiled out of the matching build:

- in `tile_variants_port.cpp`, under `LRR_DETAIL_PORT_NAMES`:
  - the port's `terrain_cell_resource_grid_load`,
    `terrain_cell_resource_grid_dispose`, `packed_grid_cell_value_get` and
    `scene_cell_visual_channels_apply`;
  - the `PORT-ONLY` adapters `tile_model_entity_register_original` (which
    returned NULL) and `cell_visual_channel_table_dispose_original`;
- in `lego_port.cpp`, with `#if !(defined(_MSC_VER) && _MSC_VER < 1200)`,
  the `PORT-ONLY` adapters:
  - `lego_projected_quad_area_adapter`, which returned 0;
  - `lego_detail_mesh_resources_clear_adapter`.

`0040ba00`, `0040bb90` and `0040bd60` keep the unit's names. Only the unit
calls them. The port's C++ models of the same addresses
(`scene_rotation_flags_apply`, `scene_frame_from_triplet_apply` and
`perspective_projection_matrix_set`) stay in the tail for `view.cpp` and
`test_view.cpp`. So do the port's models of `0040b470`, `0040b5f0`,
`0040b7d0` and `0040b810`, which take hooks or the ledger namespace's
table. Each annotation in the tail became a `/* Port model of 0x… */`
comment.

Natively the unit compiles as C++ under its own names, and nothing calls
it. The tail serves every port name.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| Container_Load `00472cd0` | `container_load` (`entity.cpp`'s adapter) | Container.c |
| Container_Remove, Clone, Hide, Mesh_GetGroupCount, Mesh_GetVertices, Mesh_SetVertices, Mesh_SetTexture, Mesh_SetPerspectiveCorrection, SetColourAlpha, SetPosition, SetOrientation, AddRotation, AddScale, AddTranslation | the C names `legoobject_unit.c`, `mesh.h` and `view.h` already use | Container.c |
| Container_LoadTexture2 `004744f0`, Container_FreeTexture `00474710`, Container_Mesh_GetGroup `00474c40`, the scale stub `00474e90`, Container_InverseTransform `00475090` | new `PORT-ONLY` adapters at the end of `tile_variants_port.cpp` | Container.c (C++ linkage in the port) |
| Container_SetMatrix `004756b0` | `legoobject_gt_container_matrix_set_adapter` (`legoobject_port.cpp`) | Container.c |
| Viewport_Transform `004772b0` | `render_interface_transform_point` (`viewports_link_names.h`) | Viewports.c |
| Maths_TriangleAreaZ `00479630` | `surface_triangle_area_calculate` | Maths.c |
| `sprintf`, `malloc`, `free`, `memset` | `crt/vc5names.h` | LIBC |

No `extern "C"` was added to any existing declaration, and no frozen
`_for_`/`_from_` header was edited. The five new adapters forward their
arguments unchanged. Container_LoadTexture2's fourth argument is the
height output. The port's is a mode word. The unit passes no outputs, so the
adapter passes 0.

### Source shapes that matched

- **Detail_GetTexture** (`0040b850`): two `unsigned` locals,
  `x = (texture & 0xf0) >> 4` and `y = texture & 0x0f`. Indexing with
  `texture >> 4` directly gives `sar` and no `esi` (33.33%).
- **`0040b880`**: the block size and the corner table are declared inside
  `if (mesh)`, so the table's stores follow the null test (32.93% to
  63.81%). A row pointer, `order = vertOrder[direction]`, gives the
  original's `lea eax, [esp + edx*4 + …]` and three loads through it
  (63.81% to 100%). `(direction + 2) % 4` keeps the original's signed
  modulo.
- **`0040b470`**: an `else if` chain, so each of the first four tests has
  its own `fsub`, and the result starts as a `0.0f` local. It matched on the
  first compile.
- **`0040bb90`**: `Detail_Vector3F dir, up, tb, tc;` at function scope
  gives the original's 0x58-byte frame. MSVC packs the four vectors into
  two slots.

## Entropy and drops

`make reccmp-check` against the baseline at `8abe8b6b`:

- 14 improved: the 10 unit addresses below 100% and the 4 Lego.c callers
  above;
- **none below baseline**, and no exact match lost (`0040bd60` stays
  exact);
- the plain-immediate check, the winapi link check and the link-order map
  check pass.

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0040b310` | 162 | 84 |
| `0040b410` | 46 | 39 |
| `0040b470` | 119 | 122 |
| `0040b5f0` | 204 | 168 |
| `0040b7d0` | 35 | 25 |
| `0040b810` | 34 | 24 |
| `0040b850` | 32 | 18 |
| `0040b880` | 116 | 126 |
| `0040ba00` | 226 | 143 |
| `0040bb90` | 182 | 170 |

`0040bd60` keeps 18 instructions. The four Lego.c callers' listings are
unchanged.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | committed `main` | same-worktree `main` | this change |
| --- | ---: | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 428 / 207 | 428 / 207 | **421 / 207** |
| `opt_ref.annotated.game_only` (by address) | 421 / 200 | 421 / 200 | 414 / 200 |

On `main`, `tile_variants.cpp.obj` lost six functions (one exact):
`0040b470`, `0040b5f0`, `0040b880`, `0040ba00`, `0040bb90` and `0040bd60`.
`0040b5f0` was lost because `lego_load_tile_meshes` called the NULL stub
`tile_model_entity_register_original`. Now `detail.c.obj` loses five, four
of them exact:

- `0040b470`, whose only caller `00433e80`
  (`lego_terrain_projected_cell_state_refresh`) is itself removed;
- `0040b880`, whose only caller `00434480`
  (`lego_terrain_cell_surface_update_dispatch_level`) is removed;
- `0040ba00`, `0040bb90` and `0040bd60`, which only `0040b880` reaches.

`0040b5f0` is kept. The other five are removed only because their Lego.c
(GAME-021) callers are: the relink drops `00433e80` and `00434480`
themselves. `tile_variants_port.cpp.obj` lists no
removed annotated function, since it has none.

## Reccmp lint ratchet

1,080 → 1,078 entries (−2, no growth). The tail's two static helpers
(`cells_dword`, `scene_frame_vector_length`) are exempt in a tail file.
`make reccmp-lint-update` removed their entries. The unit's inline helpers
live in `detail_maths.h`, as the other units' Maths.h copies do.

## Native behaviour

Unchanged:

- the tail serves every port name natively, and the port's C++ models stay;
- `detail.c` compiles natively as C++ under its own names, and nothing
  native calls it.

There is no new native gap, so no `port` issue is filed.

**Matching-build runtime** (`make reccmp-run` only; not run for this
change). Before this change Lego.c's calls reached `PORT-ONLY` stubs that
returned NULL (Detail_LoadMeshes) and 0 (the screen area). They now reach
the unit, whose Container calls go to the port's Container functions,
directly or through the adapters above.

## Rebased onto #677 (`aeaae5b4`)

The stack puts this change on #677 (GAME-047 Upgrade.c, finding 617), which
sits on #676 (G98-025 AnimClone.c, finding 616). The rebase had conflicts
only in the shared lists (`link_order.cmake`, `meson.build`,
`test_c_boundaries.cpp`, the findings README), each resolved by keeping both
sides. The generated files were regenerated with `make match-progress`.
Against #677's head:

| | #677 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 199,500 | **201,327** |
| Accuracy, full / game-only | 87.52% / 86.29% | 87.75% / 86.54% |
| Annotated exact functions (full) | 1,701 | 1,710 |
| `opt_ref` annotated full: removed / exact removed | 419 / 213 | 414 / 215 |
| `opt_ref` annotated game-only | 412 / 206 | 407 / 208 |
| Reccmp lint ledger (entries) | 1,073 | 1,071 |

`make reccmp-check` reports the same 14 improvements (the unit's ten
addresses below 100% and the four Lego.c callers), none below baseline and
no exact match lost. Instruction counts change only at the unit's addresses.
`detail.c.obj` again loses five functions, four of them exact, through the
same dropped Lego.c callers. Exact-removed rises by two because three of
those five are now exact, and on #677 only `0040bd60` among them was.
`make unit-data-offsets` gives the same GAME-011 row (unit summary: full 0,
gaps-only 12, partial 22, n/a 193). `make tools-test`,
`make winapi-link-check` and native ctest (37 of 37) pass.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. `tools/binary_progress_inputs.txt` was rewritten for
  the new sources (`binary_progress_hash.py --write-manifest`).
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`).
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `detail.c`.
