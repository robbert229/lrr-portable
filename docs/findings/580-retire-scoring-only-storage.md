# 580. Retiring the scoring-only storage and the named-section padding

Issue #448, stage S6a of the #388 plan (ADR 0006 decision 5). Measured on #597's head `d965c04d` (GAME-021 part 3, on `main` `c58c20c9`), which this change is stacked on. A first measurement on `main` `3791f0dd` gave the same 17 drops with the same causes. This change removes every layout device that finding 516 lists under "Layout devices already in `src/`" apart from the LIBC `/include` directives. It records the score drops instead of restoring them.

## Summary

1. **The three named-section devices are gone.** The matching build's `.data` raw size falls from 1,005,056 to 92,160 bytes (original 73,216). The initialised `.data` in the original-mode link (`opt_ref`) falls from 1,002,797 to 90,124 bytes.
   - `frontend.cpp` `.data$zzzzzz` (741,632 bytes): `s_menu_action_table_gap[0x3aad0]`, `s_reference_menu_data_pad0[0x79144]` and `s_menu_global_bytes[0x13c0]`, all zero, plus the 74-pointer `s_interface_menu_action_labels`.
     - The two gaps existed only to land the menu globals and the label table at their original offsets (`0x4dced0` for the labels).
     - Since #365, every menu word the original addresses individually has its own annotated `.bss` global (`s_fe_flow_*`, `s_menu_wipe*`, `DAT_0055613c`, `DAT_00556f50/54`).
     - The winapi `s_fe_flow` macro into `s_menu_global_words` had no remaining reader: `FE_FLOW_*` resolve to the named globals.
     - The label table keeps its initialiser and no annotation. It now falls where the link puts it: `.rdata`, since it is `const` and no longer in a named section (+288 bytes there).
     - The `MENU_ACTION_TABLE_ALLOC` macro in `frontend_for_interface.hpp` went with it. That is a preprocessor-only edit to a private relocation header, and `interface` never used the macro.
   - `audio.cpp` `.data$yyyyyy` (171,041 bytes): `MenuCompatData` held `0.11111111f` at `+0x25a08` and `"Null"` at `+0x29c1c`, to put both at original `.data` offsets. Nothing referenced it after `frontend_unit.cpp` gained `s_menu_volume_scale` (`0x0049e1e0`) and `s_menu_null_label` (`0x004a23f4`), so it was dead storage.
   - `cmd.cpp` `.data$cmdpool`: already empty. The pool trio (`s_blocks`, `s_free` and the rest) moved to `aitask_port.cpp`, where `s_free` sits at `0x4b31f8` in declaration order. Only the empty `#pragma data_seg` pair and its comment were left, and they are removed.
2. **`key_names` loses its spare slot** and is back to the original's 256 slots (`unit-data-offsets`: `005765e0:_key_names(size=1024)`). The three devices ADR 0006 allows were tried in order:
   - **Annotate the next table.** Not possible yet. In the original, Files.c's directory-token table follows at `005769e0`. In the rebuild, the next common is `rewardGlobs` (`008cb320` after `key_names` at `008caf20`), because common order is not the original's (S6b). An annotation on Files.c's table would not sit after `key_names` in the rebuild, and it would also edit `gods98/files.cpp`, which #593 owns.
   - **A Reccmp `data_sources` row.** Prototyped with `address,type,size` / `0x005765e0,global,0x404`, rerunning Reccmp on the same build. The row sizes only the original-side entity: Reccmp's CSV ingest calls `batch.set(ImageId.ORIG, …)`. The original side then reads `key_names[256] (OFFSET)`, but the rebuilt side still reads `rewardGlobs (DATA)`. The score stays 96.77%, so the row was not kept.
   - **Accept 96.77%.** This is what the change does. `Key_Find` (`0x00485e30`) differs only in its loop bound's name. The bytes are identical, and the whole-binary report counts them.
3. **16 more drops share one Reccmp artifact.** These functions test or set the flag bit `0x800000`.
   - The original image ends at `0x771000`, so on the original side `0x800000` stays a constant.
   - The rebuilt image is larger (`.bss` is 3.8 MB against about 2.9 MB). Before this change, `0x800000` fell in rebuilt `.bss` that no PDB symbol covers, so it also stayed a constant (the old listings read `or ecx, 0x800000`).
   - With 912,673 bytes less `.data`, `.bss` moves down. `0x800000` now falls inside the port-tail array `s_entity_path_motion_runtime` (`entity.cpp`, `0x77fdb0`), so Reccmp names the rebuilt immediate `s_entity_path_motion_runtime[815].start (OFFSET)`. (On `3791f0dd` the array was at `0x77cd20` and the name was `[834].samples+168`.)
   - Nothing about the code changed. The artifact goes once rebuilt `.bss` stops covering `0x800000`, or the port tail's array moves.
   - Restoring it would mean storage made up for scoring again, which rule 5 forbids.
