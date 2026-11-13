# Finding 588 — LegoObject.c (GAME-023) part 3 rebuilt in original shape; `legoobject.cpp` retired to the port tail

Issue #615, part 3 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
It follows [finding 572](572-legoobject-unit-part-1-original-shape.md) (part 1)
and [finding 578](578-legoobject-unit-part-2-original-shape.md) (part 2), and
retires the port file as [finding 577](577-lego-unit-part-3-and-lego-port-tail.md)
did `lego.cpp`. This change appends LegoObject.c's last range,
`00445d70`–`0044c710`, to
[`src/game/legoobject_unit.c`](../../src/game/legoobject_unit.c), and renames
`legoobject.cpp`, which now holds only port code, to
[`legoobject_port.cpp`](../../src/game/legoobject_port.cpp), a `tail` entry.

"Before" is `main` at `2f988e92` (stack #622: #619, #620, #621). "After" is
this change. Scores come from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 95 functions, 26,273 bytes without `0x90` fill) | 21.74% | **89.63%** |
| Exact functions in the range | 4 | **65 of 95** |
| Mapped functions in the range | 83 (one a Ghidra fragment) | 95 |
| Exact functions, full | 1,412 | **1,476** |
| Accuracy, full / game-only (`binary-progress.json`) | 79.02% / 76.95% | **80.94% / 79.07%** |
| Exact-function code bytes | 156,894 (24.42%) | **168,566 (26.23%)** |
| `opt_ref` annotated removed, full (exact) | 571 (280) | **501 (230)** |
| `opt_ref` annotated removed, game-only (exact) | — | 484 (219) |
| `legoobject_unit.c`: annotated / removed by `/OPT:REF` (exact) | 191 / 21 (17) | 286 / 29 (22) |
| `legoobject.cpp` | 84 annotated port definitions, 25 annotated globals | retired (`legoobject_port.cpp`, tail) |
| Reccmp lint ledger | 1,312 | 1,291 |

The `opt_ref` "before" full figure is `main` `2f988e92`'s; its game-only
figure was not measured there (on `984a1213`, before #622, it was 568
(264) game-only and 585 (275) full).

The unit map lists 85 functions in the range. The real count is **95**:

- `0044702d` is not a function: it is the tail of `00446e40` (which runs to
  `004470ca`), split off by Ghidra. Its annotation in `legoobject.cpp` is gone;
  Reccmp reports it as newly unmapped.
- Ten RunThroughLists, `ptrstack_for_each` or `qsort` callbacks were merged
  by Ghidra into their neighbours: `00445df0` and `004460f0` (pushed by
  `00445d70`), `00447920` (`00447900`), `00449c20` (pushed by `00449cf0`),
  `00449cf0` (pushed by `00449c00`), `0044a7a0` (the `qsort` comparator of
  `0044a5d0`), `0044b850` (`0044bc30`, `0044bf00`), `0044bab0` (`0044bae0`),
  `0044c500` (pushed by Message.c's `004526e0`) and `0044c5f0` (`0044c550`).
  A scan for aligned starts after `0x90` fill and for pushed in-range
  addresses found the same set (`00449cf0` came from the helper that wrote
  group S).
- `0044bc30`, `0044be30` and `0044bf00` had no annotated port definition.

All 95 are written and annotated. Byte weights are the original's code
bytes without fill; an unmapped function counts as 0%.


### What is left

| Address | Score | What is left |
| --- | ---: | --- |
| `0044b500` | 41.70 | frame 0x34 against 0x38: the original keeps the route lists in the dead `obj`/`selector` parameter slots |
| `00446e40` | 45.95 | the original keeps `diagonals` on the stack and `edi` as a zero, and stores a zero at `[esp+0x10]` before the call; the three local tables' store order follows |
| `004473b0` | 60.91 | the original puts the object's block in the dead `bx`/`obj` parameter slots (frame 0x10 against 0x18); block-scoped locals and the permuter (150 s, 7 candidates) changed nothing |
| `0044b140` | 62.82 | argument evaluation order of the three random draws and the frame (0x44 against 0x3c) |
| `00447ea0` | 67.46 | 2,336 bytes: frame slot order of the boulder's vectors and x87 scheduling of the inlined Vector3D normalise |
| `00446610` | 71.66 | which of the four flag masks is spilled, and two param-slot reuses |
| `00449980` | 75.52 | `push edi` is delayed past the early return, and `eax` for the second `flags1` store |
| `00447950` | 77.46 | x87 scheduling of the inlined Vector2D normalise / dot / cross |
| `004469c0` | 81.79 | scalar frame order (two direction tables and three path arrays match) |
| `004492b0`, `00449cf0`, `0044afb0`, `00447210`, `0044bf00`, `00449ff0`, `004460f0` | 84–90 | param-slot reuse, two swapped registers, block order; `00449ff0` only adapter calls |
| the rest at 91–99.9% | | calls to PORT-ONLY adapters (below), two x87 instructions (`0044c1b0`), one reload (`0044a200`) |

`0044a390` and `0044a4b0` keep the port's naked asm (100%): their array and
count share one struct in the original, so it rereads the count after each
store, and in C MSVC keeps it in a register. Plain `[count][0]` indexing and a
`Point2I` cast of the array were tried; the C form is the `#else`.
`0044adc0`, the third naked function, is now C at 100%.

## How the conversion is laid out

- As finding 577 recommends, part 3's declarations do not go in
  `legoobject_unit.h`. The unit's `.c` file has one part-3 prototype block
  after part 2's last function (opened with its own `extern "C"` for the
  native C++ build), then for each group P–T its state and imports and its
  functions: P `00445d70`–`004473b0`, Q `004474f0`–`00447ea0`, R
  `004487c0`–`00449ac0`, S `00449c00`–`0044ae50`, T `0044af90`–`0044c5f0`.
  No part-1/2 symbol counter moves from part 3's own declarations.
