# Tutorial frontend integration checkpoint

The eight-tutorial parity goal is paused. This change contains implemented frontend and required owner/caller repairs; it does not claim complete tutorial parity. Moving has completed in discovery runs. Drilling still has a bounded Loose-rock progression blocker, and the other six tutorials lack end-to-end candidate acceptance. The detailed discovery ledger remains in [wine-tutorial-frontend-matrix.md](wine-tutorial-frontend-matrix.md).

The original integration was based on `origin/main` at `3cf227171effd923541b0b4da3be94d0507a4fab`. The report-conflict refresh rebases it onto `82a533a9c10826b4db081246adca46619e4a002b`; the current results are recorded below. The previous branch and its seven commits were preserved. The three staged AG1 notes are unrelated and excluded from the integration change.

## Source and regression preparation

Temporary Moving/Drilling/Next traces, independent probe storage, exception observers and runtime diagnostic-fixture ingress were removed. Genuine activity, command, scene, pointer and terrain owners remain. Main's flat configuration-record API and native build repairs were retained while resolving the frontend integration.

Merge preparation additionally fixes:

- GOPS-marked portable saves normalize music, brightness, SFX and speed before either typed or per-slot readers consume them.
- Ordinary and reward saves snapshot the effective music setting instead of persisting a stale slot value. The numeric slider-to-save-to-reload regression starts with an existing slot so bootstrap cannot conceal the defect.
- Pointer-selected sliders admit their actual track and resource-sized arrows after hover selection is cleared.
- A missing Pointers group no longer invokes an unsafe single-component fallback through the new configuration API.

Tests use bounded static GameContext fixtures where the expanded model otherwise exceeds the normal native stack. The wake test now requires an unbound activity owner to retain zero progress instead of expecting fabricated time. Other fixture repairs retain main's test fixes and explicitly initialize action-catalog count state.

The authored drill work-node regression runs the real configured-name, current-activity lookup and work-position function bodies with controlled pose leaves. It checks 15 cases, including missing resources and unchanged type-1 behavior. It does not validate real COM transforms.

## Save compatibility

GOPS identifies an old portable format and is migrated automatically. Another historical writer stored music at header +0x10 without a marker. Its bytes cannot reliably distinguish it from a retail save with muted music; automatic guessing would corrupt valid retail settings. For a file known to come from that writer, [migrate_portable_save.py](../tools/migrate_portable_save.py) explicitly converts a copy:

```sh
python3 tools/migrate_portable_save.py --legacy-music-header --brightness 8 old.sav converted.sav
```

Brightness was not stored in that old format, so the desired value is required. The input remains unchanged, an existing destination is refused, and all other header, record and trailer bytes are preserved. No protected save was converted during this work.

## Validation

