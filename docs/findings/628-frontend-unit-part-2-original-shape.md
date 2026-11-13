# Finding 628 — FrontEnd.c (GAME-017) part 2 rebuilt in original shape; `frontend_port.cpp` joins the port tail

Issue #707, part 2 of 2, under [ADR 0005](../adr/0005-original-shape-c-units.md);
part 1 is [finding 623](623-frontend-unit-part-1-original-shape.md). This
change rebuilds `00414df0`-`00418190` of FrontEnd.c in
[`src/game/frontend_unit.c`](../../src/game/frontend_unit.c), after part 1:

- the options callbacks;
- the level sets and their link graph;
- the movies and splash screens;
- the menu loader (Front_LoadMenus);
- the screen-menu entry points;
- the save files.

`frontend_port.cpp` keeps no annotation, so it moves from a `legacy` entry
to the port tail.

Stacked on #714 (`match/709-nearmiss-sweep3`, `674cd564`), on #713. "Before"
is #714's committed *match baseline* and `binary-progress.json`; no part-2
score differs from `main` `7600fb4c`, where the change was first measured.
"After" is the full matching build of this change (`make reccmp-check`,
`make match-progress`).

## Result

| | before | after |
| --- | ---: | ---: |
| Part 2 similarity (79 functions, weighted by span, 13,216 bytes) | 16.89% | **97.85%** |
| Exact functions in part 2 | 18 | **65** |
| Mapped functions in part 2 | 62 | **79** |
| Exact-function code bytes (`binary-progress.json`) | 252,058 | **260,058** |
| Accuracy, full / game-only | 93.28% / 92.66% | **94.44% / 93.94%** |
| `opt_ref` annotated (full): removed / exact removed | 262 / 163 | **192 / 126** |
| `opt_ref` annotated (game-only) | 256 / 157 | 186 / 120 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,113,328 | 1,115,552 |
| Gate image end (`make pe-audit`) | `0x86e000` | `0x871000` |
| Reccmp lint ledger (entries) | 971 | **964** |

Spans run to the next function, padding included; an unmapped function
counts as 0%. Everything is C: no naked asm, no `Deviation (scoring only)`
and no filler declarations.

**The unit has 79 functions here, not 66.** The decompile manifest misses 13 that are reached only
through pointers or pushes:

- the eight menu callbacks `00414e50`-`00414f20` and `00415190`;
- `00417240` and `00417250` (Front_LevelLink_RunThroughLinks visitors);
- the level-select callbacks `00417460` and `00417700`;
- the menu-item visitor `004177e0`.

Their starts come from the immediates in Front_LoadMenus (`004161f0`),
Front_Load(Levels) (`00415a10`) and `004179b0`, and from the call graph.
`00417250` had a port annotation; the other twelve had no source at all.
The names follow OpenLRR's `Front_*` in snake case (`inferred`). In the
matching build, 44 of them take the port's link names (below); the table
gives the unit's own names.

