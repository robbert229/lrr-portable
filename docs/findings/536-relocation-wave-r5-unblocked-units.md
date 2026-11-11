# Finding 536 — Relocation wave R5: 21 unblocked game units

Refs #446, stage S3 of the [#388 plan](../plans/388-original-link-order.md), [ADR 0006](../adr/0006-original-link-order.md) decisions 3–4 and entropy rules 2–3. Rebased on `main` `c25633b9`, after #495, #489, #493, #498, #492 and #503. The rebase regenerated the measurement files; scores and listing digests are unchanged from the prior PR head.

## What moves

| Unit | File | Functions moved | Owned globals | Closure | Not in `src/` |
| --- | --- | ---: | ---: | ---: | ---: |
| GAME-024 | `game/lighteffects.cpp` (LightEffects.c) | 18 | 26 | 6 | 2 |
| GAME-025 | `game/loading_progress.cpp` | 2 | 0 | 0 | 3 |
| GAME-026 | `game/terrain_map.cpp` | 56 | 0 | 23 | 6 |
| GAME-028 | `game/tutorial_script.cpp` | 29 | 7 | 11 | 2 |
| GAME-029 | `game/objective_unit.cpp` (Objective.c) | 12 | 2 | 20 | 2 |
| GAME-030 | `game/object_state_file.cpp` | 7 | 3 | 0 | 1 |
| GAME-031 | `game/object_overlays.cpp` | 8 | 4 | 13 | 0 |
| GAME-033 | `game/pointer.cpp` (Pointer.c) | 9 | 61 | 2 | 0 |
| GAME-035 | `game/level_id_remap.cpp` | 3 | 0 | 0 | 0 |
| GAME-036 | `game/map_view.cpp` | 10 | 0 | 6 | 1 |
| GAME-038 | `game/terrain_overlay_quads.cpp` | 4 | 0 | 2 | 5 |
| GAME-039 | `game/info_track_overlay.cpp` | 8 | 1 | 2 | 4 |
| GAME-040 | `game/placement_footprint.cpp` | 2 | 1 | 1 | 1 |
| GAME-041 | `game/sfx.cpp` (SFX.c) | 34 | 14 | 15 | 1 |
| GAME-042 | `game/grid_cell_records.cpp` | 9 | 0 | 3 | 0 |
| GAME-044 | `game/record_chains.cpp` | 9 | 2 | 8 | 0 |
| GAME-045 | `game/text.cpp` (Text.c) | 8 | 4 | 9 | 2 |
| GAME-046 | `game/tooltip.cpp` (ToolTip.c) | 15 | 1 | 2 | 1 |
| GAME-047 | `game/upgrade_levels.cpp` | 4 | 0 | 0 | 1 |
| GAME-048 | `game/vehicle_model.cpp` | 24 | 0 | 0 | 2 |
| GAME-049 | `game/cell_flood_passes.cpp` | 8 | 0 | 12 | 4 |

The tool moves 279 functions, 126 owned globals and 135 closure items. Body hashes are recorded in `docs/coverage/relocations/`. The GAME-041 prerequisite rename described below is a separate source change; the relocation's body hashes compare against that renamed source. No shared header is edited. `screen_pick.cpp` becomes a port-tail entry.

## Commits (ADR 0006 rule 3)

- `aadd714b` Relocate GAME-024
- `73ef9211` Relocate GAME-025
- `6af52f38` Relocate GAME-026
- `3155c196` Relocate GAME-028
- `64c8f00d` Relocate GAME-029
- `1570e6e3` Relocate GAME-030
- `517afd89` Relocate GAME-031
- `2db9189c` Relocate GAME-033
- `4a390d0d` Relocate GAME-035
- `23f2b1cd` Relocate GAME-036
- `1529d6e2` Relocate GAME-038
- `e6f75683` Relocate GAME-039
- `3edea4a2` Relocate GAME-040
- `bc4265c2` Fix-up: define GAME-040's naked helper before its caller
- `a4a07c27` Disambiguate sound queue count before GAME-041 relocation
- `7e26002d` Relocate GAME-041
- `aacc60a9` Relocate GAME-042
- `2dd3b6bf` Relocate GAME-044
- `6c043dab` Relocate GAME-045
- `86c3f886` Relocate GAME-046
- `65e85647` Relocate GAME-047
- `df59bc44` Relocate GAME-048
- `2891c22f` Relocate GAME-049
- `7aae39df` Fix-up: keep relocation private headers out of convert_unit's boundary includes

