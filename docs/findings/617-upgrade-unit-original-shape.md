# Finding 617 — Upgrade.c (GAME-047) rebuilt in original shape; Vehicle.c starts at `0046c3d0`

Issue #674, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-047 is the original's `Upgrade.c`: the upgrade parts. An upgrade part
is one model (a mesh or an activity container) that an upgrade adds to a
vehicle or building. An upgrades model (a vehicle's `+0x124`, a building's
`+0x8c`) lists the parts each upgrade level shows:

- Upgrade_Load reads them from the object's `Upgrades::LevelWXYZ` arrays,
  one `null,frame[,weapon]` item a part.
- Upgrade_SetUpgradeLevel replaces the shown parts with one upgrade-part
  LegoObject per part of the new level.

The unit map listed 10 functions (`0046c030`-`0046c6f0`): 7 low-confidence,
and one, `0046c160`, with no source. **Upgrade.c is 7 functions,
`0046c030`-`0046c3d0`.** The other three (`0046c3d0`-`0046c510`) set a
vehicle's activity and hang its upgrade parts on their nulls. They are
Vehicle.c's (GAME-048) first three functions (see [Unit extent](#unit-extent)).

This change:

- rebuilds Upgrade.c as the C unit [`src/game/upgrade.c`](../../src/game/upgrade.c),
  with [`upgrade.h`](../../src/game/upgrade.h),
  [`upgrade_imports.h`](../../src/game/upgrade_imports.h) and the
  matching-build link names
  [`upgrade_link_names.h`](../../src/game/upgrade_link_names.h);
