# 638. LegoObject.c's callers switched to the originals: 32 `/OPT:REF` drops revived

Issue #730, part of #388 and #562. This follows finding 637 (#723), which
traced the remaining `/OPT:REF` drops to a few live original callers whose
rebuilds still call port substitutes, and finding 634 (#725), which switched
`lego_game_config_init`'s calls the same way. It was measured on `main`
`db760978`, with one full matching build before the change and one after.

## Summary

- **9 call sites in 7 callers** now call the annotated originals in the
  matching build. Each switch is an object-like `#define`, guarded by
  `_MSC_VER < 1200`. Native builds keep the port names.
- **`opt_ref`: 124 removed / 102 exact → 92 / 78.** 32 annotated functions
  (24 exact) are no longer removed. None is newly removed.
- **7 functions rise and none falls** (`make reccmp-check`). No function
  becomes exact, so exact-function code bytes stay at 289,516 (45.06%).
  The image end stays at `0x875000`.
- **The symbol counter does not move (rule B, finding 558).** Each alias
  comes before every declaration of its name in the translation unit, and
  the target was not declared there before. `make match-function` over
  `legoobject_unit.c`, `aitask.c`, `animation_object.c` and
  `vehicle_model.c`, with and without the change, differs only at the
  switched callers.
- **The sim interaction update needed no change.** On `db760978`,
  `lego_main_sim_interaction_and_cell_update` (`0x426520`) is no longer
  anyone's dropped root. #725 (finding 634, section 1) had already aliased
  its `interface_do_mouse` and `legoobject_building_placement_preview_update`
  calls.

## Per call site

| Caller | Port substitute | Original | Where the alias is |
| --- | --- | --- | --- |
| `0043c3e0` legoobject_type26_command_completion_gate, `0043c810` legoobject_update_callback | `entity_centers_within_configured_radius` | `weapons_object_in_range` (`00472080`) | `legoobject_unit_imports.h` |
| `0043c3e0`, `0043c810` | `object_pair_ray_hit_check` | `weapons_object_pair_ray_hit` (`004718d0`) | `legoobject_unit_imports.h` |
| `0043c810` | `entity_flag100_and_shared_eligibility_check` | `weapons_object_can_be_hit` (`00471ce0`) | `legoobject_unit_imports.h` |
| `0043c810` | `entity_velocity_slot_dispatch` | `weapons_object_update_weapons` (`00471fc0`) | `legoobject_unit_imports.h` |
| `00437940` legoobject_deferred_olist_create_and_link | `legoobject_gb_tutorial_counter_increment_adapter` | `nerps_monster_destroyed` (`00454880`) | `legoobject_unit_imports.h` |
| `00404280` aitask_cmd_entity_command_chain_tick | `aitask_adjacent_position_find_adapter` | `legoobject_adjacent_position_candidate_find` (`00443710`) | `legoobject_port_aliases.h` |
| `00404fc0` aitask_place_command_eligibility_check (2 calls) | `aitask_object_command_gate_adapter` | `legoobject_type_two_target_resolve_and_apply` (`0044b4c0`) | `legoobject_port_aliases.h` |
| `00406990` entity_type_config_resource_row_load (3 calls), `0046c6f0` lego_vehicle_load_adapter (2 calls) | `mesh_lod_load` (`entity.cpp`, returns NULL) | `legoobject_poly_variant_section_model_build` (`0044a7d0`) | `entity.h` |

Contracts:
- **The four Weapons.c port models** take the original's arguments.
  `00471fc0` was already called through a `(void*, float)` cast, which is
  the original's signature.
- **`00454880`** takes no argument. The original passes the object's type,
  which the callee does not read, and the caller pops it. The adapter's
  declaration keeps that one argument, so the call is unchanged.
- **`00443710`** is called by AITask.c with a fourth argument that the
  original pushes. LegoObject.c's three-argument definition does not read it.
- **`0044a7d0`** has `mesh_lod_load`'s five arguments (finding 549).

`entity.h` is included only by `animation_object.c`, `vehicle_model.c`
(through `vehicle_model_imports.h`) and the native test
`test_c_boundaries.cpp`. `entity.cpp`, which defines `mesh_lod_load`, does
not include it. The two `legoobject_port_aliases.h` macros are in its
adapter block. Of the files that include the header, only `aitask.c` and
`aitask_imports.h` use those names. `aitask_port.cpp`, which defines both
adapters, does not include the header.

## Per function (`make reccmp-check`, full and game-only alike)

| Address | Function | Before | After | What is left |
| --- | --- | ---: | ---: | --- |
| `0043c3e0` | legoobject_type26_command_completion_gate | 78.79 | **84.85** | `ecx`/`edx` for `eax` on the two `aiTask->targetObject` reloads (register choice) |
| `0043c810` | legoobject_update_callback | 89.06 | 89.21 | other callees under port names, stack slots |
| `00437940` | legoobject_deferred_olist_create_and_link | 81.93 | 82.35 | as before |
| `00404280` | aitask_cmd_entity_command_chain_tick | 82.96 | 83.07 | as before |
| `00404fc0` | aitask_place_command_eligibility_check | 99.21 | 99.43 | as before |
| `00406990` | entity_type_config_resource_row_load | 98.08 | 99.52 | as before |
| `0046c6f0` | lego_vehicle_load_adapter | 99.48 | 99.83 | as before |

## `/OPT:REF` before and after

`make opt-ref` on the default relink (`/OPT:REF`, with LINK's implicit
`/OPT:ICF`), as `binary-progress.json` records it:

| | `db760978` | This change |
| --- | ---: | ---: |
| Removed / removed exact (full) | 124 / 102 | **92 / 78** |
| Removed / removed exact (game-only) | 119 / 97 | 87 / 73 |
| Removed bytes (`make opt-ref-callers --summary`) | 22,956 | 14,844 |
| Live-caller cause | 109 (87) | 77 (63) |

The 32 functions that are no longer removed:
- **Weapons.c, 22 (15 exact):**
  - `weapons_object_update_weapons` and its fire and aim chain:
    `0046f1a0`, `0046ff90`, `0046ffb0`, `00470690`, `00470760`, `004712c0`,
    `004712f0`, `00471310`, `00471a20`, `00471a40`, `00471a50`, `00471a60`,
    `00471c70`, `00471d20` and `00471f00`;
  - the ray trace `00471330`, `00471370` and `00471860`;
  - the four direct callees `004718d0`, `00471ce0`, `00471fc0` and
    `00472080`.
- **Below them, 3 (2 exact):**
  - Lego.c's `00426230`;
  - Mesh.c's `00482620`, under the lazer mesh;
  - Collision.c's `00408bf0`, under the ray candidate callback.
- **LegoObject.c, 4 (4 exact):** `00443710` and its callback `00443840`,
  `0044a7d0` and `0044b4c0`.
- **terrain_map.c, 2 (2 exact):** `004519b0` and `00451ab0`, under
  `0044a7d0`.
- **NERPs.c, 1 (1 exact):** `00454880`.

## What is left, and who owns it

| Removed | Hangs below | Owner |
| --- | --- | --- |
| Weapons.c `0046f380`, `0046f3b0`, `0046f550`, `0046f610`, `0046f770`, and LegoObject.c `0044c030` (under `0046f770`) | `lego_engine_tick` (`004232e0`) calls `simtick_sfx_bank_original` instead of `weapons_update` | #731 (`lego_unit.c`) |
| LegoObject.c `00438cd0`, `00438d30`, `00438d90` | `entity_placement_commands_build_once` (`00409f20`), a C++ port model in `entity_cell_effects.cpp` that calls `entity_build_placement_command_setup` | GAME-008 conversion: unconverted, no issue yet |
| Lego.c `00432360`, `004323c0` | `entity_cell_path_effects_apply_current` (`00409d50`), the same file, which calls a namespaced port entry | GAME-008 conversion, as above |
| LegoObject.c `00438ed0` | folded by `/OPT:ICF` | the gate's eventual `/OPT:NOICF` (finding 637) |
| LegoObject.c `00438390` | nothing; unreferenced in the original as well | the floor (finding 637) |

GAME-008 is 5 functions in `entity_cell_effects.cpp`, outside this issue's
files, so it is not converted here. Its two callers can be aliased once it is
in original shape.

## Files

- `src/game/legoobject_unit_imports.h`: the four Weapons.c aliases and the
  NERPs alias.
- `src/game/legoobject_port_aliases.h`: the two AITask.c adapter aliases.
- `src/game/entity.h`: `mesh_lod_load` aliased to `0044a7d0`.
- `docs/coverage/`: the match baseline, listing digests and binary progress,
  from `make match-progress`. `make unit-data-offsets` leaves the tree
  unchanged.

No file owned by #731, #732 or #733 is edited. `tutorial_script.c` is only
named as the link target of `00454880`.

## Reproducing

```sh
make reccmp-check                                  # 7 up, 0 down
make match-progress && make unit-data-offsets
make opt-ref-callers OPT_REF_ARGS=--summary        # total 92 / 78
python3 tools/match_function.py --source src/game/legoobject_unit.c --json
```
