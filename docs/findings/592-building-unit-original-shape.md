# Finding 592 — Building.c (GAME-006) rebuilt in original shape, with a port tail

Issue #623, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-006 is the original's `Building.c` (text `00407d60`-`004089d0`): the
building models. Building_Load reads each building type's `.ae` config once:

- the activity container;
- the named nulls (carry, camera, tool, deposit, fire, entrance, the two
  pivots) and their frame counts;
- the pivot limits and the upgrades;
- the footprint shape and the optional power-level scene.

Every building object clones that model. Upgrading a building rebuilds its
upgrade parts, hangs each on its null and finds the weapon nulls on the
parts that carry a weapon.

#496 relocated the unit verbatim into `src/game/building_model.cpp`. This
change rebuilds it as the C unit
[`src/game/building_model.c`](../../src/game/building_model.c), with:

- the unit's C header [`building_model.h`](../../src/game/building_model.h);
- the unit-private imports header
  [`building_model_imports.h`](../../src/game/building_model_imports.h);
- the matching-build link names
  [`building_model_link_names.h`](../../src/game/building_model_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`building_model_port.cpp`](../../src/game/building_model_port.cpp).

"Before" is the *match baseline* on `main` at `2dcc7dec`, after #626
(LegoObject.c part 3, finding 588). "After" is this change rebased onto it,
from the full matching build (`make reccmp-check`). No unit score differs
from the first measurement on `2f988e92`.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 22 functions, 3,184 bytes) | 25.78% | **94.74%** |
| Exact functions in the unit | 5 | **19 of 22** |
| Mapped functions in the unit | 21 | 22 |
| Exact-function code bytes (`binary-progress.json`) | 168,566 (26.23%) | **169,567** (26.39%) |
| Exact functions, full / game-only | 1,476 / 1,276 | 1,490 / 1,290 |
| Accuracy, full / game-only | 80.94% / 79.07% | 81.32% / 79.49% |
| `opt_ref` annotated (full, by address): removed / exact removed | 501 / 231 | **498 / 229** |
| `opt_ref` annotated (game-only, by address) | 484 / 220 | 481 / 218 |
| Reccmp lint ledger (entries) | 1,291 | **1,290** |

The weights are the functions' spans up to the next function, padding
included. The unmapped function counts as 0%.

The names follow OpenLRR's Building.c, in the repository's snake case. "Port
name" is the name the port and the callers use. In the matching build the
unit's definition takes it (see [Link names](#link-names)).