| Address | Function | Span | Before | After |
| --- | --- | ---: | ---: | ---: |
| `00414df0` | `front_callback_slider_music_volume` | 32 | 100.00 | **100.00** |
| `00414e10` | `front_get_music_volume_slider` | 64 | 50.00 | **100.00** |
| `00414e50` | `front_callback_cycle_wall_detail` | 48 | unmapped | **100.00** |
| `00414e80` | `front_callback_cycle_auto_game_speed` | 32 | unmapped | **100.00** |
| `00414ea0` | `front_callback_cycle_music` | 32 | unmapped | **100.00** |
| `00414ec0` | `front_callback_cycle_sound` | 32 | unmapped | **100.00** |
| `00414ee0` | `front_callback_cycle_help` | 48 | unmapped | **100.00** |
| `00414f10` | `front_callback_trigger_replay_objective` | 16 | unmapped | **100.00** |
| `00414f20` | `front_callback_slider_game_speed` | 112 | unmapped | **100.00** |
| `00414f90` | `front_update_game_speed_slider` | 160 | 33.73 | **100.00** |
| `00415030` | `front_calc_slider_game_speed` | 128 | 48.57 | **100.00** |
| `004150b0` | `front_callback_select_load_save` | 160 | 26.83 | **100.00** |
| `00415150` | `front_update_options_slider_levels` | 64 | unmapped | **100.00** |
| `00415190` | `front_callback_trigger_back_save` | 144 | unmapped | **100.00** |
| `00415220` | `front_load_selected_level` | 160 | 16.67 | 97.83 |
| `004152c0` | `front_save_get_save_summary` | 160 | unmapped | **100.00** |
| `00415360` | `front_refresh_game_speed_slider` | 16 | 25.00 | **100.00** |
| `00415370` | `front_options_update` | 320 | 21.19 | **100.00** |
| `004154b0` | `front_level_set_load_level_names` | 592 | 16.95 | **100.00** |
| `00415700` | `front_play_movie` | 192 | unmapped | **100.00** |
| `004157c0` | `front_play_intro_splash` | 336 | unmapped | 95.89 |
| `00415910` | `front_play_intro_movie` | 128 | 18.33 | **100.00** |
| `00415990` | `front_play_level_movie` | 128 | 11.68 | **100.00** |
| `00415a10` | `front_load_levels` | 736 | 0.00 | **100.00** |
| `00415cf0` | `front_save_clear_save_number` | 16 | 100.00 | **100.00** |
| `00415d00` | `front_load_menu_text_window` | 1104 | 8.42 | 97.46 |
| `00416150` | `front_play_level_stream` | 80 | 62.07 | **100.00** |
| `004161a0` | `front_play_tutorial_stream` | 80 | 55.38 | **100.00** |
| `004161f0` | `front_load_menus` | 1824 | unmapped | **100.00** |
| `00416910` | `front_save_options_to_save_data` | 48 | 16.22 | **100.00** |
| `00416940` | `front_restore_options` | 128 | 9.41 | **100.00** |
| `004169c0` | `front_prepare_screen_menu_type` | 704 | 12.00 | **100.00** |
| `00416c80` | `front_run_screen_menu_type` | 128 | 22.22 | **100.00** |
| `00416d00` | `front_is_front_end_enabled` | 80 | 10.00 | **100.00** |
| `00416d50` | `front_is_mission_selected` | 16 | 100.00 | **100.00** |
| `00416d60` | `front_is_tutorial_selected` | 16 | 100.00 | **100.00** |
| `00416d70` | `front_get_selected_level` | 96 | 10.00 | 96.67 |
| `00416dd0` | `front_is_quit_selected` | 16 | 100.00 | **100.00** |
| `00416de0` | `front_is_quit_mission_selected` | 16 | 100.00 | **100.00** |
| `00416df0` | `front_is_restart_mission_selected` | 16 | 100.00 | **100.00** |
| `00416e00` | `front_get_current_level_set` | 32 | 100.00 | **100.00** |
| `00416e20` | `front_level_set_index_of` | 80 | 100.00 | **100.00** |
| `00416e70` | `front_levels_reset_visited` | 96 | 100.00 | **100.00** |
| `00416ed0` | `front_level_set_set_level_visited` | 112 | 32.79 | **100.00** |
| `00416f40` | `front_level_set_is_level_visited` | 112 | 41.86 | **100.00** |
| `00416fb0` | `front_level_set_set_level_link` | 112 | 32.79 | **100.00** |
| `00417020` | `front_level_set_get_level_link` | 112 | 41.86 | **100.00** |
| `00417090` | `front_level_set_load_level_links` | 304 | 17.69 | 90.32 |
| `004171c0` | `front_level_link_run_through_links` | 128 | 88.24 | **100.00** |
| `00417240` | `front_level_link_callback_inc_count` | 16 | unmapped | **100.00** |
| `00417250` | `front_level_link_callback_find_by_link_index` | 48 | 85.71 | 85.71 |
| `00417280` | `front_level_link_find_by_link_index` | 80 | 100.00 | **100.00** |
| `004172d0` | `front_level_link_get_set_index` | 32 | 100.00 | **100.00** |
| `004172f0` | `front_levels_update_available_recurse` | 240 | 27.17 | **100.00** |
| `004173e0` | `front_levels_update_available` | 80 | 30.14 | **100.00** |
| `00417430` | `front_save_get_level_score` | 48 | 90.91 | 90.91 |
| `00417460` | `front_callback_select_mission_item` | 672 | unmapped | 94.61 |
| `00417700` | `front_callback_select_tutorial_item` | 224 | unmapped | **100.00** |
| `004177e0` | `front_level_link_callback_load_menu_item` | 464 | unmapped | 69.33 |
| `004179b0` | `front_levels_load_menu_items` | 224 | 14.39 | **100.00** |
| `00417a90` | `front_save_read_save_file` | 320 | 11.18 | **100.00** |
| `00417bd0` | `front_save_write_save_file` | 544 | 14.35 | 99.38 |
| `00417df0` | `front_save_read_save_files` | 96 | 29.70 | 96.97 |
| `00417e50` | `front_save_get_save_data_at_index` | 32 | 33.33 | **100.00** |
| `00417e70` | `front_save_get_current_save_data` | 32 | 100.00 | **100.00** |
| `00417e90` | `front_save_get_save_number` | 16 | 100.00 | **100.00** |
| `00417ea0` | `front_save_set_save_number` | 16 | 100.00 | **100.00** |
| `00417eb0` | `front_save_set_level_completed` | 112 | 10.53 | 95.45 |
| `00417f20` | `front_save_set_save_struct_18` | 32 | 76.92 | **100.00** |
| `00417f40` | `front_save_set_reward_level` | 80 | 34.78 | 96.43 |
| `00417f90` | `front_save_get_reward_level` | 48 | 75.68 | 94.44 |
| `00417fc0` | `front_save_write_current_save_file` | 32 | 10.81 | **100.00** |
| `00417fe0` | `front_is_load_save_menu` | 16 | 100.00 | **100.00** |
| `00417ff0` | `front_set_load_save_menu` | 16 | 100.00 | **100.00** |
| `00418000` | `front_save_reset_current_save` | 64 | 10.00 | **100.00** |
| `00418040` | `front_save_copy_save_data` | 128 | 24.64 | **100.00** |
| `004180c0` | `front_save_restore_save_data` | 80 | 40.00 | **100.00** |
| `00418110` | `front_levels_set_update` | 16 | 100.00 | **100.00** |
| `00418120` | `front_save_is_game_100_percented` | 112 | 35.29 | **100.00** |

