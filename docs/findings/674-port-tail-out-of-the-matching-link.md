# 674. Port tail out of the matching link: tickers.cpp, code roots, resource IID

Issue #805 implements finding 668's follow-ups 2, 4 and 6. Follow-up 4
excludes AITask's adapters (#806) and NERPs' table (#804). The change
was measured step by step on `main` `316b70f6`, then rebased onto #809
(`identity/804-nerps-table` `0376419f`, finding 673) and measured again.
The summary and the first table give the figures against `0376419f`; the
per-step table is the `316b70f6` run. Every figure comes from a matching
build under the gate's link mode (`/DEBUG /OPT:REF /OPT:NOICF`), measured
with `tools/port_tail_inventory.py` and the image's PE headers.

## Summary

1. **`tickers.cpp` is native-only.** Its `_TICKERS` table kept 86,576
   bytes of tail code linked. Only two functions pulled it, and the link
   discards both: `lego_port.cpp`'s `EngineTick` and `gods98/frameloop.cpp`'s
   application loop. They are now compiled out of the winapi build. On
   `316b70f6`, tail `.text` falls from 203,168 to 116,592 bytes, exactly
   the anchor's figure. On `0376419f` the whole change takes tail `.text`
   from 177,056 to 81,776 bytes, and the image ends 24 pages earlier:
   `0x7d8000` instead of `0x7f0000`. SizeOfImage falls from 4,128,768 to
   4,030,464 bytes.
2. **Eleven unit references now name the original functions**, not the
   tail's adapters. Five tail objects stop being roots:
   `terrain_cell_objects_port`, `interface_port`, `entity.cpp`, `flic.cpp`,
   and `frontend_port`'s `movie_play_wait`. 13 functions rise and none
   falls. Seven of them become exact. On `316b70f6`, tail `.text` falls
   by another 480 bytes, because the remaining roots still pull the rest
   (finding 668, section 5).
3. **IID_IDirect3DRMFrame3 (`0049f988`) is defined in `container.c`**, in
   the original's GUID block between IID_CntFrame (`0049f968`) and
   IID_CntMesh (`0049f9a8`). No unit references `resource.cpp` any more.
   Tail `.rdata` falls by 16 bytes.
4. **Lego.c's two light setters are left to #807.** Retargeting them from
   `lego_unit_imports.h` moves `lego_unit.c`'s symbol counter. Two Lego.c
   functions then fall below the baseline (section 3).
5. **Identity metrics do not move.** Exact-function code bytes rise from
   340,131 to 342,065 (53.24%). The entry point moves 32 bytes further
   from the original's. The cause is two port-only accessors in unit
   files, AITask.c's and Weapons.c's, which only the dropped tail code
   called. `/OPT:REF` now removes them (section 4).

## Before and after

| Measure | #809 `0376419f` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **340,131 (52.93%)** | **342,065 (53.24%)** |
| Exact functions, full / game-only | 2,615 / 2,327 | 2,622 / 2,334 |
| Accuracy, full / game-only | 97.35% / 97.07% | 97.36% / 97.08% |
| `.text` identical at original addresses | 4.49% | 4.58% |
| Correct-size functions, full | 2,784 (451,034 B) | 2,784 (451,034 B) |
| Identical prefix, raw / masked | 11 / 528 B | 11 / 528 B |
| Units with size agreement (sizes, start, end) | 0 of 227 (166, 2, 1) | same |
| Function-order inversions, full / game-only | 222 / 1 pairs | same |
| Entry point (original `0048f000`) | `0048ea60` (−1,440) | `0048ea40` (−1,472) |
| SizeOfImage | 4,128,768 | 4,030,464 |
| Image end (gate) | `0x7f0000` | `0x7d8000` |
| `.text` / `.rdata` / `.data` raw | 821,760 / 23,040 / 96,256 | 726,528 / 22,528 / 94,720 |
| Tail `.text` / `.bss` / `.data` / `.rdata` | 177,056 / 285,817 / 20,943 / 15,209 | 81,776 / 285,785 / 19,629 / 14,777 |
| Tail objects referenced from units | 17 | 12 |

### The tail after each step (on `main` `316b70f6`)

These are `port_tail_inventory.py` totals for the objects linked from
`lrr_tail.lib`. The last column counts tail objects that units reference.

| Step | `.text` | `.bss` | `.data` | `.rdata` | Image end | SizeOfImage | Unit roots |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| base | 203,168 | 285,821 | 21,002 | 15,265 | `0x7f5000` | 4,149,248 | 18 |
| 1. `tickers.cpp` native-only | 116,592 | 285,789 | 19,900 | 14,853 | `0x7df000` | 4,059,136 | 18 |
| 2. code roots retargeted | 116,112 | 285,789 | 19,900 | 14,853 | `0x7df000` | 4,059,136 | 14 |
| 3. IID to `container.c` | 116,112 | 285,789 | 19,900 | 14,837 | `0x7df000` | 4,059,136 | 13 |

The image only shrinks, and `make pe-audit` passes at every step. On
`0376419f`, the margin below the flag constant `0x800000` grows from
65,536 to 163,840 bytes.

## 1. `tickers.cpp`

`src/link_order.cmake` marks `gods98/tickers.cpp` `native`, so the winapi
build does not compile it. The winapi build has no caller of its two
pullers, and LINK rejects an unresolved external even in a discarded
function. The pullers therefore go too, under
`#if !defined(LEGORRR_BACKEND_WINAPI)`:

- In `lego_port.cpp`: `EngineTick`, the `engine_game_config_callback_registry_init`
  port model that registers it, and the four static callbacks of that
  registry. The registry's only port caller is the native application
  init. The matching build's callers reach `lego_unit.c`'s `0041fa20`
  through `lego_port_aliases.h`.
- In `gods98/frameloop.cpp`: the native application loop. This is
  `frameloop_level_return_begin` through `frameloop_main_menu_frame`, then
  `frameloop_application_init` through `frameloop_application_shutdown`.
  `LegorrrRun` was already native-only (#576), and these functions serve
  only it.

Dropping `_TICKERS` also drops everything that only its `ticker_*` stubs
reached. That is 86,576 bytes, finding 668's anchor figure exactly. The
native build keeps all of it.

## 2. Code roots

Each switch uses one of finding 626's counter-neutral forms. All of them
apply only in the matching build (`_MSC_VER < 1200`, and C only where the
header is also compiled as C++).

| Unit | Port adapter (tail object) | Original | Form, file |
| --- | --- | --- | --- |
| GAME-013 | `legoobject_block_position_get` (`terrain_cell_objects_port`) | `004424f0` `legoobject_position_to_terrain_cell` | alias, `terrain_cell_objects.c` |
| GAME-013 | `level_block_surface_update` (same) | `0042f6f0` `lego_cell_init_pass` | alias, `terrain_cell_objects.c` |
| GAME-020 | `lego_object_get_container` (`interface_port`) | `0043a5f0` `legoobject_visual_resource_get_or_create` | alias, `interface_unit.c` |
| GAME-020 | `container_get_position` (same) | `004754c0` Container_GetPosition (`ddiscene_object_position_get`) | alias, `interface_unit.c` |
| GAME-002 | `entity_linked_activity_event_dispatch` (`entity.cpp`) | `0043f5b0` `legoobject_linked_activity_event_dispatch` | alias, `aitask_imports.h` |
| GAME-032, GAME-033 | `flic_width` (`flic.cpp`) | `004897d0` (`anim_clone_is_lws`) | alias, `panel_unit.c`, `pointer_imports.h` |
| GAME-032, GAME-033 | `flic_height` (same) | `004850e0` Flic_GetHeight (`object_field10_get`) | alias, same |
| GAME-032 | `ledger_stub_00425c30_TransitionControl` (`lego_port`) | `00425c30` `lego_transition_control` | alias, `panel_unit.c` |
| GAME-023 | `panel_selection_highlight_toggle` (`lego_port`) | `00435aa0` `lego_panel_selection_highlight_toggle` | post-declaration macro, `legoobject_unit_imports.h` |
| GAME-041 | `lego_container_create_adapter` (`lego_port`) | `00472950` Container_Create (`ddiscene_object_allocate`) | alias, `sfx_imports.h` |
| GAME-037 | `movie_play_wait` (`frontend_port`) | `00415700` `front_play_movie` | alias, `reward.c` |

- An **alias** renames one declaration and its uses. For every alias, a
  preprocessor pass (`gcc -E -D_MSC_VER=1100`) confirmed that the new name
  is not otherwise declared in that translation unit (finding 538).
- **LegoObject.c already declares `lego_panel_selection_highlight_toggle`**,
  through the `_adapter` alias of finding 626. Its switch therefore comes
  after the declaration, and only the use changes.
- **The flic width getter is tagged `Deviation (scoring only)`.**
  Flic_GetWidth was folded into `004897d0`, and the tree names that
  function `anim_clone_is_lws` (findings 616 and 632). Finding 626 handled
  LegoObject.c's flock test the same way.
- **Panel.c's TransitionControl reference was not in finding 668's
  pairing.** The positional pairing missed it. `nm` found it: the one
  GAME-032 reference into `lego_port.cpp` was `_ledger_stub_00425c30_TransitionControl`.
- **`frontend_port.cpp` stays a root.** Reward.c's second reference into
  it is the save buffer `g_front_save_slot_buffer` (`00557418`), which is
  data and outside this issue.

Scores (Reccmp, before → after):

| Function | Before | After |
| --- | ---: | ---: |
| `0040cea0` electric_fence_block_surfaces_update | 96.67 | **100** |
| `0040cf50` electric_fence_add | 95.00 | **100** |
| `0040d0a0` electric_fence_remove | 96.77 | **100** |
| `0040d720` electric_fence_building_clear_callback | 95.45 | **100** |
| `00403d10` aitask_entity_command_unlink_by_kind | 99.39 | **100** |
| `0045cc70` pointer_cursor_draw_target | 97.62 | **100** |
| `00461410` reward_loop | 99.66 | **100** |
| `0040d5e0` electric_fence_building_spark_callback | 92.24 | 93.10 |
| `0041f4d0` interface_callback_can_pick_up | 86.87 | 88.89 |
| `0043c810` legoobject_update_callback | 89.35 | 89.38 |
| `0045a730` simtick_manager_tick | 93.91 | 94.92 |
| `0045adb0` panel_cursor_interaction_update | 82.48 | 82.69 |
| `00465430` transient_effect_emitter_create | 94.38 | 94.64 |

## 3. Lego.c's light setters stay (#807)

`lego_unit.c` (GAME-021) calls `lego_light_penumbra_set_adapter` twice and
`lego_light_umbra_set_adapter` twice (three references in the object).
Their originals are Container.c's Container_Light_SetSpotPenumbra
(`00473460`) and Container_Light_SetSpotUmbra (`00473480`). Container.c
serves them under the link names `light_interface_call_slot_40` and
`light_interface_call_slot_3c`.

`lego_unit.c` already declares both link names at line 8621, with the
port's float-bits contract (`int (void*, uint32_t)`). The file declares
them after it uses the adapters, so neither counter-neutral form fits:

- An alias renames the adapter declarations (`void (void*, float)`), so
  `lego_unit.c` would then declare each name twice with conflicting types.
- A post-declaration macro would name functions that are not declared
  yet at the call site.

The one try that compiled moved the declarations into
`lego_unit_imports.h`, with a function-like macro that passes the float's
bits. That makes the declarations at line 8621 redeclarations, which
allocate no symbol. Every function after them drifts. Two Lego.c functions
fell below the baseline: `00433380` (100 to 90.91) and `00434530` (100 to
82.61), with the same instruction counts and commuted `fadd` operands.

The try was reverted. Finding 671 names the same obstacle. The fix belongs
in `lego_unit.c`, which #807 owns: declare the two setters before their
first use, in the original's form. Until then `lego_port.cpp` stays a root
through GAME-021's three references.

## 4. The entry point and two port accessors

The unit segment ends 32 bytes earlier, at `0048b9c0` instead of `0048b9e0`.
The cause is two port-only accessors, 16 bytes each, that sit in unit files:

- `aitask_port_priority_states`, in AITask.c (GAME-002);
- `weapons_port_stats_slot`, in Weapons.c (GAME-050).

Only tail code that step 1 removed called them, so `/OPT:REF` now discards
them. No original function changes size. Correct-size functions and unit
size agreement are unchanged.

The unit spans move as follows:

- AITask.c goes from 48 to 64 bytes short of the original. This one moves
  away from it.
- GAME-050 goes from 48 to 32 bytes long, closer to the original.
- The entry point goes from 7,984 to 8,016 bytes early on `316b70f6`,
  and from 1,440 to 1,472 on `0376419f`.

The spans had counted port code as if it were original. ADR 0006
decision 4 compiles port-only code in a unit file out of the matching
build anyway.

## 5. What still references the tail

12 tail objects remain referenced from units on `0376419f` (#809 removed
`tutorial_script_port`'s 172 NERP-table references). All of them are
another issue's:

- `aitask_port` (GAME-002's adapters, #806);
- `lego_port` (GAME-021's light setters, section 3);
- `frontend_port` (the save buffer `00557418`);
- `startup_config` (G98-028's streams);
- the Main.c globals and key arrays of finding 668's follow-up 3:
  `port_state`, `input`, `engine`, `mesh_port`, `view`, `lws_port`,
  `viewports_port` and `platform/winapi/frameloop.cpp`.

As finding 668 section 5 predicts, the tail's remaining 81,776 bytes of
code and its `.bss` stay linked until the last of these roots goes.

## Checks

- `make reccmp-check` passes: no function falls, and the map check is OK.
- `make match-progress` rewrites the baseline with only the 13 rises above.
- `make pe-audit` and `make unit-data-offsets` pass. In the matching link,
  `0049f988` is now `container.c.obj`'s, at `004bacf8`.
- `make tools-test` passes without `--allow-growth`, and so does
  `make winapi-link-check`.
- The native build compiles. Its 39 ctest tests pass with the real WADs.
  `LegoRR` starts with the offscreen driver: it plays the intro, loads
  Lego.cfg and opens the main menu.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json
make pe-audit
```