A final measurement commit records the generated files, input manifest and this finding.

The relocation tool generated each unit commit in link order. Both former fix-ups remain necessary after #493: GAME-040's `static` naked helper is still forward-declared and fails MSVC 5.0 (C2375); its separate fix-up places the unchanged helper before its caller. GAME-024 still adds the `random_float_range` prototype to `util_shared.hpp`, causing two `convert_unit` hand-conversion tests to fail; the separate fix-up excludes private headers from boundary-test includes.

A new prerequisite became necessary after #489: GAME-041 would export sound.cpp's `s_queue_count`, colliding with the unrelated exported `info.cpp` variable. The separate `a4a07c27` commit renames only the sound variable and its references to `s_sound_queue_count` before the relocation. Native tests and the matching measurements below cover this change.

## Layout (`binary-progress.json`), main `c25633b9` → this PR

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 395,256 | 319,385 (−75,871) |
| Pair inversions, game-only | 381,653 | 305,459 (−76,194) |
| Adjacent inversions, full / game-only | 756 / 696 | 634 / 575 |
| Unit-order pair inversions | 3,303 | 3,165 (−138) |
| Unit-order adjacent inversions | 82 | 68 |
| Unit contiguity | 75.12% (163) | 82.95% (180) |
| Link-order entries | 83 | 103 |
| Size-exact units | 26 | 26 |
| Unit-relative identity | 36,674 B (6.62%) | 36,608 B (6.61%) |
| Exact-function code bytes | 44,702 | 44,636 (−66) |
| Mean score, full / game-only | 55.20 / 51.04 | 55.18 / 51.02 |
| `.text` identical at original addresses | 18,298 B (2.85%) | 17,920 B (2.79%) |
| `.rdata` identical at original addresses | 1,276 B (11.36%) | 1,277 B (11.37%) |
| `.data` identical at original addresses | 13,679 B (18.68%) | 11,943 B (16.31%) |
| Functions identical at original address | 0 | 0 |

At-address identity moves with object layout and remains near chance. Unit-relative identity loses 66 bytes, corresponding to `lava_erosion_delay_from_cell`.

**Exact-match loss for owner review:** #498's guard still hides the earlier `s_erosion_delay_scale` declaration from `lava_erosion.cpp`, so the floating-point multiplication order remains correct (finding 538 rule A). GAME-026 and subsequent relocations add declarations to `level_shared.hpp`; the function now swaps the integer `imul` operands while keeping 19 instructions. This matches finding 538's rule B: declarations shift the compiler's symbol-counter phase. No padding was added. The refreshed baseline records this loss for owner review.

**`make unit-data-offsets`:** 0 full / 1 gaps-only / 8 partial / 216 n/a on main → 0 / 1 / 16 / 208 here. GAME-024, 028, 029, 030, 031, 033, 041 and 045 newly resolve as partial due to within-unit data gaps; the other 13 units lack two attributable items in any section.

## Score changes (ADR 0006 rule 2): every changed function

The table includes every changed listing digest and score in both scopes: 34 listings, 13 scores down and 4 up. **One exact match is lost:** `0x0040e9c0` falls from 100.00% to 84.21%, losing 66 exact-function bytes. Labels and instruction counts come from the drift classifier; causes come from full-build listing diffs and `match_function.py --json` in both trees. The single-unit listings for `0x004078c0` and `0x0043af30` are identical; their changes appear only after linking. No changed function is classified `source-changed`. The 12 instruction-count changes and all `drift-review` drops are flagged for the owner. The baseline is refreshed under ADR 0006 rule 2.

