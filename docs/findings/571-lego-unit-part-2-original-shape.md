# Finding 571 — Lego.c (GAME-021) part 2 rebuilt in original shape

Issue #574, part 2 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
It follows [finding 568](568-lego-unit-part-1-original-shape.md) (part 1,
#571) exactly. This change appends Lego.c's second range, `0042b510`–`00432490`,
to [`src/game/lego_unit.c`](../../src/game/lego_unit.c). Part 3
(`004324d0`–`00435aa0`) stays in `lego.cpp`.

The range is 72 functions in the unit map. Ghidra merged none of them:
every scaffold size is one function. Two functions that Ghidra shows only
inside other functions' pseudocode (`004311d0`, `004313b0`) are separate
functions in the binary, and six functions had no definition in the port
(`0042b850`, `0042de40`, `0042f0c0`, `004311d0`, `004314c0`, `00431530`).
All 72 are written and annotated.

This change is stacked on #581 (GAME-023 part 1, `4e536b53`, which includes
main at `1d31a556`). "Before" is that head's *match baseline*. "After" is
this change. Scores come from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 72 functions, 28,041 bytes) | 21.49% | **88.78%** |
| Exact functions in the range | 17 | **34 of 72** |
| Mapped functions in the range | 66 | 72 |
| Exact functions, whole binary | 1,078 | 1,095 |
| Accuracy, full / game-only (`binary-progress.json`) | 68.50% / 65.26% | **69.83% / 66.74%** |
| Exact-function code bytes (share of `.text`) | 93,923 (14.62%) | **98,353 (15.31%)** |