### What is left

- **`004177e0` (69.33%)**: the code and the instruction count are the
  original's. `x` is spilled to its stack slot one statement earlier, so
  Reccmp aligns the FrontEndX/FrontEndY blocks one block apart.
  - Tried: the struct access inline or through a local;
  - `msvc_permuter.py` (300 s, no candidate);
  - a sweep of the locals' declaration order, stopped as too large
    (5,040 orders).
- **`00417090` (90.32%)**: the original's branch layout, reached with a
  nested `if` and one `goto failed`. The level name and the index trade
  `ebx` and `esi`.
- **`00417460` (94.61%)**, **`004157c0` (95.89%)**: scheduling of the first
  loads, and the order of the 0.0f compare against the store.
  `00417460`'s 300-byte buffer gives the original's 0x170 frame.
- **`00415370`** reaches 100% on #713: Objective.c's Objective_IsEnded
  (`004589a0`) now takes the name `objective_is_ended_original` in the
  matching build (finding 631), which the unit already calls.
- **`00415220` (97.83%)** and **`00417bd0` (99.38%)**: ObjectRecall.c's
  `00459360` has no port entry, and `004593e0` (`osf_write`) takes the game
  context. The unit calls two unannotated no-op adapters at the end of
  `frontend_port.cpp`.
- **`00417250`, `00417430`, `00417f90`, `00417eb0`, `00417f40`**: register
  or address-operand order (`[ecx+eax]` against `[eax+ecx]`).
  `00417430` was 90.91% before (effective match).

### Part 1's naked asm

Not revisited. The brief asked to revisit `00410a10`, `00410d50`,
`00413f90` and `004103f0` if part 2 suggested a C form. Part 2 has no
store-order or flag-fold case like theirs, and part 1's code is unchanged
here. They stay as finding 623 left them.

## Unit extent

**Finding 623's extent holds**: `004102b0`-`00418190`, one object, now
145 functions (66 in part 1 and 79 here).