| Address | Function | Score (%) | Instructions | Label | Cause |
| --- | --- | --- | --- | --- | --- |
| `0x00404f50` | `object_filter_matches_action_target` | 25.00 = | 84 → 84 | `drift-reordered` | independent `filt_dword` calls reordered. |
| `0x004078c0` | `ledger_stub_004078c0_EntityCursorTargetStepResolveAndTimerAdvance` | 10.26 = | 42 → 42 | `drift-review` | linked literal loses its `LoadSel` string name; single-unit listings are identical. |
| `0x004089d0` | `segment_2d_point_distance` | 40.89 → 43.56 | 120 → 120 | `drift-review` | x87 stack operands swap (`fld st(n)`, two `fmul [esp+..]`). This reverses finding 527's change |
| `0x0040e9c0` | `lava_erosion_delay_from_cell` | 100.00 → 84.21 | 19 → 19 | `drift-review` | integer `imul` operand order swaps; floating-point order from the #498 guard remains correct. **⚠ Exact match lost (−66 B); owner review.** |
| `0x00411360` | `menu_mouse_coordinate_update` | 21.12 = | 314 → 314 | `drift-reordered` | independent `fe_legacy_dword` calls rescheduled |
| `0x00435230` | `random_variant_cell_scatter` | 33.78 = | 70 → 70 | `drift-reordered` | independent `add`/`sub` reordered |
| `0x0043af30` | `creature_spawn_from_emergence` | 22.07 = | 304 → 304 | `drift-review` | link-level only: three unnamed `.bss` addresses move, and an end pointer now names another symbol. The single-unit compile is identical |
| `0x00444460` | `ledger_stub_00444460_FlaggedVectorStepCommandIssue` | 23.61 → 22.72 | 291 → 291 | `drift-review` | x87 `fld`/`fmul` operand form changes; a linked offset gains a different symbol name. **Owner review.** |
| `0x00446610` | `ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet` | 12.27 → 12.07 | 189 → 193 | `drift-review` | x87 subtraction and multiplication rescheduled, adding `fld`, `fxch` and `fstp` operations. **⚠ Instruction count; owner review.** |
| `0x0044d0d0` | `motion_drift_offset_step` | 13.79 → 13.27 | 110 → 105 | `drift-review` | `edi` → `esi`: one register save fewer, an `fcom`/`fstp` pair merges, and a CSE'd `lea` goes. **⚠ Instruction count; owner review.** |
| `0x0044e0c0` | `terrain_create_retail` | 11.73 → 11.44 | 393 → 395 | `drift-review` | byte-load registers and a stack temporary change; conditional tests are rescheduled. **⚠ Instruction count; owner review.** |
| `0x0044e6d0` | `terrain_cell_material_transition_begin` | 6.43 = | 211 → 211 | `drift-review` | commutative `imul` swaps (2) |
| `0x0044edf0` | `terrain_cell_surface_orientation_set` | 10.22 = | 197 → 197 | `drift-review` | byte/flag temporaries use different registers and linked data addresses move. |
| `0x0044fe70` | `terrain_authored_surface_height_interpolate` | 9.89 = | 103 → 103 | `drift-review` | two table addresses move (data layout), and the x87 schedule changes |
| `0x004512d0` | `terrain_cell_normal_sample` | 13.27 → 9.73 | 100 → 96 | `drift-review` | commutative `fadd` operands and x87 schedule change; a `lea` and spill/reload disappear. **⚠ Instruction count; owner review.** |
| `0x00451450` | `terrain_cell_surface_vector_offset_add` | 14.16 = | 60 → 60 | `drift-review` | commutative `fadd` operands swap. |
| `0x00458670` | `objective_announce_current` | 27.40 → 27.95 | 186 → 186 | `drift-review` | registers renamed (`edi`/`ebp`/`ebx` → `esi`/`ebx`/`ebp`); a `lea` folds into a displacement |
| `0x00459470` | `osf_read` | 21.20 → 12.62 | 147 → 136 | `drift-review` | different register allocation (`edi` → `esi`/`ebp`): the frame grows from 8 to 0x10 bytes, and a null check moves after the second test. **⚠ Instruction count; owner review.** |
| `0x00459b00` | `legacy_entity_screen_ring_render` | 13.15 → 9.66 | 320 → 323 | `drift-review` | a float global is copied through `ecx` to a stack slot rather than loaded with `fld`/`fmul`; stack slots shift by 4. **⚠ Instruction count; owner review.** |
| `0x00463d20` | `view_blit_rect_positions_apply` | 9.43 = | 102 → 102 | `drift-review` | commutative `fadd` operands swap. |
| `0x00464110` | `rotated_pairs_scratch_fill` | 13.95 → 10.85 | 65 → 65 | `drift-reordered` | argument loads scheduled later, `edx` → `edi` |
| `0x00464720` | `surface_visibility_set` | 9.76 = | 22 → 22 | `drift-reordered` | a global load moves above the branch |
| `0x00465970` | `transient_effect_registry_release_all` | 29.79 → 30.43 | 19 → 18 | `drift-review` | the list head is kept in `esi`, not `eax`, which removes one `mov`. **⚠ Instruction count; owner review.** |
| `0x00466750` | `grid_cell_invalidate_and_clear_flags` | 16.67 = | 26 → 26 | `drift-reordered` | a global load moves above the branch; an `OFFSET` operand names another symbol (layout) |
| `0x0046b7c0` | `timed_effect_runtime_update` | 21.05 = | 147 → 147 | `drift-review` | commutative `fadd` swaps (2) |
| `0x0046b8b0` | `rectangular_grid_overlay_geometry_draw` | 7.94 → 7.97 | 149 → 148 | `drift-review` | x87 `fadd` operand form and `fxch` schedule. **⚠ Instruction count; owner review.** |
| `0x0046e9d0` | `staged_cell_candidate_collect` | 15.09 → 13.33 | 122 → 120 | `drift-review` | frame shrinks from 0x20 to 0x1c; zero is kept in registers. **⚠ Instruction count; owner review.** |
| `0x0046ffb0` | `ledger_stub_0046ffb0_LaserSpatialSoundAndImpact` | 10.47 = | 190 → 190 | `drift-review` | `fld`/`fmul` operands swap |
| `0x00484e50` | `flic_decode16_bit_delta` | 19.50 → 19.44 | 163 → 164 | `drift-review` | multiply inputs are loaded into different registers; one extra `mov`. **⚠ Instruction count; owner review.** |
| `0x004882e0` | `scene_node_transform_matrix_apply` | 14.00 → 12.13 | 293 → 294 | `drift-review` | x87 schedule with one more spill (`fstp`/`fmul [esp+0x80]`). **⚠ Instruction count; owner review.** |
| `0x0048c690` | `parse_srfs` | 10.57 → 7.11 | 123 → 122 | `drift-review` | register allocation: a subtraction is computed once and kept in a stack slot. **⚠ Instruction count; owner review.** |
| `0x0049c840` | `wad_payload_decode_dispatch` | 25.71 = | 71 → 71 | `drift-reordered` | independent byte load and `lea` swap order. |
| `0x0049c8e0` | `wad_payload_decode_type1` | 52.13 = | 105 → 105 | `drift-reordered` | two independent instructions swap |
| `0x0049cc70` | `wad_bitstream_output_initialize` | 26.09 = | 41 → 41 | `drift-reordered` | `shr`/`and` on swapped registers |