| Address | Function | Port name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00407d60` | `building_load` | `entity_building_type_config_record_parse` | 1408 | 20.51 | 99.35 |
| `004082e0` | `building_change_power_level` | `scene_object_scalar_adjust_by_direction` | 128 | 26.67 | **100.00** |
| `00408360` | `building_get_shape_points` | `task_field84_get_and_field88_out` | 32 | 56.00 | **100.00** |
| `00408380` | `building_set_owner_object` | `scene_assign_entity_owner` | 32 | 100.00 | **100.00** |
| `004083a0` | `building_set_upgrade_activity` | `entity_scene_slot_table_rebuild` (kept) | 464 | 7.41 | 66.88 |
| `00408570` | `building_set_activity` | none (caller switched) | 128 | unmapped | **100.00** |
| `004085f0` | `building_get_camera_null` | `scene_activity_change_secondary` | 48 | 56.52 | 90.00 |
| `00408620` | `building_clone` | `scene_object_record_clone_retain_handles` | 80 | 20.00 | **100.00** |
| `00408670` | `building_hide` | `scene_object_pair_apply_mode` | 48 | 100.00 | **100.00** |
| `004086a0` | `building_is_hidden` | `animation_object_visibility_get` | 32 | 100.00 | **100.00** |
| `004086c0` | `building_set_orientation` | `scene_object_pair_set_render_parameters` | 80 | 54.55 | **100.00** |
| `00408710` | `building_set_position` | `scene_object_pair_set_callback_scalar` | 96 | 35.96 | **100.00** |
| `00408770` | `building_get_carry_null` | `scene_object_indexed_handle_get_or_create` | 64 | 35.48 | **100.00** |
| `004087b0` | `building_get_deposit_null` | `scene_object_primary_handle_get_or_create` | 48 | 50.98 | **100.00** |
| `004087e0` | `building_get_entrance_null` | `scene_object_secondary_handle_get_or_create` | 48 | 50.98 | **100.00** |
| `00408810` | `building_get_tool_null` | `scene_object_indexed_secondary_handle_get_or_create` | 64 | 35.48 | **100.00** |
| `00408850` | `building_get_carry_null_frames` | `scene_object_flags_get` | 16 | 100.00 | **100.00** |
| `00408860` | `building_move_animation` | `animation_loop_time_adjust` | 208 | 12.24 | **100.00** |
| `00408930` | `building_get_trans_coef` | `constant_zero_float_get` | 16 | 100.00 | **100.00** |
| `00408940` | `building_remove` | `entity_payload_teardown` | 48 | 55.81 | **100.00** |
| `00408970` | `building_can_upgrade_type` | `task_flag_bit_query` | 48 | 50.00 | **100.00** |
| `004089a0` | `building_set_upgrade_level` | `entity_payload_state_set` | 48 | 20.00 | **100.00** |

"(kept)" marks the one port name whose callers are all port models with
another contract. The unit keeps its own name for that function, and the
port tail serves the port name in every build. `00408570` had no source
before; `legoobject_unit.c` now calls it by the unit's name.

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `00407d60` Building_Load | 99.35 | Call operands only; the code is identical. Two Container_Load calls go to `gods98/entity.cpp`'s `container_load` adapter, because the port's `00472cd0` has another contract. Upgrade_Load (`0046c160`) goes to Vehicle.c's `vehicle_upgrade_load_adapter` (finding 590), because the port has no annotated entry with its contract. |
| `004083a0` Building_SetUpgradeActivity | 66.88 | Same instructions, same frame, same stack slots. Only register names differ (`edx` against `eax` for the fire-null pointer, `eax`/`ecx`/`edx` for the name arguments), plus two scheduling swaps. Tried without a gain: 250 random declaration orders, `index`, `loop` and `instance` as `int` and `unsigned`, a separate inner-loop counter, a count pointer, a result temporary, three orders of the slot set-up statements, and `msvc_permuter.py` (no candidates). A throwaway run of 0-65 inert declarations before the function never changed the score, so this is not finding 538's counter phase (rule B). |
| `004085f0` Building_GetCameraNull | 90.00 | The original saves `edi` before the name test, and this compile saves it after. Seven source shapes were tried (early return, a result variable, an `else`, a cache pointer, returning the name) and scored 66.67-90.00; `msvc_permuter.py` had no candidates. Creature.c's camera-null getter (`00406f80`, 81.82% on `main`) has the same shape and the same gap. |

No naked asm, and no `Deviation (scoring only)` in the unit. No unit
function uses the flag `0x800000`, and #608's plain-immediate check passes.

## Unit extent

The map gave 20 of the 22 functions low confidence ("boundary:GAME-007"),
and `00408570` had no source. The extent is confirmed, and no function
moves. The unit is `00407d60`-`004089d0`.

- **The model.** All 22 functions read or write the 0x14c-byte building
  model that `00407d60` fills, at the offsets it writes:
  - `+0x04`, the activity container;
  - the null names at `+0x08`-`+0x24` and their caches at `+0x28`-`+0x6c`;
  - the frame counts at `+0x70`-`+0x78`;
  - the power-level scene and level at `+0x7c`/`+0x80`;
  - the shape at `+0x84`/`+0x88`;
  - Upgrade.c's model at `+0x8c`;
  - weapons.c's weapon nulls from `+0xa0`;
  - the flags at `+0x148`.

  `lego_unit.c` allocates the building table at 0x14c bytes a type
  (`LegoBuildingModelE`).
- **Call graph** (`docs/coverage/call-graph/edges.csv`). The 22 functions
  are called only by each other and by:
  - LegoObject.c (30 call sites);
  - Lego.c (5);
  - AITask.c (`00404fc0`);
  - GAME-008's `00409d50` (Building_GetShapePoints).

  They call Container.c, the animation-object unit (GAME-004), Config.c,
  Upgrade.c (GAME-047), `0046c130` (GAME-046) and the CRT.
- **`.rdata`.** `0049e070`-`0049e084` (`1.0f`, `-0.1f`, `0.1f`, `10.0f`,
  `1/9`, `0.0f`) is used only by `004082e0`, `00408860` and `00408930`.
  Bubble.c's `0049e060`-`0049e06c` comes before it, and `004089d0`'s
  `0049e088` comes after it.
- **Strings.** `004a1cbc`-`004a1d4c` follow Bubbles' `"Bubbles"`
  (`004a1cb4`) and come before `"LWO"` (`004a1d5c`). Only `00407d60` uses
  them. Six of them are pooled with Vehicle.c, and `"%s\\%s"` with eight other
  functions.
- **The next unit.** `004089d0` is GAME-007's first point-to-segment
  distance. It works on its own `.rdata` (`0049e088`/`0049e08c`) and is
  called only by GAME-007's `00408b00`.
- **The previous unit.** Bubble.c's two medium-confidence functions
  (`004078c0`, `00407a10`) read the Bubble commons (`005578xx`) and
  Bubble.c's `.rdata` (`0049e060`).

`translation-units.spec.csv` records this evidence in GAME-006's note. The
boundary confidence is computed by `tools/translation_units.py` and stays
`medium`.

## How the conversion is done

### The port tail

The port's building code is another contract:

- `EntitySlot` scene-slot sidecars;
- the resolver and release hooks;
- `NULL` guards.

Native code (`entity.cpp`, `lego_port.cpp`, `legoobject_port.cpp`,
`aitask_port.cpp`, `view.cpp`'s ledger stub) and `test_record.cpp` and
`test_crt.cpp` use it. As in findings 585, 590 and 591,
`building_model.cpp` is renamed `building_model_port.cpp` and becomes a
`tail` entry. Each `// FUNCTION` becomes a `/* Port model of 0x… */`
comment. The port's `// GLOBAL: LEGORR 0x0049e084` on its zero carrier is
removed: the unit's literal now takes that address.