Byte weights are the original's code bytes. An unmapped function counts as
0%. Single-function scores (`make match-function`) are higher for about 15
functions whose only difference is an operand name the full link resolves
differently (offset reads, adapters, unannotated port models).

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0042b510` | `lego_map_load_pipeline` | 27.40 | 97.84 |
| `0042b850` | `lego_surface_text_character_map_initialize` | — | **100.00** |
| `0042bb60` | `lego_texture_set_config_load` | 6.67 | 98.08 |
| `0042bd20` | `lego_load_predug_layer` | 9.74 | **100.00** |
| `0042bf40` | `lego_load_emerge_layer` | 33.66 | 88.79 |
| `0042c060` | `lego_load_layer_m` | 34.36 | **100.00** |
| `0042c120` | `lego_load_layer_l` | 25.34 | **100.00** |
| `0042c330` | `lego_emerge_map_trigger_spawn` | 27.95 | 99.03 |
| `0042c440` | `lego_level_record_timer_countdown` | 26.23 | 96.15 |
| `0042c480` | `lego_load_erode_cryore` | 36.55 | **100.00** |
| `0042c5b0` | `lego_terrain_cell_byte_state_classify_current` | 57.93 | **100.00** |
| `0042c6a0` | `lego_load_path_ai` | 31.33 | **100.00** |
| `0042c760` | `lego_load_fallin` | 20.13 | 99.45 |
| `0042c9d0` | `lego_load_block_pointers` | 26.47 | **100.00** |
| `0042cb70` | `lego_camera_grid_expiry` | 25.24 | 96.67 |
| `0042cc90` | `lego_load_terrain_map` | 31.06 | **100.00** |
| `0042cd50` | `lego_upgrade_name_entry_find` | 48.28 | 94.12 |
| `0042cda0` | `lego_entity_vehicle_type_table_load` | 24.40 | 92.78 |
| `0042cf50` | `lego_entity_minifigure_type_table_load` | 24.19 | 92.83 |
| `0042d100` | `lego_entity_rockmonster_type_table_load` | 23.60 | 92.83 |
| `0042d2b0` | `lego_entity_building_types_load` | 23.81 | 93.47 |
| `0042d460` | `lego_upgrade_types_load` | 18.68 | 93.14 |
| `0042d600` | `lego_object_names_load` | 5.97 | 86.38 |
| `0042da20` | `lego_object_the_names_load` | 5.97 | 86.38 |
| `0042de40` | `lego_viewport_target_position_clamp` | — | 91.50 |
| `0042dfc0` | `lego_object_pointer_registry_clear_match` | 22.22 | 82.35 |
| `0042dff0` | `lego_object_pointer_registry_get_slot` | 30.30 | 84.62 |
| `0042e020` | `lego_object_list_load_file` | 13.96 | 68.30 |
| `0042e8b0` | `lego_entity_type_pair_lookup` | 25.43 | 82.43 |
| `0042ed70` | `lego_entity_at` | 10.71 | **100.00** |
| `0042ef40` | `lego_stats_level_record_count_for_stat_type` | 86.79 | 90.57 |
| `0042efc0` | `lego_play_movie` | 6.80 | 95.00 |
| `0042f0c0` | `lego_level_free` | — | 82.52 |
| `0042f2e0` | `lego_terrain_cell_edge_flag_set_and_update` | 45.61 | **100.00** |
| `0042f350` | `lego_terrain_neighbor_coordinate_check` | 16.30 | 96.26 |
| `0042f6f0` | `lego_cell_init_pass` | 9.05 | 77.84 |
| `004302b0` | `map_cell_tracking_flags_clear_and_refresh_current` | **100.00** | **100.00** |
| `00430320` | `terrain_cell_work_flag_clear_current` | **100.00** | **100.00** |
| `00430470` | `lego_terrain_neighbor_flag_set` | 17.54 | 96.88 |
| `00430530` | `lego_terrain_cell_mutate` | 12.41 | 95.31 |
| `00430df0` | `lego_terrain_neighbor_refresh` | 18.48 | 90.17 |
| `00430ee0` | `lego_terrain_cell_initialize` | 11.27 | 52.37 |
| `004310f0` | `lego_terrain_cell_flag20_clear_and_refresh` | 32.84 | **100.00** |
| `00431140` | `lego_terrain_cell_queued_work_transition` | 40.43 | **100.00** |
| `004311d0` | `lego_block_activity_create` | — | 67.15 |
| `004313b0` | `lego_block_activity_update_all` | 31.37 | 95.00 |
| `00431450` | `lego_terrain_neighbor_flag_refresh` | 9.52 | 83.78 |
| `004314c0` | `lego_block_activity_remove` | — | **100.00** |
| `00431530` | `lego_block_activity_remove_all` | — | **100.00** |
| `00431560` | `level_current_owner_get` | **100.00** | **100.00** |
| `00431570` | `level_current_terrain_get` | **100.00** | **100.00** |
| `00431580` | `lego_terrain_neighbor_work_flag_scan` | 15.38 | 96.20 |
| `00431780` | `lego_terrain_stamp_impact_handle` | 11.76 | 68.90 |
| `004319b0` | `lego_terrain_cell_material_get` | **100.00** | **100.00** |
| `004319e0` | `level_map_cell_get` | 93.88 | **100.00** |
| `00431a30` | `selected_entities_any_predicate_match` | 63.72 | **100.00** |
| `00431ab0` | `lego_any_object_can_occupy_terrain_cell` | 50.77 | 98.08 |
| `00431b20` | `lego_terrain_placement_blocked_adjacent_cell_scan` | 11.11 | 88.12 |
| `00431c70` | `lego_first_free_neighbor_cell_pick` | 18.29 | 70.94 |
| `00431da0` | `lego_terrain_cell_route_and_occupancy_check` | 44.19 | 99.69 |
| `00432100` | `lego_terrain_cell_flag100_set_and_track` | **100.00** | **100.00** |
| `00432170` | `lego_terrain_cell_secondary_flag_100` | **100.00** | **100.00** |
| `004321a0` | `lego_terrain_cell_update_queue_flush` | **100.00** | **100.00** |
| `00432200` | `lego_terrain_cell_previous_state_track` | **100.00** | **100.00** |
| `00432270` | `lego_terrain_cell_visited_flag_set_and_track` | **100.00** | **100.00** |
| `004322d0` | `lego_terrain_cell_visited_flag` | **100.00** | **100.00** |
| `00432300` | `terrain_cell_visited_flags_clear_all_current` | **100.00** | **100.00** |
| `00432360` | `lego_terrain_cell_primary_flags_mask_clear_and_refresh` | **100.00** | **100.00** |
| `004323c0` | `lego_terrain_cell_secondary_bit0_clear` | **100.00** | **100.00** |
| `004323f0` | `lego_terrain_cell_flags_set_and_refresh` | **100.00** | **100.00** |
| `00432450` | `lego_terrain_cell_flags_set_mask100400` | **100.00** | **100.00** |
| `00432490` | `lego_terrain_cell_flags_set_mask20108000` | **100.00** | **100.00** |

## How the work was split

Step 0 ran as part 1 did: `tools/convert_unit.py GAME-021 --stem lego_p2
--scores baseline` in a scratch export with the unit's two link-order
entries removed (the tool refuses a unit already listed). Its stubs and
pseudocode for this range were cut into seven groups by address:

| Group | Range | Functions | Done by |
| --- | --- | ---: | --- |
| 2A | `0042b510`–`0042c440` | 9 | agent |
| 2B | `0042c480`–`0042d460` | 13 | agent |
| 2C | `0042d600`–`0042dff0` | 5 | parent |
| 2D | `0042e020`–`0042ef40` | 4 | agent |
| 2E | `0042efc0`–`0042f6f0` | 5 | agent |
| 2F | `004302b0`–`00431570` | 15 | agent |
| 2G | `00431580`–`00432490` | 21 | agent |

A first commit put a `begin`/`end` marker pair per group in each shared
file: `lego_unit.c` (state and functions), `lego_unit.h`,
`lego_unit_imports.h` and the adapter area at the end of `lego.cpp`. Each
group wrote only between its markers, so git merged the sections without
conflicts. The few conflicts came from adjacent deletions of part-1 adapters
and from two groups giving the same declaration C linkage.

Before the split, inert declarations were added to measure how sensitive
the existing code is to the new declarations (finding 538, rule B):

- 13, 40, 97 and 251 at the top of `lego_unit.c` changed no part-1 score;
- 13, 40 and 97 before the part-2 block of `lego.cpp` changed no part-3
  score.

So part 2 adds its declarations to the existing headers rather than
mid-file.

### Reconciliation after the merge

- **Guessed names.** Groups declared each other's functions under guessed
  names (2A: `lego_load_*_layers`, `lego_map_file_cell_get`,
  `lego_terrain_cell_mutation`; 2E: `..._records_release_all`). The calls
  now use the real names and the guesses are gone.
- **Adapters that became dead** were deleted with their declarations:
  `map_pipeline_original`, `camera_grid_timers_original`,
  `camera_grid_expiry_original`, `upgrade_types_load_original`,
  `simtick_minion_states_original`, `object_list_load_file_original`,
  `entity_type_pair_for_name_original`, `lego_play_movie_adapter`,
  `lego_level_names_free_adapter`, `viewport_target_position_clamp_original`,
  `lego_map_get_block_adapter` and
  `lego_terrain_activity_build_record_update_adapter`.
- **Duplicates.** The groups added 27 declarations that were already in
  `lego_unit_imports.h`, plus redundant `struct` forward declarations and
  two views of the record-object registry (`005563b8`). One copy of each
  remains.
- **Unused declarations.** Eight declarations of port functions the unit
  no longer calls (the four type-table loaders,
  `terrain_cell_initialize`, `terrain_cell_mutate`,
  `terrain_cell_route_and_occupancy_check_current`,
  `terrain_neighbor_flag_set`) were removed.
- **C4761.** `0042b510` passes its `int` to 2B's `char` modifier of
  `0042c6a0`; the explicit `(char)` cast keeps the matching build's
  diagnostics clean with no score change.

### Rule B after the merge

Five baseline-exact functions at the end of the range (`00432170`,
`004322d0`, `004323c0`, `00432360`, `004323f0`) lost their match in the
first merged layout. The listings differed only in register choice and the
order of `imul` operands. A sweep of 0–40 inert declarations at the top of
the file (not kept) showed a period of 32, with a window where all five,
`004314c0`, `0042c120` and `0042c330` match at once. Redeclarations count:
13 copies of an existing prototype moved the phase like 13 new names.
`struct` forward redeclarations do not count.

Three real cleanups were measured alone and together:

| Cleanup | Exact in the unit |
| --- | ---: |
| none (part-2 duplicates removed) | 80 |
| remove the 29 duplicate prototypes in `lego_unit.h` | 76 |
| merge the duplicated point typedefs | 76 |
| remove the eight unused declarations | **81** |
| two or three of them together | 74–81 |

The change keeps the third. All baseline-exact functions match. The cost is
2F's new `00431450` (Lego_UpdateBlockActivity-like neighbour refresh): it
matches exactly only in phases that break the five, so it stays at 83.78%
(9.52% before). No source shape changed that (three tried). Removing part
1's own duplicates was also tried: it breaks the C++ compile (some copies
differ by `#if` or type) and costs `0041fb50` 28 points, so it is not done.

