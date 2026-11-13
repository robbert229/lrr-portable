# Finding 627 — Dead port `.bss` leaves the matching build

Issue #700. This follows finding 602 (the port-only `.bss` blockers) and
the cuts in findings 613, 614 and 622 (#679, #680, #689). Measured on
`main` `d4c4cf5c` (GAME-038, Roof.c), before and after this change, from
full matching builds in one worktree.

## Summary

1. **266,144 bytes of port `.bss` leave the matching build.** That is
   fifteen port tables in ten files. In the matching build no reachable
   code reads them; at most a reset clears them. Native builds and the tests
   keep every one of them unchanged.
   - Most are port copies of state that a converted unit now owns:
     Dependencies.c's table, Erode.c's erosion lists, the effects unit's rock-fall
     styles, Panel.c's panels, and two LegoObject.c tables.
   - The rest is dead in every build: no code adds a tutorial token or
     touches the NERP cell table.
2. **The gate image ends at `0x850000`, down from `0x892000`.** It is still
   327,680 bytes past `0x800000`, so `tools/reccmp_plain_immediates.py` is
   still needed (finding 584).
3. **Port `.bss` alone cannot get the image under `0x800000`.** The port's
   `.text` and `.rdata` move `.data` 630,784 bytes later than in the
   original. With an original-sized `.data`, the image would still end at
   about `0x80b000`. See [What remains](#what-remains-above-0x800000).
4. **No score moved.** Exact-function code bytes stay at 208,965, and the
   baseline changes only in its input hash. `make unit-data-offsets` gives
   the same output before and after.

## Method

### Measuring

- **By symbol.** Each linked object's COFF `.bss` sections are split at the
  object's CodeView data symbols. Static symbols are included, unlike in the
  map. A symbol owns the bytes up to the next symbol, so the sizes below
  include alignment padding.
- **By group.** `make pe-audit` gives the totals by link-order category.
- **Liveness.** The `/OPT:REF` relink (`tools/opt_ref_link.py relink`)
  keeps only code that something live reaches. For each port table I listed
  every function whose relocations name it (statics through the `.bss`
  section symbol and its addend). I then checked which of those functions
  the relink keeps. A table whose users are all removed is dead in the
  matching build. One whose only kept user clears it, or reads it through a
  count that only removed code raises, is dead as well.

### Retiring

The technique is #689's: a matching-build guard. Each table and its uses
are under `#if !(defined(_MSC_VER) && _MSC_VER < 1200)`. This is the
condition the port tails already use for the names their units take. The
MinGW reference build and native builds compile the code unchanged.

In the matching build, each accessor that stays linked answers as it did
for the all-zero table:
- a count of 0, a NULL name or a 0 word;
- "no free slot" or "every slot free";
- a reset that only zeroes its counter.

Port functions that no build calls are compiled out with their table. No
frozen `_for_`/`_from_` header is edited. Their `extern` declarations stay,
and nothing in the matching build refers to them.

## Before: where the port `.bss` was

Gate `.bss` (map group) was 3,352,144 bytes. It breaks down as:
- commons 2,282,128 (with Files.c's `fileRedundantList`, 2,048,000);
- port tail 524,272;
- units 297,780;
- legacy 225,672;
- unnamed 21,164;
- LIBC 1,128.

Port initialised `.data` is small: tail 27,916 and legacy 6,736.

The largest port `.bss` symbols before this change:

| Bytes | Symbol | Object | In the matching build |
| ---: | --- | --- | --- |
| 64,000 | eight erosion tables (2 × 16,000, 3 × 8,000, 2 × 4,000) | `lava_erosion_port.cpp` | only cleared (`level.cpp`); Erode.c's `erodeGlobs` is the state |
| 58,888 | `s_grid_rec` | `screen_effect_quads_port.cpp` | no live user; Dependencies.c's `dependencyGlobs` is the state |
| 49,320 | `s_nerp_runtime` | `tutorial_hud.cpp` | 12 live users, but its `cells[2048]` (49,156) is never accessed |
| 41,160 | `s_wads` | `resource.cpp` | live (`resource_open`, `wad_port.cpp`) |
| 25,440 | `s_timed_effect_records` | `frontend.cpp` | live (tooltip, interface and effects port code) |
| 25,160 | `s_interface_images` | `frontend.cpp` | live |
| 19,200 | `cmd_pending_command_counts` | `aitask_port.cpp` | live (2 functions) |
| 18,432 | `s_scene_resource_name_slots` | `resource.cpp` | no live user |
| 12,288 | `s_scene_availability_styles` | `resource.cpp` | read only by the 0040c3a0 port model; nothing fills it |
| 10,224 | `s_legacy_panels` | `tutorial_hud.cpp` | no caller in any build; Panel.c's `panelGlobs` is the state |
| 9,728 | `s_weapon_definitions` | `weapon.cpp` | live |
| 8,800 | `s_grid_object_records` | `entity.cpp` | no live user; LegoObject.c's `objectGlobs.recordObjs` is the state |
| 8,800 | `s_deferred_olist` | `cmd.cpp` | only cleared; only removed code adds an entry |
| 8,800 | `s_deferred_olist_records` | `entity.cpp` | live |
| 8,192 | `s_rock_fall_style_names` | `resource.cpp` | no live user |
| 8,112 | `s_frameloop_application` | `frameloop.cpp` | live |
| 8,000 × 2 | `s_model_resource_primary`, `_secondary` | `model.cpp` | live |
| 6,496 | `s_default_counter_state` | `upgrades.cpp` | live: the initialised pointer `s_counter_state` targets it |
| 6,144 | `s_scene_cell_table_pairs` | `resource.cpp` | no live user |
| 6,144 | `s_tutorial_tokens` | `tutorial_hud.cpp` | only cleared; no build adds a token |
| 4,096 + 2,048 | `s_call_rect1`, `s_call_rect3` | `crt.cpp` | no live user |
| 5,120 | `s_pending_record_rows` | `util.cpp` | no caller in any build |

The units' own large statics are the original's state, not port storage:
- `s_sound3d` 185,024 (`sound3d.c`);
- `s_queue` 81,920 (`message_unit.cpp`);
- `erodeGlobs` 64,020;
- `dependencyGlobs` 58,804;
- `aiTaskGlobs` 20,124;
- `lwsGlobs` 16,428;
- `handles` 16,000 (`g98_024.c`).

`frontend_unit.cpp`'s `s_string_queue` (15,540) is FrontEnd.c's state,
which #696 owns.

### The gap before

| | Bytes |
| --- | ---: |
| Image end past `0x800000` | 598,016 |
| `.data` start, against the original's `0x4a1000` (port `.text` and `.rdata`) | +634,880 (`0x53c000`) |
| `.data` virtual size, against the original's 2,936,532 | +520,988 |

The original image ends 585,728 bytes below `0x800000`, so the two excesses
together cost 1,155,868 bytes against that margin. The port `.text` share
is the larger one.

## What changed

| Object | Table | Bytes | Matching-build behaviour |
| --- | --- | ---: | --- |
| `lava_erosion_port.cpp` | `s_lava_erosion_pending_cells`, `s_lava_erosion_stage_coordinates`, `s_erosion_marks`, `s_lava_erosion_stage_timers`, `s_ts_coordinates`, `s_ts_stamps`, `s_ts_active` | 64,000 | Every port function that fills them was already compiled out there (`LRR_LAVA_EROSION_PORT_NAMES`). `level.cpp`'s clear now resets only the counters. `lava_erosion_pending_cell_get` and `lava_erosion_stage_get` return 0 (both were already removed by the relink). |
| `screen_effect_quads_port.cpp` | `s_grid_rec` (Dependencies.c's `004b8bc8`) | 58,888 | `dependencies_port.cpp`'s four `_target` accessors and `level.cpp`'s `cell_flag4_is_set` answer for an empty block. All five were removed by the relink. |
| `tutorial_hud.cpp` | `s_nerp_runtime.cells[2048]` and `cell_count` | 49,160 | No code names either field in any build. `tutorial_hud_shared.hpp` leaves them out of the matching build's struct, so the defined object is 160 bytes. See [the NERP runtime layout](#the-nerp-runtime-layout). |
| `resource.cpp`, `rock_fall_styles_port.cpp` | `s_scene_resource_name_slots`, `s_scene_availability_styles`, `s_rock_fall_style_names`, `s_scene_cell_table_pairs`, `s_scene_pose_slot_claimed`, `s_scene_act_parents` | 48,896 | Their fillers have no caller in any build. These are `rock_fall_styles_config_load`, `scene_act_slot_load_and_register`, `rock_fall_style_names_install` and the three `scene_*_install` functions, and they are compiled out with the tables. `scene_resource_availability_sweep` (the 0040c3a0 port model, still called by `simtick.cpp`) skips the style loop. An empty slot never advances. |
| `tutorial_hud.cpp` | `s_legacy_panels` | 10,224 | The four functions that use it have no caller in any build, and they are compiled out with it. |
| `entity.cpp`, `legoobject_port.cpp` | `s_grid_object_records` | 8,800 | `grid_object_record_state_install` has no caller in the matching build, so the count stays 0. The install keeps its hooks, and the release frees no record. |
| `cmd.cpp`, `legoobject_port.cpp` | `s_deferred_olist` | 8,800 | `deferred_olist_entity_queue_add` (removed by the relink) acts as on a full queue. `deferred_olist_reset` zeroes the counter, and `deferred_olist_entity_queue_take_cell` returns 0. |
| `tutorial_hud.cpp` | `s_tutorial_tokens` | 6,144 | `tutorial_token_add` has no caller in any build. The clear, the inline renderer's two scans and the name/value getters see no token, as before. |
| `crt.cpp` | `s_call_rect1`, `s_call_rect3` | 6,144 | The rect witnesses of the surface-call recorder, which nothing reachable records into. A recorded call reports no witness. The test `test_frontend.cpp` keeps them natively. |
| `util.cpp` | `s_pending_record_rows` | 5,120 | Its only writer, `pending_record_table_install`, has no caller in any build, and it is compiled out. |

The per-table sizes include alignment padding and sum to 266,176. The
`.bss` group shrinks by 266,144.

Left alone, and why:
- **The `roofGlobs` duplicates** that finding 619 named (`record.cpp`'s
  `s_overlay_word`/`s_overlay_cache`, `resource.cpp`'s
  `s_indexed_surface*`) are about 70 bytes. `frameloop.cpp`, `engine.cpp`,
  `tickers.cpp` and `level.cpp` still reach them in the matching build.
  They change no page.
- **`s_default_counter_state`** (6,496) is reachable: the initialised
  pointer `s_counter_state` targets it.

### The NERP runtime layout

`NerpRuntimeState` is declared in three headers:
- `tutorial_hud_shared.hpp`, which `tutorial_hud.cpp` includes and which
  defines the object;
- two frozen copies, `tutorial_hud_for_object_overlays.hpp` and
  `tutorial_hud_for_tutorial_script.hpp`.

Only the first changes. In the matching build its struct ends at
`pointer_y`. The frozen copies still list `cells` and `cell_count` after
it, so the 41 fields both layouts share keep their offsets.

The one place that sized the object through a frozen copy is
`tutorial_display_resources_release`'s `memset`
(`tutorial_script_port.cpp`). In the matching build it now clears
`offsetof(NerpRuntimeState, cells)` bytes. A comment in the shared header
records the rule: code that sees a frozen copy must not touch `cells` or
clear past it.

The mangled name does not depend on the struct's size, so the link is
unchanged. The matching link reports no new warning.

## Before and after

"Gate" is the `/DEBUG` link that Reccmp scores (`make pe-audit`). "Relink"
is the `/OPT:REF` link (`binary-progress.json`).

| | before | after |
| --- | ---: | ---: |
| Gate `.bss` (map group) | 3,352,144 | **3,086,000** (−266,144) |
| — port tail | 524,272 | 264,272 (−260,000) |
| — legacy | 225,672 | 219,528 (−6,144) |
| — units / commons | 297,780 / 2,282,128 | unchanged |
| Gate `.data` virtual size | 3,457,520 | 3,191,376 |
| Gate `.data` initialised (map group) | 105,324 | 105,324 |
| Gate `.text` | 1,289,061 | 1,285,237 (−3,824) |
| Gate `.data` start | `0x53c000` | `0x53b000` |
| Gate image end | `0x892000` | **`0x850000`** |
| Past `0x800000` by | 598,016 | **327,680** |
| Relink `.bss` | 1,304,096 | **1,037,952** |
| Relink `.text` | 1,006,981 | 1,006,645 |
| Relink image end | `0x658000` | **`0x617000`** |
| Exact-function code bytes | 208,965 | 208,965 |
| `opt_ref` annotated (full): removed / exact removed | 398 / 220 | 394 / 216 |
| `opt_ref` annotated (game-only) | 391 / 213 | 387 / 209 |

### The `opt_ref` change is identical-code folding

No annotated function's callers changed. The relink folds identical
function bodies, and keeps one name for each folded body. The names it
keeps changed for some of those bodies. This is what findings 613 and 622
saw.

For example, LegoObject.c's `legoobject_stats_flag10_test`,
`_flag20_test` and `_flag40_test` are now the kept copies. Before, their
port twins in `legoobject_port.cpp` were. The same code is in both images.

Comparing the two relink maps by symbol:
- 14 unit symbols are kept now and were removed before;
- 9 were kept before and are folded away now.

Among the folded-away ones are `cmd_rand15`, `level_current_owner_get` and
`ui_render_record_free`. Each has a kept identical body under another name.

## Data placement

`make unit-data-offsets` gives the same output before and after: full 0,
gaps-only 12, partial 23, n/a 192. This covers the units whose state the
retired tables copied:
- GAME-010 Dependencies.c;
- GAME-012, the effects unit;
- GAME-015 Erode.c;
- GAME-023 LegoObject.c;
- GAME-028 NERPs.c;
- GAME-032 Panel.c.

Their own `.bss` and commons do not move within their units.

## What remains above `0x800000`

The image is 327,680 bytes past `0x800000`. The two causes, measured
against the original:

| Cause | Bytes | Note |
| --- | ---: | --- |
| Port `.text` and `.rdata` (`.data` starts at `0x53b000`, not `0x4a1000`) | 630,784 | On its own, this ends the image at about `0x80b000` |
| `.data` virtual size above the original's | 254,844 | On its own, this ends the image at about `0x7b0000` |

Both have to shrink. Removing every remaining byte of port `.bss` would
still leave the image about 45 KB past `0x800000`. The `/OPT:REF` relink,
which drops 160 KB of unreferenced port `.text`, already ends at
`0x617000`.

### Port `.bss` that is left

These are the symbols of 4 KB or more, by owner. "Live" means the relink
keeps a user that reads or writes the table, not only one that clears it.

| Rank | Bytes | Symbol | Owner | What removes it |
| ---: | ---: | --- | --- | --- |
| 1 | 41,160 | `s_wads` | `resource.cpp` | The port's `resource_open`/`ares_*` layer still reads it in the matching build, and `wad_port.cpp` mounts every archive into it as well as into Wad.c's tables (finding 591). Files.c and Wad.c are converted, so this needs a caller switch from the port resource layer to `File_Open`, not a conversion. |
| 2 | 25,440 | `s_timed_effect_records` | `frontend.cpp` | The port copy of ToolTip.c's records (`0054c000`, `tooltip.c`'s `toolTipGlobs`, finding 597). The live users are `tooltip_port.cpp`, `interface_port.cpp`'s `timed_effect_persistence_set_corpus` and `effects.cpp`'s `timed_effect_record_storage_get`. It goes when those port callers use `tooltip.c`. |
| 3 | 25,160 | `s_interface_images` | `frontend.cpp` | Live (`interface_images_config_load`, `interface_image_peek`). It goes with the FrontEnd.c conversion's later parts (#696 owns part 1). |
| 4 | 19,200 | `cmd_pending_command_counts` | `aitask_port.cpp` | The port's state for `004b34f4` (GAME-002). Two live port functions use it: `terrain_cell_record_count_update` and `_prune`. It goes when their callers reach the unit. |
| 5 | 16,000 | `s_model_resource_primary`, `_secondary` | `model.cpp` | Live (`model_resource_resolve_or_load`), and no converted unit owns it yet. It goes with the conversion that owns the model resource cache. |
| 6 | 15,540 | `s_string_queue` | `frontend_unit.cpp` | FrontEnd.c's own state (#696). |
| 7 | 13,160 | `s_active_effect_records`, `s_active_effect_paths` | `effects.cpp` | The port's records for the original's `00503920` table. Six live users. It goes with the conversion that owns `00503920`. |
| 8 | 9,728 | `s_weapon_definitions` | `weapon.cpp` | The port copy of Weapons.c's table (`weapons.c`'s `weaponGlobs`). Live through `weapon_definition_at` and `entity_indexed_float_lookup_with_minus_one_fallback`. It goes when those callers reach `weapons.c`. |
| 9 | 8,832 | `s_winapi_startup_adapters` | `surface.cpp` (legacy) | The WinAPI port's start-up dialog. Init.c (LATE-002, finding 613) is converted but has no caller switch yet. It goes when the WinAPI frame loop calls Init.c. |
| 10 | 8,800 | `s_deferred_olist_records` | `entity.cpp` | Live (`deferred_olist_entity_level_end`, `_queue_install`). It goes with LegoObject.c's remaining port callers. |
| 11 | 8,112 | `s_frameloop_application` | `frameloop.cpp` (legacy) | The WinAPI frame loop's state. It goes with Main.c. |
| 12 | 8,192 | `s_cd_root`, `s_resource_directory_root` | `resource.cpp` | Files.c's paths (`fileGlobs`). They go with the same caller switch as `s_wads`. |
| 13 | 6,496 | `s_default_counter_state` | `upgrades.cpp` | Live through `s_counter_state`. |
| 14 | 5,760 | `s_help_info_strings`, `s_help_info_string_copies` | `helpwindow_port.cpp` | HelpWindow.c is converted. The native loader `help_window_info_config_strings_load_native` is still live. |
| 15 | 5,120 | `s_terrain_activity_build_records` | `level.cpp` (legacy) | Live (4 functions, `lego_port.cpp`). |
| 16 | 4,640 | `s_entity_object_name_load_state` | `entity_config.cpp` | Live (3 loaders). |

Everything else is under 4 KB per symbol. The port tail and legacy groups
hold 483,800 bytes in all. That figure still counts G98-009's
`s_sound3d` (185,024) under `audio.cpp`, as finding 622 explains.

### Port `.text` that is left

The gate's `.text` is 1,285,237 bytes against the original's 642,549. Of
that, the port tail is 570,096 and the legacy objects 97,536. The relink
drops 160,400 bytes of the port tail as unreferenced. The gate keeps it
because `/DEBUG` turns `/OPT:REF` off (finding 576).

The largest port objects by `.text`:
- `legoobject_port.cpp` 76,384 (LegoObject.c's tail);
- `lego_port.cpp` 47,520 (Lego.c's adapters);
- `frameloop.cpp` 44,784 (legacy, Main.c);
- `frontend.cpp` 36,048 (FrontEnd.c);
- `aitask_port.cpp` 31,088;
- `scene_render.cpp` 25,248;
- `entity.cpp` 25,136;
- `interface_port.cpp` 22,688;
- `surface.cpp` 21,440 (legacy);
- `level.cpp` 21,264 (legacy).

Each shrinks as the callers of its converted unit are switched and its
dead models are deleted.

## Verification

- `make reccmp-check`: no function fell below the match baseline. The
  refreshed baseline differs only in `input_sha256`. The plain-immediate
  check, `winapi-link-check` and `check-map` report OK. The build prints the
  same three compiler warnings as before, all in files this change does
  not touch.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`. Accuracy is unchanged (89.03% full, 87.97%
  game-only).
- `make unit-data-offsets`: identical output before and after.
- `make opt-ref-callers` was run. The by-symbol changes are the folding
  swaps above.
- `make pe-audit` and `make tools-test` (no `--allow-growth`) pass.
- Native: `cmake -S . -B build`, then `cmake --build build` and `ctest`:
  37 of 37 pass. The change only adds `_MSC_VER < 1200` guards, so the
  native preprocessed source is unchanged.
- MSVC 5.0: only preprocessor guards, `(void)` casts, early returns and
  `offsetof` from `<stddef.h>`.
