# Finding 581 — Stats.c (GAME-043) rebuilt in original shape: 63 of 63 exact

Issue #591, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-043 is the original's `Stats.c`: `Stats_Initialise`, which loads every
object's per-level stats from the game config's `Stats` block, and the
getters that read them (text `004667e0`–`0046a370`, 63 functions). It was
relocated verbatim into `src/game/entity_stats.cpp`. This change rebuilds it
as the C unit [`src/game/entity_stats.c`](../../src/game/entity_stats.c),
with the boundary header [`entity_stats.h`](../../src/game/entity_stats.h).
The port's implementation moves, unannotated, to the port tail
[`entity_stats_port.cpp`](../../src/game/entity_stats_port.cpp).

"Before" is `main` at `3791f0dd` (#590), where this change was made and
measured. "After" is this change. Scores come from the full matching build
(`make reccmp-check`). The change was then rebased onto stack #600, on
#598's head (`6654a85a`); [Rebased onto stack #600](#rebased-onto-stack-600)
gives the figures there.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 63 functions, 14,604 bytes without `0x90` fill) | 10.20% | **100.00%** |
| Exact functions in the unit | 59 | **63 of 63** |
| Mapped functions in the unit | 62 | 63 |
| Naked-asm functions in the unit | 14 | 0 |
| Exact functions, whole binary (full / game-only) | 1,174 / 974 | **1,178 / 978** |
| Accuracy, full / game-only (`binary-progress.json`) | 71.59% / 68.70% | **71.65% / 68.77%** |
| Exact-function code bytes | 105,485 | **118,712** |
| Unit-relative identity | 105,286 bytes (19.01%) | 118,400 bytes (21.38%) |
| `opt_ref` full: removed / exact removed | 1,303 / 585 | 1,303 / 585 |
| `opt_ref` game-only: removed / exact removed | 1,266 / 556 | 1,266 / 556 |

`Stats_Initialise` (`004667e0`, 12,961 bytes) was unmapped: the port had no
definition at its address. Before this change the 59 exact functions were
getters, 14 of them naked asm.