4. **The rule.** `Deviation (scoring only)` is now the required comment phrase, under ADR 0005 entropy rule 4, which adds a recipe step 8 item and a `convert_unit.py` checklist item.
   - The existing declaration-only cases carry the phrase. That is 33 sites: `view.hpp` 17, `record.hpp` 4, `entity.hpp` 3, and one each in `view_shared.hpp`, `simtick.hpp`, `surface.hpp`, `crt.hpp`, `util.hpp`, `ui_list.hpp`, `frontend.hpp`, `resource.cpp` and `winapi/entry.cpp`. `git grep "Deviation (scoring only)"` lists them all.
   - Each tag is an in-line, comment-only edit. No line moves, so no includer's code or line table changes.
   - No storage deviation remains.
   - `make reccmp## Before and after

Matching build (gate link, `/DEBUG`), unless the row names `opt_ref` (the original-mode link, finding 576).

| | #597 `d965c04d` | This change | Original |
| --- | ---: | ---: | ---: |
| File size | 2,151,936 | **1,239,552** | 735,744 |
| `.data` raw | 1,005,056 | **92,160** | 73,216 |
| `.data` virtual | 4,815,536 | 3,902,800 | 2,936,532 |
| `.data` named-section groups (map) | `$zzzzzz` 741,632, `$yyyyyy` 171,041 | none | none |
| `.data` group / `.bss` group (map) | 91,948 / 3,810,848 | 91,948 / 3,810,800 | |
| `.rdata` | 24,328 | 24,616 | 11,232 |
| `opt_ref` `.data` initialised / `.bss` | 1,002,797 / 3,810,816 | **90,124** / 3,810,768 | 73,136 initialised |
| Image size | 5,976,064 | 5,066,752 | 3,608,576 |
| Entry point RVA (gate / `opt_ref`) | `0xfefc0` / `0xbfd00` | `0xfefc0` / `0xbfd00` | `0x8f000` |
| `.data` identity at original offsets | 10,173 (13.89%) | 10,174 (13.90%) | |
| `.rdata` identity | 1,291 (11.49%) | 1,292 (11.50%) | |
| `.text` identity | 24,695 (3.84%) | 24,712 (3.85%) | |
| Exact-function code bytes | 108,678 (16.91%) | **108,385** (16.87%) | |
| `opt_ref` annotated exact, full / game-only | 1,196 / 996 | 1,194 / 994 | |
| Accuracy, full / game-only | 72.36% / 69.56% | 72.36% / 69.56% | |
| `make unit-data-offsets` | | full 0, gaps-only 3, partial 27 | |

The `.bss` group falls by 48 bytes: `key_names` loses 4 bytes and common and LIBC alignment loses the rest. Identity changes outside `.data` are chance (the `.text` image did not move).

## Score changes

17 functions fell and none rose. Every one is drift, with no instruction-count change, classified against #597's head (`match_baseline.py check --base-ref origin/convert/587-game-021-part-3`). Against `origin/main`, the default, the tool calls `0x00404280` and `0x0041fb50` `source-changed`, because #597 edits their files; this change does not. Each listing has the same instructions, and only one operand's name differs.

| Address | Function | Before | After | Cause |
| --- | --- | ---: | ---: | --- |
| `0x00402510` | `aitask_terrain_cell_command_create` | 100.00 | 98.78 | `0x800000` artifact (**exact lost**) |
| `0x00404280` | `aitask_cmd_entity_command_chain_tick` | 79.23 | 79.01 | `0x800000` artifact |
| `0x00404fc0` | `aitask_place_command_eligibility_check` | 98.64 | 98.53 | `0x800000` artifact |
| `0x00407a10` | `pending_type_value_slot_select` | 51.95 | 50.97 | `0x800000` artifact |
| `0x00409600` | `ledger_stub_00409600_TerrainDescriptorTilesResetAndFree` | 15.26 | 14.46 | `0x800000` artifact |
| `0x0041fb50` | `lego_game_config_init` | 93.64 | 93.61 | `0x800000` artifact |
| `0x0042b330` | `lego_load_tile_meshes` | 99.08 | 98.17 | `0x800000` artifact |
| `0x0042f6f0` | `lego_cell_init_pass` | 77.84 | 76.86 | `0x800000` artifact |
| `0x00430530` | `lego_terrain_cell_mutate` | 95.31 | 95.03 | `0x800000` artifact |
| `0x0043bbc0` | `entity_detach_carried_object_and_reset_state` | 63.89 | 61.11 | `0x800000` artifact |
| `0x0043bc40` | `objective_sidecar_append_gate` | 39.16 | 38.64 | `0x800000` artifact (two operands) |
| `0x0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 28.47 | 27.76 | `0x800000` artifact |
| `0x00447ea0` | `ledger_stub_00447ea0_EntityTypeBehaviorUpdate` | 14.40 | 13.94 | `0x800000` artifact (three operands) |
| `0x0044bfd0` | `lego::configured_monster_spawn_behavior_tick` | 77.19 | 73.68 | `0x800000` artifact |
| `0x00477bf0` | `winmain_parse_options` | 80.35 | 79.91 | `0x800000` artifact |
| `0x00482b50` | `render_state_flags_apply` | 60.14 | 59.44 | `0x800000` artifact |
| `0x00485e30` | `named_option_index_find_case_insensitive` (`Key_Find`) | 100.00 | 96.77 | spare slot removed: the loop bound reads `rewardGlobs` (**exact lost**) |

The two exact matches lost, `0x00402510` and `0x00485e30`, account for the 293 exact-function code bytes. On this base `opt_ref` keeps both (its exact removals are unchanged at 339 / 312), so its exact counts fall by two.

 | spare slot removed: the loop bound reads `rewardGlobs` (exact lost) |

The two exact functions lost account for the 293 exact-function code bytes. `opt_ref` removes one of them, so it shows one fewer exact removal.

## Checks

- `make reccmp-check` passes after `make reccmp-baseline-update`. `make match-progress` regenerated the baseline, listing digests and `binary-progress.json`.
- `make tools-test` passes without `--allow-growth`, and `make winapi-link-check` passes.
- The native build passes ctest, 37/37 (rebased tree). On the native side the only change is `key_names`' size, 257 to 256: `MENU_ACTION_TABLE_ALLOC` was already empty there, and the removed storage was winapi-only.
- The source stays MSVC 5.0-compatible. The source changes are removals and comment edits.
