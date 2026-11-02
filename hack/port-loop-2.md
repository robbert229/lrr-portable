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

- Full report: **97.32% implemented**, **45.17% accuracy**, **43.96% progress**
  (2,326 compared entries out of 2,390).
- Game-only report: **98.87% implemented**, **39.91% accuracy**,
  **39.46% progress** (2,107/2,131).
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
- A follow-up moved `InputStatePoll` (`0x47eef0`) from a two-argument
  frameloop adapter to a no-argument body in `input.cpp`. It now reads the
  fullscreen and application-active gates, clears current/previous snapshots
  when inactive, copies and clears snapshots before polling, and preserves the
  acquire retry path. The target improved from 10.81% to 31.45%; frame-pump
  dispatch remains aligned at 100% for `0x478040`, and `main_loop_frame_pump`
  remains 60.47%. The DirectInput device calls still use the existing callback
  seam because this runtime has no installed DirectInput device object.
  Compared with the preceding report, scores also shifted at `0x411360`
  (21.03% → 21.12%), `0x432e90` (15.00% → 39.02%), `0x4388b0`
  (14.49% → 18.12%), `0x444680` (6.39% → 6.74%), `0x444fb0`
  (26.04% → 25.56%), `0x4512d0` (9.48% → 7.58%), `0x47d520`
  (33.82% → 32.65%), `0x47d800` (28.74% → 29.17%), `0x47e190`
  (18.33% → 19.12%), and `0x4882e0` (12.50% → 12.48%). Those bodies were
  not edited in the input polling pass. A repeated build produced identical
  per-function scores, so the deltas are repeatable; their source-level cause
  has not been isolated. Latest totals at that checkpoint were
  **97.32% / 44.93% / 43.72%** full and **98.87% / 39.64% / 39.19%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.
- `WindowStateInitialize` (`0x47c170`) now has a direct mapped body that resets
  the reference's renderer/window globals in the decompiled order. The
  portable `WindowRuntimeState` sidecar is reset by the separate wrapper. The
  target rose from 10.53% to 100%. Compared with the preceding report, the
  other changed rows were `0x432170` (39.02% → 19.51%), `0x4322d0`
  (18.60% → 37.21%), `0x4388b0` (18.12% → 14.49%), `0x459470`
  (21.20% → 12.62%), `0x479630` (17.37% → 17.31%), `0x47e190`
  (19.12% → 19.92%), `0x4882e0` (12.48% → 11.01%), and `0x48c690`
  (10.57% → 7.11%). Those bodies were not directly edited in this pass.
  A repeat `make reccmp-report` produced identical scores for all 2,326
  compared functions. Latest totals are **97.32% / 44.96% / 43.75%** full
  and **98.87% / 39.67% / 39.23%** game-only. Build and
  `git diff --check` passed; tests were not run.
- The Win32 window-procedure ports now have separate retail-shaped
  `__stdcall` bodies that operate on the reference mouse/application globals;
  the cdecl runtime adapters mirror those values into `WindowRuntimeState`.
  The WinAPI bodies call `DefWindowProcA`, `SetCapture`, and `ReleaseCapture`
  directly. `GameWindowProcedure` (`0x4784c0`) rose 17.25% → 23.71%, and
  `MainWindowActivationAndCloseHandler` (`0x4786c0`) rose 20.00% → 24.31%.
  Other changed rows from the prior checkpoint were `0x432ec0` (12.00% →
  33.96%), `0x444680` (6.74% → 6.39%), `0x444fb0` (25.56% → 26.04%),
  `0x44fb90` (22.89% → 22.40%), `0x4512d0` (7.58% → 9.48%), `0x4882e0`
  (11.01% → 14.63%), `0x489c30` (32.40% → 32.30%), and `0x48c690`
  (7.11% → 10.57%). Their bodies were not edited in this window-procedure
  pass. The latest report is **97.32% / 44.97% / 43.77%** full and
  **98.87% / 39.69% / 39.25%** game-only; `make reccmp-report` and
  `git diff --check` passed. Tests were not run.