| Address | Unit function | Matching-build link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `004667e0` | `stats_initialise` | `stats_initialise` | unmapped | **100.00** |
| `00469a90` | `stats_add_tool_task_type` | `entity_indexed_value_append` | 75.00 | **100.00** |
| `00469ac0` | `stats_get_cost_ore` | `entity_indexed_record_word_at_b0_get` | **100.00** | **100.00** |
| `00469af0` | `stats_get_cost_crystal` | `entity_indexed_record_word_at_b4_get` | **100.00** | **100.00** |
| `00469b20` | `stats_get_cost_refined_ore` | `stats_table_word_at_b8` | **100.00** | **100.00** |
| `00469b50` | `stats_object_get_crystal_drain` | `stats_instance_word_at_bc` | **100.00** | **100.00** |
| `00469b80` | `stats_object_get_capacity` | `stats_instance_word_at_e0` | **100.00** | **100.00** |
| `00469bb0` | `stats_object_get_max_carry` | `stats_instance_word_at_ec` | **100.00** | **100.00** |
| `00469be0` | `stats_object_get_carry_start` | `stats_instance_word_at_f0` | **100.00** | **100.00** |
| `00469c10` | `stats_object_set_object_level` | `entity_building_index_set` | 9.59 | **100.00** |
| `00469cb0` | `stats_object_get_route_speed` | `entity_stats_float_at20` | **100.00** | **100.00** |
| `00469cc0` | `stats_object_get_drill_time_type` | `entity_stats_float_at24_indexed` | **100.00** | **100.00** |
| `00469ce0` | `stats_object_get_rubble_coef` | `entity_stats_float_atd4` | **100.00** | **100.00** |
| `00469d00` | `stats_object_get_wake_radius` | `entity_stats_float_at9c` | **100.00** | **100.00** |
| `00469d20` | `stats_object_get_path_coef` | `entity_stats_float_atd8` | **100.00** | **100.00** |
| `00469d40` | `stats_object_get_coll_radius` | `entity_stats_float_at6c` | **100.00** | **100.00** |
| `00469d50` | `stats_object_get_coll_height` | `entity_stats_float_at74` | **100.00** | **100.00** |
| `00469d60` | `stats_object_get_pick_sphere` | `entity_stats_float_at78` | **100.00** | **100.00** |
| `00469d70` | `stats_object_get_pain_threshold` | `entity_stats_float_ate8` | **100.00** | **100.00** |
| `00469d90` | `stats_object_get_alert_radius` | `entity_stats_float_at70` | **100.00** | **100.00** |
| `00469da0` | `stats_object_get_coll_box` | `entity_stats_field_at_a0_corpus` | **100.00** | **100.00** |
| `00469db0` | `stats_object_get_track_dist` | `entity_stats_float_at7c` | **100.00** | **100.00** |
| `00469dc0` | `stats_object_get_health_decay_rate` | `entity_stats_float_at80` | **100.00** | **100.00** |
| `00469de0` | `stats_object_get_energy_decay_rate` | `entity_energy_decay_rate_get` | **100.00** | **100.00** |
| `00469e00` | `stats_get_oxygen_coef` | `stats_table_float_at_dc` | **100.00** | **100.00** |
| `00469e20` | `stats_object_get_oxygen_coef` | `entity_stats_float_atdc` | **100.00** | **100.00** |
| `00469e40` | `stats_object_get_survey_radius` | `entity_stats_word_atac` | **100.00** | **100.00** |
| `00469e60` | `stats_object_get_stats_flags1` | `entity_stats_word_get` | **100.00** | **100.00** |
| `00469e80` | `stats_object_get_stats_flags2` | `entity_state_flags_get` | **100.00** | **100.00** |
| `00469ea0` | `stats_object_get_stats_flags3` | `entity_capability_flags_get` | **100.00** | **100.00** |
| `00469ec0` | `stats_get_stats_flags1` | `stats_table_word_at_144` | **100.00** | **100.00** |
| `00469ee0` | `stats_get_stats_flags2` | `stats_table_word_at_148` | **100.00** | **100.00** |
| `00469f00` | `stats_get_stats_flags3` | `stats_table_word_at_14c` | **100.00** | **100.00** |
| `00469f20` | `stats_object_get_repair_value` | `entity_stats_float_at88` | **100.00** | **100.00** |
| `00469f40` | `stats_get_levels` | `building_upgrade_admission_check` | **100.00** | **100.00** |
| `00469f60` | `stats_get_water_entrances` | `stats_table_word_at_d0` | **100.00** | **100.00** |
| `00469f90` | `stats_object_get_drill_sound_type` | `entity_stats_word_at_c0_or_c4` | **100.00** | **100.00** |
| `00469fc0` | `stats_object_get_engine_sound` | `entity_stats_word_atc8` | **100.00** | **100.00** |
| `00469fe0` | `stats_object_get_rest_percent` | `entity_stats_float_at8c` | **100.00** | **100.00** |
| `0046a000` | `stats_object_get_carry_min_health` | `entity_stats_float_at90` | **100.00** | **100.00** |
| `0046a020` | `stats_object_get_attack_radius` | `entity_stats_float_at98` | **100.00** | **100.00** |
| `0046a040` | `stats_object_get_stamp_radius` | `entity_stats_float_at94` | **100.00** | **100.00** |
| `0046a060` | `stats_object_get_num_of_tools_can_carry` | `ai_activity_cap` | **100.00** | **100.00** |
| `0046a080` | `stats_object_get_upgrade_time` | `entity_stats_float_atf4` | **100.00** | **100.00** |
| `0046a0a0` | `stats_object_get_function_coef` | `entity_speed_component_get` | **100.00** | **100.00** |
| `0046a0c0` | `stats_get_upgrade_cost_ore` | `entity_mapped_indexed_record_word_at_fc_get` | **100.00** | **100.00** |
| `0046a0f0` | `stats_get_upgrade_cost_studs` | `stats_table_word_at_100` | **100.00** | **100.00** |
| `0046a120` | `stats_find_tool_from_task_type` | `entity_grouped_code_table_find_value_index` | 94.12 | **100.00** |
| `0046a170` | `stats_object_get_flocks_height` | `entity_stats_float_at130` | **100.00** | **100.00** |
| `0046a190` | `stats_object_get_flocks_randomness` | `entity_stats_float_at134` | **100.00** | **100.00** |
| `0046a1b0` | `stats_object_get_flocks_turn` | `entity_stats_float_at120` | **100.00** | **100.00** |
| `0046a1d0` | `stats_object_get_flocks_tightness` | `entity_stats_float_at128` | **100.00** | **100.00** |
| `0046a1f0` | `stats_object_get_flocks_speed` | `entity_stats_float_at124` | **100.00** | **100.00** |
| `0046a210` | `stats_object_get_flocks_size` | `entity_stats_word_at13c` | **100.00** | **100.00** |
| `0046a230` | `stats_object_get_flocks_goal_update` | `entity_stats_float_at12c` | **100.00** | **100.00** |
| `0046a250` | `stats_object_get_flocks_attack_time` | `entity_stats_float_at138` | **100.00** | **100.00** |
| `0046a270` | `stats_object_get_awareness_range` | `entity_collision_radius_get` | **100.00** | **100.00** |
| `0046a290` | `stats_object_get_pusher_dist` | `entity_stats_float_at114` | **100.00** | **100.00** |
| `0046a2b0` | `stats_object_get_pusher_damage` | `entity_stats_float_at118` | **100.00** | **100.00** |
| `0046a2d0` | `stats_object_get_laser_damage` | `entity_stats_float_at11c` | **100.00** | **100.00** |
| `0046a2f0` | `stats_object_get_freezer_damage` | `entity_stats_float_at110` | **100.00** | **100.00** |
| `0046a310` | `stats_object_get_freezer_time` | `entity_stats_float_at10c` | **100.00** | **100.00** |
| `0046a330` | `stats_object_debug_toggle_self_powered` | `entity_record_flag400_toggle_and_refresh` | **100.00** | **100.00** |

