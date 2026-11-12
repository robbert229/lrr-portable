# Finding 566 — Electric-fence unit (GAME-013) rebuilt in original shape, with a port tail

Issue #556, under [ADR 0005](../adr/0005-original-shape-c-units.md). The
relocated `src/game/terrain_cell_objects.cpp` (GAME-013, text
`0040cdc0`–`0040e490`) is the original's `ElectricFence.c`. It is rebuilt as
the C unit [`src/game/terrain_cell_objects.c`](../../src/game/terrain_cell_objects.c),
with the C boundary header
[`terrain_cell_objects.h`](../../src/game/terrain_cell_objects.h).

- **Fences.** A fence is a type-0xb object on one block. Each fence owns a
  0x14-byte node from a list set (32 lists, list *i* holding 2^*i* nodes,
  one free list). A live node points `nextFree` at itself.
- **The block grid.** One 0xc-byte record per map block: the fence node,
  the connection stud (a type-0xe object) and a flag word (bits 0–3: beams
  by direction; 0x100: visited this frame).
- **Each frame.** `electric_fence_update` clears every fence's and
  building's flags, counts down the fence timers, then sends a spark from
  each powered building's shape along the chains of fences
  (`electric_fence_spark`, recursive), spawning beams between neighbours.
  An object on a live block is struck with the FenceSpark weapon.

The port's own model (a calloc'd `LevelCellKindGrid` per level, explicit
level arguments, scene-object hooks pinned by the native tests) is a
different contract. As in findings 556, 560 and 561, its implementations
move **unannotated, under the port's names**, to the port tail
[`terrain_cell_objects_port.cpp`](../../src/game/terrain_cell_objects_port.cpp)
(ADR 0006 decision 4), and native callers keep calling them. The only
callers switched are the five in GAME-021 part 1's `lego_unit.c` (#571),
which is original-shaped and natively dead; every other caller is in
`lego.cpp`, `legoobject.cpp` or another unconverted file whose contract is
the port's.

