# Finding 534 — Relocation waves R1–R3: the ten units `relocate_unit.py` can move after #483

Issues #442 (R1), #443 (R2) and #444 (R3), stage S3 of the
[#388 plan](../plans/388-original-link-order.md)
([ADR 0006](../adr/0006-original-link-order.md) decisions 3 and 4, entropy
rules 2 and 3). It follows wave R1's first PR
([finding 527](527-relocation-wave-r1-unblocked-units.md)) and uses the tool
as extended by [finding 530](530-relocating-named-namespace-members.md)
(named namespaces and R1's three defect fixes). Measured on `main` at
`504f5ef6` (after #483, #484 and #488). The branch was first built on
`5f01c2ce` and rebuilt twice, by rerunning the tool, after #484 and #488
merged; see Method.

## Summary

1. **Ten units relocate**, each in one commit generated only by
   `tools/relocate_unit.py`. There are no hand fix-ups, and neither
   `relocate_unit.py` nor `cxx_items.py` is modified.

   | Wave | Unit | File | Functions moved | Owned globals | Closure | Not in `src/` |
   | --- | --- | --- | ---: | ---: | ---: | ---: |
   | R1 | GAME-008 | `game/entity_cell_effects.cpp` | 3 | 1 | 0 | 1 |
   | R1 | GAME-010 (Dependencies.c) | `game/dependencies.cpp` | 9 | 1 | 4 | 0 |
   | R1 | GAME-012 | `game/rock_fall_styles.cpp` | 26 | 1 | 8 | 1 |
   | R1 | GAME-013 | `game/terrain_cell_objects.cpp` | 23 | 4 | 19 | 1 |
   | R3 | GAME-019 (Info.c) | `game/info.cpp` | 35 | 9 | 13 | 1 |
   | R2 | G98-008 | `gods98/bitmap_font.cpp` | 9 | 0 | 4 | 2 |
   | R2 | G98-013 (Input.c) | `gods98/input_unit.cpp` | 6 | 0 | 0 | 0 |
   | R2 | G98-016 (Flic.c) | `gods98/flic_unit.cpp` | 23 | 0 | 7 | 1 |
   | R2 | G98-017 | `gods98/text_window.cpp` | 13 | 0 | 8 | 0 |
   | R2 | G98-028 | `gods98/file_list_streams.cpp` | 4 | 0 | 0 | 0 |

   - The 151 functions come from 18 legacy files.
   - Every body hash agrees. 155 heads lose `static`, and all of them are
     recorded in `docs/coverage/relocations/<unit>.csv` (397 rows).
   - No shared header is edited. Shared statics go through private
     `<file>_shared.hpp` headers.
   - **Libraries change where ADR 0006 decision 8 says they should.**
     GAME-012, GAME-013 and GAME-019 take game functions out of
     `gods98/resource.cpp`, `gods98/simtick.cpp` and `gods98/message.cpp`
     into `game`. G98-008 and G98-028 take gods98 functions out of
     `game/frontend.cpp` and `game/startup_config.cpp` into `gods98`.
   - **`gods98/flic.cpp` is drained.** No original function is left in it,
     so it becomes a port-tail entry.
2. **Layout improves** (`binary-progress.json`):

   | Measure | Before | After |
   | --- | ---: | ---: |
   | Pair inversions, full | 493,667 | 408,393 (−85,274) |
   | Unit-order pair inversions | 3,533 | 3,323 (−210) |
   | Unit contiguity | 70.51% (153 units) | 74.65% (162 units) |
3. **Scores drift as rule 2 expects.**
   - 5 functions fall below baseline and 10 rise. 26 listings change in
     total, the same set in both scopes. None of them has changed source.
   - **No exact match is lost. One is gained:**
     `ledger_stub_0048b290_StartupStreamsClose` (72.73% → 100%, +85 exact
     bytes). Its listing does not change; Reccmp now names and pairs the
     original's array-element operands.
   - Mean score rises from 55.16% to 55.19% (full), and from 51.00% to 51.04%
     (game-only).
   - **Seven listings change instruction count** (+1, −7, −1, +1, +1, +2,
     −11), and four of the five drops are `drift-review`. All are flagged
     for the owner below.
4. **Unit-relative data offsets** (`make unit-data-offsets`, finding 529).
   All ten units now have dedicated objects. Two resolve, both as
   `partial`: GAME-013 and GAME-019 (`.bss` gap order). The unit summary
   goes from 0 full / 1 gaps-only / 6 partial / 218 n/a to 0 / 1 / **8** /
   **216**.

## Method

1. `python3 tools/relocate_unit.py --survey` on `5f01c2ce`. All ten units
   report `ok`, and G98-013 and G98-016 report `ok with --stem`. Then
   `--dry-run` for each unit: all ten plan.
2. **GAME-010 claim check.** The plan's claim rule excludes units that have
   an open conversion PR. GAME-010 has none: the open PRs are #484 only,
   and the claims the plan names (#412, #414, #415) are closed.
3. **Stems.** Six units are unnamed. Their stems follow their unit notes in
   `translation-unit-ranges.csv`:

   | Unit | Note | Stem |
   | --- | --- | --- |
   | GAME-008 | level stored value, cell path effects, placement commands | `entity_cell_effects` |
   | GAME-012 | "rock-fall styles and misc effect objects" | `rock_fall_styles` |
   | GAME-013 | "terrain cell object records and fence sparks" | `terrain_cell_objects` (`terrain_grid.cpp` exists) |
   | G98-008 | "bitmap font load/draw" | `bitmap_font` |
   | G98-017 | "text-window ring buffer + wrapped draw" | `text_window` |
   | G98-028 | "file-list stream state" | `file_list_streams` |

   G98-013 and G98-016 use `input_unit` and `flic_unit`, which the survey
   suggests because `input.cpp` and `flic.cpp` exist. GAME-010 and GAME-019
   take their default stems (`dependencies`, `info`).
4. **One tool run and one commit per unit (rule 3), in link order:**
   GAME-008, 010, 012, 013, 019, then G98-008, 013, 016, 017, 028. GAME-010
   is placed before GAME-012 so that data ownership follows link order.
5. **One full matching build at the end**, in the developer distrobox:
   `make reccmp-check`, then `make match-progress`, then
   `make unit-data-offsets`.
6. **Attributing the drift.** I built `main` (`5f01c2ce`) in a scratch
   worktree with the same matching build. It reproduces `main`'s baseline
   with no drop. Then:
   - `tools/match_function.py` ran on all 26 changed addresses in both
     trees. The single-unit compile reproduces every score change below.
   - The rebuilt instruction sequences of the two trees were diffed.
   - For the listings that did not change in the single-unit compile
     (`0x004078c0`, `0x0048b260`), the full Reccmp reports of both builds
     were diffed.

   No bisect was needed, because every change is attributable to the file
   it sits in.
7. **Rebuilds.** `main` moved twice while the PR was open: #484 (Config's
   Direct3DRM data prefix, `config.c` and the generated files), then #488
   (the Reccmp lint ratchet, tools only). Each time I reset the branch onto
   the new `main` and reran the tool for each unit, in the same order.
   Nothing was hand-merged. Each rebuilt tree differs from the one before
   only by what `main` brought in. Then I regenerated the three files from
   a fresh matching build on `504f5ef6`. The changed-function table below
   is the same against `504f5ef6`'s baseline, row for row: #484 changed no
   function in it. A scratch build of `504f5ef6` reproduces its baseline
   and gives its `unit-data-offsets` summary.

## Tool behaviour

No tool defect needed a workaround. One cosmetic issue, for a follow-up:

- **The port-tail notice is printed twice.**
  `python3 tools/relocate_unit.py G98-016 --stem flic_unit --dry-run` on
  `5f01c2ce` prints "`src/gods98/flic.cpp` holds no original function any
  more: it moves to the port tail" twice. `src/link_order.cmake` gets one
  `tail` entry, which is correct, so only the report is affected.

## Gates

- **Native:** `cmake -S . -B build && cmake --build build` passes, and
  `ctest` passes 35 of 35.
- **`make tools-test`** fails only at its first step, the `reccmp-lint`
  ratchet from #488 (below). Every other step passes when run on its own,
  including `link_order.py check`, the Meson source-parity check and the
  `relocate_unit` tests. The tool adds the five game files to `game_sources`
  and the five gods98 files to their Meson list. `binary_progress_inputs.txt`
  is regenerated for the 18 new sources (10 unit files, 8 new private
  headers).
- **`make reccmp-check` (MSVC 5.0):** the 5 drops below. It passes after
  the baseline refresh. `link_order.py check-map build-reccmp/LegoRR.map`:
  OK.
- **`make match-progress`:** all three generated files are committed.
  `pr_match_progress.py check` against `main`: fresh.

### Link-order list

There are ten new `unit` entries at their original positions, and
`gods98/flic.cpp` becomes a `tail` entry. Fifteen legacy keys move to the
lower median of their remaining markers:

| Legacy file | Key before | Key after |
| --- | --- | --- |
| `game/dependency_grid.cpp` | `0040b000` | `00418920` |
| `game/frontend.cpp` | `0041baa0` | `0041b9c0` |
| `gods98/input.cpp` | `0041e820` | `0041cab0` |
| `game/level.cpp` | `00432600` | `00432aa0` |
| `game/entity.cpp` | `00444680` | `00444fb0` |
| `gods98/simtick.cpp` | `00449c00` | `0044cf70` |
| `game/util.cpp` | `00419f10` | `0044d0d0` |
| `game/terrain_grid.cpp` | `004410f0` | `0044f380` |
| `gods98/message.cpp` | `00419b20` | `00456550` |
| `gods98/heap.cpp` | `00485160` | `0045a620` |
| `gods98/record.cpp` | `004712c0` | `0046d330` |
| `gods98/surface.cpp` | `0047d2d0` | `0047d410` |
| `gods98/resource.cpp` | `00477220` | `0047f980` |
| `game/startup_config.cpp` | `0048b290` | `0048b4a0` |
| `crt/crt.cpp` | `00491f97` | `00491ff5` |

The list goes from 73 entries to 82 (+10 units, −1 legacy entry that
became a tail entry). The static check and the map check pass.

### The `reccmp-lint` ratchet (#488) sees key churn, not new debt

`lint_reccmp_annotations.py` reports **20 new entries, and 20 entries that
can be removed**. They pair one to one, by name and scope. Each is an
unannotated definition (lint debt that already existed) whose head lost
`static` in this relocation, and each of those removals is recorded in the
relocation CSVs:

- **GAME-010:** `cell_flag4_is_set_target`, `cell_flag100_set_target`,
  `cell_flag100_is_set_target` and `terrain_cell_record_get_target` (×2
  each: both alternatives of a naked/C `#if` pair), and
  `unported_ledger_stubs::dependency_grid_current_level`,
  `dependency_grid_current_storage` and
  `dependency_grid_cell_entries_ensure_loaded`;
- **GAME-008:** `unported_ledger_stubs::entity_cell_path_effects_apply_level`;
- **GAME-013:** `level_cell_kind_grid_storage_release`;
- **G98-008:** `fe_font_cell`, `fe_font_keys` and `fe_font_advance`;
- **G98-013:** `input_global_current_keys_copy`;
- **G98-016:** `le16`, `le32` and `flic_resource_route_resolve`.

The key is the scope, the name and a hash of the head text. The head
includes `static`, so a relocation that shares a static changes the key,
even though no definition is added. The debt count does not change. I did
not update `reccmp-lint-baseline.txt`, as the owner asked: accepting the new
keys needs `make reccmp-lint-update
RECCMP_LINT_UPDATE_ARGS=--allow-growth`, which removes the 20 old keys and
adds the 20 new ones. Two alternatives would avoid this for every later
relocation: keying the ratchet without storage class, or having
`relocate_unit.py` rewrite the baseline keys it changes.

## Layout metrics

From `binary-progress.json`, `main` (`504f5ef6`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 493,667 | 408,393 (−85,274) |
| Pair inversions, game-only | 479,964 | 394,790 (−85,174) |
| Adjacent inversions, full | 832 | 764 |
| Adjacent inversions, game-only | 771 | 704 |
| Unit-order inversions, pairs | 3,533 | 3,323 (−210) |
| Unit-order inversions, adjacent | 88 | 83 |
| Unit contiguity | 70.51% (153 units) | 74.65% (162 units) |
| Link-order list entries | 73 | 82 |
| Unit-relative identity (masked) | 6.62% | 6.62% |
| Size-exact units | 26 | 26 |
| Exact-function code bytes | 44,551 | 44,636 (+85, `0x0048b290`) |
| Mean score, full / game-only | 55.16% / 51.00% | 55.19% / 51.04% |
| `.text` identical at original addresses | 18,331 B (2.85%) | 18,245 B (2.84%) |
| `.rdata` identical at original addresses | 1,482 B (13.19%) | 1,519 B (13.52%) |
| `.data` identical at original addresses | 12,863 B (17.57%) | 13,918 B (19.01%) |
| Functions identical at original address | 0 | 0 |

`.text` at original addresses stays at chance (about 2.8%). The 86-byte
fall comes from objects changing position, not from lost matches. The
`.rdata` and `.data` gains also come from objects changing order: the ten
new objects link at their units' positions, so their constants and
initialised data move with them. Neither section is size-exact yet, so
these numbers can move either way with the next relocation.

## Unit-relative data offsets (`make unit-data-offsets`)

| Unit | `main` | This change |
| --- | --- | --- |
| GAME-008, 010, 012 | n/a: no dedicated object (1 unknown `.bss` each) | n/a: one attributable `.bss` item each |
| GAME-013 | n/a: no dedicated object (4 unknown `.bss`) | **partial**: 4 `.bss` items, gap 3 first `004c7dfc` (4/−4) |
| GAME-019 | n/a: no dedicated object (1 unknown `.rdata`, 8 unknown `.bss`) | **partial**: 8 `.bss` items, gap 7 first `004dc6fc` (a4/3c8); 1 `.rdata` item |
| G98-008, 013, 016, 017, 028 | n/a: no dedicated object | n/a: fewer than two attributable items |
| **Summary** | full 0, gaps-only 1, partial 6, n/a 218 | full 0, gaps-only 1, partial **8**, n/a **216** |

- **The unknowns are gone.** The 16 data annotations of these units that
  were unknown before (15 `.bss`, 1 `.rdata`) are all attributed now.
- **Why the two new results are partial.** Both new partial results are
  within-unit `.bss` order. The relocation keeps the port's declaration
  order, so this is expected: it is work for the units' later conversions,
  not for link order.
- **The gods98 units.** The five gods98 units own no annotated data, so
  they stay `n/a`.

## Every changed function, with its classifier label

The changes are the 5 drops and 10 rises reported by `make reccmp-check`
against `main`'s baseline (`504f5ef6`), plus every listing whose format-4
digest changed.
They are the same in both scopes. The "Cause" column comes from the
`match_function` diff of `main` against this tree. "(moved)" marks
functions that this change relocates. Every other function sits in a
legacy file that the relocation edited.

| Address | Function | File | Score | Instr. | Label | Cause |
| --- | --- | --- | --- | --- | --- | --- |
| `0x00419830` | `entity_linked_list_entries_remove_by_key` (moved) | `info.cpp` | **8.96 → 8.82** | 38 → **39** | `drift-review`, +1 | register allocation: the list head is reloaded through `esi` instead of being reused from `ecx`, which adds one `mov`. **Drop; OWNER REVIEW (instruction count)** |
| `0x00419da0` | `incident_message_panel_refresh_if_active` (moved) | `info.cpp` | **23.33 → 20.00** | 30 → 30 | `drift-reordered` | the loads of `s_idx_head` and `s_idx_count` are hoisted above the test and swap order. **Drop** |
| `0x0047d800` | `surface_bitmap24_bit_surface_convert` | `surface.cpp` | **29.34 → 29.17** | 164 → **165** | `drift-review`, +1 | stack slots of the spilled mask values reassigned, and the byte-unpack loop's registers reallocated (one more `xor edx, edx`). **Drop; OWNER REVIEW (instruction count)** |
| `0x004844b0` | `flic_frame_record_walk` (moved) | `flic_unit.cpp` | **17.65 → 16.67** | 111 → 111 | `drift-review` | `edx`/`ecx` swap in the loop body. One encoding is a byte longer (296 B), so the branch displacements shift by 1. **Drop; OWNER REVIEW** |
| `0x00485390` | `text_buffer_draw_wrapped` (moved) | `text_window.cpp` | **5.99 → 5.96** | 294 → **296** | `drift-review`, +2 | the `flags & 4` temporary moves from `edx` to `edi`; the clip test reloads `[esp+0x454]` and recomputes the end pointer with `mov`/`add` instead of one `lea`. **Drop; OWNER REVIEW (instruction count)** |
| `0x0048b290` | `ledger_stub_0048b290_StartupStreamsClose` (moved) | `file_list_streams.cpp` | 72.73 → **100.00** ↑ | 22 → 22 | `drift` | listing unchanged. On the original side, Reccmp now names the operands `s_startup_streams+4`/`+8`; before they were unnamed `<OFFSETn>` and did not pair. The array's only declaration change is the lost `static` (G98-028.csv), so it is now an external symbol. **Exact match gained (+85 B)** |
| `0x0048b260` | `ledger_stub_0048b260_StartupStreamStateReset` (moved) | `file_list_streams.cpp` | 19.05 → 57.14 ↑ | 10 → 10 | `drift` | listing unchanged; the same operand pairing as `0x0048b290` |
| `0x0041a1a0` | `ui_entry_hit_test_and_dispatch` (moved) | `info.cpp` | 32.38 → 35.47 ↑ | 137 → **130** | `drift-review`, −7 | `ebp`/`ebx` swap. Two cached field pointers (`[ebp+0x10]`, `[ebp+0xc]`) are no longer kept in stack slots, so the frame shrinks from `0x2c` to `0x24`. **OWNER REVIEW (instruction count)** |
| `0x00485710` | `text_wrap_measure_and_draw_lines` (moved) | `text_window.cpp` | 22.48 → 24.94 ↑ | 225 → **214** | `drift-review`, −11 | one stack temporary fewer (frame `0x834` → `0x830`); spills and reloads removed and slots renumbered. **OWNER REVIEW (instruction count)** |
| `0x0044fb90` | `terrain_state_surface_point_project_offset` | `level.cpp` | 23.19 → 23.65 ↑ | 252 → **251** | `drift-review`, −1 | x87: an `fld`/`fsub` pair reordered and one `fxch` dropped. **OWNER REVIEW (instruction count)** |
| `0x004512d0` | `terrain_cell_normal_sample` | `level.cpp` | 11.34 → 13.27 ↑ | 99 → **100** | `drift-review`, +1 | commutative `fadd` operands swap, and a CSE'd `lea edx, [ecx+0xc]` is added: #477's change again, after #483 reverted it. **OWNER REVIEW (instruction count)** |
| `0x004089d0` | `segment_2d_point_distance` | `placement.cpp` | 40.89 → 43.56 ↑ | 120 → 120 | `drift-review` | x87 stack operands (`fld st(n)`, `fmul [esp+..]`) swap back to their pre-#477 order |
| `0x0047d520` | `surface_palette8_to_16bit_surface_expand` | `surface.cpp` | 32.65 → 33.82 ↑ | 166 → 166 | `drift-reordered` | stack slots of spilled locals reassigned; two byte loads swap |
| `0x00482620` | `mesh_segment_vertex_fill` | `record.cpp` | 26.09 → 31.30 ↑ | 57 → 57 | `drift` | `ecx`/`edx` swap (format 4 masks registers) |
| `0x00483420` | `screen_effect_item_opacity_set_target` | `record.cpp` | 30.51 → 36.16 ↑ | 88 → 88 | `drift-reordered` | prologue pushes and the index computation interleave differently |
| `0x00404f50` | `object_filter_matches_action_target` | `entity.cpp` | 25.00 = | 84 → 84 | `drift-reordered` | two independent `filt_dword` calls are scheduled in the other order; `cmp` operands swap |
| `0x004078c0` | `ledger_stub_004078c0_…` | `entity.cpp` | 10.26 = | 42 → 42 | `drift-review` | instructions identical; a pushed literal loses its `(string)` name, the reverse of finding 530's change |
| `0x00411360` | `menu_mouse_coordinate_update` | `frontend.cpp` | 21.12 = | 314 → 314 | `drift-reordered` | independent `fe_legacy_dword` calls rescheduled; `ebx`/`ebp` arguments swap (the reverse of #477's change) |
| `0x0041c1c0` | `advisor_overlay_hit_resolve` | `frontend.cpp` | 11.49 = | 93 → 93 | `drift-review` | commutative `fadd` operands swap (3), the reverse of #477's change |
| `0x004198f0` | `terrain_cell_linked_entries_remove` (moved) | `info.cpp` | 9.26 = | 60 → 60 | `drift-review` | `push esi` hoisted; the loop reloads `[ebx]` into `esi`; pop order changes |
| `0x00447950` | `entity_directional_activity_start` | `entity.cpp` | 33.87 = | 127 → 127 | `drift-review` | x87 `fdiv`/`fxch`/`fst` schedule |
| `0x0044fe70` | `terrain_authored_surface_height_interpolate` | `level.cpp` | 9.89 = | 103 → 103 | `drift-review` | x87 commutative operand order and the `fmul`/`fadd` schedule |
| `0x0049c7c0` | `wad_expand_payload` | `resource.cpp` | 58.27 = | 62 → 62 | `drift-reordered` | `shr`/`and` byte-swap steps reordered |
| `0x0049c840` | `wad_payload_decode_dispatch` | `resource.cpp` | 25.71 = | 71 → 71 | `drift-reordered` | `mov ch`/`mov cl`/`lea` reordered |
| `0x0049c8e0` | `wad_payload_decode_type1` | `resource.cpp` | 52.13 = | 105 → 105 | `drift-reordered` | `mov dl`/`mov dh`/`xor` reordered |
| `0x0049cc70` | `wad_bitstream_output_initialize` | `resource.cpp` | 26.09 = | 41 → 41 | `drift-reordered` | `shr`/`and` byte-swap steps reordered |

Notes:

- **Source.** No changed function has changed source; the classifier finds
  every body unchanged. This is ADR 0006's renumbering drift. Its signature
  is the same as in findings 523, 527 and 530: register and stack-slot
  assignment, the order of independent or commutative operations, and x87
  `fxch` scheduling.
- **Oscillation.** Several of these functions changed in #477 or #483 and
  change back here (`0x004089d0`, `0x00411360`, `0x0041c1c0`, `0x004512d0`,
  `0x004078c0`). They react to every edit of the legacy file that holds
  them, so they will keep moving until those files are drained.
- **For the owner:**
  - 5 drops, all small. The largest is `0x00419da0`, −3.33 points.
  - 7 instruction-count changes: `0x00419830` +1, `0x0047d800` +1,
    `0x00485390` +2, `0x004512d0` +1, `0x0041a1a0` −7, `0x00485710` −11,
    `0x0044fb90` −1. Three of them are drops.
  - The baseline is refreshed with these under rule 2.

The net effect is −5 scores, +10 scores, +85 exact bytes and +0.03 points
of mean score (full). Against that, pair inversions fall by 85,274 and
unit-order inversions by 210.

## What remains in each wave

Survey on `5f01c2ce`. A unit lists only the first blocking construct.

- **R1 (#442).** Done: GAME-001, 004, 006, 007, 009, 014, 015, 016 and,
  here, 008, 012, 013 and 010 (the plan lists GAME-010 under R5). Two units
  remain, each needing a one-line source fix in its own commit
  (finding 530):
  - **GAME-005**: `effects.cpp:50` defines `ScreenEffectTickRecord` and
    `s_screen_effect_tick_records` in one declaration, and both sides need
    it. Fix: split the declaration.
  - **GAME-011**: `view.cpp:3078` `scene_rotation_flags_apply` has an
    unannotated overload. Fix: annotate or rename it.
- **R2 (#443).** Done here: G98-008, 013, 016, 017 and 028. The rest:
  - **Anonymous-namespace members that both sides need** (the next tooling
    lever in finding 530):
    - G98-002: `scene_pool.cpp` `lego::s_scene_object_blocks` and 10 more;
    - G98-015: `spawn.cpp` `lego::s_result_check_hook`/`_context`;
    - G98-020: `scene_resources.cpp` `lego::s_release_hook` and 7 more.
  - **`#if` blocks and macros defined per compiler:**
    - G98-003: `SCENE_RESOURCE_COM_CALL`;
    - G98-010: `s_iid_direct_draw4`, `SURFACE_COM_CALL`;
    - G98-011: `SURFACE_COM_CALL`;
    - G98-014: `savegame.cpp` `LRR_MKDIR`;
    - G98-025: `VIEW_COM_CALL`, which is also redefined;
    - G98-030: `resource.cpp:3868` `fixed_record_slot_pointer_get`;
    - G98-031: `model.cpp:21` `strcasecmp`;
    - G98-012: `crt.cpp:931`, a conditional definition of `0x0047e7e0`.
  - **G98-009**: `simtick.cpp:460`, `static int s_sound_on, s_sound_active;`
    is one shared declaration. A one-line source fix is enough: split it.
  - **G98-005** (Main.c): its sources are `winapi`-only. That is out of
    scope for the tool.
  - **G98-001**: it already has its own unit entry
    (`game/video_graph.cpp`), but it is still in the game library. That
    move is not a relocation by this tool.
- **R3 (#444).** Done here: GAME-019 (Info.c). The rest:
  - **GAME-017** (FrontEnd.c): `crt.cpp:613`, the conditional `_ftol`
    declaration around `0x00417430`. It has the same shape as the one
    finding 531 split for GAME-032. This is a candidate one-line source fix.
  - **GAME-018** (HelpWindow.c): the anonymous-namespace members
    `upgrades.cpp` `lego::s_building_upgrade_begin_hook`/`_changed_hook`
    are needed by both sides.
  - **GAME-020** (Interface.c): `frontend.cpp:76` `MENU_ACTION_TABLE_ALLOC`
    is defined in an `#if` block (the survey counts two `#if` blocks and two
    redefined macros).

## Not done here

- **The units above.** The one-line source fixes (GAME-005, GAME-011,
  G98-009 and possibly GAME-017) belong in a separate change, following
  finding 531.
- **Owner review.** The five drops and seven instruction-count changes are
  accepted into the baseline under rule 2 by this change, and are flagged
  for the owner.