- **objectGlobs.** `legoobject.cpp`'s 25 annotated LegoObject.c globals move
  to the very end of `legoobject_unit.c`, after every function, so no
  function's counter moves (finding 577): 23 of them, plus `004ea7c4` (the
  camera cycle's visited list, newly annotated). `activityName` (`004decc4`)
  and `abilityName` (`004ead60`) stay in `legoobject_port.cpp`, annotated:
  their initialisers point at the port's name strings, and `Activity_Stand`
  is `lego_port.cpp`'s C++ object, which C cannot reference. Moving the
  definitions changed no score (the unit's `match_function` scores were
  identical before and after). `make unit-data-offsets`, GAME-023 row:
  `.data` gaps-only (121, 0 unknown); `.rdata` n/a (one item); `.bss`
  partial (2, 9 unknown; was 30 unknown), gap 1 first `004eadc0`.
- **Strings**: `"TinyRM"`, `"TinyIM"`, `"SmallSpider"`, `"Expand"`, `"Long"`,
  `"Teleport"`, `"Melt"`, `"Disintegrate"`, `"Activity_"`, `"%s%s"`, and
  others the helpers marked; pooled literals other units annotate
  (`"Boulder"`, `"FlapInWind"`, `"Normal"`, `"NULL"`) stay unmarked. One
  helper marker duplicated `advisor.c`'s `004a13f4`; Reccmp rejects
  duplicate STRING addresses, so it was removed.

## Retiring `legoobject.cpp`

`git mv src/game/legoobject.cpp src/game/legoobject_port.cpp`; its
`legacy 00449ac0` entry in `link_order.cmake` becomes a `tail` entry after
`lego_port.cpp`, and `meson.build` follows. `link_order.py check` and
`check-map` pass. The 84 annotated definitions in the range became
`/* Port model of 0x… */` plus `// PORT-ONLY` (as in part 2), and the file's
port models and adapters stay unannotated. Comments elsewhere that named
`legoobject.cpp` now name `legoobject_port.cpp`.

Rule B across the move: the move changes no declaration in any unit. The
tail file declares the moved globals itself (`extern "C"`), since it used
their definitions.

#621's change to `legoobject.cpp` (it deleted #610's Vehicle_SetPosition
stand-in `legoobject_gm_vehicle_set_position_adapter` and switched the unit's
four callers to `vehicle_set_position`) is kept: the rebase onto `2f988e92`
took #621's side of `legoobject_unit_imports.h`, and the stand-in is not in
`legoobject_port.cpp`.

## Adapters