### Link names

As in findings 581, 585 and 590, `building_model_link_names.h` renames 20
of the unit's definitions to the port's names, under `#if
defined(_MSC_VER) && _MSC_VER < 1200`. Only `building_model.c` includes it,
first. In the matching build the C callers of these names all pass a
building model of the original's layout:

- `lego_unit.c`'s building table (`legoGlobs_buildingData`), which Lego.c
  loads through Building_Load;
- LegoObject's `+0x018`, which `legoobject_unit.c` fills with
  Building_Clone.

So the C callers (`aitask.c`, `lego_unit.c`, `legoobject_unit.c`,
`terrain_cell_objects.c`) and the C++ ones reach the unit with no caller
edit. `building_model_port.cpp` compiles its definitions of the 20 names
out of the matching build behind `LRR_BUILDING_MODEL_PORT_NAMES`, set by an
explicit `#if` (finding 590). Natively the port tail keeps serving them.

Three of the 20 were added on the rebase onto #626:

- `scene_activity_change_secondary` (`004085f0`);
- `animation_loop_time_adjust` (`00408860`);
- `constant_zero_float_get` (`00408930`).

Before #626 only port models called them. #626's `legoobject_unit.c` calls
them from `00447c40`, `00447b30` and `00446610` with `obj->building`. That
is the original's contract, so the unit takes those names too. Without
them, those three calls would land on unannotated port definitions.

