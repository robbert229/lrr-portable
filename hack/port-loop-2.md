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

- Full report: **97.32% implemented**, **44.91% accuracy**, **43.70% progress**
  (2,326 compared entries out of 2,390).
- Game-only report: **98.87% implemented**, **39.62% accuracy**,
  **39.17% progress** (2,107/2,131).
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
  At the previous checkpoint Reccmp was **97.32% / 44.63% / 43.43%** full and
  **98.87% / 39.30% / 38.86%** game-only. `0x460f00` rose 10.37% → 13.20%,
  `0x404fc0` rose 7.33% → 7.39%, and `0x459900` rose 14.81% → 15.84%.
  `0x461790` fell 6.92% → 4.89% and `0x4599c0` fell 24.19% → 17.19% after
  live fallback/state work; inspect those diffs before compiler-oriented
  changes. FLIC presentation still needs a renderer surface/blit contract.
- `distrobox enter developer -- make reccmp-report` built the full program
  and generated both reports. `git diff --check` passed. Tests were not run.
- The 2026-11-01 startup/CRT refinement pass completed the WinMain command
  parser from Finding 398 and wired `_startlevel` / `_nosound` into the
  application startup. `winmain_parse_options` (`0x477bf0`) improved from
  27.03% to 79.91%; `WinMain` is 28.49% and `main_loop_frame_pump` is 15.15%.
  Removing the redundant shift mask under the proven `position < 32` bound
  brought both `pop_count32` (`0x47d700`) and `bit_count32_by_position`
  (`0x47e7e0`) to 100%. The latter now has the reference `__stdcall` ABI.
  The combined report is **97.32% / 44.71% / 43.51%** full and
  **98.87% / 39.39% / 38.95%** game-only. No score regression appeared in
  the compared report rows. `git diff --check` and the Reccmp build passed;
  tests were not run. The parsed programmer mode is recorded but has no
  reconstructed runtime consumer yet.
- A follow-up aligned `window_runtime_display_caps_query` (`0x477f80`) with
  the reference's direct `GetDC` and ignored `GetDeviceCaps(..., 12)` calls,
  using the platform's existing `0x00505800` window global. Its score rose
  from 14.81% to 100%*. The latest report is **97.32% / 44.74% / 43.55%**
  full and **98.87% / 39.43% / 38.99%** game-only; the full report diff
  changed only this target. `git diff --check` and Reccmp passed; tests were
  not run.
- The next loop refinement aligned `renderer_begin_frame_once_retail`
  (`0x477fa0`) to read the engine feature word directly, call the initialized
  Direct3DRM device's `+0x38` method, then set the once-per-frame bit. Its
  score rose from 9.52% to 90.00%. The adjacent surface bitmap converter rose
  29.17% → 29.34% in the same build; no score regressed. The latest report is
  **97.32% / 44.78% / 43.58%** full and **98.87% / 39.47% / 39.03%**
  game-only. The mapped body has no direct production caller in this tree;
  `main_loop_frame_pump` still uses the generic portable begin-frame helper,
  so runtime frame-loop parity remains incomplete. `git diff --check` and
  Reccmp passed; tests were not run.
- `window_runtime_adjust_rect` (`0x478310`) now reads the engine fullscreen
  bit and saved window style directly and calls `AdjustWindowRect` with the
  reference's no-menu argument. Its score rose from 27.59% to 100%. The latest
  report is **97.32% / 44.81% / 43.61%** full and
  **98.87% / 39.51% / 39.06%** game-only; no other target score changed.
  `git diff --check` and Reccmp passed; tests were not run.
- The next frame-pump pass moved the `0x478040` annotation from the unrelated
  three-argument platform event converter onto a no-argument Win32 message
  dispatcher. It also split out `window_input_state_poll` at `0x47eef0` and
  changed `main_loop_frame_pump` to call both helpers, the retail begin-frame
  body, and the retail present path. The pump improved from 20.41% to 60.47%;
  message dispatch is 100%, but input polling is only 10.81% and needs further
  ABI/state alignment. `render_surface_present` (`0x47c8d0`) is 100% and its
  windowed blit helper (`0x47cc50`) is 88.64%. The latest report is
  **97.32% / 44.91% / 43.70%** full and **98.87% / 39.62% / 39.17%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

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
