# Finding 603 — Collision.c (GAME-007) and Construction.c (GAME-052) rebuilt in original shape, with a port tail

Issue #648, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-007 was the unit map's "2D point/segment distance and placement-search
helpers", 31 functions `004089d0`-`00409d40`, relocated verbatim into
`src/game/placement.cpp`. **It is two original objects**:

- **Collision.c**, `004089d0`-`00408c80` (4 functions): 2D distance and
  projection helpers. It is rebuilt as
  [`src/game/collision.c`](../../src/game/collision.c) and keeps the id
  GAME-007.
- **Construction.c**, `00408c80`-`00409d40` (27 functions, plus a static
  callback the unit map did not list): construction zones. It is rebuilt as
  [`src/game/construction.c`](../../src/game/construction.c) and takes the new
  id **GAME-052**. See [Unit extent](#unit-extent).

Each unit comes with a C header ([`collision.h`](../../src/game/collision.h),
[`construction.h`](../../src/game/construction.h)) and link names
([`collision_link_names.h`](../../src/game/collision_link_names.h),
[`construction_link_names.h`](../../src/game/construction_link_names.h)).
Collision.c has inline vector helpers
([`collision_maths.h`](../../src/game/collision_maths.h)) and Construction.c a
unit-private imports header
([`construction_imports.h`](../../src/game/construction_imports.h)).

The port's implementation, `placement.cpp`, moves unannotated to the port tail
[`placement_port.cpp`](../../src/game/placement_port.cpp).

Construction.c calls the converted units where the original does: AITask.c's
`aitask_*` functions, Lego.c's block functions (`lego_unit.c`), LegoObject.c
(`legoobject_unit.c`), Map3D (`terrain_map.c`), Stats.c, Info.c, Interface.c
and ElectricFence. The port model called the port's own path records, hook
tables and level sidecars. That is why its addresses showed up as drift drops
in #583, #585 and #610: they changed with every edit to the port headers they
used. Those addresses now compile from their own unit.

"Before" is the *match baseline* on `main` at `18cc9483` (#646). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Collision.c similarity (byte-weighted, 4 functions, 688 bytes) | 46.55% | **83.14%** |
| Construction.c similarity (byte-weighted, 28 functions, 4,288 bytes) | 27.70% | **92.43%** |
| All 32 addresses (4,976 bytes) | 30.31% | **91.15%** |
| Exact functions of the 32 | 4 | **24** |
| Mapped functions of the 32 | 30 | **32** |
| Exact-function code bytes | 183,830 (28.61%) | **186,139 (28.97%)** |
| Exact functions, full / game-only | 1,582 / 1,380 | **1,603 / 1,401** |
| Accuracy, full / game-only | 84.26% / 82.73% | 84.90% / 83.43% |
| `opt_ref` annotated (full, by address): removed / exact removed | 500 / 243 | **484 / 228** |
| `opt_ref` annotated (game-only, by address) | 481 / 230 | 465 / 215 |
| Reccmp lint ledger (entries) | 1,239 | **1,235** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%. Before, `00409270`'s span
included the then-unlisted callback `00409290`.

The units' names follow OpenLRR's `Collision_*` and `Construction_*`
functions where known, in the repository's snake case, and are otherwise
descriptive. "Link name" is the name the unit's definition takes in the
matching build (see [Link names](#link-names)); "—" means the unit's own name.

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `004089d0` | `collision_segment_point_distance` | — | 304 | 43.56 | 91.79 |
| `00408b00` | `collision_polyline_point_distance` | `point_to_polyline_minimum_segment_distance` | 96 | 59.26 | **100.00** |
| `00408b60` | `collision_project_onto_line` | `point_project_onto_line2d` | 144 | 40.43 | 62.26 |
| `00408bf0` | `collision_project_onto_direction` | `point2d_project_onto_direction` | 144 | 50.53 | 74.51 |
| `00408c80` | `construction_get_building_base` | `string_table_index_lookup` | 96 | 34.29 | **100.00** |
| `00408ce0` | `construction_zone_place_resource` | `cmd_operation_group_append_object` | 144 | 11.24 | **100.00** |
| `00408d70` | `construction_zone_needs_more_of_resource` | `terrain_descriptor_coordinate_count_limit_check` | 160 | 39.52 | **100.00** |
| `00408e10` | `construction_zone_count_of_resource_placed` | `count_matching_coordinates_in_list` | 64 | 13.79 | **100.00** |
| `00408e50` | `construction_zone_request_resource` | — | 112 | 33.33 | **100.00** |
| `00408ec0` | `construction_spawn_barriers` | — | 480 | 11.11 | 98.68 |
| `004090a0` | `construction_zone_exists_at_block` | `terrain_cell_predicate_check_current` | 32 | 100.00 | **100.00** |
| `004090c0` | `construction_zone_find_by_handle_or_at_block` | `terrain_descriptor_find_exact_current` | 80 | 44.44 | **100.00** |
| `00409110` | `construction_zone_complete_path` | `simtick_selection_group_release` | 64 | 100.00 | **100.00** |
| `00409150` | `construction_zone_cancel_path` | `simtick_selection_group_clear` | 144 | 48.00 | **100.00** |
| `004091e0` | `construction_update_all` | — | 144 | 55.77 | **100.00** |
| `00409270` | `construction_zone_origin_is_clear` | — | 32 | 12.90 | **100.00** |
| `00409290` | `construction_zone_origin_object_callback` (static) | — | 112 | unlisted | **100.00** |
| `00409300` | `construction_zone_start_path` | `tutorial_path_descriptor_create` | 80 | unmapped | **100.00** |
| `00409350` | `construction_zone_create` | — | 96 | 55.07 | **100.00** |
| `004093b0` | `construction_flood_path_blocks` | — | 160 | 24.43 | **100.00** |
| `00409450` | `construction_find_resource_object` | — | 32 | 100.00 | **100.00** |
| `00409470` | `construction_find_resource_object_recurse` | — | 224 | 26.34 | **100.00** |
| `00409550` | `construction_zone_request_path_resources` | — | 176 | 60.27 | **100.00** |
| `00409600` | `construction_zone_cancel_building` | `lego_path_remove` | 400 | 15.26 | 67.48 |
| `00409790` | `construction_zone_start_building` | — | 432 | 22.91 | 69.50 |
| `00409940` | `construction_update_surrounding_blocks` | `terrain_cell_refresh_eight_neighbors_current` | 144 | 15.38 | **100.00** |
| `004099d0` | `construction_remove_all` | `path_record_release_all` | 32 | 100.00 | **100.00** |
| `004099f0` | `construction_zone_free` | — | 80 | 42.11 | **100.00** |
| `00409a40` | `construction_zone_remove_barriers` | — | 80 | 19.35 | **100.00** |
| `00409a90` | `construction_place_building` | — | 160 | 33.33 | 90.00 |
| `00409b30` | `construction_zone_complete_building` | — | 416 | 18.34 | 90.28 |
| `00409cd0` | `construction_update_cardinal_blocks` | — | 112 | 7.84 | **100.00** |

- **The function with no source** was `00409300` Construction_Zone_StartPath.
  `level.cpp` had an unannotated port model under its link name. It is now
  written and mapped.
- **The unlisted function** is `00409290`, the LegoObject_RunThroughLists
  callback that `00409270` passes by address. No direct call reaches it, so
  the call graph and the unit map did not list it. It is now annotated, and
  `translation_units.py build` lists it.
- **The medium-confidence function** was `00409cd0`
  (`boundary:GAME-008`). It is confirmed as Construction.c's last function:
  its only caller is `00409b30`, and GAME-008 at `00409d40` is another object
  (below).

Callers outside the units that rose with no source change of their own, or
with only a caller switch (below):

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `0043c570` | `legoobject_kind_four_post_create_resolve` | 97.26 | **100.00** |
| `0044a910` | `legoobject_building_placement_preview_update` | 98.46 | 99.69 |
| `00437940` | `legoobject_deferred_olist_create_and_link` | 81.51 | 81.93 |
| `0042e020` | `lego_object_list_load_file` | 68.59 | 68.74 |
| `004232e0` | `lego_engine_tick` | 97.09 | 97.16 |
| `0041dca0` | `interface_do_action` | 84.32 | 84.43 |

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `004089d0` `collision_segment_point_distance` | 91.79 | One x87 schedule: the original loads `1.0f` after both `fsqrt`s, this compile between them. Tried: all four orders of the subtract, modulus, normalise and scale statements, both local declaration orders, and 0-300 probe declarations (rule B, finding 538): none moves it. |
| `00408b60`, `00408bf0` the two projections | 62.26, 74.51 | The original subtracts `point - origin` for x and y on either side of the `fsqrt`, keeps `origin` in `ecx` from the top and loads `out` last; this compile loads `point.y` first and `out` early. Tried, all at the same score: the relative vector as a temporary and inline; the dot product in both operand orders and both term orders; Maths_RayEndPoint against `out = t * dir + origin`; a scaled temporary; a struct copy against member copies of the direction (that one falls to 48%). Rule B moves `00408b60` to 73.58% and `00408bf0` to 78.43% at some probe counts (5-7, 11, 13, 16, 21-23, 37-39), never further, so the remaining difference is source shape. |
| `00408ec0` `construction_spawn_barriers` | 98.68 | Two compares address `shape[i + 1].y` as `[next + 4]` in the original and `[cur + 0xc]` here. `shape[1 + i]`, pointer arithmetic, int-pair indexing and swapped operands give the same or worse. |
| `00409600` `construction_zone_cancel_building` | 67.48 | The block index: the original loads `level->blocks` before `lea ecx, [i + i*8]`, and addresses the shape point as `[i*8 + shape]`; this compile does both the other way round, in every block write. Tried: the index in four spellings (`width * by + bx`, `bx + by * width`, pointer arithmetic, `idx[blocks]`), the shape pointer as a local and re-read from the zone, all six local declaration orders with both initialisation orders, and a 0-63 rule-B sweep. `msvc_permuter.py` (300 s) found no candidate. |
| `00409790` `construction_zone_start_building` | 69.50 | The same block-index schedule as `00409600`, and the original places the `entrance = FALSE` branch after the function's `ret`. Inverting the condition, the shape as a cast of the parameter instead of a local, and the index spellings above change nothing; the permuter (300 s) found no candidate. |
| `00409a90` `construction_place_building` | 90.00 | Store order of the zone's fields: the original loads the block position first and stores `objID` before `direction`. All 24 orders of the four field groups, and a struct copy of the position (120 orders), reach 90.00% at best. |
| `00409b30` `construction_zone_complete_building` | 90.28 | The shape point is addressed `[i*8 + shape]` in the original (as in `00409600`); and the entrance branch reloads `x` for the call. |

No naked asm, and no `Deviation (scoring only)`. Construction.c sets
LegoObject `flags3` bit `0x800000` (`00409600`, `00409a40`), a plain constant
past the original image; the plain-immediate wrapper (finding 584) keeps it
plain, `00409a40` is exact, and the plain-immediate check passes.

## Unit extent

The unit map's high-confidence boundary at `004089d0`-`00409d40` covers two
objects. The cut is at `00408c80`.

- **The literal pool decides it**, as in findings 597 and 599. MSVC 5.0 emits
  each float literal once per object. The `.rdata` run is:

  | Address | Value | Used by |
  | --- | --- | --- |
  | `0049e088` | `1.0f` | `004089d0`, `00408b60`, `00408bf0` |
  | `0049e08c` | `0.0f` | `004089d0` |
  | `0049e090` | `8.8f` | `00408ec0` |
  | `0049e094` | `0.5f` | no code reference (`00408ec0` stores it as an immediate) |
  | `0049e098` | `1.0f` | `00408ec0` |
  | `0049e09c` | `1.5707964f` | `00409b30` |
  | `0049e0a0` | `0.0f` | GAME-008's `00409d50` |
  | `0049e0a4` | `1.0f` | no code reference (GAME-008) |

  `00408ec0` loads its own `1.0f`, so it was compiled in another object from
  `004089d0`. Building.c's pool ends with its own `0.0f` at `0049e084`, so
  Collision.c is not Building.c's either.
- **The data agrees.** Collision.c has no data. Construction.c's state is
  one `.bss` block, `004b8a20`-`004b8a54` (`constructionGlobs`, 0x34 bytes):
  the next zone handle (`004b8a20`), the zone list (`004b8a24`), the
  building-base names (`004b8a3c`) and their count (`004b8a50`). No
  Collision.c function touches it.
- **The calls agree.** Collision.c's functions call only each other and the
  CRT's `acos`. LegoObject.c and Weapons.c call them. Nothing in Construction.c
  calls Collision.c.
- **The far edge.** GAME-008 at `00409d40` writes `.bss` `004b8a54`, right
  after Construction.c's block, and its pool (`0049e0a0`-`0049e0a4`) has its
  own `1.0f`: another object. `00409cd0` has no literal; its only caller is
  `00409b30`, so it ends Construction.c.
- **The names.** OpenLRR names the object Construction.c, which fits the
  zone list and the original's alphabetical object order: Building.c
  (GAME-006), then Collision.c, then Construction.c. The Collision.c stem is
  inferred from that order and from LegoObject.c's existing import comment.

`translation-units.spec.csv` records this evidence, with `inferred` names:

- GAME-007 becomes `Collision.c`, `004089d0`-`00408c80`, 4 functions.
- **GAME-052** `Construction.c`, `00408c80`-`00409d40`, is new. The next free
  id is used rather than renumbering every later unit.

`python3 tools/translation_units.py build`: 0 conflicts. `link_order.cmake`
lists `game/collision.c` (GAME-007) and `game/construction.c` (GAME-052) in
their original positions, and `game/placement_port.cpp` as a `tail` entry.

## How the conversion is done

### State

`static Construction_Globs constructionGlobs` (`004b8a20`), laid out as the
original `.bss`:

| Offset | Address | Field |
| ---: | --- | --- |
| `0x00` | `004b8a20` | `nextHandle` |
| `0x04` | `004b8a24` | `constructList` |
| `0x08` | `004b8a28` | 0x14 bytes with no code reference |
| `0x1c` | `004b8a3c` | `buildingBaseName[5]` |
| `0x30` | `004b8a50` | `buildingBaseCount` |

Nothing in the original's `.text` writes the building-base names or their
count, so Construction_GetBuildingBase always answers 0 there.

The zone is `Lego_ConstructionZone` (0xd4 bytes, `construction.h`): building
type, direction, handle, origin, shape points and count, the requested and
placed counts, the list link, a progress float set to 125 on completion, the
40 placed resources and their count, and the flags (`READY`, `LAYPATH`,
`REQUESTED`, `USESTUDS`). The `.rdata` floats are literals in the code.

The three `// GLOBAL` annotations of the port file are removed: the unit owns
`004b8a20`.

`make unit-data-offsets`:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-007 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |
| GAME-052 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 7, partial 25, n/a 195.

### The port tail

The port's construction zones are another contract: path records with native
sidecars for the 64-bit pointer slots, hook tables for the native renderer
and HUD (`WorldObjectCreateHooks`, the bobbin spawn and mark hooks), NULL and
bounds guards, and the peeks that `level.cpp`, `frameloop.cpp` and the tests
use. As in findings 590 and 592-601, `placement.cpp` is renamed
`placement_port.cpp` and becomes a `tail` entry. Each `// FUNCTION` becomes a
`/* Port model of 0x… */` comment.

### Link names

In the matching build `collision_link_names.h` gives three Collision.c
functions, and `construction_link_names.h` twelve Construction.c functions,
the port's names (table above). Each port name has C linkage, and every caller
passes the original's arguments: a 2D point, a block position, a zone handle,
an object type and index, a zone. So `legoobject_unit.c`, `weapons.c`,
`lego_unit.c`, `interface_unit.c`, `aitask.c` and `entity_stats.c` reach the
units with no edit.

- `placement_port.cpp` compiles its definitions of those names out of the
  matching build behind `LRR_PLACEMENT_PORT_NAMES`, set by an explicit `#if`
  (finding 590).
- `level.cpp`'s port model of `00409300` (`tutorial_path_descriptor_create`)
  and `interface_port.cpp`'s `lego_path_remove` adapter for `00409600` are
  compiled out the same way, with a plain `#if`.

`tutorial_path_descriptor_create` returns the new record in the port and
TRUE or FALSE in the original; its one matching-build caller,
`interface_unit.c`, ignores the value.

Natively the units keep their own names, and the port keeps serving every
port name.

### Caller switches

The callers whose contract is the original's, and that called a
`PORT-ONLY` adapter that did nothing, now call the unit. Each import line is
replaced one for one, with the same number of named parameters, so no symbol
counter moves (finding 588, "Parameter names count").

| Caller | Original callee | Was | Now |
| --- | --- | --- | --- |
| `lego_unit.c` `004232e0` Lego_Update | `004091e0` | `simtick_bobbins_original` (`lego_port.cpp`) | `construction_update_all` |
| `lego_unit.c` `0042e020` Lego_LoadOLObjectList | `00409a90` | `world_object_create_with_transform_original` (`lego_port.cpp`, answered NULL) | `construction_place_building` |
| `legoobject_unit.c` `00437940` | `00409a90` | `legoobject_gb_world_object_create_adapter` (answered NULL) | `construction_place_building` |
| `legoobject_unit.c` `0043c570` | `00409450`, `004093b0` | `legoobject_gi_nearby_placement_candidate_find_adapter`, `legoobject_gi_placement_cell_search_expand_adapter` | `construction_find_resource_object`, `construction_flood_path_blocks` |
| `legoobject_unit.c` `0044a910` | `00409790`, `00408ec0`, `00408e50` | `legoobject_gs_construction_register_adapter`, `…_path_shape_adapter`, `…_resource_add_adapter` (did nothing) | `construction_zone_start_building`, `construction_spawn_barriers`, `construction_zone_request_resource` |

The eight adapters are deleted from `lego_port.cpp` and
`legoobject_port.cpp`. `construction_place_building` returns `void*` (the
LegoObject): `lego_unit_imports.h` does not name `Lego_Object`, and a new
struct tag there would add a declaration to every `lego_unit.c` function.

No file owned by #649 (`gods98/avi_player.cpp`), #650
(`gods98/bitmap_font.cpp`), #651 (`map_view.cpp`) or #652 (`text.cpp`) is
edited. Outside the units and the port tail, this change edits
`lego_unit.c`, `lego_unit_imports.h`, `lego_port.cpp`, `legoobject_unit.c`,
`legoobject_unit_imports.h`, `legoobject_port.cpp` (the switches above),
`level.cpp` and `interface_port.cpp` (the two `#if`s), `test_c_boundaries.cpp`
and the build lists.

### Source shapes that matched

- **Direction tables** (`004093b0`, `00409470`, `00409cd0`): indexed,
  `DIRECTIONS[i].x += blockPos[0]`, not through a pointer to the entry. The
  pointer form loads `y` before the call's `push` (89.83% to 100%).
- **`00409940`**: the eight neighbours run clockwise from the north-west,
  NW N NE E SE S SW W. The port's table (NW N NE W E SW S SE) gave an
  effective match only.
- **`00409150`, `00409a40`**: the placed object as a local
  (`obj = zone->placedObjects[i]`), so MSVC uses `and [obj + 0x3ec], ebp`;
  in `00409a40` the flag before the health.
- **`00409290`**: the block position as a `Point2I` local, not two ints; MSVC
  then gives it its own 8-byte frame instead of a parameter slot.
- **`00409b30`**: the heading `direction * (π/2)` as a local initialised
  before Map3D_BlockToWorldPos, and the shape read through the zone in each
  statement.

**Rule B (finding 538).** A 0-63 probe sweep at the top of `construction.c`
(a throwaway experiment, single-function loop) moved only `00409b30` (88.89%
to 90.97% at 17 and 19 probes, before its heading local was added); no other
Construction.c function is held back by the counter's phase. The committed
include set has no filler.

## Entropy and drops

`make reccmp-check` against the baseline at `18cc9483`:

- 32 improved: 26 of the 32 addresses and the six callers above;
- 2 newly mapped: `00409290`, `00409300`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at the units' addresses (listing digests,
`[full]`). No listing outside them changed, so the six callers' gains are
call operands that now name the original's callee.

| Address | before | after |
| --- | ---: | ---: |
| `004089d0` | 120 | 104 |
| `00408b00` | 41 | 41 (operands only) |
| `00408b60` | 48 | 53 |
| `00408bf0` | 49 | 51 |
| `00408c80` | 54 | 46 |
| `00408ce0` | 48 | 44 |
| `00408d70` | 85 | 69 |
| `00408e10` | 49 | 25 |
| `00408e50` | 49 | 43 |
| `00408ec0` | 9 | 151 |
| `004090c0` | 32 | 31 |
| `00409150` | 61 | 50 |
| `004091e0` | 52 | 51 |
| `00409270` | 14 | 9 |
| `00409290` | — | 40 |
| `00409300` | — | 29 |
| `00409350` | 32 | 27 |
| `004093b0` | 71 | 56 |
| `00409470` | 116 | 87 |
| `00409550` | 72 | 70 |
| `00409600` | 132 | 125 |
| `00409790` | 179 | 133 |
| `00409940` | 76 | 45 |
| `004099f0` | 42 | 29 |
| `00409a40` | 29 | 27 |
| `00409a90` | 59 | 40 |
| `00409b30` | 186 | 145 |
| `00409cd0` | 27 | 40 |

`004090a0`, `00409110`, `00409450` and `004099d0` keep their listings (and
their exact matches). `00408ec0` was a 9-instruction forwarder to a port
model before.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `18cc9483`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 500 / 243 | **484 / 228** |
| `opt_ref.annotated.game_only` (by address) | 481 / 230 | **465 / 215** |

`make opt-ref-callers` lists 2 removed functions in each new object, none
exact; the other 28 of the 32 are kept:

- `collision.c.obj`: `00408b60` and `00408bf0`. Their only callers,
  Weapons.c's `00470540` and `00471370`, are removed themselves (root
  `weapons.c.obj`).
- `construction.c.obj`: `00408ec0` and `00409790`. Their only caller,
  LegoObject.c's building placement preview `0044a910`, is removed (root
  `legoobject_unit.c.obj`).

The net change is 16 fewer removed annotated functions (15 exact). On `main`
the port's `placement.cpp` definitions were the annotated functions; most were
reached only from port code that the relink drops, and the original-shape
callers reached them through adapters that did nothing. Now `lego_unit.c`,
`legoobject_unit.c`, `interface_unit.c`, `aitask.c` and `entity_stats.c` call
the units' functions, so the relink keeps them. The per-function list on
`main` was not rebuilt for this change, so no *`opt_ref` swap* is
claimed or ruled out; the totals above are both from full builds.

## Reccmp lint ratchet

1,239 → 1,235 entries (−4, no growth). The port's static helpers
`bobbin_mark`, `bobbin_record_remove_and_free`, `cmd_path_descriptor_find` and
`path_record_storage_release` are now in a tail file, which the lint exempts.
`make reccmp-lint-update` removed their entries.

## Native behaviour

Unchanged:

- the port tail serves every port name natively, and `level.cpp` and
  `interface_port.cpp` keep their models;
- the units compile natively as C++ under their own names;
- the switched callers are `lego_unit.c` and `legoobject_unit.c` functions
  that nothing calls natively (their port aliases are matching-build only).

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The units' state and the
port's are separate. In the matching build, callers of the link names use
`constructionGlobs`; the port's remaining queries (`path_record_last_peek`,
`path_record_count_peek` and the sidecars) read the port's own list. This is
the split that findings 590, 592 and 599 accept until the callers' units are
converted.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding
  584), `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`.
