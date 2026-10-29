# Port-loop-2 closeout report

Final state of the Reccmp match-improvement loop started from
`hack/port-loop-2.md`. All work is uncommitted on top of `30ff059f`
(24 modified files); PR #367 remains open for human merge — nothing was
merged by the loop, per the workflow rule.

## Headline numbers

| Metric | Session start | Final |
|---|---|---|
| Exact-matched nonstub functions | 155 | **217** |
| Accuracy | 37.98% | **42.38%** |
| Implemented | — | 51.52% (1253/2432) |
| Progress | — | 21.84% |

Verification gates at closeout (all green on the final tree):

- `make reccmp-report` (wine-ge-8-26 + reccmp) — report above
- `reccmp-datacmp --target LEGORR --all` — **252 variables, 3 documented
  issues** (unchanged baseline set from `docs/coverage/blocked.md`)
- `git diff --check` — clean
- Native meson tests: **not runnable on this host** (SDL3 dev package
  removed, no sudo; steady state all session). The winapi32 reccmp build
  compiles every change, which is the effective compile gate.

## Batches

74 verified batches (b1–b74). Every batch ended with: each target at
exact or safest-achievable state, **zero nonstub regressions** vs the
fresh `/tmp` anchor, datacmp steady, diff-check clean. Highlights of the
recent window (b60–b74):

- **b71**: `0x401420` vec_dist 44.9→**100** (naked-asm, 10th class win);
  `0x474ff0` mesh-color-set →85.7; `0x485a00` state-pair-get →76.2;
  `0x406760` vector2-set-length →75.0.
- **b72**: `0x492068` double-exponent-bits 46.2→**100** (naked-asm);
  `0x49b4c0` env-entry-find →80.4; `0x4634b0` overlay-vis →78.9;
  `0x411790` menu-pick →75.5 (corpus-verified semantics change).
- **b73**: `0x479530` triangle-normal 43.3→**98.8** (naked-asm; residual
  is a scorer-side function-size table artifact in `reprobit.toml` —
  scoring data, deliberately untouched); `0x45a6e0` indexed-pair-set
  →76.2 (RequestRecord consolidation); `0x47fed0` file-handle →76.0.
- **b74**: `0x403af0` task-state-update →**74.1**; `0x40ff70`
  vector3-set →**83.3** (corpus blind-deref + per-store reloads, zero
  production callers); `0x402e40` type13 →48.9.

## Winning change classes (measured, reusable)

1. **Naked-asm x87/data recovery** — 12 EXACT-class wins (vec_dist,
   double-exponent-bits, float96 shifts, dispatch slots). Only for tight
   self-contained bodies; must be corpus byte-verified line-for-line.
2. **Callee-signature conversion** (e.g. `cmd_produce` pair-pointer).
3. **Helper-CALL beats inlining** (4x confirmed: `0x483240`, `0x493ca0`,
   `0x471330`, staging-helper routes) — and the inverse for pure getters:
   **per-branch twin re-calls** (`0x402960`, `0x41b680`).
4. **Direct typed loads** via native-width `__builtin_memcpy` where the
   corpus loads slots directly (`0x408670` EXACT idiom).
5. **Unsigned-vs-signed gates** — only when nonnegativity is provable.
6. **Store-schedule reorder / failure-first inversion / lazy operand
   loads**; **wait-prefix**, **plain-assign RMW**.
7. **Corpus-faithful behavior changes** accepted only when proven against
   the corpus AND scored (`0x48f1d0` DBCS, `0x49b4c0` blind root deref,
   `0x40ff70`/`0x403af0` guard removals with zero callers).

## Measured vetoes (things that look right but scored WORSE)

- Split-if rewrites of fused `||` (3x); do/while bottom-test reshape.
- Pointer-walk replacing indexed loops — 3-for-4 regressions even when
  the corpus steps the pointer inside the loop body (`0x407540` walk
  −9.7, batch 75; VC5 still generated better indexed code). Follow
  corpus addressing semantics, keep the indexed form unless measured.
- Count-increment-before-store did not transfer positional→alias
  (`0x464fa0` −5.2 despite `0x465090` already holding that shape).
- Direct-store expansion of init/table functions (`0x485a20` 43.5→6.5).
- Staging-helper → inline stores is a per-function lottery (`0x402fb0`
  won +18, `0x402510` lost −2.4, `0x402e40` won): never both siblings.
- Unsigned-countdown replacing signed-toward-zero (`0x43a910` −13.8).
- Full-width sign fold vs byte fold (`0x4979e0`); branch-local loads
  unless dossier proves in-branch load (`0x490540`); validated idioms do
  not transfer between siblings (`0x4975d0`).

## Remaining barriers (needs human/runtime-design decisions)