Part 3 calls 13 original-ABI `PORT-ONLY` adapters at the end of
`legoobject_port.cpp`, where the port's entry has C++ linkage, another
contract, or none. Nothing calls them natively.

| Adapter | Original callee | Why |
| --- | --- | --- |
| `legoobject_gq_vehicle_set_activity_adapter` | `0046c3f0` Vehicle_SetActivity | `upgrade_levels.cpp`'s entry has another contract |
| `legoobject_gq_upgrade_move_animation_adapter` | `0046c110` Upgrade_MoveAnimation | `tooltip.cpp`'s entry takes two arguments |
| `legoobject_gq_building_set_activity_adapter` | `00408570` Building_SetActivity | no port entry |
| `legoobject_gs_container_hidden_set_adapter`, `…_is_hidden_adapter` | `00473b20`, `00473ba0` | C++ linkage (`container_unit.cpp`) |
| `legoobject_gs_object_is_hidden_adapter` | `004086a0` | C++ linkage |
| `legoobject_gs_vehicle_camera_flip_adapter` | `0046c3d0` | C++ linkage (`upgrade_levels.cpp`) |
| `legoobject_gs_placement_footprint_check_adapter`, `…_construction_register_adapter`, `…_construction_path_shape_adapter`, `…_construction_resource_add_adapter` | `004641c0`, `00409790`, `00408ec0`, `00408e50` | other contracts |
| `legoobject_gt_flock_has_position_adapter` | `004897d0` | a one-line getter the linker folded with `flic_width`; `0044bc30` reads `SteeringGroup.hasPosition` |
| `legoobject_gt_container_matrix_set_adapter` | `004756b0` | C++ linkage (`view.hpp`) |

After rebasing onto #620 and #621 the unit calls Vehicle.c's and the effects
unit's original functions directly: `vehicle_get_trans_coef` (`0046db70`),
`vehicle_move_animation` (`0046d1c0`), `vehicle_get_camera_null` (`0046da50`),
`vehicle_is_hidden` (`0046d330`), `vehicle_get_drill_null` (`0046da90`) and
`effect_spawn_explosion` (`0040c750`); the six adapters written for them
were deleted. `0046d330` and `0046da90` are named by
`vehicle_model_link_names.h`'s link names in the matching build, which the
unit mirrors in group S's imports. Three `view.hpp` declarations gained
`extern "C"` (`animation_loop_time_adjust`, `00408860`;
`point_to_polyline_minimum_segment_distance`, `00408b00`;
`scene_activity_change_secondary`, `004085f0`): preprocessor lines only, so
no includer's count moves.

The 17 part-1/2 adapters whose callers now call the unit (`gb`, `gc`, `gh`,
`gj`, `gm`, `gn`, `go` and `legoobject_activity_tick_adapter`) are deleted.

## Callers

### Parts 1 and 2 call part 3

Every import line in `legoobject_unit_imports.h` that named a part-3 address
through a port name or adapter now declares the unit's function, one line
for one, and parts 1 and 2 call it: `00445d70`, `00446610`, `004469c0`,
`004474f0`, `004475c0`, `00447730`, `00447780`, `004477d0`, `00447900`,
`00447b30`, `00447c40` (40 calls), `00449700`, `00449980`, `00449f50`,
`0044a200` (40 calls), `0044a2a0`, `0044a4f0`, `0044ade0`, `0044b250`,
`0044b370`, `0044b480`, `0044b680`, `0044bc30`, `0044be30`, `0044bf00`,
`0044bfd0`, `0044c110`, `0044c1b0`, `0044c530`.