- **The literal pool.** Part 2's float and double literals continue part
  1's pool, `0049e170`-`0049e22c`, with no repeated value. In order of first
  use:
  - `0.11111111f` (`0049e1e0`) and `9.0f` (`0049e1e4`) in `00414df0`,
    `00414e10`;
  - the game-speed steps `0049e1e8`-`0049e1f4` (`00414f20`, `00415030`);
  - the doubles `0049e1f8`-`0049e218` (`00414f90`);
  - `-1.0f` (`0049e220`, `00415700`), `-1000.0f` (`0049e224`, `004157c0`)
    and `500.0f` (`0049e228`, `00417460`/`00417700`).

  `0.5f` (`0049e1b0`), `1.0f` (`0049e174`), `2.0f` (`0049e1a8`), `25.0f`
  (`0049e1a4`), `100.0f` (`0049e180`) and `0.0f` (`0049e170`) are part 1's
  entries, reused. So the two halves are one object (findings 599 and 603).
- **`.bss`.** `004db8e0`/`004db8e4` (Front_Options_Update's menu and menu
  set, `00415370`) close FrontEnd.c's `.bss` before HelpWindow.c's
  `004db8e8`. `004db8c4` (`s_loadSaveTextShown`) is shared with part 1.
- **`.data`/strings.** `004a1f10`-`004a1f30` (`Menu::SaveText`,
  `Menu::LevelText`, `Menu::Overwrite`) and `004a24fc`-`004a2794`. Every
  reference comes from the unit, except:
  - `"Levels"` (`004a2574`), which `entity_stats.c` marks;
  - `"%s"` (`004a1d60`) and `"rb"` (`004a2770`), which `files.c` marks.
- **frontGlobs.** `00556fc0`-`00557840`, as finding 623, now with the menu
  sets, the two level sets (0x14 each), the start links, the menu values
  (`00557010`, indexed by menu type in `004179b0`), the six save slots
  (0xb8 each, `00557080`-`005574d0`), the save number and the level-select
  stream state (`00557834`-`00557840`).

`translation_units.py build`: 0 conflicts. Only the source column of
`translation-units.csv` changes.

## How the conversion is laid out

- **Declarations after part 1.** Part 2's records (Front_LevelLink,
  Front_LevelSet, Front_SaveData and its rewards, the visitors' states),
  stand-ins, imports and prototypes are declared in one block after part 1's
  last function. The original declared them all in its headers, before every
  function. Moving them ahead of part 1 would shift MSVC's symbol counter at
  every part-1 function (finding 538, rule B):
  - a probe with 1-32 inert `extern`s ahead of part 1 left every exact
    function exact;
  - but it moved near misses both ways (`00411870` falls at 2,
    `00411360` rises at 4).

  As placed, part 1's code is unchanged in the full build. Part 1's two
  views of the 0x728-byte menu text-window record now name every field (the
  same declarations); `00412750`'s level and tutorial texts read the
  fields at `+0x528` and `+0x628` by those names.
- **State.** frontGlobs fields are annotated stand-ins (`frontGlobs_*`).
  `00556ff8` takes Reward.c's name `g_front_save_tree` in the matching
  build, and `frontend_port.cpp` compiles its own definition out there.
  The port annotations these replace were removed; the port keeps its
  variables:
  - `savegame.cpp`: `00556fd0`, `00556fdc`, `00556fe0`, `00556fe4`,
    `00556ff0`, `00556ff4`, `005574d0`, `00557080`, `0055781c`;
  - `frontend.cpp`: `0055701c`, `00557020`, `00557024`, `00557048`,
    `0055704c`, `00557500`;
  - `frontend_port.cpp`: `0049e1e0`.
- **Link names.** `frontend_unit_link_names.h` gives 44 part-2 functions
  the port's names in the matching build, as Container.c (finding 624) and
  part 1. A port name qualifies when it has C linkage and the original's
  contract; a port prototype may add trailing arguments that the original
  ignores. `frontend_port.cpp` compiles its definitions of those names out
  of that build (47 blocks with the shared guard), so Lego.c, Reward.c and
  the port's C++ callers reach the original. Natively the port's
  definitions stay live and nothing calls the unit, except where noted.
  `extern "C"` was added in place to the port declarations of 15 of those
  names:
  - `savegame.hpp`: 8;
  - `options.hpp`: 3;
  - `sound.hpp`: 2;
  - `audio.hpp`: 1, plus `music_mixer_volume_set`;
  - `frontend.hpp`: 1.

  `audio.cpp`'s `ledger_stub_004789a0_MusicMixerVolumeGet` gets C linkage
  and returns its result. It is still a tail call: 100%.