- `PointerOverrideExpiryTick` (`0x462700`) was mapped to the wrong reward-screen
  helper. Its annotation now sits on the actual modal frame sequence in
  `frameloop.cpp`: expire the media cursor override, select and draw the cursor,
  detect the global transition, clear the mouse word, tick sound and timed
  effects, then pump the frame. Its score rose from 4.59% to 93.94%. Keeping
  the scale factors as float literals preserves the reference's separate x87
  multiplications. The modal
  ledger adapter now calls that body. Cursor client coordinates and the renderer
  words at `0x76ab24/0x76ab28` now share one storage location; this removed the
  duplicate global annotations and raised their accessor rows (`0x410b30` and
  `0x410b40`) from 50% to 100%. At that checkpoint the report was
  **97.32% / 45.04% / 43.84%** full and **98.87% / 39.77% / 39.32%** game-only.
  A full rebuild also changed
  unrelated rows: `0x432e90` fell 39.02% → 15.00%, `0x444fb0` 26.04% → 25.56%,
  `0x4512d0` 9.48% → 7.58%, `0x47d800` 29.17% → 28.74%, `0x47e190` 19.92% →
  18.33%, and `0x4882e0` 14.63% → 12.11%. Their sources were not edited in this
  pass; investigate before attributing these codegen/report shifts. The
  developer-distrobox Reccmp build and `git diff --check` passed; tests were not
  run. Reccmp still reports the existing duplicate `0x505f88` global annotation
  in `platform/winapi/entry.cpp`.
- `CursorPositionUpdate` (`0x47f010`) now uses the target's window global
  directly, leaves the `GetCursorPos` output uninitialized before the API
  fills it, and clamps against the reference dimension globals
  `DAT_00505924/28`. The runtime wrapper initializes those globals from the
  platform client size, and the WinAPI `WM_SIZE` path refreshes them on resize.
  Client coordinates now share the renderer's `DAT_0076ab24/28` storage, so
  cursor sampling, input snapshots, and modal cursor drawing use the same live
  coordinates. The target rose from 44.86% to 98.08%; the remaining Reccmp
  difference is a single stack initialization. Latest totals are
  **97.32% / 45.07% / 43.87%** full and **98.87% / 39.80% / 39.35%** game-only.
  Other changed rows from the preceding report include `0x444680` (6.74% →
  6.39%), `0x444fb0` (25.56% → 26.04%), `0x4512d0` (7.58% → 8.53%),
  `0x459470` (12.62% → 21.20%), `0x47d800` (28.74% → 29.34%), `0x47e190`
  (18.33% → 19.12%), and `0x48c690` (10.57% → 7.11%). Their bodies were not
  edited in this pass; the `0x48c690` regression and the earlier
  `0x432e90` drop to 15% remain to be investigated. The developer-distrobox
  Reccmp build and `git diff --check` passed; tests were not run.
- `ActivityNameTableInit` (`0x436c20`) now retains the reference's writes to
  its activity, ability, tool-name, and activity-slot tables. I mapped the 93
  referenced name strings to their decompile addresses and used volatile
  lvalues for the initializer writes, leaving ordinary lookup reads unchanged.
  Its score rose from **1.63% to 82.45%**. The remaining difference is the
  reference's two broad contiguous clears versus the port's owner-specific
  sidecar and pool cleanup calls. Reccmp completed in the developer distrobox
  and `git diff --check` passed; tests were not run.
- `MapCellTrackingFlagsClearAndRefresh` (`0x4302b0`) now uses the current-level
  global and retail voxel-record addressing, then dispatches the activity,
  coordinate, and FLIC callbacks in reference order. Its score rose from
  **3.85% to 40.00%**. The newly mapped no-argument
  `EntityActivitySceneStateUpdate` (`0x439990`) and
  `GlobalObjectField98Clear` (`0x41f8c0`) each score **100%**. Latest report:
  **97.32% / 45.17% / 43.96%** full and **98.87% / 39.91% / 39.46%** game-only.
  The related `0x4322d0` score is down to **18.60%** and needs follow-up.
  Reccmp completed in the developer distrobox and `git diff --check` passed;
  tests were not run.

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