**Portable-layout (compact-struct) class** — port structs differ from
corpus offsets; matching requires adopting corpus layouts (runtime
design). Never respell displacements:

- **EntitySlot**: corpus +0x2f0/+0x304/+0x308/+0x324/+0x334/+0x340/+0x348/+0x3e0/+0x3e4/+0x3e8/+0x3ec/+0x408 vs port +0xbc/+0xc0/+0xc4/+0xfc/+0x100/+0x104/+0x108/+0x110/+0x114/+0x204.
  Blocks `0x443670`, `0x469da0`, `0x43bbc0`, `0x4377b0`/`0x4377d0`,
  `0x41f720` (+0x3f0), many queue/registry functions.
- **CmdRec**: +0x3c-vs-+0x64 link, `+0x50 global_next` → blocks
  `0x4041e0` chain-remove, `0x406400`.
- **EngineState/EngineContext vs global roots**: `DAT_00556ed0` debug
  word (`0x426360`), `DAT_005560d4`, `camera_state_root`,
  `ctx->state_block` (`0x46a730`, `0x4538a0`, `0x462800`),
  `objective_score_text_origin_set` family.
- **SavListWindow header layout** (+0x10/+0x0c vs +0x0c/+0x08) →
  `0x417020`/`0x416f40` siblings.
- **LevelState** wide fields (+0x73d0 etc.) vs module statics →
  `0x450a80`; terrain global head `0x4b8a24` → `0x4090a0`/`0x4090c0`.
- **simtick link layout** (`[eax+0x24]` vs `[eax]`) → `0x4091e0`,
  `0x47aba0` (coupled bobbin_drop stub).

**Import/API class**: Win32 fd/stream/locale functions (`0x4908a0`,
`0x494d10`, `0x4990a0`, `0x499240`, `0x4962f0`, `0x496190`, `0x490280`),
dsound family (`0x464f60`, `0x464fa0`, `0x465090`, music mixer), file
mount path (`0x47f640`). Inlined `rep movsd/movsb/stosd` shapes
(portable C cannot force rep strings from VC5).

**Unobservable data class**: reference `.rdata` string/table contents
(`0x41a300` menu labels, keyboard names beyond table-walk shape,
`0x419430` message tab texts).

**Scorer-data class**: `0x479530` (98.8%) and possibly other near-exact
tails blocked by `reprobit.toml` original-size entries; fixing requires
editing reccmp scoring data — deliberately out of loop scope.

**Test-vs-corpus discrepancy (needs review)**: `0x4041e0`
`entity_command_chain_head_remove` tests state bit `0x10000` where the
corpus tests `test ch,0x20` (bit `0x2000`); pinned by
`test_cmd.cpp:1533–1542`. Changing needs test-owner decision.

## Notes for the human

- Commit the 24-file working tree when reviewed (HEAD `30ff059f`).
- PR #367: open, **not merged** — merge is yours.
- `s_bank_sound_on` default and `global_config_value_set` /
  `deferred_callback_set` untouched per handoff constraints.
- Batch anchors kept in `/tmp/reccmp-baseline-batchNN-*.json`; residual
  dossiers in `/tmp/opencode/residual-0x*.txt`; corpus dump helper
  `/tmp/opencode/pe_dump.py`.

## Batch 75: per-candidate barrier table (final closeout batch)

Every remaining 38–41% candidate got an instruction-level audit against
`reference/LegoRR.exe` (all corpus citations primary-verified from PE
bytes). Verdicts: **WIN** scored improvement kept, **REV** attempted and
reverted on measurement, **CEIL** barrier with cited residual evidence.
Anchor `/tmp/reccmp-baseline-batch75-20261029-005456.json`; batch run
Accuracy 42.38→42.40, 217 exact, datacmp 252/3, `git diff --check` clean.

