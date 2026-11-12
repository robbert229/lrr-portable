# Finding 582 — Weapons.c (GAME-050) rebuilt in original shape, with a port tail

Issue #592, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-050 is the original's `Weapons.c`: the `WeaponTypes` block (per-type
damage grids, slow death, recharge, discharge, ammo, range and wall destroy
times), the projectiles and laser beams that weapons fire, and the vehicle and
building weapon models (aim, recharge, fire). Text `0046eb80`-`004721e0`. It
was relocated verbatim into `src/game/weapons.cpp` (#496). This change rebuilds
it as the C unit [`src/game/weapons.c`](../../src/game/weapons.c), with the
unit-private imports header [`weapons_imports.h`](../../src/game/weapons_imports.h)
and inline vector helpers [`weapons_maths.h`](../../src/game/weapons_maths.h).
The port's implementation moves, unannotated, to the port tail
[`weapons_port.cpp`](../../src/game/weapons_port.cpp).

This change is stacked (stack #600) on #601 (GAME-043 Stats.c, `8ecf1aac`),
after #597 (Lego.c part 3), #595 (finding 580) and #598 (Files.c). Unit
scores are from `main` at `3791f0dd` (#590) for "before"; the unit's code is
the same on #601's head. Whole-binary figures are #601's head against this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 47 functions, 13,611 original code bytes) | 17.94% | **70.69%** |
| Exact functions in the unit | 13 | **28 of 47** |
| Mapped functions in the unit | 45 | 47 |
| Exact-function code bytes (`binary-progress.json`) | 113,553 | **115,442** |
| Accuracy, full / game-only | 73.17% / 70.46% | 73.97% / 71.34% |
| `opt_ref` annotated (full): removed / exact removed | 754 / 335 | 762 / 346 |
| `opt_ref` annotated (game only): removed / exact removed | 735 / 322 | 743 / 333 |
| Reccmp lint ledger | 1,377 | 1,366 |

Byte weights are the original's code bytes without the trailing `0x90` fill;
an unmapped function counts as 0%. Two functions are newly mapped: the
`legoobject_run_through_lists` callback `0046f610` (Weapon_Callback_UpdateObject:
recharge, recharge sound and slow-death ticks) and `0046f920` (the projectile
mover). Neither had an annotation; Ghidra folds them into their neighbours.
The scaffold's unit table did not list `0046f610` and `00471370` (the ray
candidate callback) either, for the same reason; both are annotated here.

| Address | Function | Port name before | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046eb80` | `weapons_initialise` | `weapon_type_config_load` | 1350 | 15.83 | 86.64 |
| `0046f0d0` | `object_type_name_index_find` | `object_type_name_index_find` | 62 | 50.67 | **100.00** |
| `0046f110` | `weapon_type_recharge_time_get_target` | `weapon_type_recharge_time_get_target` | 34 | 100.00 | **100.00** |
| `0046f140` | `weapon_type_discharge_rate_get_target` | `weapon_type_discharge_rate_get_target` | 34 | 100.00 | **100.00** |
| `0046f170` | `weapon_type_range_get_target` | `lego::weapon_type_range_get_target` | 34 | 100.00 | **100.00** |
| `0046f1a0` | `weapon_type_wall_destroy_time_get_target` | `lego::weapon_type_wall_destroy_time_get_target` | 41 | 100.00 | **100.00** |
| `0046f1d0` | `weapons_get_damage_for_object` | `lego::entity_indexed_float_lookup_with_minus_one_fallback` | 151 | 17.65 | **100.00** |
| `0046f270` | `weapons_generic_damage_object` | `ledger_stub_0046f270_TypeMotionScaleApplyDispatch` | 270 | 32.18 | **100.00** |
| `0046f380` | `weapons_gun_hit_object` | `entity_apply_impulse_and_direction` | 44 | 43.33 | **100.00** |
| `0046f3b0` | `weapons_projectile_hit_check` | `ledger_stub_0046f3b0_RayHitScanAndEffectApply` | 416 | 15.47 | 74.48 |
| `0046f550` | `weapons_update` | `simtick_sfx_bank` | 181 | 18.18 | 96.43 |
| `0046f610` | `weapons_callback_update_object` | `-` | 343 | unmapped | **100.00** |
| `0046f770` | `weapons_projectile_hit_object` | `effect_apply_by_kind_branch` | 422 | 31.65 | **100.00** |
| `0046f920` | `weapons_projectile_update` | `-` | 457 | unmapped | 45.69 |
| `0046faf0` | `weapons_projectile_create` | `entity_spawn_record_slot_init` | 378 | 16.79 | 84.50 |
| `0046fc70` | `weapons_projectile_create_curved` | `object_flight_curve_record_init` | 640 | 17.25 | 57.22 |
| `0046fef0` | `weapons_get_known_weapon_object_type` | `effect_mode_surface_query_type_resolve` | 125 | 100.00 | **100.00** |
| `0046ff70` | `weapons_projectile_find_free` | `active_effect_record_slot_find` | 31 | 40.00 | 90.00 |
| `0046ff90` | `weapons_lazer_find_free` | `active_segment_visual_record_slot_find` | 28 | 35.71 | **100.00** |
| `0046ffb0` | `weapons_fire_lazer` | `ledger_stub_0046ffb0_LaserSpatialSoundAndImpact` | 676 | 10.47 | 56.49 |
| `00470260` | `weapons_object_collision_check` | `ledger_stub_00470260_EntitySurfaceSegmentCollisionCheck` | 68 | 61.02 | **100.00** |
| `004702b0` | `weapons_object_quad_intersect` | `ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect` | 649 | 27.35 | 55.68 |
| `00470540` | `weapons_object_center_proximity_check` | `entity_segment_center_proximity_check` | 237 | 21.89 | 98.78 |
| `00470630` | `weapons_object_line_side_check` | `entity_directed_line_side_check` | 88 | 37.50 | **100.00** |
| `00470690` | `weapons_lazer_create` | `timed_entity_segment_visual_create` | 198 | 33.33 | **100.00** |
| `00470760` | `weapons_mesh_build_segment` | `mesh_build_eight_sided_segment` | 2911 | 2.66 | 25.59 |
| `004712c0` | `weapons_get_weapons_model` | `entity_velocity_state_record_get` | 35 | 100.00 | **100.00** |
| `004712f0` | `weapons_model_timer_get` | `entity_velocity_slot_value_get` | 25 | 100.00 | **100.00** |
| `00471310` | `weapons_model_timer_set` | `entity_velocity_slot_value_set` | 29 | 100.00 | **100.00** |
| `00471330` | `weapons_z_within_bounds` | `interpolated_z_within_bounds` | 59 | 47.27 | **100.00** |
| `00471370` | `weapons_callback_ray_candidate` | `cmd_place_ray_candidate_check` | 702 | 15.76 | 84.38 |
| `00471630` | `weapons_ray_block_sweep` | `ray_hit_cell_distance_minimize_sweep` | 554 | 17.80 | 97.93 |
| `00471860` | `weapons_ray_trace_nearest` | `scene_segment_trace_nearest_hit` | 104 | 21.51 | **100.00** |
| `004718d0` | `weapons_object_pair_ray_hit` | `object_pair_ray_hit_check` | 143 | 36.52 | 88.00 |
| `00471960` | `weapons_see_through_walls_check` | `see_through_walls_ray_check` | 189 | 16.00 | 94.74 |
| `00471a20` | `weapons_model_fire_null_get` | `read_indexed_pair_slot_dword` | 19 | 100.00 | **100.00** |
| `00471a40` | `weapons_model_x_pivot_get` | `velocity_slot_primary_object_get` | 13 | 100.00 | **100.00** |
| `00471a50` | `weapons_model_y_pivot_get` | `velocity_slot_secondary_object_get` | 13 | 100.00 | **100.00** |
| `00471a60` | `weapons_model_aim` | `entity_velocity_steer_blend_update` | 521 | 13.29 | 69.30 |
| `00471c70` | `weapons_model_fire_dir_get` | `velocity_slot_vector_read` | 36 | 100.00 | **100.00** |
| `00471ca0` | `weapons_object_center_get` | `entity_world_center_position_read` | 60 | 37.84 | **100.00** |
| `00471ce0` | `weapons_object_can_be_hit` | `entity_flag100_and_shared_eligibility_check` | 54 | 100.00 | **100.00** |
| `00471d20` | `weapons_model_fire_update` | `ledger_stub_00471d20_VelocitySlotSteerDispatchAndSound` | 473 | 11.11 | 95.60 |
| `00471f00` | `weapons_model_aim_update` | `pose_offset_velocity_slot_blend_update` | 189 | 17.91 | **100.00** |
| `00471fc0` | `weapons_object_update_weapons` | `entity_velocity_slot_dispatch` | 191 | 12.94 | **100.00** |
| `00472080` | `weapons_object_in_range` | `entity_centers_within_configured_radius` | 167 | 33.33 | 90.16 |
| `00472130` | `weapons_object_in_touch_range` | `entity_centers_within_object_radius` | 167 | 15.79 | 90.16 |

### What is left

The non-exact functions and what was tried. "Rule B" is finding 538's counter
phase; "param home" is MSVC 5.0 packing a local into a dead parameter's stack
slot.

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `0046eb80` | 86.64 | register choice (`esi`/`edi` swap between the property pointer and the surface-name walk) and the name copy's scheduling; three declaration orders, no change |
| `0046f3b0` | 74.48 | x87 scheduling of the hit position, which the original interleaves with the call's pushes; explicit sums, the inline ray end point, and the end point as the call's argument all score the same |
| `0046f550` | 96.43 | the two `Mesh_Remove` calls (`004820d0`) go to `lws_port.cpp`'s unannotated `mesh_remove` adapter, so their operands do not map |
| `0046f920` | 45.69 | the original passes the position components to `Container_SetPosition` through `fld`/`fstp` and keeps the scaled step on the x87 stack across both branches; casts and the inline ray end point do not reproduce it; one shared `SetOrientation` call (as the original) is kept |
| `0046faf0` | 84.50 | `eax`/`edx` swaps in the two struct copies and the operand order of the first multiply of the inline normalise |
| `0046fc70` | 57.22 | the original keeps its two out-parameters in locals where ours packs one into a param home, which shifts every stack offset; struct copies, inline subtract, normalise and distance applied; permuter (150 s) found nothing |
| `0046ff70` | 90.00 | the loop ends at `0x5056a0`, an address past `weaponGlobs` (in the next unit's `.bss`), which has no name in the rebuild |
| `0046ffb0` | 56.49 | the original packs the address-taken hit object into `fromPos`'s param home (`0x24` frame against our `0x28`); block-scoped `blockPos`/`dir2D` (as the original) and inline scale and add applied |
| `004702b0` | 55.68 | edge-test branch order and an extra spill; three forms of the absolute-value test score the same |
| `00470540` | 98.78 | one `fstp st(1)` against `fstp st(0)` |
| `00470760` | 25.59 | the original builds the octagon from a basis it computes with `sqrt(2.0)` at run time (`fld qword 2.0; fsqrt`), not the folded `0.41421354` the pseudocode shows; the face and texture-coordinate tables (48 and 128 dwords, as initialiser lists in the original's declaration order) and the stack layout now match; baseline was 2.66 |
| `00471370` | 84.38 | stack offsets (one extra local) and x87 scheduling in the side tests |
| `00471630` | 97.93 | the operand order of the first end-point sum; both source orders give the same code (rule B) |
| `004718d0` | 88.00 | the subtract's stores fall between different argument pushes; 120 declaration orders (all tie), passing `&obj` (worse, 46%) |
| `00471960` | 94.74 | the same push scheduling as `004718d0` |
| `00471a60` | 69.30 | x87 scheduling of the two normalises; the separate direction vector and `len * dir + fireDir` (as the original) took it from 37.8% |
| `00471d20` | 95.60 | param home choice: the original keeps the loop counter in `obj`'s home and the discharge rate in `slot`'s; ours swaps them. Declaration order, block scope and `int`/`unsigned` change nothing; permuter (240 s) found nothing |
| `00472080`, `00472130` | 90.16 | one `fxch` and the place of `add esp, 8` around the inline distance (rule B) |

## How the conversion is done

### The port tail

The port's weapons code is a different contract: hook tables
(`s_sfx_bank_hooks`, `s_active_effect_build_hooks`, `s_entity_ray_hit_sweep_ops`
and others), the port's `EntitySlot` and NULL guards that native tests pin.
As in findings 560 and 570 it stays a port: `weapons.cpp` is renamed
`weapons_port.cpp`, every `// FUNCTION` becomes a `/* Port model of 0x... */`
comment, the `// GLOBAL` on its `s_weapon_indexed_records` is dropped, and the
file is a `tail` entry in the link-order list. Native callers and tests keep
calling it under the port's names.

Five entries keep the port's names and serve native callers too, because their
contract is the same once the port's loader shares the unit's table:
`object_type_name_index_find` and the four stats getters
(`0046f0d0`-`0046f1a0`). Their port definitions are deleted.

- The getters read `weaponGlobs.weaponStatsList`, a `Weapons_Stats` (0x4b68
  bytes) array, the port's `WeaponIndexedTypeRecord` layout. `weapon.cpp` and
  `weapons_port.cpp` reach the pointer through the `// PORT-ONLY` accessor
  `weapons_port_stats_slot`, behind `#define s_weapon_indexed_records`
  placed after the frozen includes (as finding 570's `s_behaviour`). The frozen
  `weapon_from_weapons.hpp`'s declaration of that name is now never defined or
  used.
- The port's loader publishes its count and a name table built from
  `s_weapon_definitions[i].name` through the `// PORT-ONLY`
  `weapons_port_type_table_set`, so `object_type_name_index_find` compares the
  same names with `_strcmpi` (the port compared them with its own ASCII
  case-folding; the result is the same, including `count + 1` for a miss and
  1 when no types are loaded).
- `weapon.cpp` gives the range and wall-time getters C linkage before the
  frozen header's declarations, as it already did for the other two
  (finding 566).

### State

Weapons.c's `.bss` is one `static Weapons_Globs weaponGlobs` at `00503870`,
laid out as the original: the type count, name list and stats list
(`00503870`-`00503878`), ten lasers of 0x10 bytes (`0050387c`), ten
projectiles of 0x2b0 bytes (`0050391c`) and the config (`005053fc`). The unit
ends at `00505400`; `translation-unit-ranges.csv` gives `005056a0`, which is
only the loop end of `0046ff70` (`&projectileList[10].object`); `00505400` is
the next unit's DirectShow string buffer. The float constants are literals
(MSVC pools them in this object), and the nine `WeaponTypes` strings are marked
where they are used; `":"` (`004a1cd4`) is `config.c`'s.

The unit declares its own views at the original's Win32 offsets:
`Weapons_Object` (LegoObject: type, id, the vehicle and building models,
`objLevel`, the slow-death timer and weapon at `+0x358`/`+0x35c`, the weapon
cool-down at `+0x360`, the laser hit object at `+0x3c4`, `flags3`/`flags4`),
`Weapons_Model` (the weapons part of a vehicle at `+0x138` and a building at
`+0xa0`: fire nulls, x and y pivots, pitch range, fire directions, targets,
weapon names, timers and count), `Weapons_Projectile` (a `BezierCurve` at
`+0x4`), `Weapons_Lazer` and the ray search record. As finding 570 found for
AITask.c, float fields are read as struct members so MSVC compares them as
memory operands.

`weapons.h` keeps its three declarations exactly (`terrain_cell_objects.c`
includes it, see below); the unit's types and prototypes are in `weapons.c`,
inside `extern "C"` for the native C++ compile.

### Callees and callers

- **`extern "C"` in place** on 14 existing declarations: `sound_object_duration_get`
  (`sound.hpp`), `scene_light_create_at_position` (`resource.hpp`),
  `scene_object_coordinate_transform_in_place`, `point_project_onto_line2d`,
  `point2d_project_onto_direction`,
  `engine_task_callback_record_allocate_and_initialize` (`view.hpp`),
  `mesh_segment_vertex_fill`, `render_object_color_vector_set` (`record.hpp`),
  `mesh_record_allocate_and_fill` (`surface.hpp`), the three
  `renderer_word_*_get` (`crt.hpp`), and `entity_one_shot_attached_effect_start`,
  `entity_pair_scalar_fields_set` (`entity.hpp`). No declaration is added, so
  no counter moves.
- **Original arguments.** `0043f6a0` takes four arguments and `0044c150` a float
  pair in the original. The matching build declares them so under their own
  names (the port's definitions ignore the extra argument; cdecl); the native
  build calls `lego_port.cpp`'s existing `_original` adapters. `00465160` returns
  `float` in the original and `double` in the port; both return on the x87
  stack, so the matching build declares it `float` (this made `0046f610`
  exact). `00431570` (Lego_GetMap) is called through an unprototyped type and
  `0041f8e0` (Lego_GetCrystalCount) through a cast, as `aitask.c` and
  `legoobject_unit.c` do.
- **Stats.c.** #601 defines Stats.c's functions under the port's names with C
  linkage (`entity_stats_link_names.h`), so the unit calls `00469da0` as
  `entity_stats_field_at_a0_corpus` and every other Stats.c entry directly;
  no adapter is needed and no symbol is defined twice. `lws_port.cpp`'s existing
  `mesh_remove` and `mesh_group_material_values_set` stand in for `004820d0`
  and `00483420`. Those calls are the remaining operand differences.
- **C callers.** The new matching-build-only `weapons_port_aliases.h` points
  `aitask.c` (`00471960`, `00472130`) at the unit, as `aitask_port_aliases.h`
  does. `lego_unit.c` calls `0046eb80` (from `0041fb50`) and `0046f1d0` (from
  `00433180`, Lego.c part 3, exact); their two aliases are `#define`s in
  `aitask_port_aliases.h`, which `lego_unit.c` already includes first. On the
  stack, including `weapons_port_aliases.h` in `lego_unit.c` instead cost
  `00431450` (100% to 83.78%) and `004311d0` (70.80% to 67.15%) to a counter
  shift, and without the `0046f1d0` alias `00433180` fell to 98.99% (its call
  operand no longer mapped).
  `terrain_cell_objects.c` calls `0046f270`; its alias is a `#define` inside
  `weapons.h`, not an include: adding `#include "weapons_port_aliases.h"` to
  `terrain_cell_objects.c`, even with every `#define` removed, moved that
  unit's counter and cost `0040cdc0` its exact match (`imul` operands swapped,
  100% to 89.47%). An include of a new file is not free under finding 538's
  rule B; a `#define` in a header already included is. `weapons.c` defines
  `LEGORRR_GAME_WEAPONS_UNIT` to keep its own name. `0040e0c0` (exact) now
  calls the unit.
- **Callers in files other threads own.** None: `lego_unit.c` (#587) is
  unchanged from #601's head. No file of #587, #588, #448, #591 or #593 is
  edited. `legoobject_unit.c` (#588) needs no change: it calls
  `weapon_type_discharge_rate_get_target`, whose name is shared.

## Entropy and drops

`make reccmp-check` against `main`'s baseline: 34 improved or newly mapped,
two below baseline, **no exact match lost**, no instruction count changed in a
dropped function. Both drops are C++ port callers in `legoobject.cpp` (#588)
that still call port models whose annotations moved to the unit, so their call
operands no longer map (operand-only; their contracts differ: hook tables and
`EntitySlot`). The baseline is refreshed for them; neither is owner-reviewed.

| Address | Function | Before | After | Original calls |
| --- | --- | ---: | ---: | --- |
| `0043c3e0` | `entity_type26_command_completion_gate` | 33.73 | 31.33 | `004718d0`, `00472080` |
| `004420d0` | `kind_effect_slot_init_dispatch` | 13.38 | 7.36 | `0046faf0`, `0046fc70` |

The classifier labels both "drift"; the listings show only the moved call
targets. They recover when GAME-023 part 2 converts their callers.

## `opt_ref` (finding 576, `make opt-ref-callers`)

On #601's head, with Lego.c's config init and LegoObject.c's update live
(#587, #588's part 1), `/OPT:REF` removes 8 more annotated functions (11 more
exact) than before this change. The port models that live port code
(`simtick_sfx_bank` from the port's tick hooks, the `cmd.cpp` and
`entity.cpp` ray and range checks, `effects.cpp`) reached are now unannotated,
and those of the unit's versions that only removed or port-model callers would
reach count as removed; finding 579's class (c). `weapons.c.obj`: 39 of 47
removed, 24 exact. The shared `object_type_name_index_find` and
`weapon_type_range_get_target` stay live.

| | #601 `8ecf1aac` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 754 / 335 | 762 / 346 |
| `opt_ref.annotated.game_only`: removed / exact removed | 735 / 322 | 743 / 333 |
| `opt_ref.removed_functions` / bytes | 2,579 / 271,558 | 2,643 / 283,558 |

No unit function shows finding 580's `0x800000` artifact (Cause A).

## Native behaviour

Unchanged apart from where the shared state lives: the getters and the name
lookup read the table the port's loader publishes. Natively nothing installs
the port's `WeaponTypes` parse hook, so no weapon types load and the name
lookup answers 1, as on `main`. That gap, and the port's loader not filling
the per-object damage grid, slow death or ammo, is filed with the `port`
label. The `.bss` grows by `weaponGlobs` (0x1b90 bytes) and the port's name
table; the port's own records stay.

## Validation

On #601's head (`8ecf1aac`):

- `make reccmp-check`: OK after the baseline refresh; the two labelled drops
  above, no exact match lost.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets` (GAME-050: nine string gaps, one `.bss` item).
- `python3 tools/translation_units.py build`; `make reccmp-lint-update` (ledger
  1,377 to 1,366); `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test`, `make winapi-link-check`, native build and `ctest`
  (37/37, synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `weapons.c`.

## Reproducing

    python3 tools/convert_unit.py GAME-050 --stem weapons   # after moving weapons.cpp to the tail
    make match-function SOURCE=src/game/weapons.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object weapons.c.obj"
    make match-progress
    make unit-data-offsets
