# Finding 527 — Relocation wave R1: the six units `relocate_unit.py` can move today

Issue #442, stage S3 (wave R1) of the [#388 plan](../plans/388-original-link-order.md)
([ADR 0006](../adr/0006-original-link-order.md) decisions 3 and 4, entropy
rules 2 and 3). The tool and its first proof (GAME-001) are
[finding 523](523-relocate-unit-tool-and-advisor-relocation.md). Measured on
`main` at `4fe92113` (after #469, #473, #474, #475 and #476).

## Summary

1. **Six units relocate**, each in one commit generated only by
   `tools/relocate_unit.py`:

   | Unit | File | Functions moved | Owned globals | Closure | Not in `src/` |
   | --- | --- | ---: | ---: | ---: | ---: |
   | GAME-004 | `game/animation_object.cpp` | 18 | 0 | 1 | 3 |
   | GAME-006 | `game/building_model.cpp` | 21 | 1 | 1 | 1 |
   | GAME-009 | `game/screen_effect_quads.cpp` | 11 | 2 | 4 | 0 |
   | GAME-014 | `game/encyclopedia.cpp` | 4 | 4 | 1 | 2 |
   | GAME-015 | `game/lava_erosion.cpp` | 14 | 17 | 1 | 1 |
   | GAME-016 | `game/steering.cpp` | 19 | 0 | 4 | 4 |

   The 87 functions come from 14 legacy files, mostly `gods98/view.cpp`,
   `gods98/record.cpp` and `game/level.cpp`; `game/mission.cpp` gives one
   global. They are game functions there, so ADR 0006 decision 8
   applies: they move from the `gods98` library to `game`. Every body hash
   agrees (`docs/coverage/relocations/GAME-0{04,06,09,14,15,16}.csv`). 90
   heads lose `static` and are recorded. No shared header is edited.
2. **Three tool defects need hand fix-ups.** They are in four separate
   commits, and none changes a moved body:
   - a scanner misparse writes `extern i < 9u;` into
     `entity_config_shared.hpp` (GAME-004, again for GAME-006);
   - the private header is included after the file's *last* `#include`. In
     `frameloop.cpp` (GAME-009) and `entity.cpp` (GAME-014), that is a stray
     late include, after the code that uses the shared names;
   - the `.bss` ownership slack puts GAME-015's first global into GAME-014.
     This is avoided by relocating GAME-015 first; no edit is needed.
3. **Layout improves** (schema 3, `binary-progress.json`):

   | Measure | Before | After |
   | --- | ---: | ---: |
   | Pair inversions, full | 593,010 | 512,033 (−80,977) |
   | Unit-order pair inversions | 3,736 | 3,585 (−151) |
   | Unit contiguity | 67.28% (146 units) | 70.05% (152 units) |
4. **Scores drift as rule 2 expects.**
   - 3 functions fall below baseline and 2 rise. The mean scores do not move
     (55.19% full, 51.03% game-only).
   - **One exact match is lost.** `lava_erosion_delay_from_cell`
     (`0x0040e9c0`, 100% → 94.74%, −66 exact bytes) is a commutative
     `fmul` operand swap in moved, unchanged code.
   - **Two listings change instruction count by one.** One gains an x87
     `fxch` around a commutative operation; the other gains a
     common-subexpression `lea`. One of them falls below baseline.
   - All three need owner review.

## Method

1. `relocate_unit.py --dry-run` for each unit. All six plan. GAME-004, 006,
   009, 015 and 016 are unnamed and need `--stem`. The stems follow the unit
   notes in `translation-unit-ranges.csv`.
2. One tool run and one commit per unit (rule 3), in the order 004, 006,
   009, **015, 014**, 016. Each fix-up is a separate commit right after the
   relocation it repairs, so every relocation-plus-fix-up pair builds
   natively.
3. Twice while this was open, `main` moved: first #475 (Meson parity) and
   #473, then #476 (`EntitySlot` offsets, which edits `entity.cpp` and
   `game.hpp`). Each time the branch was rebuilt on the new `main` by
   rerunning the tool for each unit and cherry-picking the four
   fix-ups. #475 changed the tool's Meson wiring (it now adds to
   `game_sources`). Each regenerated tree differs from the one before only
   by what `main` brought in (`meson.build`; then #476's lines in
   `entity.cpp` and `game.hpp`). The relocation CSVs change only in source
   line numbers.