One port name keeps its port definition in every build, because its callers
are all port models with another contract: `entity_scene_slot_table_rebuild`
(`004083a0`). The unit keeps its own name for it.

**Caller switch (`00408570`).** Building_SetActivity has no port name.
#626 called it through `legoobject_port.cpp`'s `PORT-ONLY` adapter
`legoobject_gq_building_set_activity_adapter`, which did nothing.
`legoobject_unit.c`'s `00447c40` now calls `building_set_activity`. The
contract is the same: building, activity name, time; the result is the
container's activity result. The change is:

- in `legoobject_unit.c`, the declaration is renamed one for one, so the
  declaration count does not change (rule B), and the call is renamed;
- in `legoobject_port.cpp`, the adapter is deleted.

Nothing native called the adapter, and `legoobject_unit.c` runs natively
only through matching-build aliases, so native behaviour is unchanged.
`00447c40` rises from 95.70% to 96.28%.

`extern "C"` is added in place to four declarations. No declaration is
added:

- `animation_object_visibility_get` (`record.hpp`), which `legoobject_port.cpp`
  calls;
- `scene_object_flag_bit3_get_target` (`record.hpp`; Container_IsHidden,
  `00473ba0`);
- `scene_object_set_animation_with_fallback` (`view.hpp`; `0046c130`);
- `render_object_set_identity_transform` (`view.hpp`; `00475610`).

**C++ callers in the matching build.** The C++ callers that use the port's
names now reach the unit in the matching build too. Five of them pass
`attached_direction_record_type4`, the building record. The port's own
definitions read that record at the original's offsets (`+0x04`,
`+0x08`/`+0x28`, `+0x14`/`+0x6c`, `+0x7c`):

- `legoobject_port.cpp`'s `00449f50` and `00449ff0` (Building_Hide,
  Building_IsHidden);
- `legoobject_port.cpp`'s `ai_set_activity`, the `PORT-ONLY` model of
  `004409e0` (Building_GetCarryNull);
- `legoobject_port.cpp`'s `0044ae50` and `aitask_port.cpp`'s
  `cmd_place_target_nested_scene_pose_get` (Building_GetEntranceNull).

One does not. `legoobject_port.cpp`'s `building_upgrade_commit`, the `PORT-ONLY`
model of `004387f0`, passes its own `EntitySlot` to
`entity_payload_state_set` when no `payload_level_set` hook is installed.
The port's definition stores the level on the `EntitySlot`. The original's
`004387f0` (`legoobject_unit.c`) passes the object's building model. In the
matching build, an upgrade that goes through this port path
(`gods98/frameloop.cpp`'s UI action, or `legoobject_port.cpp`'s own caller)
would hand the unit an `EntitySlot`. This is a risk for `make reccmp-run`
only; natively nothing changes. `gods98/frameloop.cpp` does not include
`legoobject_port_aliases.h`, so its call does not reach
`legoobject_unit.c`'s `004387f0`. Not renaming `004089a0` would leave
`legoobject_unit.c`'s and `lego_unit.c`'s calls on an unannotated port
function, and their scores would fall. Fixing the port model would change
native behaviour, which this change keeps identical, so it is listed under
[Follow-ups](#follow-ups).

### The headers

- `building_model.h` is now the unit's API, by its own names. Before, it
  declared only `task_field84_get_and_field88_out` for the electric fences
  (GAME-013, finding 566). That declaration moves to the new
  [`building_model_ports.h`](../../src/game/building_model_ports.h).
  `terrain_cell_objects.c` includes that header instead, so its declaration
  count, and so its symbol numbering, does not change (finding 538).
- `test_c_boundaries.cpp` includes all three of the unit's C headers.

### State

The unit has no `.bss`. BuildingModel (0x14c bytes) is declared at the
original's offsets, with weapons.c's `Weapons_Model` fields that the unit
writes:

- the fire nulls at `+0xa0`;
- the x and y pivots at `+0xb8`/`+0xc4`;
- PivotMaxZ/PivotMinZ at `+0xd0`/`+0xd4`;
- the fire-null counts at `+0xd8`;
- the weapon parts at `+0x12c`;
- the weapon count at `+0x144`.

Upgrade.c's parts are reached as Vehicle.c reaches them. The model's
`+0x10` is the first part's LegoObject, LegoObject `+0x20` is its
Upgrade_Part, and the part's `+0x08` is the next object. This is the walk
`0046c340` makes.

The `.rdata` constants are literals.

Strings:

- The unit's five strings that had no marker anywhere (`"ToolNullName"`,
  `"ToolNullFrames"`, `"EntranceNullName"`, `"Shape"`, `"PowerLevelScene"`)
  are marked `// STRING` where they are used.