The final checkout passes the default native build and all **31 CTest tests**, including movie support. Six tooling tests pass, including the 15-case actual-owner regression and conversion/refusal fixtures:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tools/tests -v
make reccmp-report
```

The isolated final MSVC 5 build completes with `/ML /Zi /O2 /Ob1`. Its source-before/source-after identities match all 211 recorded source/build inputs. Exact source, configuration, compiler commands, EXE, PDB, full/game reports and logs are retained externally. The build has 32 compiler warnings and 52 Reccmp ERROR lines, including missing symbol/debug-data coverage; this is successful compilation/report generation, not a clean analysis or runtime acceptance result.

The last active discovery process exited normally before source cleanup. All 258 afterstate rows were reviewed and restored. Controller verification and an independent comparison both reported zero differences for the reference and game-prefix roots. The canonical lock was independently reacquired and the journal/socket were absent. No merge-candidate game launch occurred. Existing production launch pins remain historical and must be reconciled before any future launch.

## Initial matching tradeoff

An exact latest-main control was built with the same MSVC toolchain and report workflow. It passes the previous baseline, so the declines below belong to this integration rather than preexisting baseline drift. Full accuracy changes from **53.50% to 53.34%**; game-only accuracy changes from **49.11% to 48.95%**. There are 34 declines, 31 improvements and seven newly mapped addresses in each scope.

The zero-score entries at 0x446610 and 0x44a310 are real forwarding adapters to the new computation/input owners, not failed builds. The 0x464720 visibility entry now admits owned software meshes before its retained retail primitive. These are explicit functional-reconstruction tradeoffs. The unchanged FLIC converter at 0x484ba0 has a register-only difference marked `effective` by Reccmp. None of these scores proves behavioral parity.

Following [the documented baseline workflow](../docs/building.md#match-baseline-gate), the baseline was regenerated with `make reccmp-baseline-update` from the final reports; `make reccmp-compare` then passes. The original failing comparison and exact-main control are retained. The baseline update is part of the reviewable source change, not a claim that binary matching improved.

| Address | Function | Main | Integration |
|---|---|---:|---:|
| 0x404280 | `cmd_entity_command_chain_tick` | 12.67% | 11.99% |
| 0x405c10 | `pending_command_entity_assignment_tick` | 14.11% | 13.48% |
| 0x4089d0 | `segment_2d_point_distance` | 39.11% | 36.44% |
| 0x4129d0 | `menu_overlay_surface_and_text_setup` | 11.49% | 8.96% |
| 0x413e20 | `frontend_open_submenu` | 11.90% | 11.36% |
| 0x417a90 | `save_read_slot_target` | 11.44% | 11.18% |
| 0x417bd0 | `sav_write_slot_file` | 16.16% | 14.35% |
| 0x417fc0 | `save_commit` | 11.32% | 10.81% |
| 0x41a080 | `incident_message_text_consumer` | 27.78% | 27.49% |
| 0x41a920 | `lego::level_state_counters_clear` | 29.41% | 24.59% |
| 0x41c4f0 | `advisor_animation_position_and_request_update_internal` | 17.02% | 13.10% |
| 0x4250c0 | `main_sim_step(struct EngineContext *, float, unsigned int *, unsigned int *, unsigned int *)` | 8.67% | 8.44% |
| 0x426520 | `simtick_vu_meter` | 11.65% | 9.62% |
| 0x43a300 | `entity_deposit_activity_gate` | 33.62% | 23.29% |
| 0x4409e0 | `ai_set_activity` | 14.81% | 12.50% |
| 0x444fb0 | `ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` | 26.04% | 25.56% |
| 0x446610 | `ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet` | 12.07% | 0.00% |
| 0x447c40 | `activity_register_bookkeeping` | 20.75% | 19.80% |
| 0x44a310 | `ledger_stub_0044a310_ObjectMovementInputAccumulate` | 91.18% | 0.00% |
| 0x44fb90 | `terrain_state_surface_point_project_offset` | 23.65% | 23.19% |
| 0x4512d0 | `terrain_cell_normal_sample` | 9.73% | 8.65% |
| 0x4520d0 | `cmd_queue_tick` | 24.50% | 21.81% |
| 0x45a2e0 | `legacy_overlay_image_record_load` | 18.39% | 13.27% |
| 0x45e940 | `view_point_to_screen_geometry_transform` | 18.90% | 17.05% |
| 0x462270 | `checkpoint_step_target` | 30.89% | 29.70% |
| 0x463f00 | `scene_quad_grid_emit` | 18.27% | 15.83% |
| 0x464110 | `rotated_pairs_scratch_fill` | 13.95% | 10.85% |
| 0x464720 | `surface_visibility_set` | 100.00% | 9.76% |
| 0x471a60 | `entity_velocity_steer_blend_update` | 13.84% | 13.29% |
| 0x480870 | `engine_task_callback_record_allocate_and_initialize` | 35.56% | 31.65% |
| 0x4838b0 | `d3d_render_state_sync` | 10.85% | 10.69% |
| 0x484ba0 | `flic_packed_triplet_table_to_word_build_target` | 100.00% | 82.76% |
| 0x485390 | `text_buffer_draw_wrapped` | 5.99% | 5.96% |
| 0x48c820 | `parse_surf_chunk` | 6.94% | 6.34% |

## Report-conflict refresh

The branch was rebased onto main at `82a533a9c10826b4db081246adca46619e4a002b`. The only Git conflict was the generated matching baseline. Main's baseline was used provisionally, then regenerated from a fresh MSVC 5 `/ML /Zi /O2 /Ob1` build of the combined source. This preserves main's newer conversions instead of selecting stale score rows from either side. Main's new binary-progress input manifest was also regenerated to include `actor_scene.cpp`, `actor_scene.hpp` and `entity_motion_owner.cpp`.

Current validation: all 31 native CTest tests pass. The expanded tooling suite runs 12 tests: 11 pass and the synthetic PE test skips because its optional pefile/capstone dependency pair is unavailable; the six integration-owned tests still pass. `make reccmp-report` and the regenerated `make reccmp-compare` pass. All 215 recorded source/build inputs match before and after the build. Full report accuracy is 53.52%; game-only is 49.15%. Relative to the latest main baseline, each scope has 33 declines, 32 improvements and seven newly mapped addresses; the earlier table remains historical. There are 32 compiler warnings and 70 Reccmp ERROR lines, so this is not clean analysis or tutorial runtime acceptance. No game or protected-state transaction was started for this conflict fix.