- adds the three Vehicle.c functions to
  [`vehicle_model.c`](../../src/game/vehicle_model.c) (finding 590's unit),
  before Vehicle_Load;
- moves the port's models, unannotated, to the port tail
  [`upgrade_levels_port.cpp`](../../src/game/upgrade_levels_port.cpp). That
  file is `upgrade_levels.cpp` under `git mv`, without the five part
  functions, which were already in original shape (finding 597).

"Before" is `main` `8abe8b6b` (the committed match baseline, and a full
matching build of it in this worktree). "After" is this change, from the
full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Upgrade.c similarity (7 functions, byte-weighted, 869 code bytes) | 32.94% | **88.67%** |
| Exact functions in Upgrade.c | 4 | **5** |
| Mapped functions in Upgrade.c | 6 | **7** |
| Vehicle.c's three new functions: exact | 1 | **3** |
| Exact-function code bytes (`binary-progress.json`) | 196,138 | **197,873** |
| Exact functions, full / game-only | 1,686 / 1,477 | **1,691 / 1,482** |
| `opt_ref` annotated (full, by address): removed / exact removed | 428 / 207 | **425 / 207** |
| Reccmp lint ledger (entries) | 1,080 | **1,078** |

Byte weights are the original sizes from the single-function loop. An
unmapped function counts as 0%.

The names are OpenLRR's, in the repository's snake case, and are
`inferred`: the binary attests only the config keys `Upgrades` and
`Level%i%i%i%i`. "Link name" is the name the definition takes in the
matching build (see [Link names](#link-names)).

| Address | Function | Unit | Link name | Bytes | Before | After |
| --- | --- | --- | --- | ---: | ---: | ---: |
| `0046c030` | `upgrade_part_load` | Upgrade.c | `render_resource_binding_initialize` | 118 | 91.11 | 91.11 |
| `0046c0b0` | `upgrade_part_clone` | Upgrade.c | `render_resource_binding_copy_and_convert` | 54 | 100.00 | **100.00** |
| `0046c0f0` | `upgrade_part_remove` | Upgrade.c | `render_resource_binding_release` | 17 | 100.00 | **100.00** |
| `0046c110` | `upgrade_part_move_animation` | Upgrade.c | (its own) | 30 | 100.00 | **100.00** |
| `0046c130` | `upgrade_part_set_activity` | Upgrade.c | `scene_object_set_animation_with_fallback` | 46 | 100.00 | **100.00** |
| `0046c160` | `upgrade_load` | Upgrade.c | (its own) | 469 | unmapped | 81.25 |
| `0046c340` | `upgrade_set_upgrade_level` | Upgrade.c | `scene_indexed_record_list_rebuild_for_value` | 135 | 23.53 | **100.00** |
| `0046c3d0` | `vehicle_is_camera_flip_dir` | Vehicle.c | (its own) | 17 | 100.00 | **100.00** |
| `0046c3f0` | `vehicle_set_activity` | Vehicle.c | (its own) | 279 | 22.59 | **100.00** |
| `0046c510` | `vehicle_set_upgrade_activity` | Vehicle.c | `scene_channel_slots_rebuild` | 469 | 7.92 | **100.00** |

`0046c030`-`0046c130` were already in original shape (finding 597). They
move from `upgrade_levels.cpp` to `upgrade.c` unchanged, except that the
part model is now the original's `Upgrade_PartModel`, not the port's
`RenderResourceBinding`. `0046c3d0` was exact before as the port's
`record_field_d8_equals_one`, which stays in the port tail.

Five functions outside the unit rose with no loss:

| Address | Function | Before | After | Why |
| --- | --- | ---: | ---: | --- |
| `004083a0` | `building_set_upgrade_activity` (Building.c) | 66.88 | **100.00** | the activity-name local found for `0046c510` (below) |
| `0044a070` | `legoobject_follow_target_pose_get` (LegoObject.c) | 99.31 | **100.00** | calls `vehicle_is_camera_flip_dir` instead of an adapter |
| `00447c40` | `legoobject_activity_register_bookkeeping` (LegoObject.c) | 96.28 | 96.85 | calls `vehicle_set_activity` instead of an adapter |
| `0046c6f0` | `vehicle_load` (Vehicle.c) | 98.80 | 98.97 | calls `upgrade_load` instead of an adapter |
| `00407d60` | `building_load` (Building.c) | 99.35 | 99.57 | calls `upgrade_load` instead of an adapter |

### What is left

- **`0046c160` Upgrade_Load (81.25%).** Every instruction lines up, but
  the registers differ. The original keeps the upgrades model in `esi` and
  the level in `edi`, and stores `count = 0` as an immediate. The rebuild
  keeps the model in `ebp` and the level in `esi`, and the zero for `count`
  is the level's register. These shapes were tried:
  - the order of the five locals (all 120 permutations);
  - `count = 0` after the allocation;
  - `memset(parts = malloc(...))`;
  - a block-scoped part pointer;
  - `while` for the inner loop;
  - indexing `parts[count]` throughout, `parts + count` and `++count`;
  - `(loop & 8) >> 3` for the level digits;
  - assignment inside the `if`;
  - removing the Vehicle.c layout and headers from the file.

  They all give 81.25%, except indexing `parts[count++]`, which gives
  88.82% but increments `count` before the stores, where the original
  increments it last. So this is finding 538's rule B (the counter's
  phase), not a source shape. `tools/msvc_permuter.py` (240 s) found no
  rewrite.
- **`0046c030` Upgrade_Part_Load (91.11%).** Unchanged: its three
  Container_Load calls go to `gods98/entity.cpp`'s `container_load` adapter
  (finding 590).

No naked asm, and no `Deviation (scoring only)`.

## Unit extent

The issue asked for the extent to be confirmed (as findings 591, 599, 603
and 611). Finding 590 had ended GAME-047 at `0046c6f0`, because
`0046c3d0`-`0046c510` "work on Upgrade.c's model".

### Upgrade.c: `0046c030`-`0046c3d0`

- **The start** is finding 597's: `0046c110` loads its own `0.0f`
  (`0049e7dc`), not ToolTip.c's (`0049e7cc`), and MSVC 5.0 pools a literal
  once an object.
- **The strings.** `0046c160` is the only user of `Upgrades` (`004aa9dc`)
  and `Level%i%i%i%i` (`004aa9e8`). These two strings sit between ToolTip.c's
  last string (`ToolTip_RadarBlock`) and Vehicle.c's first (`HoldMissing`,
  `004aa9f8`).
- **The data.** `0046c160` and `0046c340` work on the 0x14-byte upgrades
  model (`parts`, `count`, `currentLevel`, `levels`, `objectList`). The part
  functions work on the 0x10-byte part model (`objID`, `cont`,
  `nextObject`, `partInfo`). No function in `0046c030`-`0046c6f0` touches
  `.data` or `.bss`. The only global read is AI.c's activity names
  (`004decc4`).

### Vehicle.c: from `0046c3d0`, not `0046c6f0`

None of `0046c3d0`, `0046c3f0` or `0046c510` reads a literal, a string or
an owned global, so the data cannot place the boundary. The code and the
call graph do:

- **Their record is the vehicle model.** All three take the 0x1ec-byte
  VehicleModel. `0046c3d0` reads `cameraFlipDir` (`+0xd8`). `0046c3f0`
  clears the null caches (`+0xcc`-`+0xd4`, `+0xe4`, `+0x10c`), sets the
  activity on `contAct1`/`contAct2` (`+0x08`/`+0x0c`) and tests `flags`
  (`+0x1e8`, `HOLDMISSING`). `0046c510` reads the upgrades model only
  through the vehicle (`+0x134`). It writes the vehicle's weapon nulls
  (`+0x138`-`+0x1dc`, weapons.c's Weapons_Model) from the vehicle's null
  names (`+0xbc`, `+0xc4`, `+0xc8`).
- **Their callees are Vehicle.c's.** `0046c3f0` calls
  Vehicle_PopulateWheelNulls (`0046d260`), and `0046c510` calls
  Vehicle_FindNull (`0046d9f0`). Upgrade.c's own functions call no Vehicle.c
  function.
- **Building.c holds the same functions.** Building_SetUpgradeActivity
  (`004083a0`) is `0046c510`'s code over the building model: the same
  clearing loop, the same weapon-null search and the same activity-name
  local. Building_SetActivity (`00408570`) is `0046c3f0`'s counterpart.
  Building_SetUpgradeLevel (`004089a0`) calls Upgrade_SetUpgradeLevel and
  then Building_SetUpgradeActivity, as Vehicle_SetUpgradeLevel (`0046cf90`)
  calls Upgrade_SetUpgradeLevel and then `0046c510`. All three Building.c
  functions are in Building.c.

The three functions were compiled at the top of `vehicle_model.c` and at
the end of `upgrade.c`. Every score was the same either way, and no
`vehicle_model.c` function moved. So the match cannot decide; the
evidence above does. Vehicle.c then opens with its three activity
functions, before Vehicle_Load.

One `.rdata` item stays unexplained. The `1.0f` at `0049e7e0` sits between
Upgrade.c's `0.0f` (`0049e7dc`) and Vehicle.c's pool (`0049e7e4`-`0049e7f0`:
`0.0f`, `0.5f`, `1.0f`, `0.4f`). No code reads it. It cannot be Vehicle.c's,
whose pool has its own `1.0f` (`0049e7ec`, read by Vehicle_SetPosition).
None of the 10 functions stores `0x3f800000` as an immediate. Vehicle_Load
(`0046ca96`) and Vehicle_Clone (`0046d06b`) do. So the item says nothing
about `0046c3d0`-`0046c510`, whichever object it belongs to.

`translation-units.spec.csv`:

- GAME-047 becomes `Upgrade.c` (`inferred`), `0046c030`-`0046c3d0`, 7
  functions.
- GAME-048 becomes `Vehicle.c` (`inferred`) and starts at `0046c3d0`, 29
  functions.

`python3 tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

### Records

`upgrade.c` declares the original's records: `Upgrade_PartInfo` (0x14
bytes: `level`, `nullFrameNo`, `nullName`, `weaponName`, `upgradeData`),
`Upgrade_Model` (0x14 bytes) and `Upgrade_PartModel` (0x10 bytes), plus the
LegoObject's `upgradePart` (`+0x20`). `vehicle_model.c`'s VehicleModel
gains the full upgrades model and the weapon nulls it writes (`+0x138`
`fireNullPairs[3][2]`, `+0x150` `xPivotNulls`, `+0x15c` `yPivotNulls`,
`+0x170` `fireNullPairFrames`, `+0x1c4` `weaponParts`, `+0x1dc`
`weaponCount`) in place of padding. weapons.c's Weapons_Model has the same
layout.

### Link names

In the matching build, `upgrade_link_names.h` gives five unit functions the
port's names:

| Function | Link name | Matching-build callers |
| --- | --- | --- |
| `0046c030` | `render_resource_binding_initialize` | `lego_unit.c` (the upgrade types) |
| `0046c0b0` | `render_resource_binding_copy_and_convert` | `legoobject_unit.c` |
| `0046c0f0` | `render_resource_binding_release` | `lego_unit.c`, `legoobject_unit.c` |
| `0046c130` | `scene_object_set_animation_with_fallback` | `building_model.c`, `vehicle_model.c` |
| `0046c340` | `scene_indexed_record_list_rebuild_for_value` | `building_model.c`, `vehicle_model.c` |

`vehicle_model_link_names.h` gives `0046c510` the name
`scene_channel_slots_rebuild`. Every caller of those names passes the
original's records, so no caller edit is needed. `vehicle_model.c` also
includes `upgrade_link_names.h`, so it calls Upgrade.c by the unit's names.

`tooltip_port.cpp` (part functions) and `upgrade_levels_port.cpp` (`0046c340`,
`0046c510`) compile their definitions of these names out of the matching
build. Natively they still serve them. `0046c110`, `0046c160`, `0046c3d0` and
`0046c3f0` keep the unit's names.

### Callers switched

| Caller | Before | After |
| --- | --- | --- |
| `vehicle_model.c` Vehicle_Load, `building_model.c` Building_Load | `vehicle_upgrade_load_adapter` (`PORT-ONLY`, loaded nothing) | `upgrade_load` |
| `vehicle_model.c` Vehicle_SetUpgradeLevel | `scene_channel_slots_rebuild` with the port's record type | `vehicle_set_upgrade_activity(vehicle, NULL)` |
| `vehicle_model.c` Vehicle_SetUpgradeLevel | `scene_indexed_record_list_rebuild_for_value` | `upgrade_set_upgrade_level` (same link name) |
| `legoobject_unit.c` `00447c40` | `legoobject_gq_vehicle_set_activity_adapter` (`PORT-ONLY`, returned 0) | `vehicle_set_activity` |
| `legoobject_unit.c` `0044a070` | `legoobject_gs_vehicle_camera_flip_adapter` (`PORT-ONLY`) | `vehicle_is_camera_flip_dir` |

The three adapters are deleted from `vehicle_model_port.cpp` and
`legoobject_port.cpp`. `lego_unit.c` needs no edit: it already calls the
part functions by their link names. `legoobject_unit.c`,
`vehicle_model.c` and `building_model.c` have no owner.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| Container_Load `00472cd0` | `gods98/entity.h`'s `container_load` adapter | not converted (finding 590) |
| Container_Clone, Container_Remove, Container_SetActivity, Container_FormatPartName, Container_SearchTree, Container_SetAnimationTime, Container_SetParent, Container_SetPosition-at-origin | `gods98/container.h`, `texture.h`, `view.h` and the units' imports headers | G98-002 |
| Container_IsCurrentActivity `00473340` | `ledger_stub_00473340_SceneActionNamePresent`, now `extern "C"` in `container_unit.cpp` | G98-002 |
| Config_BuildStringID, Config_FindArray, Config_GetNextItem | `gods98/config.h` | G98-006 |
| Util_Tokenise, Util_StrCpy | `gods98/utils.h` | G98-004 |
| Lego_GetUpgradeData's lookup `0042cd50` | `lego_upgrade_name_entry_find` | Lego.c |
| LegoObject_Create, LegoObject_Remove | `legoobject_create`, `legoobject_remove` | LegoObject.c |
| MeshLOD_RemoveTargets `00451c30` | `linked_record_field4_clear` (`gods98/model.h`) | GAME-026 |
| `sprintf`, `atoi`, `malloc`, `free`, `strcpy`, `strlen`, `memset` | `crt/vc5names.h`, `<stdio.h>`, `<stdlib.h>`, `<string.h>` | LIBC |

### Source shapes that matched

- **Vehicle_SetUpgradeActivity** (`0046c510`, 62.30% at first):
  - `const char* standName = s_activity[0];` in a block for the fallback
    activity (95.74%). Without the local, MSVC loads both arguments before
    the pushes. Upgrade_Part_SetActivity (`0046c130`) already used it.
  - The fire-null pair cleared by an inner `for` over its two slots
    (100%). Two stores, a chained store or a pair pointer keep `ecx` as the
    induction pointer instead of copying it to `ebp`.
- **Building_SetUpgradeActivity** (`004083a0`, 66.88% to 100%): the same
  `standName` local. Its `memset` of the pair gives the same code as the
  inner loop, so it stays.
- **Upgrade_Load** (`0046c160`): `arrayItem = …; if (arrayItem) levels |=
  loop; for (; arrayItem; …)` gives the original's second test of
  `arrayItem` (79.61% to 81.25%). `if ((arrayItem = …)) { levels |= loop;
  for … }` folds it away.
- **Upgrade_SetUpgradeLevel** (`0046c340`) and **Vehicle_SetActivity**
  (`0046c3f0`) matched as first written from the pseudocode.

## Entropy and drops

`make reccmp-check` against the baseline at `8abe8b6b`:

- 8 improved: `0046c340`, `0046c3f0`, `0046c510`, and the five callers above;
- 1 newly mapped: `0046c160`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at these addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004083a0` | 154 | 154 |
| `0046c160` | — | 160 |
| `0046c340` | 69 | 58 |
| `0046c3f0` | 131 | 99 |
| `0046c510` | 57 | 154 |

`004083a0`'s count is unchanged; only its listing digest changed (the
`standName` load). The callers that now call the unit (`00407d60`,
`00447c40`, `0044a070`, `0046c6f0`) changed call operands only and keep
their listings.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`8abe8b6b`, rebuilt here) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 428 / 207 | 425 / 207 |
| `opt_ref.annotated.game_only` (by address) | 421 / 200 | 418 / 200 |