- The six pooled with Vehicle.c are not marked again (finding 590).
- `"%s\\%s"` stays `files.c`'s marker, and the `animation_object.c` and
  `advisor.c` strings stay theirs.
- `":"` (`004a1cd4`) and `""` (`004b8a14`) are `config.c`'s
  `s_config_rgb_separator` and `s_config_angle_empty` (finding 581).

`make unit-data-offsets`, GAME-006 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-006 | partial (12, 1 unknown), gap 10, first `004a1cd8`: `0x1c`/`0x18` | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |

The `.data` gap is the literals' spacing. The rebuild places `"Shape"`
`0x1c` bytes after its predecessor, and the original places it `0x18`
bytes after. The unit keeps its own copies of the literals that the
original pools with other units, as Vehicle.c does (finding 590).
Unit summary: full 0, gaps-only 5, partial 29, n/a 191.

### Source shapes that matched

- **Building_Load** (`00407d60`, 73.30% to 99.35%). The shape loop reads
  each token into a temporary and finds the `x,y` comma with `strstr(str,
  ",")`, not `strchr`: the original pushes the pooled `","` (`004a13e4`).
  The shape token count is its own variable. Together these give the
  original's `0x99c` frame.
- **Building_ChangePowerLevel** (`004082e0`, 85.11% to 100%).
  `0.111111112f * ((level - 0.1f) * 10.0f)`. The other operand order, and
  `* 10.0f / 9.0f`, fold the two constants into one (finding 554's
  `0045c580` shape).
- **Building_MoveAnimation** (`00408860`, 85.71% to 100%). The animation
  result and the overrun are two variables, with `else overrun = time;`.
  The original copies one into the other through the dead `building`
  argument slot.
- **Building_SetUpgradeActivity** (`004083a0`, 60.33% to 66.88%). The first
  object is read before the weapon slots are cleared. Each slot's fire-null
  pair is cleared with `memset`. Each part name is formatted into a
  temporary before Container_SearchTree, so the search's `0` arguments are
  pushed after the format call.
- **The cached-null getters** (`00408770`-`00408810`) are Vehicle.c's shape
  (finding 590): `if (cache) return cache; return cache = find(…);`.

### Callees

- **Existing adapters, reused**: `gods98/entity.cpp`'s `container_load`
  (Container_Load), and Vehicle.c's `PORT-ONLY`
  `vehicle_upgrade_load_adapter` (Upgrade_Load, `0046c160`). Its comment
  now names Building.c too.
- **Container_SetActivity's fallback** `0046c130` (tooltip.cpp's
  `scene_object_set_animation_with_fallback`) takes the part as its binding
  record, whose `+0x04` is the part's container.
- **The activity names** (`004decc4`) are `legoobject_port.cpp`'s `s_activity`,
  declared in `building_model_imports.h` as `legoobject_unit_imports.h`
  declares them.
- **The animation-object null search** (`00406f50`): the getters,
  Building_SetUpgradeActivity and Building_GetCameraNull call the converted
  `animation_object_null_find`. This closes finding 579's gap
  (`building_model.cpp` → `animation_object_null_find`). The port's getters
  called `view.cpp`'s resolver hook instead. `/OPT:REF` no longer removes
  `00406f50`.