- **savegame.h's types.** Part 1 already includes `savegame.h` and calls
  two of these names, so the unit's Front_Save_GetSaveDataAtIndex,
  Front_Save_GetCurrentSaveData and Front_Save_GetRewardLevel return its
  `void*`, and their users cast (`FRONT_CURRENT_SAVE()`). The code is the
  same, and the matching compile has no warnings.
- **Callers renamed instead.** Some port prototypes take another argument
  list. For these, the C headers rename their callers in the matching
  build only, outside the unit (`LEGORRR_FRONTEND_UNIT_C`). Each macro
  renames its own declaration one for one, so the callers' symbol numbering
  holds:
  - `savegame.h` and `savegame.hpp`: `sav_tree_load` and `save_commit`;
  - `save_mark_record(ctx, i)` passes `i` to Front_Save_SetLevelCompleted;
  - `frontend_api.h`: `menu_enter(fe, mode)` passes `mode` to
    Front_RunScreenMenuType.

### Callers switched (matching build)

| Caller | Before | After |
| --- | --- | --- |
| Lego.c (`lego_unit_imports.h`) | eleven original-ABI adapters in `lego_port.cpp`, `options.cpp` and `frontend_port.cpp`, for `00414f90`, `00415150`, `00415220`, `00415370`, `004157c0`, `00415910`, `00415990`, `004161f0`, `00416c80`, `00416d00`, `00416d70` | the unit's functions (`#define` in the matching-build block) |
| Reward.c (no source change) | the port's definitions of the 44 linked names and `front_campaign_snapshot_create` | the unit's |
| Reward.c `00462030`, `00462270` | `save_mark_record(NULL, i)`, `menu_enter(NULL, 4)`, `sav_tree_load`/`save_commit` (port models) | Front_Save_SetLevelCompleted(i), Front_RunScreenMenuType(4), the unit's (macros in `savegame.h`, `frontend_api.h`) |
| `frontend.cpp`, `savegame.cpp`, `frameloop.cpp`, `options.cpp` and the other port callers of the 44 names | the port's definitions | the unit's (the definitions are compiled out) |
| part 1 (`00412750`, `00413a60`, `00413b80`, ...) | `sav_build_ctx_target`, `save_read_slot_target`, `save_menu_refresh_target` (port) | the unit's, under those names |

### What the shapes needed

- `00414f20`: case 2 shares the default's 1.0f (the jump table has six
  entries).
- `00414e10`: the half-sum goes through a float local, so MSVC does not fold
  `0.5f * 9.0f`.
- `004152c0`: the completed count over the unsigned mission count, without
  a cast. That gives the original's qword `fild` and dword `fidiv`.
- `00416150`/`004161a0`: the miss path has no `return`. The original leaves
  SFX_GetType's 0 in `eax`; native C++ returns 0 under `#ifdef __cplusplus`.
- `00415700`, `004157c0`: `while (1)` loops with `return`/`break`, not
  `do`/`while`. MSVC rotated the latter.
- `00417df0`: an unsigned counter, and the shared trailing read after an
  `if`/`else`.
- `00415370`: one `result` returned at the end.
- `004173e0`: the state's fields assigned in declaration order. This one
  went from 70% to 100% after a sweep of the six orders.
- `00417460`: initialised declarations in the original's order
  (`char levelNumber[64] = { 0 }` after the lookup).
- `004154b0`, `00417090`: the null-name branch first, and nested `if`s with
  one shared exit.
- STRING markers bind to the first literal on the next line. Several
  `string_path_join` calls are split so each marked literal starts its
  line; Reccmp reported the first draft's 17 misbound markers.

## Entropy and drops

`make reccmp-check` against `main`'s baseline:

- 52 functions improved and 17 were newly mapped (against #714's baseline,
  the same sets);