Before, `/OPT:REF` removed `upgrade_levels.cpp.obj`'s `0046c3f0`: its only
original caller, `00447c40`, called `legoobject_gq_vehicle_set_activity_adapter`
instead. `make opt-ref-callers OPT_REF_ARGS="--object upgrade.c.obj --object
vehicle_model.c.obj --object building_model.c.obj --min 1"` now lists no
removed function in any of the three objects.

## Reccmp lint ratchet

1,080 → 1,078 entries (−2, no growth). The relocated port models were
already annotated, and their tail is exempt. Two upgrade-related port
helpers with no original function are now marked `PORT-ONLY`:
`level.cpp`'s `upgrade_resource_check_current` and `frameloop.cpp`'s
`frame_ui_action_building_upgrade_commit`. The three deleted adapters were
already `PORT-ONLY`.

## Files outside the unit

The change edits no file owned by #670 (`crt.cpp`), #671
(`cell_flood_passes.cpp`, `view.cpp`), #672 (`tile_variants.cpp`) or #673
(`gods98/com_object_tree.cpp`). No frozen `_for_` or `_from_` header is
edited. Outside `upgrade.c` and its headers, the change edits:

- `vehicle_model.c`, `vehicle_model.h`, `vehicle_model_imports.h`,
  `vehicle_model_link_names.h`: the three Vehicle.c functions, the records
  and the calls to Upgrade.c;
