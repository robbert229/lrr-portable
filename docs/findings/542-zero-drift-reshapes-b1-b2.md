# 542. Zero-drift reshapes B1 and B2: six files, no baseline change; the dry-runs move on

Issue #502, tickets **B1** and **B2** of the
[#490 plan](../plans/490-relocation-conditional-code.md), under ADR 0006's
"Addendum (2026-11-11): reshape commits". Measured on `main` at `cd80158d`
(after #497/#505, #496/#506 and #491/#509). Patterns and rows refer to the plan and to
[finding 539](539-conditional-relocation-blockers.md).

## Summary

1. **Six reshape commits, one per file, all zero drift.**
   `tools/reshape_check.py` reports identical tokens (native, matching, and
   emulated MinGW reference), identical includes and an identical
   matching-build object outside debug sections for every file. This holds
   for each commit against its parent and for the six files together
   against `cd80158d`.

   | File | Reshape | Plan rows |
   | --- | --- | --- |
   | `game/entity.cpp` | 13 split naked heads become whole alternative definitions (P1); `s_entity_rock_fall_in_name` hoisted into its own `#if` above `0x00433180`'s annotation (P3a) | 1, 5 |
   | `game/select.cpp` | 1 split head (P1), `0x004525c0` | 2 |
   | `gods98/record.cpp` | 1 split head (P1), `0x004712c0` | 3 |
   | `game/cmd.cpp` | `CMD_TASK_NAME_SET`/`CMD_PRIORITY_NAME_SET` hoisted out of `0x00401cf0`'s `#else`: defined in a non-MSVC-x86 `#if` before the annotation, `#undef`'d in one after the `#endif` (P3a) | 4 |
   | `game/mission.cpp` | the four region macros renamed `mission_blob_*_at` (63 occurrences) and their `#undef`s dropped (P4) | 18 |
   | `crt/crt.cpp` | `_ftol`'s declaration hoisted out of `0x00417430`'s MSVC-x86 branch (P3a); `0x0047e7e0`'s annotation moved below the `LEGORRR_CRT_STDCALL` group onto its definition | 6, 16 |

2. **No score or listing changed.** `make reccmp-check` passed with no
   drop. `make match-progress` rewrote only the `input_sha256` line of
   `match-baseline.txt` and `match-listing-digests.txt`, so no function's
   score and no listing digest changed. In `binary-progress.json` only
   the three hash fields changed.

   One caveat for anyone reproducing this: that file's positional
   `.text` `identical_bytes` depends on the build path. MSVC 5.0 mangles
   anonymous-namespace names with the absolute source path, and the
   path's length can move a static (`scene_resources.cpp`'s
   `s_shared_texture_root`, by 0x18 bytes). On the intermediate base
   `8e8d4da3` (#506), this worktree measured 17924 against the committed
   17918. `main` built at the same path also gave 17924, with `.text`,
   `.rdata`, `.data` and `.idata` byte-identical to this branch. On
   `cd80158d` the committed figure and this worktree's agree.

3. **The P1 shape was lint debt, so the lint learned it first.** The
   Reccmp annotation lint accepted an annotation only directly above a
   definition. The relocation tool needs one annotation above a group of
   whole alternative definitions, so every branch of such a group counted
   as debt (`float_to_long_x87` already had two entries). Splitting the 15
   heads would have added 15 entries, and the `cmd.cpp` hoist would have
   re-keyed 2. A preceding commit changes the lint: a definition in any
   branch of an `#if`/`#elif`/`#else` group, or of a group nested in one,
   counts as annotated when an annotation sits directly above the group's
   opening `#if`, with only blank lines and unconditional directives
   between them. The key format is unchanged. The ratchet only shrinks:
   296 entries were already annotated this way (4436 → 4140), and the
   `crt.cpp` annotation move removes one more,
   `bit_count32_by_position` (→ 4139). No commit adds an entry.

4. **Where each unit's `--dry-run` now stops.** #485 (overloads, anonymous
   namespaces; PR #493) landed after the plan was measured, so its
   predictions of "→ #485" are already partly spent.

   | Unit | Before | After | Plan's prediction |
   | --- | --- | --- | --- |
   | GAME-037 | P4 macros (row 18) | **plans cleanly** (49 functions) | relocates ✓ |
   | GAME-043 | 13 split heads (row 1) | row 20: `ENTITY_STATS_FLOAT_READ`/`WORD_READ` (P2), and the header needs statics `stats_rec_float`/`stats_rec_word` (T2) | row 20 ✓ |
   | GAME-021 | P3a group (row 5) | `s_entity_rock_fall_in_name`, a one-item `#if` group (P3b) | P3b ✓ |
   | GAME-027 | split head (row 2) and `s_cmd_rand_state` | `s_cmd_rand_state` only (row 8, P3b) | row 8 ✓ |
   | GAME-017 | P3a group (row 6) | `_ftol` declarations, `crt.cpp:596` and `:982` (P3b) | P3b ✓ |
   | GAME-032 | `_ftol` declarations | unchanged: `_ftol` (P3b) | P3b ✓ (not a B1 target) |
   | G98-012 | annotation above the macro group (row 16) | `LEGORRR_CRT_STDCALL` alternatives (P2) | P2 ✓ |
   | GAME-050 | split head (row 3) | anonymous-namespace items used before their definition (`ascii_case_equal`, `weapon_definition_reset`) | #485 ✓ |
   | GAME-002 | P3a group (row 4) | the hoisted `CMD_*_NAME_SET` macros (now a P2/P3b group with a trailing `#undef` group), and `CMD_POOL_LINK_GET`/`SET` alternatives at `cmd.cpp:356`–`364` (P2) | overload `cmd_create_move` (#485) ✗ |

   GAME-002 is the one miss. #485 has since taught the tool overloads, so
   the overload no longer blocks it. Its next blockers are T1's (P2/P3b):
   the plan's own description of P3a ("the hoisted helper is P3b"), plus
   one P2 group the plan's table did not list, `CMD_POOL_LINK_*`
   (pointer-width alternatives, finding 539's probe resolved it away).
   The survey therefore counts GAME-002 as "`#if` block ×6, macro ×6"
   instead of "`#if` block ×1"; that is the same unit, now blocked by
   constructs T1 handles.

5. **Survey.** 4 of 26 units relocate as they are, up from 3 (GAME-037).

## Consequences

- W (the relocation wave) can take GAME-037 now.
- T1 (whole conditional groups) unblocks GAME-002's next stage, GAME-017,
  021, 027 and 032, and G98-012's macro; T2 (exporting a static) is
  GAME-043's other blocker.
- The 15 duplicated heads are the plan's accepted cost of P1: a signature
  change needs two edits until #389 collapses the dual paths.

## Reproducing

    python3 tools/relocate_unit.py --survey
    python3 tools/relocate_unit.py GAME-002 --stem scratch --dry-run --no-footprint
    python3 tools/reshape_check.py src/game/entity.cpp src/game/select.cpp \
        src/gods98/record.cpp src/game/cmd.cpp src/game/mission.cpp \
        src/crt/crt.cpp --base cd80158d --head HEAD
    make reccmp-lint
