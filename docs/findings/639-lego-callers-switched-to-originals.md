# Finding 639 — Lego.c's callers switched to the originals: `lego_engine_tick` and `lego_game_config_init`

Issue #731, part of #388 and #562. Follows finding
[637](637-opt-ref-reevaluation.md), which traced most remaining `/OPT:REF`
drops to a few live original callers whose rebuilds call port substitutes,
and finding [634](634-lego-near-miss-sweep-4.md), which aliased most of
`lego_game_config_init`'s calls and left two because of the symbol counter.

Measured with `make reccmp-check`, first on `main` `db760978` and again
after rebasing onto #735 (`4b50f313`, LegoObject.c's callers switched to
Weapons.c's originals). The scores and exact bytes are the same on both
bases. The `opt_ref` rows give both.

| Measure | `main` `db760978` | This change |
| --- | ---: | ---: |
| `0041fb50` lego_game_config_init (11,520 bytes) | 99.94 | **100.00** |
| `004232e0` lego_engine_tick | 99.15 | **99.93** |
| Functions that rise / fall | | 2 / 0 |
| Exact functions (full) | 2,146 | 2,147 |
| Exact-function code bytes | 289,516 (45.06%) | **300,991 (46.84%)** |
| `opt_ref` removed / removed exact | 124 / 102 | **99 / 84** |
| `opt_ref` under `/OPT:NOICF` (folded excluded) | 119 / 97 | 94 / 79 |
| `opt_ref` on #735 (`4b50f313`) | 92 / 78 | **73 / 64** |
| `opt_ref` under `/OPT:NOICF` on #735 | 87 / 73 | 68 / 59 |
| Image end | `0x875000` | `0x875000` |

No function fell below the baseline and no exact match was lost, so there
are no drops to label. `make match-progress` and `make unit-data-offsets`
were run on the final tree and the generated files are committed.

## The switches

Every switch is a `#define` in `src/game/lego_unit_imports.h`, in the
matching build only (`_MSC_VER < 1200`, C only: `lego_port.cpp` includes the
same header and keeps its adapters). Natively the port's adapters and names
are unchanged, so native behaviour does not change.

### `lego_engine_tick` (`004232e0`)

| Port name called | Original now called | Owner | Contract |
| --- | --- | --- | --- |
| `camera_registry_probe_original` | `lego_camera_registry_probe` (`00435a50`) | lego_unit.c | same |
| `simtick_sfx_bank_original` | `weapons_update` (`0046f550`) | weapons.c | same |
| `simtick_post_cluster_original` | `electric_fence_update` (`0040d450`) | terrain_cell_objects.c | same |
| `world_render_pass_a_original` | `lego_tutorial_block_icons_update` (`00435300`) | lego_unit.c | same |
| `world_render_pass_b_original` | `nerps_draw_tutorial_icon` (`00454600`) | tutorial_script.c | same; the original's third parameter is unused, the caller pushes the world time as a float |
| `info_message_queue_tick` | `info_draw` (`00419f30`) | info_unit.c | same |
| `incident_message_text_consumer` | `info_draw_panel` (`0041a080`) | info_unit.c | same |
| `message_cooldown_tick` | `info_update_timers` (`0041a2c0`) | info_unit.c | same |
| `keep_running_original` | `lego_keep_running` (`00435940`) | lego_unit.c | same (function-like macro, below) |
| `simtick_sound_keepalive_original` | `simtick_sound_keepalive` (`0047aba0`, Sound3D_Update) | sound3d.c | the matching build's DirectSound body does not read the port's context parameter |
| `panel_crystals_draw_original` | `panel_crystals_draw` (`0045c0d0`) | panel_unit.c | the original also ignores the frame time the caller passes |

The first eight are finding 637's rows for this caller. The last three were
the remaining callee names in the function's diff; their callees were not
removed (they have other callers), so they only raise the score.

What is left in `004232e0` (0.07 points) is one `fmul [literal]` scheduled
earlier in the original (x87 scheduling, not a call).

### `lego_game_config_init` (`0041fb50`)

| Port name called | Original now called |
| --- | --- |
| `scene_lights_build_from_config_original` | `lego_scene_lights_build_from_config` (`00433770`) |
| `render_quality_config_apply_original` | `lego_render_quality_config_apply` (`004334f0`) |

### Smaller Lego.c callers from finding 637's table