**Parameter names count.** Two import changes changed the number of
parameter names: `00449980` (the port's 5 to the original's 3) and
`0044a2a0` (2 to 3). The second cost `0043c200` its exact match: removing
one name restores it, adding inert declarations does not (rule B, finding
538; the experiment's dummies are not committed). `0044a2a0`'s import has
its two output parameters unnamed, tagged `Deviation (scoring only)`; it is
this change's only scoring deviation, and it changes no storage.

**A `match_function` artefact.** Calling `00447b30` (defined later in the
same file) from `0043a300` drops that function from 68.53% to 14.90% in
`match_function`, whatever the callee's body or name, while an undefined or
end-of-file callee leaves it at 68.53%. The full build scores it 70.39%,
unchanged, so this is the single-function loop's synthetic image, not MSVC.

### Other units

- `legoobject_port_aliases.h` gains 23 entries for port names in the range
  whose contract is the original's (`activity_register_bookkeeping`,
  `entity_type_component_get`, `ai_activity_scan`, `ai_can_do`,
  `global_scatter_spawn_enable_and_drain` and others), and 21 in the adapter
  block: `aitask_port.cpp`'s 18 original-ABI adapters (`00446e40`–`0044b500`),
  `lego_port.cpp`'s `simtick_render_step_original` (`00449c00`) and
  `entity_type_operation_dispatch_adapter` (`00449f50`), and `lego_unit.c`'s
  `ledger_stub_0044b140_SmallSpiderCellSwarmSpawn`. Names in namespaces
  (`lego::`, `unported_ledger_stubs::`), the overloaded
  `selection_clear_marker_and_reset_hud`, and entries with other contracts
  (`entity_flag_gated_callback_dispatch`, `entity_try_create_linked_at_position`
  with float cells, `entity_kind6_tick_down_marker_spawn` and
  `entity_activity_terrain_eligibility_transition` with a GameContext,
  `entity_work_object_position_get`) are not aliased.
- Units that do not include the alias header get matching-build-only
  macros, one declaration each: `camera_control_imports.h` (`0044a070`,
  `0044a200`; camera_control.c keeps 27 of 27 exact), `weapons_imports.h`
  (`0044a200`, `00447950`, `0044c030`, `0044c150`),
  `rock_fall_styles_imports.h` (`0044a200`, #620's file: the one edit there,
  needed to keep `0040beb0` exact) and `ai.h` (`0044c550`, for panel_unit.c).

### `opt_ref`

`/OPT:REF` now removes **501 annotated functions (230 exact)** against 571
(280) on `main`. The unit's own count barely moves (21 to 29 removed) while
it grows from 191 to 286 functions; `legoobject.cpp`'s are gone with it.
Remaining gaps (`make opt-ref-callers`): Message.c's `004526e0` calls the
port's namespaced `00444fb0` and overloaded `0044c4a0`; GAME-013's
`00409470` port model calls the port's `00438670`/`00438720`;
`0043abf0`'s `00435230` adapter (Lego.c's contract differs).

## Entropy and drops

`make reccmp-check` against `main` (`2f988e92`): 92 improved, 12 newly
mapped, `0044702d` newly unmapped (the Ghidra fragment), and **one drop,
no exact match lost**:

- `004526e0` `selection_candidate_filter_active` (Message.c) 75.42% →
  73.73%, `drift; instructions +0`: it calls three port entries at
  addresses that now name the unit's functions (`00444fb0`, a namespaced
  ledger stub; the overloaded `0044c4a0`; and the `0044c500` callback,
  declared by the frozen `select_from_legoobject.hpp` with C++ linkage). None
  can be renamed by a macro without editing a frozen header. Not
  owner-reviewed. Under ADR 0005's entropy rule 2 the baseline is refreshed.

#610's 21 drift drops: the 19 in the range are rewritten, and all 19 are
now above their pre-#610 scores, 12 of them exact (`004470d0` 67.46% →
100% among them). `00409f20` (`entity_cell_effects.cpp`)
is not recovered: its differing calls are to parts 1 and 2 through port
names, unrelated to part 3. `004526e0` is above.