This change is stacked on #571 (GAME-021 part 1, on #572, #566, #565, #559
and #558). "Before" is the *match baseline* of #571 at `d900ef8e`; the
unit's own functions have the same scores there as on `main` at
`63da6269`. "After" is this change, from the full matching build
(`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 27 functions, 5,604 bytes) | 15.74% | **91.49%** |
| Exact functions in the unit | 0 of 23 mapped | **19 of 27** |
| Mapped functions in the unit | 23 | 27 |
| Exact functions, full scope | 1,003 | 1,022 |
| Accuracy, full (`binary-progress.json`) | 65.85% | 66.58% |
| Accuracy, game-only | 62.32% | 63.13% |
| Exact-function code bytes | 84,712 | 87,662 |

Byte weights are the original's code bytes without the `0x90` fill; an
unmapped function counts as 0%. The unit range lists 24 functions; Ghidra
folded three LegoObject and list callbacks (`0040d5e0`, `0040d720`,
`0040d770`) into `0040d4f0`, and `0040d490` had no port definition. All
four are annotated and mapped now.

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `0040cdc0` | `electric_fence_initialise` | 108 | 7.89 | **100.00** |
| `0040ce30` | `electric_fence_shutdown` | 72 | 73.47 | **100.00** |
| `0040ce80` | `electric_fence_restart` | 19 | 33.33 | **100.00** |
| `0040cea0` | `electric_fence_block_surfaces_update` | 167 | 22.68 | 96.67 |
| `0040cf50` | `electric_fence_add` | 47 | 79.07 | 95.00 |
| `0040cf80` | `electric_fence_create` | 161 | 19.58 | 70.48 |
| `0040d030` | `electric_fence_add_list` | 111 | 12.96 | **100.00** |
| `0040d0a0` | `electric_fence_remove` | 88 | 30.77 | 96.77 |
| `0040d100` | `electric_fence_remove_node` | 99 | 11.11 | **100.00** |
| `0040d170` | `electric_fence_debug_place` | 119 | 26.97 | **100.00** |
| `0040d1f0` | `electric_fence_debug_remove` | 66 | 31.11 | **100.00** |
| `0040d240` | `electric_fence_can_place_at_block` | 420 | 16.39 | **100.00** |
| `0040d3f0` | `electric_fence_block_is_building` | 84 | 19.05 | **100.00** |
| `0040d450` | `electric_fence_update` | 52 | 30.77 | **100.00** |
| `0040d490` | `electric_fence_run_through_lists` | 90 | — | **100.00** |
| `0040d4f0` | `electric_fence_spark_from` | 226 | 17.48 | **100.00** |
| `0040d5e0` | `electric_fence_building_spark_callback` | 308 | — | 92.24 |
| `0040d720` | `electric_fence_building_clear_callback` | 73 | — | 95.45 |
| `0040d770` | `electric_fence_node_update_callback` | 217 | — | **100.00** |
| `0040d850` | `electric_fence_spark` | 961 | 9.58 | 59.62 |
| `0040dc20` | `electric_fence_beam_create` | 365 | 14.57 | **100.00** |
| `0040dd90` | `electric_fence_block_vector_get` | 174 | 12.66 | **100.00** |
| `0040de40` | `electric_fence_object_strike` | 632 | 24.03 | 99.51 |
| `0040e0c0` | `electric_fence_object_beam` | 282 | 16.07 | **100.00** |
| `0040e1e0` | `electric_fence_stud_fits` | 357 | 16.41 | **100.00** |
| `0040e350` | `electric_fence_stud_set` | 268 | 12.86 | **100.00** |
| `0040e460` | `electric_fence_block_is_fence` | 38 | 12.00 | **100.00** |

`0040de40` is an exact "effective" match in the single-function loop
(registers only). The header comments give the original names
(`ElectricFence_*` where known).

## The unit

### State

- **`.bss`**: `electricFenceGlobs` at `004c7df8` (0x90 bytes, `static`):
  `fenceGrid`, `level`, `listSet[32]`, `freeList` (`004c7e80`),
  `listCount` (`004c7e84`). The four port globals that carried the old
  annotations stay in the tail, unannotated.
- **Lego.c's templates**: `legoGlobs.contElectricFence` (`0055638c`) and
  `contElectricFenceStud` (`0055639c`), the objects the unit clones, are
  defined and annotated in `lego_unit.c` (finding 568). The unit declares
  them `extern void*`, as `lego_unit.h` does.
- **`.rdata`**: 0.0, 200.0 and 0.5 (`0049e114`–`0049e11c`) are literals;
  MSVC pools them.
- **String**: `"FenceSpark"` (`004a1e8c`) is used twice in the unit and by
  no other; one `STRING` marker sits above its first literal.

`make unit-data-offsets`, GAME-013 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-013 | n/a (1, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

### Callees

- **`extern "C"` added in place** (no symbol-count change):
  `transient_effect_render_slot_configure` (`0040c2f0`, `gods98/resource.hpp`),
  `task_field84_get_and_field88_out` (`00408360`, `gods98/record.hpp`),
  `rotated_pairs_scratch_fill` (`00464110`, `gods98/view.hpp`),
  `object_type_name_index_find` (`0046f0d0`, `game/weapon.hpp`, inside
  `namespace lego`), and on the definitions of
  `weapon_type_recharge_time_get_target` (`0046f110`) and
  `ledger_stub_0046f270_TypeMotionScaleApplyDispatch` (`0046f270`) in
  `weapons.cpp`. The frozen `weapon_from_weapons.hpp` declares
  `0046f110` with C++ linkage; `weapon.cpp`, its only includer and a tail
  file, declares it `extern "C"` first, so the frozen redeclaration
  inherits C linkage (finding 556's technique). The frozen header is not
  edited.
- **New C boundary headers**, included only by the unit and
  `test_c_boundaries.cpp`, one per original module so that no other C
  unit's symbol numbering changes: `game/legoobject.h` (GAME-023),
  `game/stats.h` (GAME-043), `game/weapons.h` (GAME-050),
  `game/lego_blocks.h` (GAME-021), `game/rock_fall_styles.h` (GAME-012),
  `game/building_model.h` (GAME-006) and `game/placement_footprint.h`
  (GAME-040).
- **Two adapters**, `// PORT-ONLY:` at the end of the port tail, because
  the owners (`lego.cpp`, `legoobject.cpp`) are not to be edited:
  - `level_block_surface_update` for `0042f6f0` Level_BlockUpdateSurface:
    the port's `cell_init_pass` takes the column as a `float` and has C++
    linkage;
  - `legoobject_block_position_get` for `004424f0` LegoObject_GetBlockPos:
    the port's entry also takes the level.

  The call operand to an adapter is the only difference left in
  `0040cea0`, `0040cf50`, `0040d0a0` and `0040d720`, and one of the
  differences in `0040d5e0`.
- **`lego_unit.c` callers.** `lego_unit_imports.h` declared the port names
  of `0040ce80`, `0040d170`, `0040d1f0`, `0040d4f0` and `0040e460`. The
  five declarations are renamed in place to the unit's functions, with two
  struct tags (`ElectricFenceLevel`, `ElectricFenceObject`) for the pointer
  parameters, and the four call sites in `0x004288e0` and `0x00429890`
  call them, with pointer casts. The single-function loop finds no other
  `lego_unit.c` function changed by the two tag declarations. Natively,
  `lego_unit.c` runs only in the matching build (finding 568), so native
  behaviour is unchanged. Without the switch, those two callers fell
  (96.75 to 96.15 and 90.40 to 90.36, call targets only); with it they
  keep their baseline scores.
- `test_c_boundaries.cpp` pairs the new headers with `weapon.hpp`,
  `level.hpp`, `resource.hpp`, `record.hpp` and `view.hpp`. The scaffold
  also added `entity.hpp`, which cannot share that translation unit with
  `steering.h` (an existing contract difference for
  `set_vector_and_notify_listeners`); it was removed, and the
  `legoobject.h`/`stats.h`/`lego_blocks.h` pairs with `entity.hpp` and
  `level.hpp` were checked in a separate `g++ -fsyntax-only` compile.

### Shape notes

- **Locals the original kept in the frame.** Where the original keeps two
  coordinates in its own frame, they are a `TerrainMapPoint2`/`Point2I`
  struct; as two scalars MSVC 5.0 puts them in dead parameter slots
  (`0040d170`, `0040d4f0`, `0040d720`, `0040dc20`, `0040e350`: each went
  to 100% or to the adapter-only difference). Where the original does use
  a parameter slot (`0040cf50`, `0040d0a0`, `0040e0c0`), scalars give it.
- **Direction tables.** The near and far offset tables are two arrays,
  the far one declared first: the original stores the 2s and −2s before
  the 1s and −1s. As one `[2][4]` array, `0040d240` scored 63.58% and
  `0040dc20` 34.31%; as two arrays, both are exact. `0040d850` is the
  exception: two arrays (either order) score 35–37%, the `[2][4]` form
  59.62%, so the latter is kept.
- **`0040de40`.** Explicit `x`, `y` locals for the neighbour block, and the
  distance written out as `sqrt(dx*dx + dy*dy)` over the two differences
  (MSVC spills the common `dx` subexpression, which is the 4-byte slot
  the original has), take it from 59.90% to 99.51%. The comparison is
  `0.0f == bestDist`: the original loads the constant first.
- **`0040dc20`.** The two beam calls each return; `if (found) {…}` wraps
  the body and the position is a `Vector3` whose `z` is the world height.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0040d850` | 59.62 | register and frame assignment of the eight loop locals; finding 538 rule B suspected. `tools/msvc_permuter.py` (600 s, 4 workers) found 5 candidates and no gain |
| `0040cf80` | 70.48 | the original tests `freeList` in `eax` and reloads it into `esi`; the zeroing copies the node pointer to `edx`. The permuter found no candidates; `memset(node, 0, sizeof *node)` and the field-wise forms compile alike |
| `0040d5e0` | 92.24 | the adapter call, and the block order of the duplicate-point skip; three source forms of the test compile identically |

## Entropy and drops outside the unit

`make reccmp-check` against #571 (`d900ef8e`): 23 improved, 4 newly mapped and 8
fell below the baseline. All 8 are labelled `drift` with the same
instruction count, and **no listing digest outside the unit changed**: the
only change in each is a call operand. Each calls a GAME-013 function and
now reaches the unannotated port copy in the tail, which follows from the
port-tail decision. None was exact, so **no exact match is lost**.
**Flagged for the owner:**

| Address | Function | File | Calls | Score | Instructions |
| --- | --- | --- | --- | --- | --- |
| `0x0042b510` | `map_pipeline` | `lego.cpp` | `0040ce80` | 27.74 → 27.40 | = |
| `0x0042e020` | `object_list_load_file` | `lego.cpp` | `0040cf50` | 14.13 → 13.96 | = |
| `0x0042f6f0` | `cell_init_pass` | `lego.cpp` | `0040e1e0`, `0040e350` | 9.28 → 9.05 | = |
| `0x00431b20` | `terrain_placement_blocked_adjacent_cell_scan` | `lego.cpp` | `0040e460` | 12.12 → 11.11 | = |
| `0x00437940` | `deferred_olist_entity_create_and_link` | `legoobject.cpp` | `0040cf50` | 17.87 → 17.49 | = |
| `0x0043bc40` | `objective_sidecar_append_gate` | `legoobject.cpp` | `0040d0a0` | 39.43 → 39.16 | = |
| `0x0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | `legoobject.cpp` | `0040de40` | 28.83 → 28.47 | = |
| `0x00452cc0` | `selected_entity_state_monitor` | `message_unit.cpp` | `0040d0a0` | 29.06 → 27.35 | = |

- **Symbol counts.** The `extern "C"` edits are in place; the one new
  declaration outside the unit is in `weapon.cpp` (a tail file with no
  annotated function). No GAME-006, -012, -040 or -050 listing changed.
- **Names.** C linkage drops the `lego::` prefix from the report names of
  `0046f0d0` and `0046f110`; their scores are unchanged.
- **Layout.** The callbacks at `0040d490`, `0040d5e0`, `0040d720` and
  `0040d770` are new mappings inside GAME-013's range that the
  translation-unit table (Ghidra's function list) does not list
  separately, so `binary-progress.json` counts them as unattributed.

## Reccmp lint ratchet

With #558's exemptions (link-order `tail` files and `// PORT-ONLY`
definitions), the 23 port models in the tail and the two marked adapters
add nothing. Thirteen entries for the old unit's static helpers, which
moved into the tail, are removed: the ledger goes from 1,503 to 1,490,
without `--allow-growth`.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the 8 drops above,
  23 improved, 4 newly mapped; after the refresh, `make reccmp-compare`
  reports no function below the baseline.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`; `tools/binary_progress_inputs.txt` was rewritten
  for the new files.
- `make unit-data-offsets`: the GAME-013 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` passes (lint ledger 1,490, Meson parity).
- `terrain_cell_objects.c` passes `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`, with `TEST_WADS_DIR` set: 39/39 pass, including
  `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-013 --stem terrain_cell_objects   # after moving the .cpp to the tail
    make match-function SOURCE=src/game/terrain_cell_objects.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