| Caller | Finding 637's row | State |
| --- | --- | --- |
| `lego_engine_shutdown` (`00424d00`) | `0x44dd40` and below | already resolved on `main` (#719's `loader_shutdown` alias); no dropped callee remains |
| `lego_main_sim_step` (`004250c0`) | `legoobject_movement_input_accumulate` | already resolved by finding 634; no dropped callee remains |
| `lego_level_load` (`00429890`) | `0x45d7e0` | **blocked**, below |

## The symbol counter (rule B)

`tools/experiments/699/counter_probe.py` against `origin/main` checked every
switch before the full build.

- **The engine-tick aliases.** The targets in other units are not declared
  in `lego_unit.c`, so their aliases are free. The two same-unit targets,
  `lego_camera_registry_probe` and `lego_tutorial_block_icons_update`, are
  forward-declared with group 3C's entries, so their aliases cost one symbol
  each from `00434570` on (−2 there). `make match-function` over the whole
  unit shows no code change in that range.
- **`keep_running_original`.** `lego_keep_running` is already declared
  through `lego_keep_running_adapter`'s alias (finding 634), so an
  object-like macro would cost a symbol from the top of the file. A
  function-like macro after the adapter's declaration keeps every
  declaration: counter unchanged.
- **The two config-init aliases.** Their targets are forward-declared at
  group 3B's entries, just before `00433380`. As object-like macros they
  cost two symbols from there on (−2): `00433380` (exact) becomes only an
  effective match and `00434530` falls from 100% to 78.26%, as finding 634
  found.
  - Moving two declarations into the gap restores the count. A prototype
    costs one symbol per named parameter as well as its name, so the
    moved declarations must be two parameterless ones: a first try with
    `terrain_cell_drill_work_add_current` and
    `terrain_cell_work_flag_circle_set_current` cost 8 (+6).
  - `ledger_stub_00435550_GlobalStateZeroTransitionDetect` and
    `lego_global_handle_position_apply_if_present` (group 3C's entries,
    unused by group 3B) now sit with group 3B's entries, tagged
    `Deviation (scoring only)`. The counter is unchanged at every function
    through `00434530`. From `00434570` on it is −4, the two engine-tick
    aliases plus the two moved declarations' old place. No group 3C
    function changes code.

No filler declaration was added and no frozen `_for_`/`_from_` header was
edited.

## Functions `/OPT:REF` keeps now

On `main`, 25 annotated functions (18 exact) are no longer removed, and
none is newly removed. On #735, which already keeps part of Weapons.c's
chain, 19 are kept (14 exact). The aliases of the two changes coexist:
#735's are in LegoObject.c's headers, and `weapons_update` is aliased only
here.

| Unit | Functions (exact) | Functions |
| --- | ---: | --- |
| GAME-050 weapons.c | 10 (8) | `weapons_update` and its projectile chain: `0046f380`, `0046f3b0`, `0046f550`, `0046f610`, `0046f770`, `004712c0`, `004712f0`, `00471310`, `00471330`, `00471370` |
| GAME-019 Info.c | 4 (4) | `info_draw`, `info_draw_panel`, `info_update_timers`, `info_get_font` |
| GAME-021 Lego.c | 4 (2) | `00433770`, `004334f0`, `00435300`, `00435a50` |
| GAME-013 terrain_cell_objects.c | 2 (1) | `electric_fence_update`, `electric_fence_building_spark_callback` |
| GAME-023 LegoObject.c | 1 (1) | `legoobject_one_shot_attached_effect_start` |
| GAME-024 LightEffects.c | 1 (1) | `light_effects_initialise` |
| G98-002 Container.c | 1 (1) | `light_interface_call_slot_38` |
| GAME-028 tutorial_script.c | 1 (0) | `nerps_draw_tutorial_icon` |
| GAME-007 Collision.c | 1 (0) | `point2d_project_onto_direction` |

Neither `lego_engine_tick` nor `lego_game_config_init` has a live-caller row
left in `make opt-ref-callers`.

## Remaining blockers

| Caller | Callee | Blocker | Owner |
| --- | --- | --- | --- |
| `lego_level_load` (`00429890`) | `0045d7e0` LevelIdentifierRemapTableLoad | the annotated function is `level_id_remap.cpp`'s C++ ledger stub in `unported_ledger_stubs`, taking an installed pair table, not the original's `(fileName, gameName)`. It needs GAME-035's conversion before the call can switch | GAME-035 (`level_id_remap.cpp`) conversion |
| `legoobject_update_callback` and its sibling | `weapons_object_*` | calls in `legoobject_unit.c` | #730 (#735, below this change) |
| NERPs (`tutorial_script.c`) | Interface.c's item setters | hook tables | #732 |
| `WinMain` | Init.c, DirectDraw.c, Files.c and the start-up chain | `entry.cpp` | #733 |

## Gate

- `make reccmp-check`: OK; 2 improved, none fell.
- `make match-progress`, `make unit-data-offsets`,
  `make opt-ref-callers OPT_REF_ARGS=--summary`: run; generated files
  committed.
- `make tools-test` (no `--allow-growth`), `make winapi-link-check`: OK.
- Native: `cmake --build build` and `ctest`: 37/37 pass.

## Files

- `src/game/lego_unit_imports.h`: the matching-build aliases.
- `src/game/lego_unit.c`: two group 3C declarations moved to group 3B's
  entries (`Deviation (scoring only)`).
- `docs/coverage/`: match baseline, listing digests and binary progress.