## What remains in R5

| Unit | Status |
| --- | --- |
| GAME-002 | conditional group in `cmd.cpp` is not whole alternatives |
| GAME-010 | relocated by #489 |
| GAME-027 | conditional blocks in `cmd.cpp` and `select.cpp` |
| GAME-032 | conditional `_ftol` declarations needed by assembly |
| GAME-034 | already converted |
| GAME-037 | `mission_blob_*` macros are redefined or undefined |
| GAME-043 | 13 conditional blocks, mostly in `entity.cpp` |
| GAME-050 | conditional block in `record.cpp`; the anonymous-namespace blocker was addressed by #493 |

The current survey reports 3 of 26 remaining unrelocated units move as-is: GAME-011, GAME-018 and GAME-022, all outside R5. Finding 535 keeps GAME-021 as one provisional object; the remaining R4 relocation is separate work.

## Gates

- Native `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`: 35/35 pass.
- `make tools-test`: passes, including lint (4,454 existing violations, zero new), link-order checks and Meson parity. The lint implementation and lint baseline are unchanged by this PR.
- `make reccmp-check` in the developer distrobox: 13 drops against main, listed above. After review and refresh, `make reccmp-compare` passes; `link_order.py check-map` passes.
- `make match-progress` in the developer distrobox with `build-venv` on PATH: all three generated files refreshed. `python3 tools/pr_match_progress.py check --base . --head .`: matching progress and baseline fresh.
- `make unit-data-offsets`: 0 / 1 / 8 / 216 → 0 / 1 / 16 / 208.
