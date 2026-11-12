# Finding 546 — GAME-032 (Panel.c) relocated; GAME-021 (Lego.c) blocked by anonymous-namespace helpers

Refs #445 #446. Stage S3 of the [#388 plan](../plans/388-original-link-order.md),
[ADR 0006](../adr/0006-original-link-order.md) decisions 3–4 and entropy
rules 2–3. Measured on `main` `02ee3fcf`, after #516 (finding 547).

The first run was on `8d548015`. After #516 merged, the branch was reset onto
`main`, and the split and the relocation were rerun with the tool. They
produced the same plan: 50 functions, 71 owned globals and 7 closure items.
Two things differ from the first run:
- **A second fix-up.** #516 had exported `s_task_name_crystals`, so the
  rebased relocation needs a second fix-up (below).
- **Different drift.** The first run's two drops (`0x00483420` and
  `0x004882e0`) do not recur. This run has one drop, `0x00482620`.

## Summary

1. **GAME-032 (Panel.c, R5) moves.** First its declaration split, as its own
   commit, with `--check-layout` reporting the data layout unchanged. Then one
   tool-generated relocation commit. It moves 50 functions, 71 owned globals
   and 7 closure items into `src/game/panel_unit.cpp`.
2. **Two fix-up commits are needed. Both are tool gaps for #514.**
   - The new `crt_for_panel_unit.hpp` includes `crt_shared.hpp` and all 15 of
     its #496 migration snapshots, so it does not compile.
   - A `const` global that #516 exported moves without a prior `extern`, so
     the native link fails.
3. **GAME-021 (Lego.c, R4) does not move.** The survey lists it as "ok after
   `--split-declarations`". After the split, the relocation refuses two
   anonymous-namespace helpers that would be used before their definition in
   the new file. Fixing that needs a change to `relocate_unit.py`, which #514
   owns. No GAME-021 split is committed.
4. **No previously relocated unit's listing changed.**
   - No earlier unit file or private header is edited.
   - All 970 reported functions in the 63 earlier unit files keep identical
     listing digests, in both scopes.
5. **Two functions change, and one score drops.** Both are in legacy files.
   - `0x00482620` drops from 31.30 to 26.09. Its label is `drift`: an
     `ecx`/`edx` swap, with the instruction count unchanged.
   - `0x00411360`'s listing is reordered with no score change.

   **No exact match is lost, and no instruction count changes.**
   Exact-function code bytes are unchanged (65,750). The full mean moves from
   56.35 to 56.34 and the game-only mean stays at 51.66. **The baseline is
   refreshed.**
6. **Layout:** pair inversions fall from 238,597 to 210,140 (−28,457), and
   unit contiguity rises from 87.21% (191 units) to 87.67% (192).

## What moves

| Unit | File | Functions moved | Owned globals | Closure | Not in `src/` | From | Private headers written |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| GAME-032 | `game/panel_unit.cpp` (Panel.c) | 50 | 71 | 7 | 2 (`FUN_0045a460`, `RectContainsOffsetPoint`) | `frontend.cpp` (21 functions, 66 globals), `crt/crt.cpp` (14, 2), `tutorial_hud.cpp` (4), `ui_list.cpp` (3), `gods98/simtick.cpp` (2, 1), `gods98/view.cpp` (2), `mission.cpp` (1, 2), `objective.cpp` (1), `gods98/heap.cpp` (1), `gods98/record.cpp` (1) | `crt_for_/crt_from_panel_unit.hpp`, `frontend_for_/frontend_from_panel_unit.hpp`, `mission_from_panel_unit.hpp`, `tutorial_hud_for_panel_unit.hpp`, `simtick_for_/simtick_from_panel_unit.hpp`, `view_for_panel_unit.hpp` |

- **The stem.** The default stem `panel` names the existing C boundary header
  `src/game/panel.h`, so the tool uses `panel_unit`. This follows
  `objective_unit`, `flic_unit` and `input_unit`.