During the work three exact functions dropped and were recovered:
`0040beb0` and `0046f770` (callers' names, fixed by the macros above) and
`0043c200` (the parameter-name count).

## How the work was split

The scaffold (`3a1f5759` before the squash) fixed the prototypes, the five
group sections and annotated stubs, and de-annotated `legoobject.cpp`.
Groups P, Q and the first nine of T were written by the parent; R, S and
T's last 19 by one Claude Opus helper at a time in its own worktree with
`match_function.py` only (the brief allowed one helper; four threads shared
the API rate limit). Each helper's branch was merged and its worktree
deleted. The parent wrote the helpers' adapter bodies, switched the callers,
moved the globals and ran every full build.

## Shape notes

- Helpers found that block coordinates as a `Point2I` struct, offsets copied
  as a `Point2F` struct, and `else return 0;` for shared epilogues each moved
  whole functions to 100%; MSVC 5 reuses dead parameter slots for locals
  without source tricks in some functions (`0044a5d0`) but not in others
  (`004473b0`, `0044b500`).
- An x87 normalise written as separate statements (`dir.x = dir.x * inv;
  dir.x = dir.x * 0.1f;`) matched where one expression did not (`004477d0`:
  80.00% to 97.70%).
- `(float)fabs(...)` casts give the original's `fst` and operand order
  (`0044a910`).
- `0044b850`'s `up[0] = (float)(up[0] - 0.0);` reproduces a fld/fstp round
  trip; it is commented in the code.
- No filler declarations. No new naked asm.

## Follow-ups

- `004526e0` (Message.c) calls the unit's `00444fb0`, `0044c4a0` and
  `0044c500` once `select_from_legoobject.hpp`'s users get a C-linkage
  declaration (a frozen header).
- Vehicle_SetActivity (`0046c3f0`), Upgrade_MoveAnimation (`0046c110`) and
  Building_SetActivity (`00408570`) with the original's contracts retire
  three adapters.
- `Lego_Object.field_32c` holds a pointer (the flock, the bat's home); it is
  an `int` in `legoobject_unit.h` and cast through `uintptr_t`.
- The unit map's function list could gain rows for the ten merged callbacks
  and drop `0044702d`.

## Validation

On `2f988e92` plus this change:

- `make reccmp-check`: the one drop above (accepted, not owner-reviewed);
  plain-immediate check OK; link order `check-map` OK; `make
  winapi-link-check` OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`; `tools/binary_progress_inputs.txt` rewritten with
  `binary_progress_hash.py --write-manifest`.
- `make unit-data-offsets`: the GAME-023 row above;
  `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` (no `--allow-growth`): passes; the lint ledger goes from
  1,312 to 1,291.
- Native: `cmake -S . -B build && cmake --build build && ctest --test-dir
  build --output-on-failure`: 37/37 pass, including `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-023 --root <copy without GAME-023's two link-order entries> --stem legoobject_p3 --scores baseline --out <dir>
    make match-function SOURCE=src/game/legoobject_unit.c
    make reccmp-check
    make match-progress
    make opt-ref-link opt-ref opt-ref-callers
    make unit-data-offsets

## Per-function table

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `00445d70` | `legoobject_flag_gated_callback_dispatch` | 116 | 12.05 | **100.00** |
| `00445df0` | `legoobject_flag_gated_callback_b` | 767 | — | 91.44 |
| `004460f0` | `legoobject_flag_gated_callback_a` | 1306 | — | 89.69 |
| `00446610` | `legoobject_movement_tick_scale_and_delta_get` | 676 | 12.07 | 71.66 |
| `004468c0` | `legoobject_route_adjacent_step_append` | 242 | 22.58 | **100.00** |
| `004469c0` | `legoobject_nearest_connected_cell_route_select` | 1147 | 12.67 | 81.79 |
| `00446e40` | `legoobject_nearest_cell_route_commit` | 650 | 21.21 | 45.95 |
| `004470d0` | `legoobject_conditional_type_five_spawn` | 224 | 67.46 | **100.00** |
| `004471b0` | `legoobject_try_create_linked_at_position` | 85 | 41.98 | **100.00** |
| `00447210` | `legoobject_adjacent_placement_cell_search` | 402 | 19.51 | 85.07 |
| `004473b0` | `legoobject_route_to_cell_command_issue` | 310 | 19.83 | 60.91 |
| `004474f0` | `legoobject_linked_proximity_response` | 205 | 38.10 | 92.09 |
| `004475c0` | `legoobject_progress_negative_delta_handle` | 357 | 41.11 | **100.00** |
| `00447730` | `legoobject_progress_advance_by_25` | 66 | 72.73 | **100.00** |
| `00447780` | `legoobject_clear_linked_object_state` | 74 | 24.49 | **100.00** |
| `004477d0` | `legoobject_path_step_lookahead_write` | 297 | 11.86 | 97.70 |
| `00447900` | `legoobject_type_four_callback_dispatch` | 24 | 54.55 | **100.00** |
| `00447920` | `legoobject_type_four_callback` | 47 | — | **100.00** |
| `00447950` | `legoobject_directional_activity_start` | 429 | 30.65 | 77.46 |
| `00447b00` | `legoobject_dispatch_event5_and_set_flag` | 46 | 82.35 | **100.00** |
| `00447b30` | `legoobject_activity_tick` | 260 | 25.24 | 98.86 |
| `00447c40` | `legoobject_activity_register_bookkeeping` | 596 | 17.82 | 95.70 |
| `00447ea0` | `legoobject_type_behavior_update` | 2323 | 14.40 | 67.46 |
| `004487c0` | `legoobject_clear_nested_state_8_fields` | 58 | 21.62 | **100.00** |
| `00448800` | `legoobject_move_to_cell_coordinates` | 116 | 29.46 | **100.00** |
| `00448880` | `legoobject_record_gated_step_command_issue` | 274 | 22.61 | **100.00** |
| `004489a0` | `legoobject_kind6_tick_down_marker_spawn` | 188 | 35.58 | **100.00** |
| `00448a60` | `legoobject_neighbor_cell_move_command_handle` | 488 | 18.87 | **100.00** |
| `00448c50` | `legoobject_set_activity_place` | 49 | 78.95 | **100.00** |
| `00448c90` | `legoobject_cardinal_step_command_issue_and_link` | 529 | 23.74 | **100.00** |
| `00448eb0` | `legoobject_proximity_step_command_issue_with_jitter` | 343 | 24.29 | **100.00** |
| `00449010` | `legoobject_kind6_record_approach_route_issue` | 144 | 63.72 | **100.00** |
| `004490a0` | `legoobject_proximity_step_command_issue_with_link` | 406 | 25.25 | **100.00** |
| `00449240` | `legoobject_walkable_target_cell_route_issue` | 99 | 41.03 | **100.00** |
| `004492b0` | `legoobject_cell_step_command_issue_with_target_mark` | 290 | 19.69 | 84.62 |
| `004493e0` | `legoobject_has_task_type_4_5_7` | 66 | 38.46 | **100.00** |
| `00449430` | `legoobject_target_cell_route_and_associate` | 238 | 18.75 | **100.00** |
| `00449520` | `legoobject_target_cell_route_marker14` | 229 | 11.27 | **100.00** |
| `00449610` | `legoobject_coordinate_route_and_mark` | 226 | 11.48 | **100.00** |
| `00449700` | `legoobject_route_step_command_issue` | 380 | 22.41 | **100.00** |
| `00449880` | `legoobject_blocked_cell_approach_weight_write` | 252 | 24.27 | **100.00** |
| `00449980` | `legoobject_activity_terrain_eligibility_transition` | 236 | 24.56 | 75.52 |
| `00449a70` | `legoobject_activity23_state_transition` | 80 | 73.91 | **100.00** |
| `00449ac0` | `legoobject_proximity_step_command_issue_clearing_gate` | 307 | 28.81 | **100.00** |
| `00449c00` | `legoobject_simtick_render_step` | 18 | 30.77 | **100.00** |
| `00449c20` | `legoobject_render_step_callback` | 167 | — | **100.00** |
| `00449cd0` | `legoobject_scene_object_state_handle_apply` | 32 | 91.67 | 91.67 |
| `00449cf0` | `legoobject_hide_check_callback` | 605 | — | 85.13 |
| `00449f50` | `legoobject_type_operation_dispatch` | 151 | 39.10 | **100.00** |
| `00449ff0` | `legoobject_type_operation_release` | 121 | 29.01 | 88.64 |
| `0044a070` | `legoobject_follow_target_pose_get` | 392 | 22.49 | 99.31 |
| `0044a200` | `legoobject_type_component_get` | 148 | 91.49 | 91.49 |
| `0044a2a0` | `legoobject_work_object_position_get` | 108 | 34.41 | **100.00** |
| `0044a310` | `legoobject_movement_input_accumulate` | 113 | 0.00 | **100.00** |
| `0044a390` | `legoobject_type11_cell_coordinate_append` | 52 | **100.00** | **100.00** |
| `0044a3d0` | `legoobject_type11_cell_coordinate_nearest_resolve` | 219 | 53.25 | **100.00** |
| `0044a4b0` | `legoobject_predug_coordinate_pool_append` | 52 | **100.00** | **100.00** |
| `0044a4f0` | `legoobject_nearest_cell_coordinate_world_resolve` | 219 | 43.72 | **100.00** |
| `0044a5d0` | `legoobject_nearest_open_cell_coord_search` | 454 | 32.95 | **100.00** |
| `0044a7a0` | `legoobject_open_cell_coord_search_callback` | 47 | — | **100.00** |
| `0044a7d0` | `legoobject_poly_variant_section_model_build` | 318 | 13.79 | **100.00** |
| `0044a910` | `legoobject_building_placement_preview_update` | 940 | 10.43 | 98.46 |
| `0044acc0` | `legoobject_sfx_grid_config_load` | 177 | 80.00 | **100.00** |
| `0044ad80` | `legoobject_sfx_at_cell_get` | 54 | 90.91 | **100.00** |
| `0044adc0` | `legoobject_scatter_spawn_gate_set` | 29 | **100.00** | **100.00** |
| `0044ade0` | `legoobject_kind_one_random_spawn_trigger` | 106 | 37.50 | **100.00** |
| `0044ae50` | `legoobject_random_kind_six_scatter_spawn` | 309 | 31.62 | **100.00** |
| `0044af90` | `legoobject_attached_spawn_record_process` | 29 | 74.07 | **100.00** |
| `0044afb0` | `legoobject_tiny_monster_spawn_and_teardown` | 395 | 26.34 | 85.71 |
| `0044b140` | `legoobject_small_spider_cell_swarm_spawn` | 268 | 30.05 | 62.82 |
| `0044b250` | `legoobject_pairing_activity_raise_and_mutual_link` | 249 | 65.28 | **100.00** |
| `0044b350` | `legoobject_activity_is_special` | 28 | **100.00** | **100.00** |
| `0044b370` | `legoobject_activity_grant` | 259 | 36.55 | **100.00** |
| `0044b480` | `legoobject_activity_scan` | 54 | 62.07 | **100.00** |
| `0044b4c0` | `legoobject_type_two_target_resolve_and_apply` | 55 | 88.00 | 92.68 |
| `0044b500` | `legoobject_position_step_command_issue_and_link` | 376 | 19.85 | 41.70 |
| `0044b680` | `legoobject_scene_object_tree_attach` | 273 | 11.76 | **100.00** |
| `0044b7a0` | `legoobject_matrix_rotate_z_with_offset` | 164 | 14.58 | 97.83 |
| `0044b850` | `legoobject_scene_node_sync_callback` | 600 | — | **100.00** |
| `0044bab0` | `legoobject_resource_dispose_callback` | 37 | — | **100.00** |
| `0044bae0` | `legoobject_resource_and_callback_state_dispose` | 78 | 22.22 | **100.00** |
| `0044bb30` | `legoobject_node_scale_update` | 241 | 31.93 | **100.00** |
| `0044bc30` | `legoobject_motion_to_scene_node_sync` | 502 | — | 98.71 |
| `0044be30` | `legoobject_auxiliary_payload_release` | 202 | — | **100.00** |
| `0044bf00` | `legoobject_timed_callback_list_update` | 196 | — | 87.14 |
| `0044bfd0` | `legoobject_monster_spawn_behavior_tick` | 93 | 77.19 | **100.00** |
| `0044c030` | `legoobject_one_shot_attached_effect_start` | 211 | 31.41 | **100.00** |
| `0044c110` | `legoobject_attached_entity_wake` | 49 | 31.58 | **100.00** |
| `0044c150` | `legoobject_pair_scalar_fields_set` | 92 | 22.54 | **100.00** |
| `0044c1b0` | `legoobject_heading_step_move` | 747 | 29.30 | 97.75 |
| `0044c4a0` | `legoobject_selection_clear_marker_and_reset_hud` | 95 | 68.00 | **100.00** |
| `0044c500` | `legoobject_selection_clear_callback` | 41 | 13.79 | **100.00** |
| `0044c530` | `legoobject_ai_can_do` | 31 | 51.85 | **100.00** |
| `0044c550` | `legoobject_scatter_spawn_enable_and_drain` | 145 | 15.62 | **100.00** |
| `0044c5f0` | `legoobject_scatter_spawn_drain_callback` | 285 | — | 93.55 |