| Address | Function | Verdict | Evidence / barrier class |
|---|---|---|---|
| `0x419430` | interface_message_tab_info_text_load | WIN 38.7→62.5 | Callee swap to annotated twin `image_surface_load` `0x47d9d0` + kept `resource_record_add_ref` (`0x47dbc0`); cache-bypass caveat audited: zero in-tree callers |
| `0x42cd50` | upgrade_name_entry_find | WIN 38.8→40.8 | Unsigned count/index gates (`0x42cd5b jbe`, `0x42cd7f jb`) + comparator swapped to annotated twin `strcasecmp_lite` `0x48ff10` (equality-only caller); install clamp verified |
| `0x464fa0` | sound_alias_play | REV → 39.5 kept | Unsigned gate + increment-before-store regressed −5.2; reverted to anchor text. Residual ceiling: DirectSound instance COM seam `0x47ad70` vs `s_play_seam` |
| `0x407540` | flagged_object_timer_registry_insert | REV → 40.0 kept | Pointer-walk (corpus steps in loop body `0x407581 add eax,8`) regressed −9.7; reverted. Residual: register-role/indexed codegen |
| `0x464f60` | sound_request_queue_submit | neutral kept | Gate unsigned per `0x464f68 jae`; 0.0000 delta, value-identical (count provably nonneg), kept |
| `0x465090` | sound_positional_sound_submit | neutral kept | Gate unsigned per `0x4650a4 jae`; 0.0000 delta, kept. Residual ceiling: same `0x47ad70` COM seam |
| `0x4652d0` | sound_batch_queue_drain | neutral kept | Unsigned drain bounds per `0x465309 jbe`/`0x46535a jb`; 0.0000 delta, kept |
| `0x47aad0` | sound_bank_free_slot_find | CEIL | Portable-layout: corpus stride `0x134` (`0x47aadc`) + end `0x5339a8` vs port `BankSlot` `0x110`; null guard pinned `test_sound.cpp:572` |
| `0x478980` | music_mixer_volume_set | CEIL | Import/API: corpus calls `WindowsMixerControlInit` `0x4789c0`; portable audio path does not model the Win32 mixer band |
| `0x47aba0` | simtick_sound_keepalive | CEIL | Vtable callee `[eax+0x68]`/`[edx+0x60]` + unobservable `.data`; gates pinned `test_simtick.cpp:208–216` |
| `0x4091e0` | simtick_bobbins | CEIL | Portable-layout: `[esi+0xd0]` flags / `[esi+0x24]` next vs compact `SimtickBobbin` (`simtick.hpp:155–159`); `0x4099f0` stub boundary; pinned `test_simtick.cpp:103–113` |
| `0x406400` | cmd_pending_record_copy | CEIL | CmdRec frozen: `+0x64` save/restore around `rep movsd` 26 dwords (`0x40640e–0x406424`); pinned `test_cmd.cpp:1707–1724` |
| `0x402f30` | entity_threshold_progress_event_queue | CEIL | CmdRec displacement: `mov [eax+0x3c],0x10` (`0x402f47`) vs port `[eax+0x68]`; pinned `test_cmd.cpp:805–822` |
| `0x41f9a0` | level_resource_balance_add_and_sound | CEIL | Global-vs-param: corpus loads level from `DAT_005560d4`, port via `s_ctx [+4]`; no-level path pinned `test_cmd.cpp:1885–1925` |
| `0x462960` | task_table_contents_release | CEIL | Portable-layout: corpus double-deref `[edi]`→count `[eax+0x1c]`/entries `[eax+0x20]` vs port direct-record; fixture `test_cmd.cpp:1784–1805` |
| `0x4377d0` | entity_pool_find_by_callback | CEIL | EntitySlot self-link `+0x408` (`0x437809` verified), flags `+0x3e8`, pool stride `0x40c` vs `0x208`, pool globals `0x4dee10/0x4de790` |
| `0x452870` | selection_candidate_eligibility_check | CEIL | EntitySlot `+0x3e8/+0x3e4/+0x3e0` vs `+0xc4/+0xc8/+0xc0`; shared-eligibility twin already called; guard pinned `test_select.cpp:156` |
| `0x431140` | terrain_cell_queued_work_transition | CEIL | Global-head/LevelState: corpus no-loop indexed cell calc off global `0x5560d4` + `[+0x14]/[+0xb8]`; `0x42f6f0` arity mismatch vs port annotation |
| `0x41ca60` | input_entry_flag8_get | CEIL | Pinned guard: corpus blind `[eax*4+0x4dd60c]` load + `and 8` (`0x41ca64` verified); port guard pinned `test_input.cpp:182` |
| `0x44c4a0` | selection_clear_marker_and_reset_hud | CEIL | EntitySlot `+0x3ec/+0x36c` vs `+0xcc/+0x100` + `DAT_00556ecc` global-vs-param; NULL guard pinned `test_select.cpp:216–218` |
| `0x459050` | objective_check | CEIL | Threaded-LevelState runtime-design: objective bits from global `0x4ffbc0` vs port `[edx+0x374]`; `0x4377b0` callback `0x459100` family |
| `0x435a50` | camera_registry_probe | CEIL | Global-vs-param: global `DAT_00556ecc`/`0x556ed0` latch vs port `[esi+8]/[esi+0xc]` |
| `0x47f640` | mount_wad_from_path | CEIL | Import/API file-mount-path class; residual is port-side `resource_unmount_all` + NULL guard (corpus has neither) |

With this table every nonstub candidate outside the documented barrier
families has been through an instruction-level audit: the 38–41% band is
now barrier-classified per target, and the two winnable shapes in the
band were captured. The pick-four loop is closed.