- **Closure:** `MISS_CRYSTAL_QUOTA`, `s_pair_count_histogram`,
  `s_pair_count_dominant`, `s_timer_handles`, `panel_load_seam_call`,
  `frontend_panel_rotation_image_load` and `panel_item_word`. The last three
  get forward declarations.
- **Shared items.** 33 `static` removals, recorded in
  `docs/coverage/relocations/GAME-032.csv`:
  - 16 moved items that remaining code still uses;
  - 17 staying items that the unit uses, among them `simtick.cpp`'s
    `s_mgr`, `s_mgr_count`, `s_repos`, `s_attach`, `s_metrics`, `s_repos_n`,
    `s_attach_n` and `s_timer_bits`, and `frontend.cpp`'s text-output
    bindings.

  The first run had 34. `s_task_name_crystals` lost its `static` in #516.
- **The port tail.** `tutorial_hud.cpp` and `ui_list.cpp` lose their last
  original functions and become port-tail entries. Seven legacy keys are
  rewritten to their new lower medians.
- **Not wired.** The conditional includes `<io.h>`, `<mbstring.h>`,
  `<mbctype.h>`, `<time.h>` (from `crt.cpp`) and `"lwsc_winapi.hpp"` (from
  `view.cpp`) are not copied. Both builds compile.
- **Body hashes.** Every body is verbatim.

## Commits (ADR 0006 rule 3)

- `dd73ec9d` Split `s_repos_n`/`s_attach_n` for the GAME-032 relocation:
  `relocate_unit.py GAME-032 --split-declarations --check-layout`.
  `simtick.cpp:254` `static int s_repos_n = 0, s_attach_n = 0;` becomes two
  declarations. Layout of `simtick.cpp`: **unchanged** (217 data symbols and
  section sizes identical). Recorded in `GAME-032-splits.csv`.
- `45a643e3` Relocate GAME-032 (Panel.c) into `src/game/panel_unit.cpp`:
  `relocate_unit.py GAME-032`, with no hand edits.
- `86acea05` **Fix-up 1:** `crt_for_panel_unit.hpp` includes `crt_shared.hpp`
  only.
- `329b1260` **Fix-up 2:** `panel_unit.cpp` declares `s_task_name_crystals`
  `extern` before its definition.
- A final measurement commit holds the three generated files, the input
  manifest (`tools/binary_progress_inputs.txt`) and this finding.

## Fix-up 1: migration snapshots included together

`crt_for_panel_unit.hpp` declares `extern void* s_panel_records[DAT_WINDOW_COUNT];`.
`DAT_WINDOW_COUNT` is defined in `crt_shared.hpp` and in each of its 15
migration snapshots (`crt_for_advisor.hpp` … `crt_for_tutorial_script.hpp`,
finding 541). The tool's rule "include the earlier private headers of L whose
names it uses" therefore included all 16. The snapshots are verbatim copies of
`crt_shared.hpp` under different include guards. All 15 have one body SHA-256
in `private-header-migration.csv`. Including two of them redefines
`struct RequestRecord`, and the native build fails in `panel_unit.cpp` and
`crt.cpp`.

The fix keeps only `#include "crt_shared.hpp"`. `crt.cpp` already includes it,
so `crt.cpp` sees no new declarations. `panel_unit.cpp` sees the one body
every snapshot has, once. No other new header includes an earlier private
header.

**For #514:** include at most one header from each migration family: the
legacy `<l>_shared.hpp` when L includes it, otherwise one snapshot.

## Fix-up 2: a const object exported by an earlier relocation

- **What #516 did.** It made `s_task_name_crystals` external. `mission.cpp`
  includes `mission_for_reward.hpp`, which declares
  `extern const char s_task_name_crystals[];`, before the definition. The
  GAME-037 unit file `reward.cpp` uses the name.
- **What GAME-032 does.** It moves the definition,
  `const char s_task_name_crystals[] = "Crystals";` (`0x004a8a08`), verbatim
  into `panel_unit.cpp`.