- `vehicle_model_port.cpp`, `legoobject_port.cpp`: the three adapters
  deleted;
- `building_model.c`, `building_model_imports.h`: `upgrade_load`, and
  Building_SetUpgradeActivity's `standName` local;
- `legoobject_unit.c`: the two direct calls, and comments;
- `gods98/container_unit.cpp`: `extern "C"` on `00473340`'s ledger stub;
- `tooltip.c`, `tooltip_port.cpp`: comments;
- `level.cpp`, `gods98/frameloop.cpp`: the two `PORT-ONLY` markers;
- `test_c_boundaries.cpp`: `upgrade.h`, `upgrade_imports.h` and
  `vehicle_model_imports.h`;
- `link_order.cmake` and `src/game/meson.build`: the unit and the tail;
- `translation-units.spec.csv` and its generated files.

## Native behaviour

Native behaviour is unchanged, and there is no new native gap. Natively the
port tails still serve every port name. `vehicle_model.c`,
`building_model.c` and `legoobject_unit.c` compile natively but are not
reached (#552 defers native gameplay), so their switch from no-op adapters
to the unit changes nothing there.

**Matching-build runtime** (`make reccmp-run` only): vehicles and buildings
now load their upgrade levels, and a vehicle's activity changes reach its
containers, wheels and upgrade parts. Before, the adapters loaded nothing
and set no activity.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline, and no exact match lost. The plain-immediate check, the winapi
  link check and the link-order map check pass.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. GAME-047 and GAME-048 have no `.data` or `.bss`
  items, so `unit-data-offsets` checks gaps only.
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`). The input manifest
  `tools/binary_progress_inputs.txt` is regenerated for the renamed and new
  files.
- Native: `cmake --build build` and `ctest --test-dir build`: all pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `upgrade.c`, `vehicle_model.c`, `building_model.c` and
  `legoobject_unit.c`.

## Reproduce

    python3 tools/convert_unit.py GAME-047 --stem upgrade --out <scratch>   # with the GAME-047 link-order entry removed: the unit was already wired
    make match-function SOURCE=src/game/upgrade.c
    make match-function SOURCE=src/game/vehicle_model.c
    make match-function ADDR=0x004083a0
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object upgrade.c.obj --object vehicle_model.c.obj --min 1"