- `collision.c` and `construction.c` pass `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **The near-misses above**, chiefly the block-index schedule shared by
  `00409600` and `00409790`, and Collision.c's projections.
- **The port's construction queries** (`path_record_last_peek`, the
  sidecars, the hook tables): they retire with the native renderer's hooks,
  once Lego.c's world update runs natively (#552).
- **GAME-008** (`00409d40`-`0040a0c0`): its own object, after Construction.c.

## Rebased onto stack #656

This section gives the figures after rebasing onto #657 (`045ab18c`,
RadarMap.c). That base contains `main` `321fdbf1`, #655 (Text.c) and #654
(Font.c). None of them touches the units, `placement.cpp` or the callers
switched here. Neither unit includes `advisor.h` or calls `00401c80`, whose
prototype #654 changed. The spec split is re-applied on #657's unit map, and
`translation_units.py build` reports 0 conflicts. Apart from `unit_order`,
only this change's rows differ from #657's map; the Text.c, Font.c, RadarMap.c
and Fallin.c (GAME-051) rows are unchanged. All the per-function scores above
are unchanged on the rebase, in the single-function loop and in the full
build.

`make reccmp-check` against #657's baseline: the same 32 improved and 2 newly
mapped, **none below baseline, no exact match lost**. No listing outside the
units' addresses changed.

| | #657 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 186,422 (29.01%) | **188,731 (29.37%)** |
| Exact functions, full / game-only | 1,605 / 1,403 | 1,626 / 1,424 |
| Accuracy, full / game-only | 84.97% / 83.51% | 85.60% / 84.21% |
| `opt_ref` full: removed / exact removed | 488 / 238 | **471 / 221** |
| `opt_ref` game-only | 469 / 225 | 452 / 208 |
| Lint ledger | 1,231 | **1,227** |

`lego_engine_tick` (`004232e0`) is 97.44% on #657 and 97.51% here.

`make unit-data-offsets`: GAME-007 and GAME-052 rows as above; unit summary
full 0, gaps-only 8, partial 24, n/a 195.

## Reproducing

    git mv src/game/placement.cpp src/game/placement_port.cpp   # and make it a tail entry
    # split the spec row at 00408c80 (GAME-052 Construction.c), then:
    python3 tools/translation_units.py build
    python3 tools/convert_unit.py GAME-007 --stem collision
    python3 tools/convert_unit.py GAME-052 --stem construction
    make match-function SOURCE=src/game/collision.c
    make match-function SOURCE=src/game/construction.c
    make reccmp-check
    make opt-ref-link opt-ref-callers OPT_REF_ARGS="--min 1"
    make match-progress
    make unit-data-offsets