- **Why the link fails.**
  - The tool includes a unit's `from` header for a const object only when L
    still uses the name. `mission.cpp` no longer does, so
    `mission_from_panel_unit.hpp` does not declare it.
  - The unit file may not include `mission_for_reward.hpp`: that would show
    it its own definitions (finding 538, rule A).
  - So nothing declares the object `extern` before its definition. A
    namespace-scope `const` then has internal linkage, and `reward.cpp`'s
    reference is undefined in the native link.
- **The fix.** One `extern const char s_task_name_crystals[];` line before the
  item. The item's text and leading comment are unchanged, so its body hash
  still matches, and no frozen header is edited.
- **For #514:** a moved const object that another file uses through an
  earlier private header of L needs a prior extern in the unit file.

## GAME-021 (Lego.c): blocked

- **The survey.** It reports GAME-021 as "ok after `--split-declarations`"
  (`simtick.cpp:558`, `static int s_minion_actions = 0, s_minion_fails = 0;`).
- **The split works.** On `8d548015`,
  `relocate_unit.py GAME-021 --split-declarations --check-layout` wrote the
  split, and the layout was unchanged (217 data symbols and section sizes
  identical).
- **The relocation then refuses:**

  ```
  refused: GAME-021 cannot be relocated safely:
    - lego::prefix_equal is in an anonymous namespace and used before its definition in src/game/lego.cpp
    - lego::upgrade_type_tables_release is in an anonymous namespace and used before its definition in src/game/lego.cpp
  ```

- **The construct.** `src/game/upgrades.cpp` defines `prefix_equal` and
  `upgrade_type_tables_release` (with `ascii_lower`) in
  `namespace lego { namespace { … } }`. Only GAME-021 functions use them:
  `level_upgrade_name_strings_load` (`0x00427f80`) and `upgrade_types_load`
  (`0x0042d460`). So they move as closure helpers. The tool places closure
  helpers after the unit's functions (ADR 0006 decision 4). It forward-declares
  a `static` helper that is used early, but it refuses to forward-declare an
  anonymous-namespace one.
- **Why the survey missed it.** This is the same blind spot finding 543 found
  for G98-009. For a unit that needs a split, the survey reports only the
  split refusal, and later refusals appear only after the split is written.
- **Nothing else is hidden** (on `8d548015`). A throwaway copy of the tool
  outside the repo turned only this refusal into a note. The dry run then
  planned the whole relocation:
  - 197 functions, 12 owned globals and 128 closure items, from 23 legacy
    files;
  - 23 new private headers.
- **Paths forward.**
  - Teach the tool to write the forward declaration inside a reopened
    `namespace lego { namespace { … } }`. This is valid C++, since one
    translation unit has one anonymous namespace. #514 is handling it.
  - The ADR 0006 addendum's reshape list does not cover moving items out of an
    anonymous namespace. That would change the mangled names, so it would not
    be zero-drift anyway.
- **No GAME-021 split is committed.** It is reproducible with the command
  above.

## Layout (`binary-progress.json`), `main` `02ee3fcf` → this change

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 238,597 | 210,140 (−28,457) |
| Pair inversions, game-only | 223,157 | 194,730 (−28,427) |
| Adjacent inversions, full / game-only | 556 / 494 | 536 / 476 |
| Unit-order pair inversions | 3,174 | 3,135 (−39) |
| Unit-order adjacent inversions | 64 | 60 |
| Unit contiguity | 87.21% (191) | 87.67% (192) |
| Link-order entries | 110 | 109 |
| Size-exact units | 24 | 24 |
| Unit-relative identity | 37,819 B (6.83%) | 37,819 B (6.83%) |
| Exact-function code bytes | 65,750 | 65,750 |
| Mean score, full / game-only | 56.35 / 51.66 | 56.34 / 51.66 |
| `.text` identical at original addresses | 18,332 B (2.85%) | 18,505 B (2.88%) |
| `.rdata` identical at original addresses | 1,266 B (11.27%) | 1,265 B (11.26%) |
| `.data` identical at original addresses | 11,257 B (15.38%) | 11,176 B (15.26%) |
| Functions identical at original address | 0 | 0 |