Two functions outside the unit rose: `0046a4c0`
`entity_flag_mask_callbacks_all_pass` (GAME-044) from 90.62% to 96.88%, and
`00429890` `lego_level_load` (GAME-021) from 90.73% to 90.82% (see
[Shared data](#shared-data)). None fell.

## The unit's edges

The unit map assigned 15 functions to GAME-043 with low confidence
(`0046a170`–`0046a330`, "boundary:GAME-044"). All 15 belong to Stats.c, and
the unit ends at `0046a370`:

- **Code.** Each of the 15 reads the object's current stats record
  (`LegoObject +0x334`) at a field offset inside the same 0x150-byte
  `StatsData` record as the high-confidence getters (`00469cb0`–`0046a0a0`).
  The offsets (`+0x10c`–`+0x13c`) are the `Freezer*`, `Pusher*`,
  `LaserDamage`, `Flocks_*` and `AwarenessRange` fields that
  `Stats_Initialise` writes. `0046a330` is `StatsObject_Debug_ToggleSelfPowered`:
  it toggles `SelfPowered` (`flags2` `0x400`) in the same record and calls
  LegoObject.c's `0043c570`.
- **Data.** None of the 15 touches GAME-044's data (`004aa450`–`004aa45c`).
  The unit's string block ends with `"Stats"` at `004aa448`, directly before
  GAME-044's first item at `004aa450`.
- **The next function.** `0046a370` is a recursive record-chain free, the
  first of GAME-044's linked-record functions.

At the other edge, `004667e0` (`Stats_Initialise`) is the only function
that writes `statsGlobs` (`00502bd8`) and the unit's literals. GAME-042
ends at `004667e0`.

## How the conversion is done

### State

Stats.c's `.bss` (`00502bd8`–`00503188`) is one `static Stats_Globs
statsGlobs` laid out as the original:

- `StatsData** objectStats[20]` (`00502bd8`);
- `unsigned int objectLevels[20][15]` (`00502c28`), the `Levels` count;
- `Stats_ToolTaskTypes toolTaskTypes[11]` (`005030d8`), three task types and
  a count per tool.

`StatsData` is the 0x150-byte per-level record. Its fields are named after
the config keys `Stats_Initialise` reads into them; `+0x000`–`+0x01f` and
`+0x104`–`+0x10b` are never loaded. `StatsObject` views the original object at
its Win32 offsets: `type` `+0x000`, `id` `+0x004`, `objLevel` `+0x330`, `stats`
`+0x334`. As finding 570 found, the float fields are read as struct members.

The port uses the same storage. Its loader (`entity_config.cpp` through
`entity.cpp`'s `stats_type_table_install`) and the port tail reach
`statsGlobs` through one `// PORT-ONLY` accessor, `entity_stats_port_globs()`,
at the end of the unit:

- `objectStats[type]` points at the port tail's fixed row storage
  (`entity_stats_port_row_table()`, set on first use);
- `stats_type_table_install` writes the integer level count to
  `objectLevels`;
- the tool-task table is the unit's.

The port's previous globals, `s_stats_type_table` and
`s_entity_indexed_value_groups`, which the frozen `entity_from_entity_stats.hpp`
still declares, are no longer defined or used. `s_stats_cell_level_limit`
stays in the port tail as the port's float copy of the count (see #599).

### The port's names: matching-build link names, not caller aliases

The 62 getters and setters keep their original contract in the matching
build. Every caller passes the port's `EntitySlot`, whose `type`, `etype`,
`indexed_record_slot_330` and `stats_record` sit at the original's `+0x000`,
`+0x004`, `+0x330` and `+0x334` there (asserted in `entity.hpp`). The table
the callers read is the shared `statsGlobs`.

So instead of an alias header in every caller (findings 570 and 572),
[`entity_stats_link_names.h`](../../src/game/entity_stats_link_names.h)
renames the unit's own definitions to the port's names, under
`#if defined(_MSC_VER) && _MSC_VER < 1200`. `stats_object_get_route_speed`
links as `entity_stats_float_at20`, and so on for 62 names. It is included
only by `entity_stats.c`, first. The port tail compiles its own definitions
of those names out of the matching build. As a result:

- every caller reaches the unit's functions in the matching build without
  an edit or an include, in C units (`lego_unit.c`, `legoobject_unit.c`,
  `aitask.c`, `interface_unit.c`, `terrain_cell_objects.c`, …) and in C++
  files alike (`legoobject.cpp`, `lego.cpp`, `weapons.cpp`, `bubbles.cpp`,
  `placement.cpp`, …);
- no caller's call operand changed, so no caller dropped;
- natively the port tail still serves every caller under the same names,
  because native `EntitySlot` has another layout.

A macro renames a definition one for one, so the symbol counter does not
move (finding 538). `stats_initialise` keeps its own name: the port's
loader, `entity_stats_config_load_from_block`, has another contract (hooks,
guarded storage, its own allocation records).

The C++ callers need C linkage. Six `entity.hpp` declarations got
`extern "C"` in place: `stats_table_float_at_dc`, `stats_table_word_at_d0`,
`stats_table_word_at_100` and `stats_instance_word_at_bc`, `_e0`, `_f0`.
`entity_stats_field_at_a0_corpus` is declared only in the frozen
`entity_from_entity_stats.hpp`, which only `entity.cpp` includes. So
`entity.cpp` pre-declares it `extern "C"` before that include, next to the
existing pre-declaration of `entity_building_index_set`. `entity.cpp` has no
`FUNCTION` annotations, so its symbol counter does not matter. No frozen
header is edited.

`00469c10` (`StatsObject_SetObjectLevel`) is switched too. Its callers
(`legoobject_unit.c` directly and through the adapter
`legoobject_gd_building_index_set_adapter`, `legoobject.cpp`, `entity.cpp`)
pass an object of the original layout. In the matching build the unit
guards the new level with the shared `objectLevels`, which the port's loader
now fills with the allocated record count. The port's guard was
`s_stats_cell_span`, the same count. The two transition counters are
LegoObject.c's `004e3944` and `004dee44`, which the unit writes directly.

### Shared data

- **Pooled `":"` and `""`.** The original's `Stats_Initialise` uses the
  linker-pooled `":"` (`004a1cd4`) and `""` (`004b8a14`) that `config.c`
  annotates as `s_config_rgb_separator` and `s_config_angle_empty`. The two
  lose `static` in `config.c` and get `extern "C"` for the C++ build through
  a macro, with no new declaration. The unit uses them in its `STATS_STR`
  macro and its `Util_Tokenise` calls. That removed 56 operand differences
  (single-function score 97.81% → 99.24%; full build 99.29%), and
  `config.c` shows no drift.
- **`"Levels"` (`004a2574`) and `"UpgradeTime"` (`004a4418`).** These are
  pooled with FrontEnd.c and Lego.c, and no file marked them. Unmarked, the
  original shows them as raw immediates while the rebuild names its literal,
  so every later `<OFFSETn>` placeholder in the function was off by one or
  two: 30 differences (full build 99.29% → 99.97%). They are now marked
  `// STRING` in this unit. The
  conversions of GAME-017 (FrontEnd.c) and GAME-021 (`lego_unit.c`, #587)
  must not mark them again. `00429890` `lego_level_load` rose with the
  `"Levels"` marker.
- **`00503188`.** `Stats_FindToolFromTaskType` (`0046a120`) ends its loop at
  `&toolTaskTypes[11]`, the address of the next `.bss` word. That word is
  GAME-044's `s_entity_flag_mask_all_pass` (`record_chains.cpp`; `0046a4c0`
  writes and reads it), which had no annotation. It now has
  `// GLOBAL: LEGORR 0x00503188`, so both sides name the operand.
  `0046a120` went from 94.12% to 100% and `0046a4c0` from 90.62% to 96.88%.

`make unit-data-offsets`, GAME-043 row: `.data` partial (129 strings, 0
unknown). The one gap is `"Stats"` (`004aa448`), 8 bytes later than in the
original: the rebuild keeps a `"Levels"` copy in this object, which the
original pools into FrontEnd.c. `.rdata` is n/a: the unit's ten float
constants are literals, rendered as `<OFFSETn>` on both sides. `.bss` is n/a
(one item, `statsGlobs`).

## `Stats_Initialise` (`004667e0`)

The first full build of a straightforward transcription scored 88.80% at
the original's size (0x32a1 bytes). Nearly every difference was a stack
slot. MSVC 5.0 packs locals and its loop temporaries into shared slots, and
the transcription already paired the same variables as the original
(`pathCoef`/`flocksTurn`, `collRadius`/`flocksTightness` and so on); only the
order of the slots differed. What closed the gap:

1. **Slot order follows use weight, not declaration order.** Reversing the
   declarations changed nothing. Counting static references per slot in the
   original (tracking push depth) against the `/FAs` listing reconciled
   exactly once the original's string temporary was split three ways. It
   shares a slot with `costRefinedOre` in the SingleWidthDig…FunctionCoef
   sections, with `buildingBase` in UpgradeCostOre/Studs, and with
   `collRadius` in the Train* sections. Block-local `char* values` in the
   first two groups, with the function's `str` elsewhere, gives the
   original's slot order (single-function score 88.78% → 97.71%). Making
   every section block-local (96.81%), or only the Train* ones (92.35%),
   does not.
2. **`0.0f == x`** for the RubbleCoef, PathCoef and CollRadius defaults:
   the original loads the constant first (`fld [0.0]; fcomp [x]`). The
   Flocks defaults keep `(x = …) == 0.0f`.
3. **The SeamDrillTime copy** is `if (seam != NULL) copy; else NULL;`.
4. **`(float)atof(s) * 25.0f`**: the original multiplies by a 4-byte
   `25.0f` (`fmul dword`); `(float)(atof(s) * 25.0f)` uses a double.
5. **No `(float)` cast in `STATS_REAL`**: with the cast, MSVC schedules
   `PickSphere`'s `fstp` six pushes later (99.97% → 100%).
6. **`stats_find_tool_from_task_type`** (`0046a120`) walks a
   `Stats_ToolTaskTypes*` with an unsigned tool index and compares
   `taskType == tool->taskTypes[i]`. This was 40% with array indexing; the
   original reloads `count` in the inner loop because it reads it through
   the pointer.

`tools/msvc_permuter.py` (900 s, 24 candidates) found nothing for the
transcription. The slot analysis above did.

## Naked asm

The 14 naked-asm functions (`00469a90`, `00469ac0`, `00469af0`, `00469b20`,
`00469b50`, `00469b80`, `00469bb0`, `00469be0`, `00469f40`, `00469f60`,
`0046a0c0`, `0046a0f0`, `0046a120`, `0046a330`) and the five `#if _MSC_VER`
direct-dereference bodies are all plain C now, and all exact.

## `opt_ref`

| | `main` `3791f0dd` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed (of mapped / exact) | 1,303 / 585 (of 2,496 / 1,174) | 1,303 / 585 (of 2,497 / 1,178) |
| `opt_ref.annotated.game_only` | 1,266 / 556 (of 2,260 / 974) | 1,266 / 556 (of 2,261 / 978) |
| Unit object: removed / exact (`make opt-ref-callers`) | `entity_stats.cpp`: 21 / 21 | `entity_stats.c`: 22 / 22 |
| Unit functions live under `/OPT:REF` | 41 of 62 annotated | 41 of 63 |

The by-symbol listing (`opt_ref_link.py callers`) differs from `main` by one
function: `stats_initialise`, newly annotated. It is removed because its
only original caller, `0041fb50` `lego_game_config_init` (`lego_unit.c`),
is itself removed (finding 579: the Lego.c config root, #587). The other 21
are removed for the same reason as before: their callers are LegoObject.c
port models or Lego.c functions that are themselves removed (#587, #588).
The by-address total is unchanged because three tiny functions
(`0x437440`, `0x438ed0`, `0x484b90`) left the removed set and two
(`0x432550`, `0x4798a0`) joined it. That is address coincidence of removed
port symbols, not a reachability change.

The link-name approach is what keeps the 41 live. Every live port caller of
a Stats.c getter now links to the unit's function in the matching build.
Under per-caller alias headers, the C++ callers (`legoobject.cpp`,
`weapons.cpp`, `lego.cpp`, …) would have kept the port's definitions alive
and left the unit's removed.

## Entropy and drops

`make reccmp-check` against `main`'s baseline: 5 improved (`00469a90`,
`00469c10` and `0046a120` in the unit, `0046a4c0` and `00429890` outside
it), 1 newly mapped
(`004667e0`), **none below baseline**. No exact match is lost and no
function outside the unit changed instruction count. The baseline is
refreshed for the improvements.

## Files outside the unit

None of the files the parallel threads own is edited: `lego_unit.c`,
`lego.cpp` (#587); `legoobject_unit.c`, `legoobject.cpp`,
`legoobject_port_aliases.h` (#588); `frontend.cpp`, `audio.cpp` (#448);
`weapons.cpp` (#592); `gods98/files.cpp` (#593). Their matching-build calls
reach the unit through the link names alone.

| File | Change |
| --- | --- |
| `src/game/entity.cpp` | port loader helpers use `statsGlobs` through the accessor; `extern "C"` pre-declaration of `entity_stats_field_at_a0_corpus` |
| `src/game/entity.hpp` | `extern "C"` on six stats declarations |
| `src/gods98/config.c` | `s_config_rgb_separator`, `s_config_angle_empty` lose `static` (C linkage via a macro) |
| `src/game/record_chains.cpp` | `// GLOBAL: LEGORR 0x00503188` on `s_entity_flag_mask_all_pass` |
| `src/game/test_c_boundaries.cpp` | includes `entity_stats.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-043 game/entity_stats.c`; `tail game/entity_stats_port.cpp` |

## Rebased onto stack #600

Rebased onto #598 (`6654a85a`: #597 GAME-021 part 3, #595 `.data`
padding, #598 Files.c). "Before" is #598's head:

| | #598 head `6654a85a` | this change, rebased |
| --- | ---: | ---: |
| Exact-function code bytes | 113,287 | **113,553** |
| Exact functions, full / game-only | 1,220 / 1,020 | 1,223 / 1,023 |
| Accuracy, full / game-only | 73.11% / 70.39% | 73.17% / 70.46% |
| `opt_ref` full: removed / exact removed | 755 / 335 | **754 / 335** |
| `opt_ref` game-only | 736 / 322 | 735 / 322 |
| Stats.c functions removed under `/OPT:REF` | 21 (of 62) | 18 (of 63) |
| Stats.c exact | 59 | 62 of 63 |

`make reccmp-check` against #598's baseline: 6 improved
(`0041fb50` 93.61% → 93.64%, `00429890`, `00469a90`, `00469c10`,
`0046a120`, `0046a4c0`), `004667e0` newly mapped, none below baseline, no
exact match lost.

- **`004667e0` is 99.92% here, with code identical to the original.** After
  #595 removed the `.data` padding, the rebuild's `.bss` reaches past
  `0x00800000`. Reccmp now renders the immediate flag `0x800000` in
  `or dword ptr [esp+0x1c], 0x800000` (Flocks_Smooth and
  DamageCausesCallToArms) as `s_entity_path_motion_runtime[802]...` on the
  rebuilt side, and the `<OFFSETn>` numbering after it shifts by one. These
  are the only three differences. It is the same scoring artifact finding
  572 recorded for `0x800000` in LegoObject.c, and it is not fixable in the
  unit. Its 12,961 bytes are therefore not in the exact-bytes figure above.
- **The `lego_unit.c` caller switch.** #597 made Lego.c's config init
  (`0041fb50` `lego_game_config_init`) a converted root, live under
  `/OPT:REF`. It called the empty `PORT-ONLY` `stats_initialise_adapter`
  (`lego_port.cpp`) where the original calls `Stats_Initialise`, so
  `004667e0` stayed removed. The contract matches: the same `config` and
  `gameName` arguments, with the result ignored. Natively nothing calls
  `lego_game_config_init`. So `lego_unit.c` now calls `stats_initialise`,
  and the declaration in `lego_unit_imports.h` is renamed in place, so the
  symbol counter does not move. `004667e0` is live, and `0041fb50` rose.
  The adapter in `lego_port.cpp` (#597's file) is now unused and is left in
  place for its owner to delete.
- **Port-tail hardening for the switch.** In the matching build the
  original loader now runs beside the port's. `Stats_Initialise` allocates
  `objectStats[type]` with the type's ID count, while the port's loader
  writes all 15 rows. `entity_stats_port_row_table()` therefore moves an
  array it did not allocate into its fixed 15-row storage, copying the
  loaded rows, before the port writes to it. Natively this path never runs.

## Follow-ups

- #599 (`port`): natively, `building_upgrade_admission_check` returns the
  raw bits of the port's float level count, so the upgrade limit never stops
  an upgrade. The matching build now has the original's integer.
- When the port reads the original object layout natively, the port tail's
  getters can go and the link-name header can apply to every build.
- Delete `lego_port.cpp`'s unused `stats_initialise_adapter` (#597's
  file).

## Validation

On `3791f0dd`:

- `make reccmp-check`: no function below baseline, 63 of 63 exact in the
  unit; winapi link check and link-order map check OK.
- `make match-progress`, `make unit-data-offsets` and
  `python3 tools/translation_units.py build`, generated files committed;
  `tools/binary_progress_inputs.txt` rewritten with
  `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test` (no `--allow-growth`): passes. The lint ledger shrinks
  from 1,442 to 1,441.
- `make winapi-link-check`: OK.
- Native: `ctest`, 37/37 pass (synthetic fixtures).
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only src/game/entity_stats.c` reports nothing.