## Entropy and drops

`make reccmp-check` against the baseline at `2dcc7dec`:

- 17 improved and 1 newly mapped: the unit's 16, plus `00447c40` (95.70%
  to 96.28%, the caller switch);
- **none below baseline**, and no exact match lost;
- no other score changed outside the unit.

Instruction counts changed only at the unit's addresses (listing digests).
`00447c40`'s count is unchanged; only its call operand now maps.

The link names keep every other caller's call operand on the same address.

Header and file edits outside the unit:

- in place: `extern "C"` on four declarations (`record.hpp`, `view.hpp`);
- moved: `task_field84_get_and_field88_out`'s declaration, from
  `building_model.h` to `building_model_ports.h`, with the same count in
  `terrain_cell_objects.c`;
- `legoobject_unit.c`: one declaration and one call renamed (`00408570`);
- `legoobject_port.cpp`: the `00408570` adapter deleted, and a comment;
- comments only: `vehicle_model_port.cpp`.

No file owned by #624 or #625 is edited.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `2dcc7dec`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 501 / 231 | **498 / 229** |
| `opt_ref.annotated.game_only` (by address) | 484 / 220 | 481 / 218 |
| The unit's object | `building_model.cpp.obj` | `building_model.c.obj`: **none removed** |
| `animation_object.c.obj` | `00406f50` removed | none removed; `00406f50` kept |

The "before" figures are `main`'s committed `binary-progress.json`. Its
`removed_exact` is 231; the brief said 230.

`make opt-ref-callers` lists nothing for either object. All 22 unit
functions are kept: `legoobject_unit.c` reaches `00408570`, `004085f0`,
`00408860` and `00408930` directly now. The unit calls
`animation_object_null_find` (`00406f50`), which closes finding 579's
`building_model.cpp` → `animation_object_null_find` gap.

On `2f988e92` (before #626), four of the unit's functions were still
removed (571/280 to 565/276). #626 and the link names above keep them.

## Reccmp lint ratchet

1,291 → 1,290 entries (−1, no growth). `pair_handle_set_pose`, a static in
the old unit file, is now in a tail file. `make reccmp-lint-update` removed
its entry.

## Native behaviour

Unchanged:

- The port tail serves every port name natively, and none of its
  definitions changed.
- The unit compiles natively as C++ under its own names. Nothing native
  calls them.
- `extern "C"` changes only the four functions' linkage, and every
  declaration and definition sees it.

There is no new native gap.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding
  584), `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated
  (`binary_progress_hash.py --write-manifest`).
- `make tools-test`, without `--allow-growth`.
- `building_model.c` passes `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **Matching-build runtime (`legoobject_port.cpp`).** `building_upgrade_commit`
  (the port model of `004387f0`) calls `entity_payload_state_set` with its
  `EntitySlot`. In the matching build that name is now
  Building_SetUpgradeLevel, which takes the building model. It should pass
  the building model, as `legoobject_unit.c`'s `004387f0` does, or go
  through its `payload_level_set` hook. That changes native code. The
  other fix is for `gods98/frameloop.cpp` to reach `004387f0` through
  `legoobject_port_aliases.h`, which is matching-build only. That needs
  `frameloop.cpp`'s entity to be a LegoObject in the matching build, which
  this change does not establish. See [Link names](#link-names).
- **Upgrade.c (GAME-047).** Its conversion can give `0046c160` its original
  contract and replace `vehicle_upgrade_load_adapter` for both units.
- **Container.c (G98-002).** Its conversion can replace the
  `container_load` adapter, which removes Building_Load's operand
  differences.
- **`004083a0` and `004085f0`**: the register-name and save-placement gaps
  above.

## Reproducing

    git mv src/game/building_model.cpp src/game/building_model_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-006 --stem building_model
    make match-function SOURCE=src/game/building_model.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object building_model.c.obj --min 1"
    make match-progress
    make unit-data-offsets
