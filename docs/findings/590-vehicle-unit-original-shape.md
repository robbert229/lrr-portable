# Finding 590 — Vehicle.c (GAME-048) rebuilt in original shape, with a port tail

Issue #617, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-048 is the original's `Vehicle.c` (text `0046c6f0`-`0046db90`): the
vehicle models. Vehicle_Load reads each vehicle type's `.ae` config once (one
or two activity containers, the wheel mesh and wheel nulls, the named nulls,
the weapon pivots, the upgrades and the medium-detail meshes). Every vehicle
object clones that model, and Vehicle_SetPosition places the clone on the
terrain. It was relocated verbatim into `src/game/vehicle_model.cpp` (#496).
This change rebuilds it as the C unit
[`src/game/vehicle_model.c`](../../src/game/vehicle_model.c), with:

- the unit-private imports header
  [`vehicle_model_imports.h`](../../src/game/vehicle_model_imports.h);
- the inline vector helpers
  [`vehicle_model_maths.h`](../../src/game/vehicle_model_maths.h);
- the matching-build link names
  [`vehicle_model_link_names.h`](../../src/game/vehicle_model_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`vehicle_model_port.cpp`](../../src/game/vehicle_model_port.cpp).

Stacked (#622) on #620 (GAME-012, `9a6cd741`), which sits on #619 (Wad.c).
"Before" is #620's head; "after" is this change rebased onto it, from the full
matching build (`make reccmp-check`). The change was first measured on `main`
`984a1213`; no GAME-048 score differs between the two bases, so the
per-function "before" column is `main`'s baseline, which #620's matches for
these addresses.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 26 functions, 5,084 code bytes) | 11.66% | **90.02%** |
| Exact functions in the unit | 6 | **23 of 26** |
| Mapped functions in the unit | 24 | 26 |
| Exact functions, full / game-only (`binary-progress.json`) | 1,392 / 1,192 | **1,412 / 1,212** |
| Exact-function code bytes | 154,804 (24.09%) | **156,894** (24.42%) |
| Accuracy, full / game-only | 78.58% / 76.47% | 79.02% / 76.95% |
| `opt_ref` annotated (full, by address): removed / exact removed | 577 / 278 | **571 / 280** |
| `opt_ref` by symbol (`make opt-ref-callers`), measured on `main` | 570 / 261 | 565 / 264 |
| Reccmp lint ledger | 1,314 | 1,314 |

Byte weights are the single-function loop's original sizes; an unmapped
function counts as 0%. Two functions are newly mapped:

- `0046c6f0` Vehicle_Load, which the port modelled in `entity_config.cpp`
  under another contract;
- `0046d380` Vehicle_SetPosition, for which #610 (finding 578) added a
  matching-build stand-in.

The names are OpenLRR's, in the repository's snake case. "Port name" is the
name the port and the callers use; in the matching build the unit's
definition takes it (see [Link names](#link-names-not-caller-aliases)).

| Address | Function | Port name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046c6f0` | `vehicle_load` | `lego_vehicle_load_adapter` | 1818 | unmapped | 98.80 |
| `0046ce10` | `vehicle_remove` | `entity_payload_type1_release` | 178 | 10.17 | **100.00** |
| `0046ced0` | `vehicle_swap_poly_medium` | `paired_resource_state_set` | 74 | 42.67 | **100.00** |
| `0046cf20` | `vehicle_swap_poly_high` | `scene_entity_animation_tracks_set_enabled` | 19 | 100.00 | **100.00** |
| `0046cf40` | `vehicle_can_upgrade_type` | `record_flag_bit_test` | 54 | 100.00 | **100.00** |
| `0046cf80` | `vehicle_get_upgrade_level` | `entity_type_mask_get` | 11 | 100.00 | **100.00** |
| `0046cf90` | `vehicle_set_upgrade_level` | `scene_record_indexed_entries_rebuild_original` | 38 | 52.17 | **100.00** |
| `0046cfc0` | `vehicle_hide_wheels` | `six_resource_slots_set_state` | 45 | 95.24 | **100.00** |
| `0046cff0` | `vehicle_clone` | `scene_record_template_instantiate` | 323 | 21.46 | 97.50 |
| `0046d140` | `vehicle_set_owner_object` | `scene_assign_owner_from_model` | 86 | 28.30 | **100.00** |
| `0046d1a0` | `vehicle_get_animation_time` | `scene_object_attached_resource_release` | 17 | 38.10 | **100.00** |
| `0046d1c0` | `vehicle_move_animation` | `animation_duration_for_loop_count` (kept) | 148 | 22.06 | **100.00** |
| `0046d260` | `vehicle_populate_wheel_nulls` | `scene_resource_slots_rebuild` | 93 | 15.19 | **100.00** |
| `0046d2c0` | `vehicle_hide` | `scene_resource_visibility_set` | 108 | 32.79 | **100.00** |
| `0046d330` | `vehicle_is_hidden` | `entity_type_flag_bit1_get_target` | 21 | 100.00 | **100.00** (naked asm) |
| `0046d350` | `vehicle_set_orientation` | `scene_resource_draw_at_origin` | 43 | 25.00 | **100.00** |
| `0046d380` | `vehicle_set_position` | `legoobject_gm_vehicle_set_position_adapter` (removed) | 1611 | unmapped | 70.36 |
| `0046d9e0` | `vehicle_get_activity_container` | `scene_object_resource_get` | 8 | 100.00 | **100.00** |
| `0046d9f0` | `vehicle_find_null` | `scene_resource_named_entry_resolve_with_fallback` (kept) | 93 | 31.33 | **100.00** |
| `0046da50` | `vehicle_get_camera_null` | `scene_activity_change_dispatch` (kept) | 62 | 35.29 | **100.00** |
| `0046da90` | `vehicle_get_drill_null` | `scene_resource_cached_slot_b4_get` | 48 | 55.00 | **100.00** |
| `0046dac0` | `vehicle_get_deposit_null` | `scene_resource_cached_slot_b8_get` | 48 | 55.00 | **100.00** |
| `0046daf0` | `vehicle_get_driver_null` | `scene_resource_cached_slot_c0_get` | 48 | 55.00 | **100.00** |
| `0046db20` | `vehicle_get_carry_null` | `attached_entity_slot_object_get` | 62 | 31.75 | **100.00** |
| `0046db60` | `vehicle_get_carry_null_frames` | `object_record_field11c_get` | 11 | 100.00 | **100.00** |
| `0046db70` | `vehicle_get_trans_coef` | `scene_object_resource_release` (kept) | 17 | 80.00 | **100.00** |

"(kept)" marks the five port names whose only callers are port models with
another contract; the unit keeps its own name for them, and the port tail
serves them in every build.

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `0046c6f0` Vehicle_Load | 98.80 | Mostly call operands that cannot map yet: three Container_Load calls go to `gods98/entity.cpp`'s `container_load` adapter (the port's `00472cd0` has another contract), two MeshLOD_Load calls go to `entity.cpp`'s `mesh_lod_load` adapter, and Upgrade_Load (`0046c160`) goes to the new adapter, since the port has no annotated entry for it. The one code difference is where the inline `memset` of `wheelNulls` schedules its `lea edi`: the three statement orders tried give 98.80, 98.45 and 96.64 |
| `0046cff0` Vehicle_Clone | 97.50 | Operands only: the three Container_EnableSoundTriggers (`00473ec0`) calls go to the new adapter. The port's `00473ec0` takes a third, hook-table argument, which the original's two arguments do not carry |
| `0046d380` Vehicle_SetPosition | 70.36 | The frame (`0xa4` bytes) and the wheel loops match. The rest is x87 scheduling of the plane normals and which stack slot each local gets; see [below](#vehicle_setposition-0046d380) |

No unit function uses the flag `0x800000`, and #608's plain-immediate check
passes.

## The unit's edges

The unit map gives GAME-048 a low-confidence boundary, and nine functions
(`0046d9e0`-`0046db70`) are marked "boundary:GAME-049". All nine belong to
Vehicle.c, and the unit ends at `0046db90`:

- **Code.** Each of the nine reads the vehicle model's own fields at the
  offsets the high-confidence functions write:
  - `+0x08`/`+0x0c`, the activity containers (`0046d9e0`, `0046d9f0`, `0046db70`);
  - the null names `+0xb4`, `+0xb8`, `+0xc0`, `+0xdc`, `+0xe0`, which
    Vehicle_Load fills, and their caches `+0xcc`, `+0xd0`, `+0xd4`, `+0xe4`, `+0x10c`;
  - `+0x11c`, CarryNullFrames.

  `0046da50`-`0046db20` all call `0046d9f0`.
- **The next function.** `0046db90` is GAME-049's first cell-flood pass. It
  works on the map's commons (`00549520`-`0054bf08`) and has its own
  `.rdata` (`0049e7f8`).
- **Data.** Vehicle.c's `.rdata` (`0049e7e4`-`0049e7f0`: `0.0f`, `0.5f`,
  `1.0f`, `0.4f`) is used only by `0046c6f0`, `0046d1c0` and `0046d380`. Its
  own strings (`004aa9f8`-`004aaa2c`) are used only by Vehicle_Load.

At the other edge, GAME-047's four low-confidence functions
(`0046c340`-`0046c510`, "boundary:GAME-048") work on Upgrade.c's model and are
not Vehicle.c's: Vehicle_SetUpgradeLevel and Vehicle_Clone call them as
callees. GAME-047 ends at `0046c6f0`. No function is left out.

## How the conversion is done

### The port tail

The port's vehicle code is another contract:

- its own `ScenePairedAnimationState` and `SceneResourceAnimationRecord`
  records, not the original's 0x1ec-byte VehicleModel;
- resource hooks with contexts, NULL guards and bounds.

Native code (`legoobject.cpp`, `upgrade_levels.cpp`, `record.cpp`,
`message_unit.cpp`) and `test_record.cpp` use it. As in findings 570, 582 and
585, `vehicle_model.cpp` is renamed `vehicle_model_port.cpp` and becomes a
`tail` entry. Each `// FUNCTION` becomes a `/* Port model of 0x… */` comment.

### Link names, not caller aliases

As in findings 581 and 585, `vehicle_model_link_names.h` renames 21 of the
unit's definitions to the port's names, under `#if defined(_MSC_VER) &&
_MSC_VER < 1200`. Only `vehicle_model.c` includes it, first. In the matching
build every caller of these names passes a vehicle model of the original's
layout:

- `lego_unit.c`'s vehicle table, which Lego.c loads through Vehicle_Load;
- LegoObject's `+0x00c`, which `legoobject_unit.c` fills with Vehicle_Clone.

The C++ callers (`legoobject.cpp`, `message_unit.cpp`, `record.cpp`,
`upgrade_levels.cpp`) pass the same LegoObject `+0x00c` pointer. So the C
callers (`lego_unit.c`, `legoobject_unit.c`) and the C++ ones reach the unit
with no caller edit and no new include. Two of the names are original-ABI
adapters that did nothing:

- `lego_vehicle_load_adapter` (`lego_port.cpp`, for `0046c6f0`);
- `scene_record_indexed_entries_rebuild_original` (the port tail, for `0046cf90`).

`vehicle_model_port.cpp` compiles its definitions of the 21 names out of the
matching build behind `LRR_VEHICLE_MODEL_PORT_NAMES`, as `lego_port.cpp` does
for its adapter. Natively the port tail keeps serving them. MSVC 5.0 does not
evaluate `defined` inside a macro expansion: the first form,
`#define LRR_VEHICLE_MODEL_PORT_NAMES (!(defined(_MSC_VER) && …))`, linked
both definitions (LNK2005). The macro is now set by an explicit `#if`.

Five port names keep their port definition in every build, because their only
callers are `legoobject.cpp` port models with other contracts:

- `animation_duration_for_loop_count` (`0046d1c0`);
- `scene_resource_named_entry_resolve_with_fallback` (`0046d9f0`);
- `scene_activity_change_dispatch` (`0046da50`);
- `scene_object_resource_release` (`0046db70`);
- the record-list form of `0046cf90`.

The unit keeps its own names for those functions.

`extern "C"` is added in place to six declarations, so C++ callers and the
port's definitions agree on linkage. No declaration is added:

- `scene_resource_cached_slot_b4_get` and `scene_resource_slots_rebuild` (`resource.hpp`);
- `scene_indexed_record_list_rebuild_for_value` and `scene_channel_slots_rebuild`
  (`resource.hpp`; Upgrade.c's `0046c340` and `0046c510`, which the unit calls);
- `entity_type_flag_bit1_get_target` (`record.hpp`);
- `scene_object_bounds_union` (`view.hpp`, Container_GetBox).

### Vehicle_SetPosition (`0046d380`)

#610 called `0046d380` through the `PORT-ONLY` adapter
`legoobject_gm_vehicle_set_position_adapter` in `legoobject.cpp`. It set only
the activity container's position at the height the callback gives, without
the original's tilt to the wheel plane, wheel drop and wheel turn. The unit's
`vehicle_set_position` is the original function with the same contract:
vehicle, x, y, height callback, map. So:

- `legoobject_unit.c`'s four callers (`004425f0`, `004428a0`, `00442b10`,
  `00442f80`) call `vehicle_set_position` directly. The edit is a rename
  plus a `(struct VehicleModel*)` cast, which C++ needs natively.
- `legoobject_unit_imports.h` replaces the adapter's declaration with the
  unit's, as `vehicle_model.h` declares it (`test_c_boundaries.cpp` includes
  both). It gains one `struct VehicleModel;` tag.
- The adapter is deleted from `legoobject.cpp`. That is the only edit there.

Natively `legoobject_unit.c`'s functions are reached only through
matching-build aliases, so these callers never run, and native behaviour is
unchanged. In the matching build the four callers rose (99.52% to **100%**,
98.40% to 98.94%, 85.60% to 85.86%, 73.26% to 73.47%). A single-function run
of all 191 `legoobject_unit.c` functions before and after the edit shows the
counter shift moved nothing: no function fell, and none changed size.

The function itself is at 70.36%. The frame (`0xa4` bytes) and its outer
structure match. `/O2` orders the stack slots by the locals' use weights and
declarations (finding 581). The original has one scratch float, at `+0x28`
between `vecA` and `dir`. It holds the normalise factor and the wheel turn,
and the dot product stays in an x87 register. A batch search compiled about
1,500 declaration orders and source shapes and read each layout from the
`/FAs` listing:

- separate, shared and block-scoped scalars;
- the dot product as a local, a `double` and an inline argument;
- the newUp normalise inline or explicit.

The best layouts get 9 of the 10 slots, never the scalar's. It lands before
`vecA` in every layout that keeps the projection. What raised the score:

| Change | Score |
| --- | ---: |
| First compile | 55.65 |
| `vecA` reused for the wheel delta; one scratch float; declaration order from the search | 66.11 |
| Each normal as `cross(subtract(…), subtract(…))`, as Maths.h's calls compose | 68.13 |
| The 0.4 blend as Maths_RayEndPoint (`newUp + 0.4 * lastUp`) | **70.36** |

The cross product is gods98's left-handed form, `(a.z * b.y - a.y * b.z, …)`.
Two forms were tried and not kept:

- The height callback's argument evaluated before the trailing `0.0f` push
  needs finding 587's comma expression, `(0, zCallback(…))`. That is +0.5
  points, not worth a `Deviation (scoring only)`. In the wheel loop it
  changes nothing.
- The permuter has no candidates for struct locals.

Original behaviour kept:

- The first loop gives every wheel null the terrain height. The plane
  normals then assume four wheels (`wheelPos[0..3]`).
- The `_finite` guard on `lastUp` tests `y` twice and never `z`.

### State

The unit has no `.bss`. VehicleModel (0x1ec bytes) is declared at the
original's offsets:

- the containers `+0x08`/`+0x0c`;
- the wheels, wheel nulls and count `+0x10`-`+0x40`;
- `lastUp` `+0x5c`, the wheel radius `+0x68` and the last wheel positions `+0x6c`;
- the null names and caches `+0xb4`-`+0x120`;
- Upgrade.c's model `+0x124` (current level `+0x12c`, the levels mask `+0x130`);
- the weapons block `+0x138`-`+0x1e0` (`weapons.c`'s `Weapons_Model`),
  with PivotMaxZ/PivotMinZ at `+0x168`/`+0x16c`;
- the two MeshLODs `+0x1e0`/`+0x1e4`, and the flags `+0x1e8`.

The `.rdata` constants are literals.

Strings:

- The unit's own five (`004aa9f8`-`004aaa2c`) are marked `// STRING` where
  they are used.
- Six literals the original pools with GAME-006 (Building.c) had no marker
  anywhere, and are marked here at their first use: `CarryNullFrames`,
  `FireNullName`, `yPivot`, `xPivot`, `PivotMaxZ`, `PivotMinZ`. Unmarked, the
  original side shows raw immediates while the rebuild names its literal
  (finding 581): Vehicle_Load went from 83.72% to 94.14% with the markers (and
  `LWO`'s) and a fixed frame. GAME-006's conversion must not mark them again.
- `LWO` (`004a1d5c`) is #620's `rock_fall_styles.c`'s marker. This change
  first marked it too; on the stack Reccmp drops the duplicate, so the unit
  leaves it unmarked and relies on #620's.
- The `CameraNull*`, `ACT`, `ae` and path literals stay `animation_object.c`'s
  and `files.c`'s. `""` is `config.c`'s `s_config_angle_empty`.

`make unit-data-offsets`, GAME-048 row: `.data` partial (5 strings, 0
unknown), one gap at `004aaa2c` (`DriverNullName`, offset `0x6c` against the
original's `0x34`). The unit keeps its own copies of literals that the
original pools with GAME-004 and GAME-006. `.rdata` and `.bss` are n/a.

### Source shapes that matched

- **The cached-null getters** (`0046da50`-`0046db20`, 55% → 100%). The
  original returns the call's result instead of reloading the cache:
  `if (cache) return cache; return cache = vehicle_find_null(…);`.
- **Vehicle_MoveAnimation** (`0046d1c0`, 84.91% → 100%). The original keeps
  the first animation result on the x87 stack across the second `if` (Ghidra's
  `float10`). The source matches with a `double` intermediate, assigned from a
  conditional expression so the `0.0f` arm stays a `dword` constant
  (`result = !(flags & 4) ? Container_MoveAnimation(…) : 0.0f;`). The overrun
  test is `0.0f != overrun`.
- **Vehicle_Load.** Three shapes mattered:
  - `0.0f == (vehicle->wheelRadius = (float)atof(…))` gives the original's
    rounding temporary and frame (`0x11d4`);
  - zeroing `contWheels[1..5]` in a five-iteration loop, which MSVC 5.0
    unrolls into five stores (`memset` gives 96.14%);
  - the string markers above.
- **Vehicle_IsHidden** (`0046d330`) stays naked asm under `_M_IX86`, with the
  C form in `#else`. The original masks the flags dword with `and eax, 0xff`
  before `>> 1 & 1`, and no C form keeps that redundant mask. The 12 tried:
  - `(flags & 0xff) >> 1 & 1`, with a local and as a division;
  - a signed mask;
  - `& 2` then `>> 1`, `!= 0`, `== 2` and a conditional;
  - an 8-bit bitfield view;
  - `(unsigned char)` casts, which make a byte load.

  Container.c's `00473ba0` (`>> 3`) has the same shape and is naked asm too.

### Callees

- **Existing original-ABI adapters**: `gods98/entity.cpp`'s `container_load`
  (Container_Load) and `entity.cpp`'s `mesh_lod_load` (MeshLOD_Load).
- **Two new `PORT-ONLY` adapters** at the end of `vehicle_model_port.cpp`:
  - `vehicle_upgrade_load_adapter` for Upgrade_Load (`0046c160`), which has no
    annotated port entry with its contract (the port's is a hook-driven ledger
    stub); it loads nothing;
  - `vehicle_container_enable_sound_triggers_adapter` for `00473ec0`, which
    calls the port's entry with no hook table, so it does nothing, safely.

  Natively nothing calls the unit, so neither runs there.
- Maths_Vector3DScale is called out of line (Advisor.c's `00401720`,
  `vec_scale`) from inside the inline normalise, as the original does. The
  other Maths.h helpers are `static __inline` in `vehicle_model_maths.h`.

## Entropy and drops

`make reccmp-check` against #620's baseline: 28 improved and 2 newly mapped,
**none below baseline**, no exact match lost. No function outside the unit
changed its code: only call or data operands now map. No instruction count
changed outside the unit. No `Deviation (scoring only)` is added.

Ten functions outside the unit improved, all through the link names, the
string markers or the SetPosition switch (on `main`, `00422850` and `004519b0`
also rose; on #620 its `LWO` marker already gives them that):

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `004288e0` | `lego_level_debug_input_cell_mutator_dispatch` | 97.05 | 97.20 |
| `0042cda0` | `lego_entity_vehicle_type_table_load` | 92.78 | 93.47 |
| `00437d00` | `legoobject_create` | 99.04 | 99.28 |
| `00439c80` | `legoobject_indexed_flag_set_and_refresh` | 96.77 | **100.00** |
| `0043bc40` | `legoobject_objective_sidecar_append_gate` | 98.88 | 99.11 |
| `0043c200` | `legoobject_state_pair_predicate` | 88.37 | **100.00** |
| `004425f0` | `legoobject_subobject_pose_delta_normalize_and_dispatch` | 99.52 | **100.00** |
| `004428a0` | `legoobject_world_position_set` | 98.40 | 98.94 |
| `00442b10` | `legoobject_movement_accumulator_apply` | 85.60 | 85.86 |
| `00442f80` | `legoobject_pose_orientation_dispatch` | 73.26 | 73.47 |

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address), on #620 `9a6cd741` | 577 / 278 | **571 / 280** |
| `opt_ref.annotated.game_only` (by address), on #620 | 560 / 267 | 554 / 269 |
| `opt_ref.annotated.full` (by address), on `main` `984a1213` | 585 / 275 | 582 / 280 |
| By symbol (`callers --min 1`), on `main` | 570 / 261 | 565 / 264 |
| GAME-048's object | `vehicle_model.cpp.obj` | `vehicle_model.c.obj`: 3 removed (3 exact) |

The by-symbol comparison was made on `main`, rebuilt once into a scratch
build directory (on #620 the change gives 554 / 264 by symbol). By symbol, five functions are newly kept by `/OPT:REF`:

- Upgrade.c's `0046c340` and `0046c510`;
- Container.c's `00473ec0` and `00474f10`;
- the port's record-list `0046cf90`.

Nothing is newly removed. The three more exact removed are the unit's
`0046d1c0`, `0046da50` and `0046db70`. They became exact here, and their only
original callers (`00447b30`, `00447c40`, `0044a070`, `00446610`) are
`legoobject.cpp` port models with other contracts (finding 579's class (c)).
They are kept when LegoObject.c part 3 is converted.

The by-address count is the one `binary-progress.json` records. It also moves
seven exact rows that `/OPT:REF` keeps in both relinks
(`engine_memory_allocate`, `ledger_stub_00472a40_…`, `rng_thunk_draw15`,
`file_malloc` and others). That is finding 579's caveat about aliases at
shared addresses, so the by-symbol figures are the change's effect.

## Files outside the unit

| File | Change |
| --- | --- |
| `src/game/legoobject_unit.c` (#615) | the four `0046d380` callers call `vehicle_set_position` (rename and cast) |
| `src/game/legoobject_unit_imports.h` | the adapter's declaration becomes the unit's, plus `struct VehicleModel;` |
| `src/game/legoobject.cpp` (#615) | the `legoobject_gm_vehicle_set_position_adapter` definition is deleted; nothing else |
| `src/game/lego_port.cpp` | `lego_vehicle_load_adapter` is compiled out of the matching build |
| `src/gods98/resource.hpp`, `record.hpp`, `view.hpp` | `extern "C"` in place on six declarations |
| `src/game/test_c_boundaries.cpp` | includes `vehicle_model.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-048 game/vehicle_model.c`; `tail game/vehicle_model_port.cpp` |

**Files other threads own.** `legoobject_unit.c` and `legoobject.cpp`
(#615, which renames `legoobject.cpp` to `legoobject_port.cpp`) get only the
caller switch and the adapter's deletion above. `legoobject_port_aliases.h`
(#615), `rock_fall_styles.cpp` (#616) and `gods98/wad.cpp` (#618) are not
edited. On rebase over #615, the adapter's deletion moves to
`legoobject_port.cpp`.

## Native behaviour

Unchanged:

- The port tail serves every port name natively, and none of its
  definitions changed.
- The unit compiles natively as C++ under its own names. Its one caller
  there, `legoobject_unit.c`, is not reached natively.
- `lego_port.cpp`'s adapter is still compiled natively.

There is no new native gap.

## Validation

On #620's head (`9a6cd741`), after the rebase:

- `make reccmp-check`: OK. None below baseline and no exact match lost. The
  plain-immediate check (finding 584), `winapi-link-check` and the link-order
  map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`),
  `make unit-data-offsets`, `python3 tools/translation_units.py build` and
  `tools/binary_progress_hash.py --write-manifest` were run, then
  `make match-progress` again, so all three files carry the final input hash.
- `make tools-test`, without `--allow-growth`.
- The lint ledger is unchanged at 1,314. The relocated file had no
  unannotated definitions to retire, unlike #611. The port tail and the
  `PORT-ONLY` adapters are exempt, so nothing is added.
- Native build and `ctest` pass (37/37, synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `vehicle_model.c`: clean.

## Follow-ups

- **LegoObject.c part 3** (`legoobject.cpp`'s `00446610`, `00447b30`,
  `00447c40`, `0044a070`): once these call the unit's `0046d1c0`,
  `0046da50` and `0046db70`, `/OPT:REF` keeps them.
- **Upgrade.c (GAME-047).** Its conversion can give `0046c160` its original
  contract and replace `vehicle_upgrade_load_adapter`.
- **Container.c (G98-002).** Its conversion can replace the
  `container_load` and `00473ec0` adapters, which removes the remaining
  operand differences in Vehicle_Load and Vehicle_Clone.
- **GAME-006** must not mark the six pooled strings again.
- **Vehicle_SetPosition's** remaining gap is the scratch float's slot and
  x87 scheduling. The batch slot search (compile many declaration orders in
  one `msvc.compile_many` call and read the `$` offsets from each `/FAs`
  listing) was a scratch script and is not committed; a tool for it would
  help other `/O2` units with large frames.

## Reproducing

    git mv src/game/vehicle_model.cpp src/game/vehicle_model_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-048 --stem vehicle_model
    make match-function SOURCE=src/game/vehicle_model.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object vehicle_model.c.obj"
    make match-progress
    make unit-data-offsets