- no exact match is lost;
- part 1 is unchanged;
- one drop is **flagged for owner review**:
  - `00462030` `reward_save_menu` (Reward.c, #709), 64.00% to 63.75%,
    instructions −3 (`drift-review`). Its calls to `00417eb0` and
    `00416c80` now pass the original's single argument, so the three extra
    `push 0`s are gone. The registers re-rotate around them. Without the
    `savegame.h`/`frontend_api.h` macros it was 62.15%, and those macros
    take `00462270` `reward_advance` from 83.84% to 90.82%.

Outside the unit, ten functions rose:

- Lego.c: `0041fb50`, `004232e0`, `004250c0`, `00425ce0` (to 100%),
  `00427f80` (to 100%), `00429890`, `00435940`;
- Reward.c: `0045f530` (to 100%) and `00462270`;
- `0047c8f0`.

The listing digests change at 127 addresses (`make match-progress`).

## `opt_ref` (finding 579)

`opt_ref` falls from 262/163 to 192/126 (−70 removed, −37 exact; on
`main` it was 327/198 to 246/153). Lego.c's
adapters and the port's C++ callers now reach part 2, and through it part
1's Front_LoadMenuSet, Front_RunScreenMenu and their callees.
`make opt-ref-callers OPT_REF_ARGS="--object frontend_unit.c.obj --min 1"`
leaves seven of the unit's functions removed (5 exact). Their roots are
Reward.c's `00462030` and `00462270`, which `/OPT:REF` removes in this
build.

## `.bss` and the image

The relink `.bss` grows by 2,224 bytes, and the gate image end moves from
`0x86e000` to `0x871000`. The unit adds frontGlobs' stand-ins
(`00556fc0`-`00557840`, mostly the six 0xb8 save slots and the four
overwrite strings). The port keeps its own copies in `savegame.cpp`,
`frontend.cpp` and `frontend_port.cpp`, which the native front end runs
on. `make unit-data-offsets`: GAME-017 `.data` partial (103, 3 unknown),
`.bss` partial (17, 0 unknown), commons as listed.

## Native behaviour

Native behaviour is unchanged. The unit's part-2 names are its own natively
(C++ linkage). Its callees are declared `extern "C"` in the part-2 block.
`frontend_port.cpp` keeps every port definition natively, and the port's
`g_front_save_tree` keeps its name (the unit's is
`frontGlobs_startMissionLink` there).

## Matching-build runtime

frontGlobs' stand-ins are not the port's state. In the matching build the
port's front end now calls part 2's accessors (the save number, the level
sets, the menu flags), and those read the stand-ins, which the port does
not set. Part 1's port models already behave this way (finding 623). The
matching build is scored, not played; a playable matching build needs the
port's front end to run on frontGlobs (`port` follow-up, #552).

## Files outside the unit

No function body is edited in a file another thread owns. Outside
`frontend_unit.c`, `frontend_unit_link_names.h` and `frontend_port.cpp`:

- `lego_unit_imports.h` (#708): eleven `#define`s in the existing
  matching-build block;
- `savegame.h`, `savegame.hpp`, `frontend_api.h`: the caller renames;
- `savegame.hpp`, `options.hpp`, `sound.hpp`, `audio.hpp`, `frontend.hpp`:
  `extern "C"` in place;
- `audio.cpp`: `004789a0`'s linkage and return;
- `savegame.cpp`, `frontend.cpp`: `GLOBAL` annotations removed;
- `link_order.cmake`: `frontend_port.cpp` from `legacy` to the port tail;
- the generated files and `translation-units.csv`.

## Validation

- `make reccmp-check`: one flagged drop (above), no exact match lost.
  - plain-immediate check: OK;
  - `make winapi-link-check`: OK;
  - link-order map check: OK.
- `make match-progress` and `make unit-data-offsets`: run, and the three
  generated files are committed.
- `make tools-test` (no `--allow-growth`): passes. The lint ledger goes
  from 971 to 964 entries: seven unannotated definitions in `frontend_port.cpp`
  are exempt now that it is a tail file.
- Native: `cmake --build build` and `ctest --test-dir build` (with
  `TEST_WADS_DIR`): 39 of 39.

## Reproduce

    make match-function SOURCE=src/game/frontend_unit.c
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object frontend_unit.c.obj --min 1"