4. One full matching build at the end: `make reccmp-check`, then
   `make match-progress`. To attribute the drift, `tools/match_function.py`
   was run on the changed addresses in this tree and in a scratch `main`
   worktree (at `ce1b332c`, before #476), and the rebuilt instruction
   sequences were diffed. The single-unit compile reproduces every listing
   change below. The final classification is against `4fe92113`'s digests
   and baseline. On #476's tree, two listings that changed against
   `ce1b332c` (`0x00444fb0` and `0x00444680` in `entity.cpp`, ±1
   instruction each) no longer change. `0x0040ff50` (75% on `ce1b332c`) is
   already 87.50% on `4fe92113`.

## Tool defects found (not fixed here)

The task did not allow tool edits. Each defect is worked around in its own
commit or by commit order, and each is a follow-up for `relocate_unit.py` /
`cxx_items.py`.

1. **A `#define` that continues through a block comment.**
   `entity_config.cpp`'s ObjectNames load macro has a `/* ... */` comment
   inside its body. The comment's inner lines have no trailing backslash,
   which is legal: comments are removed before line splicing matters.
   `cxx_items.py` ends the macro at the first such line, so the tail of the
   macro scans as top-level items. One of them is taken as a symbol that the
   moved code uses: `extern i < 9u;` appears in the private header. Fix-up
   commits remove the line (twice, because GAME-006 appends it again).
2. **Private-header placement.** The tool includes `<file>_shared.hpp` after
   the source's last `#include`. That is wrong when the file has a late
   include and the moved types, macros or variables are used before it:
   - `frameloop.cpp` has `#include "game/util.hpp"` at line 2028. The
     header carries `FrameloopApplicationState`, `RewardFramePumpContext`,
     `MenuSoundTickClock` and the window macros, which are used from line 59
     on.
   - `entity.cpp` has a second include block at line 916, and uses
     `s_item_name_ore` and `s_item_name_power_crystal` at line 460.

   Each fix-up moves the one include line to the end of the top include
   block. The verbatim check cannot see this because the moved bodies are
   untouched; only a compile finds it.
3. **Ownership slack across unit boundaries.**
   `conversion_footprint._in_ranges` accepts addresses up to `hi + 8` for
   `.bss` and `.data`. GAME-014's `.bss` ends at `004c7ea8`, so it also
   claims `004c7eb0` (`s_lava_erosion_pending_cells`), the first address of
   GAME-015's `.bss`. Relocating GAME-015 first gives the array to its own
   unit. GAME-014 then reports "GLOBAL 0x004c7eb0 stays in
   src/game/lava_erosion.cpp". The slack should not cross into a range that
   another unit names exactly.

## Gates

- **Native:** `cmake --build build` passes, and `ctest` passes 35 of 35,
  after each relocation-plus-fix-up pair and on the final tree.
- **`make tools-test`** passes, including `link_order.py check` and #475's
  Meson source-parity check. The tool adds the six files to `game_sources`
  in `src/game/meson.build`.
- **`make reccmp-check` (MSVC 5.0):** the 3 drops below. It passes after the
  baseline refresh. `link_order.py check-map build-reccmp/LegoRR.map`: OK.
  The 26 "Dropped duplicate address" Reccmp errors (for example
  `0x45ab30` twice in `crt/crt.cpp`) exist on `main` too.
- **`make match-progress`:** all three generated files, plus
  `tools/binary_progress_inputs.txt` (`binary_progress_hash.py
  --write-manifest`; 17 new sources: 6 unit files and 11 private headers).

### Link-order list

Six `unit` entries are added at their original positions. GAME-004, 006 and
009 go after `game/bezier.c`. GAME-014, 015 and 016 replace `ptrstack.cpp`'s
old slot after `dependency_grid.cpp`.

Thirteen legacy keys move to their new lower medians. Nine entries stay in
place: `util.cpp`, `frontend.cpp`, `simtick.cpp`, `entity_config.cpp`,
`level.cpp`, `entity.cpp`, `frameloop.cpp`, `heap.cpp` and `crt.cpp`.
`ptrstack.cpp` (`0040fe40` → `004628e0`), `record.cpp` (`00464720` →
`0046db60`), `view.cpp` (`004734c0` → `004750b0`) and `resource.cpp`
(`00476d10` → `00477220`) move down the list. The static check and the map
check pass.

## Layout metrics

From `binary-progress.json`, `main` (`4fe92113`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 593,010 | 512,033 |
| Pair inversions, game-only | 579,179 | 498,210 |
| Adjacent inversions, full | 877 | 842 |
| Adjacent inversions, game-only | 816 | 781 |
| Unit-order inversions, pairs | 3,736 | 3,585 |
| Unit-order inversions, adjacent | 90 | 88 |
| Unit contiguity | 67.28% (146 units) | 70.05% (152 units) |
| Link-order list entries (inversions) | 66 (0) | 72 (0) |
| Unit-relative identity (masked) | 36,674 B (6.62%) | 36,674 B (6.62%) |
| Exact-function code bytes | 44,617 | 44,551 (−66, `lava_erosion_delay_from_cell`) |
| Mean score, full / game-only | 55.19% / 51.03% | 55.19% / 51.03% |
| `.text` identical at original addresses | 17,932 B (2.79%) | 18,285 B (2.85%) |
| `.rdata` identical at original addresses | 1,277 B (11.37%) | 1,423 B (12.67%) |
| `.data` identical at original addresses | 13,683 B (18.69%) | 12,774 B (17.45%) |
| Functions identical at original address | 0 | 0 |

`.text` at original addresses rises by 353 B even though one exact function
(66 B) is lost: objects change position, and this measure is at chance
(about 2.8%). It stays at chance (2.8%), as in finding 523.

The `.rdata` and `.data` changes come from objects changing order. The six
new objects link early in the game segment, and their constants and
initialised data now come before those of the legacy files they left. At
original addresses both sections are still far from size-exact, so these
numbers move with every relocation, in both directions. The 903-byte `.data`
fall has not been traced byte by byte. Unit-relative identity, which does
not depend on absolute addresses, does not change.

## Every changed function, with its classifier label

The changes are the 3 drops and 2 rises reported by `make reccmp-check`
against `main`'s baseline, plus every listing whose format-4 digest changed.
They are the same in both scopes. The cause comes from the `match_function`
diff of `main` against this tree.

| Address | Function | Source file | Score | Instr. | Label | Cause |
| --- | --- | --- | --- | --- | --- | --- |
| `0x0040e9c0` | `lava_erosion_delay_from_cell` (moved) | `lava_erosion.cpp` | **100.00 → 94.74** | 19 → 19 | `drift` | `fmul [s_erosion_delay_scale]; fmul [s_erosion_ticks_per_second]` become the other order. **Exact match lost; OWNER REVIEW** |
| `0x004089d0` | `segment_2d_point_distance` | `view.cpp` | 43.56 → 40.89 | 120 → 120 | `drift-review` | x87: two `fmul [esp+..]` stack operands and two `fld st(n)` swap. **OWNER REVIEW** |
| `0x00471370` | `cmd_place_ray_candidate_check` | `cmd.cpp` | 15.85 → 15.76 | 188 → **189** | `drift-review`, +1 | one `fxch st(1)` before a `faddp` appears. **OWNER REVIEW (instruction count)** |
| `0x004512d0` | `terrain_cell_normal_sample` | `level.cpp` | 11.34 → 13.27 (up) | 99 → **100** | `drift-review`, +1 | commutative `fadd` operands swap, and a CSE'd `lea edx, [ecx+0xc]` is added. **OWNER REVIEW (instruction count)** |
| `0x00444460` | `ledger_stub_00444460_FlaggedVectorStepCommandIssue` | `entity.cpp` | 22.72 → 23.61 (up) | 291 → 291 | `drift-review` | `fld [eax]; fmul st(1)` becomes `fld st(0); fmul [eax]` |
| `0x00411360` | `menu_mouse_coordinate_update` | `frontend.cpp` | 21.12 (same) | 314 → 314 | `drift-reordered` | independent `fe_legacy_dword` calls scheduled in another order, `ebx`/`ebp` arguments swapped |
| `0x00418da0` | `help_window_button_hit_test` | `frontend.cpp` | 35.49 (same) | 144 → 144 | `drift-review` | commutative `fadd` operands swap |
| `0x0041c1c0` | `advisor_overlay_hit_resolve` | `frontend.cpp` | 11.49 (same) | 93 → 93 | `drift-review` | commutative `fadd` operands swap (3) |
| `0x0042c5b0` | `terrain_cell_byte_state_classify_current` | `level.cpp` | 57.93 (same) | 74 → 74 | `drift-review` | commutative `imul` operands swap, one load hoisted |
| `0x00435230` | `random_variant_cell_scatter` | `level.cpp` | 33.78 (same) | 70 → 70 | `drift-reordered` | independent `add`/`sub` reordered |

Notes:

- **Source.** No changed function has changed source. Every one sits in a
  file the relocation edited: moved into a new unit file, or left in a
  legacy file that lost functions above it. This is ADR 0006's renumbering
  drift. Its signature is the same as in finding 523: the operand order of
  commutative x87 and integer operations, and the `fxch` that goes with it.
- **The lost exact match.** `0x0040e9c0` is reproduced by a single-unit
  compile (`match_function`: 94.74%, effective match). It is not caused by
  the linkage change alone: making `s_erosion_delay_scale` external in
  `main`'s `level.cpp` keeps 100%. The order comes from the new file's
  context: its declarations and the symbols before the function. The
  operands are `DATA` names, which format 4 masks, so the digest is
  unchanged and the classifier says `drift`. The whole difference is the
  order of two `fmul`s.
- **Instruction-count changes.** In `0x00471370` an `fxch` appears; in
  `0x004512d0` a common-subexpression `lea` appears. Both are listed here
  for the owner as ADR 0006 rule 2 requires. `0x004512d0` rises in score.

The net effect is −3 scores, +2 scores, −66 exact bytes and no change in the
mean scores, against a fall of 80,977 pair inversions and 151 unit-order
inversions.

## Not done here

- The namespace-blocked R1 units (GAME-005, 007, 008, 011, 012, 013) wait
  for #471.
- The three tool defects above are follow-ups for `relocate_unit.py` and
  `cxx_items.py`.
- The lost exact match at `0x0040e9c0` and the two instruction-count
  changes are accepted into the baseline under rule 2 by this change, and
  are flagged for the owner.