## The unit

### State

Part 2 adds no `legoGlobs` model. New stand-ins follow part 1's rule (one
annotated global per field the port does not model):

- `legoGlobs_f_556514` (2E; `interface_port.cpp` keeps an unannotated copy,
  `lego_object_request_count`);
- `legoGlobs_f_556f18` (2B, "fall-ins loaded"; the port keeps
  `g_camera_grid_enabled`);
- `s_object_f_4eabc4`, `s_object_f_4eabc8` (2E; LegoObject.c's `.bss`,
  `ai.cpp`'s unannotated `s_lego_object_tool_tip_gate` covers `004eabc8`).

Fields the port defines with C++ linkage in other files are read at their
offsets from `legoGlobs_config`, as part 1 reads the object-name tables:

- the "the" object-name tables (`005561b0`, `00556268`–`00556278`);
- the record-object registry and its count (`005563b8`, `005563e0`);
- `s_catalog_counts` (`0055627c`).

Only the operand names differ in the linked build. The block-update lists
(`00556528`–`00556e90`, `level.cpp`) get C linkage through
pre-declarations before the frozen `level_for_lego.hpp` in `lego.cpp` and
`level.cpp`. New types: `LegoPoint2I` (2A's, `LegoPoint2I2F`/`2I2G` are the
same shape), the emerge trigger record (`LegoEmergeTrigger2A`, 0x4c bytes)
and `LegoBlockActivity2F` (Lego_Level +0x88).

### Shared functions

These keep the port's name:

- `level_map_cell_get` (`004319e0`, now returns `uint32_t`: the original
  zero-extends and its callers use all of `eax`);
- `selected_entities_any_predicate_match` (`00431a30`).

Four more are shared only under MSVC x86, in the pattern 2F introduced: the
unit defines them under `#if defined(_MSC_VER) && defined(_M_IX86)` and
`lego.cpp` keeps the port model under the opposite `#if`, because on 64-bit
the level's pointer words are not pointers. They are `004302b0`,
`00430320`, `00431560`, `00431570` and `00432300`. `00432300`'s port model
was already a naked copy of the same code on the same globals under MSVC.

Every other function keeps its port model unannotated in `lego.cpp`, marked
`// PORT-ONLY: port model of 0x…, retires to the tail in GAME-021 part 3`.

### Naked bodies

- `00430320` (2F): the original computes the block address once for its
  first two tests and again, re-reading width and blocks, for the third.
  MSVC 5 merges the three tests in every C form tried (an `&&` chain,
  if-return, nested ifs, a block pointer, inline helpers taking the level,
  the coordinates or the pair, both operand orders, value copies); the best
  is 74.42%.
- `00432200` (2G): the original writes the previous-frame list at +0x320
  from the current one, because both are fields of one `legoGlobs`. A
  single-struct model of the block lists reproduces the loop; the port keeps
  them as separate `level.cpp` globals, so the naked body stays until they
  become one stand-in.

Both keep a C body under `#else`.

### Shape notes

- `0042b850`: exact only with the local arrays in their original order.
- `0042b510`: `result` is assigned from each test; nested ifs or early
  returns lose the shared failure epilogue.
- `0042bd20`: the offsets array is in an inner block after the wall call,
  with the ninth `(0, 0)` entry implicit.
- `0042c120`: `value--` comes before `% 6` and `/ 6`.
- `0042c5b0`: `if (cryOre-- == 0) return 0;` then `counts[cryOre / 4]`.
- `0042c760`: contains `00430320` inlined; it needs a local copy of
  `currLevel`, or MSVC reuses the block address across the third test.
- `0042cb70`: typed `f_10[]` access (char-pointer arithmetic gives
  `fcompp`), and the stores in the order `y` then `x`.
- `0042cc90` needs an `int` temporary; `0042c6a0`'s modifier is `char`.
- Type-table loaders: names are copied with
  `memcpy(dst, src, strlen(src) + 1)`.
- `0042de40`: `bx`/`by` are a two-int array; as scalars MSVC puts them in
  dead parameter slots.
- `0042e020`: the block coordinates are `unsigned` (the original converts
  them with `fild qword`).
- `0042ef40`: no default return; the case-2 body comes before case 1 in the
  source to match the jump table.
- `0042f6f0`: unsigned `bx`/`by`, unsigned-char direction; the frame size
  needs block scopes for `blockPos`, `pos` and the heights.
- The 2-wall and 1/3-wall cases of `0042f6f0` need two identical switch
  blocks: the original duplicates the jump tables, and the first switch in
  the 1/3 case is unreachable.
- `0042f350` and `0042f6f0` read uninitialised byte locals, as the original
  does.
- `00431c70`: the original inlines a 0/1 helper. A `static __inline`
  helper reproduces it (70.94%), but it is a new unannotated definition
  for the lint. The comma-expression form in the caller gives the same
  70.94% and is used.
- `00430530`: the 8-direction table is in an inner block, so it shares a
  stack slot with the 4-corner table.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00430ee0` | 52.37 | register allocation (level in edi, the original uses esi); stable over a 1–32 counter sweep |
| `0042e020` | 67.70 | one extra stack slot (0x9cc against 0x9c8); bx/by in ebp/ebx; two pooled operand names |
| `004311d0` | 67.15 | the original pushes the direction floats through the FPU; rule-B sensitive (67–71) |
| `00431780` | 68.90 | stack slot of `count`, register rotation around the Map3D calls |
| `00431c70` | 70.94 | the inlined 0/1 result spills to memory instead of staying in `eax` |
| `0042f6f0` | 76.86 | `flags1 & 0x8000` folds into `& 0x8400`; `fcompp` against `fcomp st(1)` + `fstp st(0)`; registers |
| `0042f0c0` | 82.52 | the original keeps flag-clear masks unfolded (`and 0xffdfffff` then `and 0xf7ffffff`) |
| `00431450` | 83.78 | rule-B phase only (see above) |
| `0042bf40` | 88.79 | `and edx,1; test dl,dl` against `cmp dl,1`; about 20 shapes fold to `test al,1` |
| `00431b20` | 88.12 | bx/by register swap, not phase-dependent |
| `00430df0` | 90.17 | order of the hoisted loads; two adapter calls |
| `0042de40` | 91.50 | the original loads `blockPos` inside each arm; MSVC hoists it (permuter: no candidates) |
| `0042cda0`–`0042d460` | 92.78–96.91 | `free(data)` in ecx not eax (not phase-dependent; 1–384 sweep); Vehicle_Load/Creature_Load adapter operands |
| the rest at 94–99.7 | | operand names: offset reads, adapters, the pooled `""` (`004b8a14`) and 0.0f (`0049e2b8`) |

## Callers

### Switched

- **Inside the unit (part 1):**
  - `0041fb50` calls the five type-table loaders;
  - `004232e0` calls `0042c440`, `0042cb70` and `004313b0`;
  - `004288e0` calls `00430470`, `00430530` and `00430ee0`;
  - `00424d00` calls `0042f0c0`;
  - `00426520` calls `00431da0` (six sites);
  - `00429890` calls `0042b510`, `0042e020`, `0042e8b0` and `0042efc0`;
  - `004295f0` calls `0042de40`.
- **Other units:**
  - `terrain_cell_objects.c` and `lego_blocks.h` (GAME-013) call
    `lego_terrain_cell_secondary_flag_100` (`00432170`);
  - `interface_unit.c` and `interface_callees.h` (GAME-020) were switched
    one for one.
- **Matching build only** (`#if defined(_MSC_VER) && defined(_M_IX86)`; one
  declaration either way, so the callers' declaration count does not
  change):
  - `reward.c`'s boundary headers name the unit's `0042e8b0`
    (`entity_api.h`) and `0042c5b0` (`level.h`) through a macro;
  - the two part-3 naked bodies in `lego.cpp` (`00432560`, `00432600`)
    call `lego_cell_init_pass`, whose four integer arguments they already
    push.
  - `legoobject_unit_imports.h` (GAME-023, #581) names the unit's
    `0042e8b0`, `0042c5b0` and `00431da0` the same way.

### Not switched

The port callers below use the port's contract (`LevelState*`,
`EngineContext`, hook tables, NULL guards pinned by tests, a float `x`).
They keep calling the port models in `lego.cpp`, natively and in the
matching build:

- `map_pipeline`, `camera_grid_timers`, `terrain_texture_set_config_load_level`
  and the layer loaders: the port's level load, `camera.cpp`,
  `test_camera.cpp`;
- `cell_init_pass`: `lava_erosion.cpp`, `placement.cpp`, other port models;
- `object_list_load_file`, `entity_at`: native object-list loading;
- `entity_type_pair_lookup`, `terrain_cell_byte_state_classify_current`:
  `reward.c` natively;
- the 2F/2G terrain-cell models: `ai.cpp`, `legoobject.cpp`,
  `placement.cpp`, `select.cpp`, `terrain_grid.cpp`, `message_unit.cpp`,
  `level.cpp`;
- `lego_goto_info` (`engine.cpp`), `info_unit.c`'s entry for `0042de40`,
  stays empty natively;
- `0042de40`'s unit function reads stand-ins.

## Callees

- **`extern "C"` in place** (guarded with `#ifdef __cplusplus` in headers C
  units include) in:
  - `gods98/view.hpp`, `engine.hpp`, `resource.hpp`, `record.hpp`,
    `heap.hpp`, `message.hpp`;
  - `game/entity.hpp`, `cmd.hpp`, `ai.hpp`, `level.hpp`, `spawn.hpp`,
    `frontend.hpp`, `crt.hpp`, `type_config_ports.hpp`,
    `tutorial_hud_ports.hpp`, `port_loop_ports.hpp`;
  - the definition of `ledger_stub_0040bdc0_SetTimeoutTo10000` in
    `rock_fall_styles.cpp`.

  No frozen `*_for_*`/`*_from_*` header was edited.
