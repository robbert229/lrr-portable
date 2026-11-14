# Finding 655 — Below-80% rewrites in FrontEnd.c, Interface.c, Weapons.c and the terrain map

Issue #761, part of #388 and ADR 0005. This change rewrites the largest
functions below 80% in four converted units to the original's shape:

- FrontEnd.c: GAME-017, `src/game/frontend_unit.c`;
- Interface.c: GAME-020, `src/game/interface_unit.c`;
- Weapons.c: GAME-050, `src/game/weapons.c`;
- the terrain map: GAME-026, `src/game/terrain_map.c`.

Each function was compared with
[the full pseudocode](../decompiles/full/LEGORR.full.c) and the original's
disassembly, looking for a structural cause (wrong locals, folded
constants, a missing inline, a parameter reused as a variable). Measured on
`main` `1e84f9da`, with one full `make reccmp-check` before the change and
one after.

| Measure | `main` `1e84f9da` | This change |
| --- | ---: | ---: |
| Functions that rose / fell | | 5 / **0** |
| New exact functions | | 0 |
| Exact-function code bytes | 329,171 (51.23%) | 329,171 (51.23%) |
| Reccmp accuracy, full / game-only | 96.98 / 96.64 | **97.00 / 96.66** |
| Exact / mapped (full) | 2,397 / 2,833 | 2,397 / 2,833 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| `opt_ref` annotated removed | 0 | 0 |