- **Link-order entries.** One unit entry is added. `tutorial_hud.cpp` and
  `ui_list.cpp` move from legacy entries to the port tail, which the count
  omits.
- **At-address identity** follows the object layout and stays near chance.

**`make unit-data-offsets`:** 0 full / 2 gaps-only / 19 partial / 204 n/a.
On `main` (finding 547) it is 0 / 2 / 18 / 205. GAME-032 is the only change:
it now has its own object, and its `.bss` resolves as partial (20 items, with
a 19-item gap from `00500110`). Its `.rdata`/`.data` rows have fewer than two
attributable items.

## Every changed function (ADR 0006 rule 2)

The table lists every function whose listing digest or score changed, in
either scope. The two scopes agree.
- **Labels and instruction counts** come from the drift classifier
  (`match_listings.classify`). It compares `main`'s committed digests and
  baseline with the new ones.
- **Causes** come from `match_function.py --json`, run in this tree and in a
  `main` checkout.
- **No function is labelled `source-changed`.** Every body is verbatim.
- **All 50 moved functions keep identical listings.**

| Address | Function | File | Score (%) | Instructions | Label | Cause |
| --- | --- | --- | --- | --- | --- | --- |
| `0x00411360` | `menu_mouse_coordinate_update` | `frontend.cpp` (legacy) | 21.12 = | 314 → 314 | `drift-reordered` | independent `fe_legacy_dword` calls are rescheduled (as in finding 543) |
| `0x00482620` | `mesh_segment_vertex_fill` | `gods98/record.cpp` (legacy) | **31.30 → 26.09** | 57 → 57 | `drift` | `ecx`/`edx` swap in the index computation (`mov`/`shl`/`sub` and the `[eax + 8]` load). The normalized listing digest is unchanged, because registers are masked. |

- **Expected drift.** Both changes are in legacy files that lost code and
  gained private-header includes. ADR 0006 rule 2 and finding 541 accept this.
  No pads are added (rule 4).
- **The drop.** `0x00482620` is the only drop. It is a pure register
  allocation change with the same instruction count. **No instruction count
  changes in this PR.**

## The check #506 was made for: earlier units unchanged

| Check | Result |
| --- | --- |
| Earlier unit files (63 `unit` entries in `main`'s link-order list) edited | **none** |
| Earlier private headers (200 `*_for_*`, `*_from_*`, `*_shared.hpp`) edited | **none**. The 10 headers written are all new files. |
| Listing digests of the 970 reported functions in those files | **0 changed** (game-only and full) |

So **it held**. Finding 541's operand-label flip on `0x004078c0`
(`bubbles.cpp`) does not occur on this base. It did occur in this PR's first
run, on `8d548015`.

## Gates

- **Native build:** `cmake -S . -B build && cmake --build build && ctest
  --test-dir build --output-on-failure` passes 36/36 after both fix-ups.
  - The relocation commit alone does not compile (fix-up 1).
  - With fix-up 1 only, `reward.cpp` does not link (fix-up 2).
- **`make tools-test`** passes after `binary_progress_hash.py
  --write-manifest`. The lint ratchet holds at 4,139 existing violations, with
  no new ones.
- **`make reccmp-check`** in the developer distrobox reports the one drop
  above.
- **`make match-progress`** then refreshes all three generated files, with
  `pefile` from a worktree-local `build-venv/`. `pr_match_progress.py check`
  reports them fresh.
- **`make unit-data-offsets`:** 0 / 2 / 19 / 204.

## What remains

- **GAME-021** needs the anonymous-namespace forward declaration in
  `relocate_unit.py` (#514).
- **#514** should also fix both fix-up cases, so later relocations from
  migrated files and of exported const objects need no hand edits.