- **Adapters** at the end of `lego.cpp`, each `// PORT-ONLY:`:
  - Vehicle_Load (`0046c6f0`, minimal, no port entry), Creature_Load
    (`00406990`);
  - `00484c90` (empty, two forms), Movie_GetSize/Movie_Update/`0047cc20`;
  - `00437c20`, `00409a90` (returns NULL; no port entry with the original's
    return), `00418830`, `0043f100`;
  - `00439370`, `004394b0`, `004424f0` (returns a `BOOL`), the placement
    check, `00435aa0`, `00456c60` (empty: the port's entry takes the pool
    table).

## Entropy and drops

Against #581's baseline (`4e536b53`), `make reccmp-check` reports 56
improved and 6 newly mapped. **No exact match is lost** and no instruction
count changed anywhere. The drops are below; all are labelled.

| Address | Function | File | Score | Label | Instructions |
| --- | --- | --- | --- | --- | --- |
| `0x0040eab0` | `lava_erosion_pending_cell_tick` | `game/lava_erosion.cpp` | 40.59 → 40.17 | drift: calls the unannotated port model `cell_init_pass` | +0 |
| `0x0040f330` | `lava_erosion_neighbor_mutate` | `game/lava_erosion.cpp` | 20.66 → 20.19 | drift: same | +0 |
| `0x0041fb50` | `lego_game_config_init` | `game/lego_unit.c` | 93.77 → 93.74 | source changed (calls the unit's type loaders); flagged for review | +0 |
| `0x00437540` | `legoobject_remove` | `game/legoobject_unit.c` (#581) | 80.11 → 79.58 | drift: register order only; its translation unit compiles the same with and without this change's header edit, so the move comes from the link | +0 |
| `0x0043b270` | `simtick_scene_pass` | `game/legoobject.cpp` | 18.31 → 15.49 | drift: calls the unannotated port models of `00432200` and `004321a0` | +0 |

Seven exact callers fell during the work and were restored before the
baseline was refreshed, all by call operand only:

- `0x00409450` (`placement.cpp`): `00432300` is shared under MSVC x86;
- `0x00432560` and `0x00432600` (`lego.cpp`, part 3): they call
  `lego_cell_init_pass`;
- `0x00460910` (`reward.c`): its boundary header names the unit function;
- `0x00438f60` and `0x00439280` (`legoobject_unit.c`, #581):
  `legoobject_unit_imports.h` names the unit's `0042e8b0`, `0042c5b0` and
  `00431da0` in the matching build, one declaration either way. This also
  restores `0x00439d50` (98.72).

`0x00460360` (`reward.c`) recovered the same way.

Improved outside the range:

- part 1: `0x004232e0` 91.52 → 95.88, `0x00424d00` 89.71 → 90.07,
  `0x004295f0` 96.75 → 97.56, `0x00422850` 89.52 → 90.00, `0x00429890`
  90.50 → 90.73;
- `0x00437d00` (`legoobject_unit.c`) 98.80 → 99.04;
- `0x004520d0` 21.67 → 21.98.

Part 3 in `lego.cpp` (`004324d0` and later) is unchanged except the two
call fixes above.

## Reccmp lint ratchet

The ledger goes from 1,475 to 1,473 entries (against #581), updated
without `--allow-growth`:

- `definition level_current_owner_get`: now shared under MSVC x86;
- `definition cell_init_pass`: its `PORT-ONLY` marker was moved directly
  above the definition (a comment had separated them).

The port models and adapters carry `// PORT-ONLY:` and add no keys.

## Native gaps (`port` label)

Filed as #582 (`port` label) rather than fixed here (the native port is
deferred, #552):

- Vehicle_Load (`0046c6f0`) and building creation with the original's
  return (`00409a90`) have no port entry; the unit's adapters create
  nothing.
- Movie_Update and `0047cc20` adapters are minimal; `00456c60`'s adapter is
  empty.
- Block activities (Lego_Level +0x88), the Lego_Block +0x18 word and every
  level pointer word are read only on 32-bit Windows.
- The port model of `00431da0` checks terrain types 6 and 7 on the block
  itself; the original checks the left neighbour and the block above.

## Follow-ups

- **`00431450`** reaches 100% at another counter phase; part 3's
  declarations will move it again.
- **One block-list stand-in.** Modelling `00556528`–`00556e90` as one
  struct would let `00432200` drop its naked body.
- **Sharing more terrain functions natively** needs `LEGO_LEVEL_PTR` to
  read `LevelState::ptr_voxels` for the blocks field on 64-bit.
- **`info_send` against `message_dispatch`.** Part 2 calls `info_send`
  (GAME-019's annotated `00419b80`); part 1 still calls `message_dispatch`.
- **Part 1's duplicate declarations** (about 65 in `lego_unit_imports.h`,
  29 in `lego_unit.h`) remain; removing them moves the counter phase of
  every function and needs its own measurement.
- **Part 3** retires `lego.cpp` to the tail (rename to `lego_port.cpp`).
  The legacy entry's key is now `004330e0`, the lower median of its
  remaining markers.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): passes after the
  refresh; the drops above are the full list against #581's `4e536b53`.
- `make winapi-link-check`: OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets`, GAME-021 row:
  - `.data`: partial (268, 12 unknown);
  - `.rdata`: n/a;
  - `.bss`: partial (5, 3 unknown), gap from `004de430`;
  - commons: the stand-ins.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` passes, including the lint and `link_order.py check`.
- `lego_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` and `g++ -std=c++17 -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass.

## Reproducing

    python3 tools/convert_unit.py GAME-021 --stem lego_p2 --scores baseline   # in a copy without the two GAME-021 link-order entries
    make match-function SOURCE=src/game/lego_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