**Rebased onto `main` `c42dc2cf`** (#763, #764), the same five functions
rise by the same amounts, and no other function moves. `00471630` stays at
its baseline, 97.93.

| Measure | `main` `c42dc2cf` | This change on `c42dc2cf` |
| --- | ---: | ---: |
| Exact-function code bytes | 330,196 (51.39%) | 330,196 (51.39%) |
| Reccmp accuracy, full / game-only | 97.00 / 96.65 | **97.01 / 96.67** |
| `opt_ref` annotated removed | 0 | 0 |
| Image end | `0x7f5000` | `0x7f5000` |

No exact match was lost, and no function fell below the baseline. `make
match-progress`, `make unit-data-offsets`, `make opt-ref-callers
OPT_REF_ARGS=--summary` and `make pe-audit` were run, and the generated
files are committed. `unit-data-offsets` changed no tracked file.

## Per-function results

Scores are from the full report. Each "Cause" entry says what was
structurally wrong and what fixed it.

| Address | Function | Before | After | Cause |
| --- | --- | ---: | ---: | --- |
| `00470760` | weapons_mesh_build_segment | 25.59 | **54.67** | Ghidra's folded constants, below |
| `0044ead0` | terrain_map_shape_flatten | 35.74 | **41.81** | `tolerance *= 0.5f` reused the parameter's slot; the original keeps the half tolerance in its own local (its frame is 4 bytes larger, `fstp [esp+0x18]`) |
| `00450b60` | terrain_map_uv_wobble_update | 32.43 | **33.62** | the original copies the block's `uvWobble` into a local `Point2F` (integer moves), writes the updated local back as a struct, and keeps the vertex UV and the target in `Point2F` locals with `cur = target` when the distance is over 0.36 |
| `0046f920` | weapons_projectile_update | 50.18 | **51.28** | the straight branch adds `direction * dist` component by component; `weapons_ray_end_point`'s scaled local gave a frame 4 bytes too large (`0x18`, original `0x14`). The distance is a local, not the reused `elapsed` |
| `0041baa0` | interface_do_mouse | 75.84 | **76.38** | wrong callee: `lego_get_resource` (a port adapter) for Lego.c's `lego_upgrade_resource_check` (`0041f900`), now aliased in the matching build |

### `weapons_mesh_build_segment` (`00470760`)

The old body was written from Ghidra's pseudocode, which constant-folds what
the original computes at run time. The original:

1. normalises the whole direction. The old body scaled only `y` and `z`,
   then multiplied `len * dir.x` back in by hand;
2. builds a perpendicular vector `perp` whose `x` is a stored `0.0f`
   (`mov [esp+0x24], 0`), not a literal;
3. computes `sqrt(2.0)` with `fld qword; fsqrt` into a float local, and
   the octagon factor tan(22.5°) as `root2 / ((1 + root2 * 0.5) * 2)`. The
   original's code is `fmul -0.5f; fsubr 1.0f; fadd st0, st0; fdivr`, and
   MSVC keeps one copy of that factor. The old body had the folded literal
   `0.41421354f`;
4. takes the cross product of `perp` and `dir` through a temporary and
   stores it back into `dir`, as the gods98 inline does;
5. scales `dir` and `perp` by the radius, then forms the eight vertices as
   `perp ± k·dir` and `dir ± k·perp`;
6. copies each vertex to its normal field by field, before normalising it.
   A struct copy gives another register choice (40.53 → 54.33 in the
   single-function loop).

A local holding the factor gives another frame (38.82 against 40.53). The
expression is therefore written through the file-local macro
`WEAPONS_OCTAGON_K`. This is not a deviation: after expansion, the tokens
are the original's expression.

Adding the cross product to `weapons_maths.h` as a `static __inline` helper
made `weapons_ray_block_sweep` (`00471630`) fall from 97.93 to 97.41. That
is rule B: the helper's declarations come before it and shift the symbol
counter (finding 538). Locals of the function itself did not move it.
Writing the cross product out in the function, with its temporary, keeps
the header unchanged and leaves `00471630` at its baseline.

The rest of the gap is in the vertex loop. The original strength-reduces
it to a byte offset (`edx` up to `0x60`) and pointer increments, while
ours indexes by `i`. Our 0x22 extra bytes are the loop's address
arithmetic. The permuter (600 s) found 2 candidates, neither better.

## Tried without gain

So that a later sweep need not repeat it:

- **`interface_do_mouse` `0041baa0`.** The rest is register roles (the
  original has two zero registers, `ebx` and `ebp`) and stack slots. The
  permuter (600 s, 15 attempts) found nothing better. The single-function
  loop shows other callee names (`timed_effect_update_request_set_corpus`
  and others), but the full report resolves them through the link-name
  headers. Only `lego_get_resource` was a genuinely wrong callee.
  `objectGlobs_toolTipIconOre` (`004eadc0`) is a `static` in
  `legoobject_unit.c`, so Interface.c still reads the port's
  `lego_tooltip_ore_icon` there. Exposing it would change its linkage.
- **`terrain_map_uv_wobble_update` `00450b60`.** The original's inner loop
  walks `uvOffsets` and `blockOffsets` through pointers and keeps a separate
  byte offset. Ours indexes by `j*8`. The permuter (420 s, 11 attempts)
  found nothing better.
- **`terrain_map_shape_flatten` `0044ead0`.** After the local, the frame
  matches. What remains is register roles (`count` in `edx`, `map` in
  `esi`). The permuter (300 s, 6 attempts) found nothing better.
- **`terrain_map_create` `0044e0c0`.** The original keeps the loaded file
  pointer in a local and reuses the `filename`/`blockSize` parameter slots
  for the `y`/`x` loop variables. Ours puts the file pointer in
  `filename`'s slot. Not reached.
- **`terrain_map_block_first_vertex_get` `00450060`.** The original
  zero-extends the block's flags byte into `ebx` before masking. A
  `(unsigned int)` cast, `? 1 : 0` and an `(int)` cast all give our
  `mov al; and eax, 1`.
- **`front_menu_item_draw_slider` `00411f10`.** Ours holds 0 in `edi` and
  reconverts `(float)y` in the minus-button block. The original pushes
  immediates and reuses the converted `y` from the parameter slot.
  Reordering the block's two stores, dropping the `(int)` cast and dropping
  `!= NULL` changed nothing or scored lower.
- **`weapons_fire_lazer` `0046ffb0`.** The original keeps `hitObject` in
  `fromPos`'s parameter slot, and its frame is `0x24` against our `0x28`.
  These were tried, with scores of 53.18–54.21:
  - other declaration orders;
  - one function-scope `blockPos`;
  - `weapons_ray_end_point`.
- **`position_tracker_box_draw` `0041cd30`.** The original computes all four
  edges first and copies them through two temporaries. `+ 1.0f` forms, the
  declaration order and a block temporary did not reproduce it.
- **`interface_callback_draw_item` `0041b800`.** The original reads `data`
  late, and ours keeps `pos` in `ebx` from the top. Using `(Point2F*)data`
  directly, or assigning `pos` late, scores the same (48.72).
- **`front_menu_update` `00412c00`, `weapons_model_aim` `00471a60`.** The
  gap is register roles throughout. The structure already matches. Not
  rewritten.
- **The `fld`/`push ecx`/`fstp [esp]` float argument copies** in
  `weapons_projectile_update`'s Container calls are the pattern that findings
  589 and 615 could not reproduce. They were not tried again.
- **Interface.c `0041dca0`.** Left to #760's case-order tool, as the brief
  said.

No search was left unfinished.

## Files

- `src/game/weapons.c`: `00470760` and `0046f920` rewritten.
- `src/game/terrain_map.c`: `00450b60` and `0044ead0` rewritten.
- `src/game/interface_unit.c`: one matching-build alias,
  `lego_get_resource` → `lego_upgrade_resource_check` (`0041f900`). It
  sits in the existing alias block, and the name is declared nowhere else
  in the unit, so the symbol counter does not change.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt` and
  `binary-progress.json`: regenerated.
