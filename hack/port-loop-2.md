# LegoRR port handoff

## Objective

Continue implementing the remaining placeholder functions in `lrr-portable`.
Use the checked-in decompile and findings to reproduce behavior, then connect
the implementation to the live runtime where the necessary state exists. A
function marked `ported` or routed through a helper is not sufficient evidence
that its target is implemented. Prefer complete behavior over an inert callback
adapter, while keeping unsupported engine boundaries explicit.

The primary evidence sources are `docs/decompiles/full/LEGORR.full.c`,
`docs/findings/`, `docs/coverage/ledger.csv`, and the current Reccmp output.
Preserve the user's existing worktree changes. C/C++ must remain compatible
with MSVC 5.0. The configured toolchain is available with
`distrobox enter developer`.

## Current verified baseline — 2026-11-01

The latest combined report is `build-reccmp/reccmp-report/index.html` and
`results.json`:

- Full report: **97.32% implemented**, **44.63% accuracy**, **43.43% progress**
  (2,324 compared entries out of 2,388).
- Game-only report: **98.87% implemented**, **39.31% accuracy**,
  **38.86% progress** (2,105/2,129).
- The coverage ledger has 2,564 rows: 2,285 `ported`, 254
  `runtime-library`, 17 `runtime-import`, and 8 across `anchored`, `anonymous`,
  and `named`. These ledger states do not guarantee high match scores or live
  runtime wiring; inspect the actual body and Reccmp row.
- The last three-agent batch improved `DDiSceneContextGet` (22.22% → 25.64%),
  `DDiSceneAnimationCallbacksSet` (13.73% → 17.98%), and
  `EntityActivityTick` (9.09% → 19.35%). Several decompile-driven changes had
  small match-rate regressions; review semantics and callers before deciding
  whether compiler-oriented adjustments are warranted.
- The following batch improved `PointerCursorConfigLoad` (8.44% → 16.09%),
  `CellPathRecordRegisterApply` (8.62% → 26.01%), and
  `ComInterfaceQueryAndContextDispatch` (13.02% → 14.04%).
  `PointerCursorDraw` dropped from 10.21% to 5.50% after moving to the
  decompiled two-argument target contract and adding a live software draw path;
  the WinAPI surface backend still cannot lock a draw target. Reward polling,
  cursor selection, and cursor drawing now share the media-owned override
  state (`0x00500a98/0x00500efc`), and `PointerCursorOverrideTick` improved
  from 55.56% to 88.89%.
- The text/WAD/terrain batch made the atlas-backed text renderer reachable,
  corrected the WAD canonical-code and reverse-bit function mappings at
  `0x49cf90/0x49d000`, fixed the type-2 decoder's returned source pointer,
  and routed path-record refresh through the shared current-grid neighbor
  walker. The report now recognizes the corrected WAD target and has 2,388
  total entries. Text-buffer native-width font sidecars add required x64
  ownership but lower the `TextBufferCreate` and `TextBufferDestroy` match
  scores; retain them unless a better pointer-width-safe model is found.
- The latest command/reward/HUD batch wired kind-3 placement relation checks,
  corrected kind-10 output gating, added reward-screen bitmap/text/stats
  fallbacks, and connected the five hunger images plus offsets to panel state.
  Reccmp remains **97.32% / 44.63% / 43.43%** full and
  **98.87% / 39.30% / 38.86%** game-only. `0x460f00` rose 10.37% → 13.20%,
  `0x404fc0` rose 7.33% → 7.39%, and `0x459900` rose 14.81% → 15.84%.
  `0x461790` fell 6.92% → 4.89% and `0x4599c0` fell 24.19% → 17.19% after
  live fallback/state work; inspect those diffs before compiler-oriented
  changes. FLIC presentation still needs a renderer surface/blit contract.
- `distrobox enter developer -- make reccmp-report` built the full program
  and generated both reports. `git diff --check` passed. Tests were not run.

## Port loop

1. Start from the current worktree and report. Check `git status` and inspect
   scoped diffs before editing; never reset or overwrite unrelated work.
2. Select a coherent function cluster with checked-in decompile or finding
   evidence. Trace callers, data ownership, and runtime installers. Prefer a
   cluster that can be made reachable end-to-end over isolated low-match edits.
3. Implement the target behavior and its live connection when the owning state
   exists. Keep target ABI/signature and annotation on the real body. Do not
   report a port complete when it only adds a wrapper or an uninstalled hook.
4. Keep agent assignments disjoint by module and address range. Agents must
   not run the shared Reccmp build while other edits are active.
5. Run `git diff --check`, then validate the combined result with
   `distrobox enter developer -- make reccmp-report`.
6. Compare every changed target and scan the complete report for regressions.
   Keep evidence-backed behavior even when code generation shifts the score;
   investigate regressions rather than changing semantics to chase a number.
   Update this handoff with the new report totals and genuine blockers.

Do not run tests unless the user asks. Report validation results and remaining
runtime or ABI gaps plainly. Never commit, push, or open a PR unless requested.

## Current architectural barriers to target

Some partial bodies depend on renderer-owned objects and tables that have no
production adapter in the portable runtime. Before adding another hook, trace
whether an existing scene, surface, texture, entity, command, or UI API can
perform the operation and bind it to a real caller. If not, document the
specific missing owner/state contract and move to another evidenced cluster.

Current examples from the latest report include low-scoring entity terrain
and movement helpers, scene resource and hierarchy lifecycle functions,
frontend display/config functions, and LWO/FLIC loading. Their scores are a
triage aid, not proof that behavior is absent: inspect the implementation and
call graph first. The report's `ledger_stub_*` names also do not by themselves
mean the implementation body is empty.
