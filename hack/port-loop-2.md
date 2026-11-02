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

## Current verified baseline — 2026-11-02

The latest combined report is `build-reccmp/reccmp-report/index.html` and
`results.json`:

- Full report: **97.32% implemented**, **48.69% accuracy**, **47.39% progress**
  (2,328 compared entries out of 2,392).
- Game-only report: **98.87% implemented**, **43.80% accuracy**,
  **43.31% progress** (2,109/2,133).
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
  `GlobalObjectField98Clear` (`0x41f8c0`) each score **100%**. At that
  checkpoint the report was **97.32% / 45.17% / 43.96%** full and
  **98.87% / 39.91% / 39.46%** game-only.
  Reccmp completed in the developer distrobox and `git diff --check` passed;
  tests were not run.
- `DisplayResolutionFilterTupleAppend` (`0x49d870`) now performs volatile
  reloads of the reference's global tuple count before each field write and
  again for the final increment. This preserves the unchecked append behavior
  while matching the decompile's repeated global reads; its score rose from
  **14.29% to 84.00%**. In the adjacent
  `DisplayModeMatchesFilterTuple` (`0x49d8d0`), I removed a port-only mode-index
  bounds check that its sole caller already guarantees, and expressed the
  reference's 0x110-byte mode and 0x110-byte tuple walks directly; its score
  rose from **29.09% to 29.27%**. I also aligned
  `TerrainCellVisitedFlagTest` (`0x4322d0`) with the current-level global and
  direct retail voxel pointer/stride calculation, removing port-only
  null/bounds checks absent from the decompile. Its score rose from
  **18.60% to 76.92%**. Applying the same target-shaped current-level access
  to `TerrainCellFlag100Test` (`0x432170`) brought that getter from **19.51% to
  100%** in the earlier report; the latest combined build measures it at
  **76.92%**, with only width/Y load order differing in the disassembly. Its
  neighboring `TerrainCellSecondaryBit0Clear` (`0x4323c0`) had a
  signature mismatch: the annotation was on a three-argument explicit-level
  helper, while the decompile takes a coordinate-pair pointer and uses the
  active-level global. The target annotation now sits on a matching direct
  body; the explicit-level helper remains for its current caller. Its score
  rose from **9.09% to 76.92%**. The analogous
  `TerrainCellSecondaryBit0Set` (`0x4325d0`) had the same ABI mismatch and now
  has a current-level coordinate-pair body; its score rose from **9.09% to
  100%**. `TerrainCellFlag8SetOrClear` (`0x4324d0`) and
  `TerrainCellFlag8Test` (`0x432520`) had explicit-level mapping signatures in
  place of the retail current-level interfaces. Their new mapped bodies use
  `(x, y, set)` and `(x, y)` arguments respectively; scores rose from **17.65%
  to 77.55%** and **12.50% to 100%**. The setter now branches before loading
  level state, matching the decompile's set/clear paths. `TerrainCellOccupancyOrFlagCheck`
  (`0x432aa0`) now uses the target's `(x, y, mode)` signature and current-level
  state; matching its mode branch order and early global load raised it from
  **16.39% to 100%**. Latest scores are 100% (`0x432520`, `0x432aa0`) and
  76.92% (`0x432170`, `0x4322d0`, `0x4323c0`, `0x4325d0`); `0x4325d0` was
  previously 100% before this combined source rebuild and now differs only in
  width/Y load order. Volatile width/Y ordering experiments regressed
  individual getters to 41.03% and were reverted. Latest report at that point:
  **97.32% / 45.41% / 44.19%** full and **98.87% / 40.17% / 39.72%** game-only.
  Reccmp completed in the developer
  distrobox and `git diff --check` passed; tests were not run.
- `TerrainCellWalkableFlagCheck` (`0x432bd0`) had an explicit-level
  three-argument mapping while the decompile takes two unsigned coordinates
  and reads the active level. Its new target-shaped body preserves the unsigned
  `dimension - 1` bounds gates, traversable bit `0x10`, and blocked bit `0x8`.
  The score rose from **12.24% to 73.33%**. Splitting the width and height
  checks restored the decompile's short-circuit order. The latest report at
  that checkpoint was **97.32% / 45.44% / 44.22%** full and
  **98.87% / 40.21% / 39.75%** game-only.
- The adjacent terrain readers `TerrainCellFlag800TestInBounds` (`0x432b00`),
  `TerrainWalkableNeighborSearch` (`0x432b50`), and
  `TerrainCellFlag20TestInBounds` (`0x432d90`) had mappings on explicit-level
  helpers. Their mapped bodies now use the two-unsigned-coordinate retail
  interface, the active-level global, and the `0x48` voxel stride while keeping
  the prior explicit-level helpers for portable callers. Each rose from
  **42.31% to 69.23%**. The remaining diff is register selection and pointer
  addressing in the generated code; a volatile pointer-slot load did not
  change their scores. `TerrainCellTypeIs8Or10Or11` (`0x432b90`) now directly
  reads the active-level record byte and scores **100%**, up from **28.57%**.
  `TerrainCellFlag08000000Test` (`0x432dd0`) now takes the decompiled
  coordinate-pair pointer and reads the active-level flag word; it scores
  **100%**, up from **8.00%**. Two more current-level readers now match the
  retail ABI: `TerrainCellFlag40000Test` (`0x432c20`) takes `(x, y)` and
  `TerrainCellTypeSpeedClassGet` (`0x432c50`) takes a coordinate-pair pointer;
  both score **100%**, up from **11.76%** and **8.00%**. The normalized
  `TerrainCellFlagsMaskC0Test` (`0x432e60`) now reads the active-level record
  directly and scores **66.67%**, up from **22.22%**. The three bounded
  readers remain at **69.23%** each; their remaining mismatch is generated
  register selection and pointer addressing. Reccmp completed in the
  developer distrobox; `git diff --check` passed and tests were not run. Latest
  report at that stage: **97.32% / 45.62% / 44.40%** full and
  **98.87% / 40.41% / 39.96%** game-only.
- The adjacent terrain mutation and record predicates now use the active-level
  global and retail record layout. `TerrainCellPredugEdgeWallFlagSet`
  (`0x432e00`) rose from **8.70% to 83.33%** after separating its set and clear
  paths and using the target's in-place flag operations. The edge-wall reader
  (`0x432e90`) rose from **39.02% to 76.92%**. The state-byte predicates
  `TerrainCellStateEquals1` (`0x432ec0`) and `TerrainCellStateEquals6`
  (`0x432f00`) now score **100%** each. A test with a volatile flag pointer
  did not change the setter score and was reverted. Latest combined report:
  **97.32% / 45.74% / 44.51%** full and **98.87% / 40.54% / 40.08%**
  game-only. Reccmp completed in the developer distrobox;
  `git diff --check` passed; tests were not run.
- The next current-level terrain query batch added the retail two-coordinate
  `TerrainCellFlagsMask18ClearInBounds` body (`0x432f40`, **72.41%**, from
  17.02%), aligned the bounded flag-4 reader (`0x432f90`, **69.23%**, from
  35.09%), and made the unchecked flag-`0x2000`, flag-`0x80`, and secondary
  bit-0 reads direct active-level accesses (`0x432fd0`, `0x433000`,
  `0x433060`, all **100%**). The coordinate-pair flag-`0x8000` reader
  (`0x433030`) is **76.92%**, from 13.33%. Additional target-shaped paths
  improved the bounded occupied-coordinate read (`0x433090`, **47.46%**, from
  10.26%), the `0x180000` mask predicate (`0x4330e0`, **66.67%**, from
  11.76%), the flag-`0x10000000` setter (`0x433120`, **54.55%**, from
  24.14%), record-byte-3 reader (`0x4332c0`, **82.35%**, from 10.26%),
  bounded work-flag setter (`0x4332f0`, **39.02%**, from 17.86%), and
  cell-type out-parameter query (`0x433330`, **62.07%**, from 18.18%). The
  target paths use the retail active-level global and record stride on Win32;
  portable builds retain owner-resolving fallbacks where they existed.
  Reccmp completed in the developer distrobox; `git diff --check` passed and
  tests were not run. Latest report: **97.32% / 45.99% / 44.76%** full and
  **98.87% / 40.82% / 40.36%** game-only.
- `CellTypeSpeedMultiplierResolve` (`0x433180`) previously mapped to a
  six-argument explicit-level helper, while the decompile takes an entity,
  coordinate pair, scale, and optional output pointer, and reads the active
  level global. The mapping now sits on a five-argument current-level body with
  the decompiled excluded-type order, owner/stats gates, cell checks, and
  optional RockFallIn multiplier. Existing explicit-level callers delegate to
  this body when their level is active and retain the original adapter path for
  other levels. Its score rose from **16.54% to 41.57%**. The `0x433120`
  flag setter now scores **83.33%** (up from 24.14% in the earlier report).
  Reccmp completed in the developer distrobox; `git diff --check` passed and
  tests were not run. Latest report: **97.32% / 46.02% / 44.78%** full and
  **98.87% / 40.84% / 40.38%** game-only.
- `TerrainCellWorkFlagClear` (`0x430320`) now has a Win32 target path that
  reads the active-level voxel record directly, applies the descriptor and
  flag gates, clears the three decompiled bits, then performs the activity,
  coordinate callback, FLIC fallback, four queued events, and final flag set
  in reference order. This avoids routing the mapped entry through the
  entity-pool context and explicit-level adapter. The score rose from **9.30%
  to 12.61%**. A volatile pointer-slot experiment on the adjacent
  `0x4332f0` work-flag setter had no score effect and was reverted. Latest
  report: **97.32% / 46.02% / 44.79%** full and **98.87% / 40.85% / 40.39%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.
- `TerrainCellPrimaryFlagsMaskClearAndRefresh` (`0x432360`) and
  `TerrainCellFlagsSetAndRefresh` (`0x4323f0`) now have Win32 target paths
  that use the active-level voxel pointer, update the decompiled primary and
  secondary flag masks, and call `CellInitPass` with the target coordinates.
  Each rose from **6.45% to 47.06%**. The combined report also moved
  `0x4324d0` from 50.00% to 43.90% and `0x4512d0` from 8.53% to 7.58%; their
  bodies were not edited in this batch. A repeat build produced identical
  scores, so these code-generation/report shifts are stable but not yet
  explained. Latest report: **97.32% / 46.05% / 44.82%** full and
  **98.87% / 40.88% / 40.42%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.
- `TerrainNeighborFlagSet` (`0x430470`) now has a Win32 path with the retail
  nonzero-coordinate gate, unsigned `dimension - 2` bounds checks, direct
  voxel-record flag transition, and `CellInitPass`/staged-slot-reset order.
  Its score rose from **14.72% to 17.54%**. The path still uses the live
  per-level staged-record query in place of the reference's global
  `WorldSpatialRecordFindAtGridCoordinate` registry; the view-side registry
  has no production installer in this checkout. The same rebuild changed the
  unedited `TerrainCellNormalSample` (`0x4512d0`) from **7.58% to 9.48%**;
  a repeat report was identical. Overall totals remain **97.32% / 46.05% /
  44.82%** full and **98.87% / 40.88% / 40.42%** game-only. Reccmp and
  `git diff --check` passed; tests were not run.
- `TerrainCellVisitedFlagSetAndTrack` (`0x432270`) and
  `TerrainCellVisitedFlagsClearAll` (`0x432300`) now use direct active-level
  voxel access on Win32 and the existing retail-addressed coordinate/count
  globals. The set path preserves the unchecked 100-pair append; the clear
  path reads the list count on each loop check and resets it after clearing
  each visited bit. Scores rose from **8.16% to 69.57%** and **17.65% to
  88.89%**. The rebuild also moved the unedited `0x4512d0` from 9.48% to
  8.53%; a repeat produced identical target and aggregate results. Latest
  report: **97.32% / 46.11% / 44.88%** full and **98.87% / 40.95% / 40.49%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.
- `PendingTypeValueSlotSelect` (`0x407a10`) now reads the retail entity and
  command-record offsets directly on Win32 x86 and selects from the same
  contiguous 39-word bubble-handle table used by the live bubble config path.
  This replaced the target body's reconstructed field/sidecar lookups while
  preserving the portable fallback. Its score rose from **1.33% to 50.97%**.
  A follow-up correction restored the decompile's type-19 activity 6/7 slot
  mapping and removed a type-3 payload null gate absent in the retail body;
  this moved the interim score from 51.12% to 50.97%. No other target score
  changed in that rebuild. That checkpoint report was **97.32% / 46.14% /
  44.91%** full and **98.87% / 40.98% / 40.52%** game-only. Reccmp completed in the
  developer distrobox; `git diff --check` passed; tests were not run.
- `motion_jitter_step` (`0x44d760`) now runs the decompiled delay, randomized
  direction, bounded candidate, scene-position update, and remaining-time
  state machine against mapped retail globals. The bounds helper is now mapped
  at `0x44d9a0` and checks the three global center/tolerance pairs. Existing
  parameter storage and scene binding populate those globals; the editor path
  and jitter path share the retail flag word at `0x004eaec8`. Scores rose from
  **4.44% to 35.83%** and **16.39% to 24.00%**, respectively. The only changed
  report rows were those two targets. Latest report: **97.32% / 46.16% / 44.92%**
  full and **98.87% / 41.00% / 40.54%** game-only. The developer distrobox
  Reccmp build and `git diff --check` passed; tests were not run.
- `D3DRMVectorRandom` (`0x479500`) and `D3DRMVectorRotate` (`0x479510`) are
  thin wrappers around the bundled D3DRM imports in the reference, but the
  Reccmp build had been using local math implementations in those mapped
  bodies. The WinAPI target now calls the imported D3DRM functions directly;
  the local implementations remain the portable-backend fallback. Their
  scores rose from **2.38% to 66.67%** and **2.92% to 85.71%**. The build now
  links `d3drm.lib` for the WinAPI target. Latest report: **97.32% / 46.24% /
  45.00%** full and **98.87% / 41.09% / 40.63%** game-only. The developer
  distrobox Reccmp build and `git diff --check` passed; tests were not run.
- `RandomWaypointTimerStepApply` (`0x44d280`) now reads and updates the
  decompiled waypoint globals directly on Win32, including the shared phase
  flags at `0x004eaec8`; the portable path continues to use its state record.
  The live motion ticker copies the bound position into the retail global
  before the step and syncs the updated vectors and phase fields back after it.
  `WaypointVectorRandomScaleApply` (`0x44d710`) now has its annotation on the
  real helper body and uses the same retail target, scale, and velocity words.
  The timer rose from **5.65% to 15.28%**; the helper is **100%**. The report
  now includes one additional function. Latest report: **97.32% / 46.26% /
  45.03%** full and **98.87% / 41.12% / 40.66%** game-only. The developer
  distrobox Reccmp build and `git diff --check` passed; tests were not run.

- `MotionJitterParametersStore` (`0x44cef0`) now maps to a target-shaped body
  in `util.cpp`: it writes the six motion range globals in reference order,
  refreshes the scene position through the existing scene accessor, stores
  the three tolerance values, and sets the motion flag. Reconstruction-only
  light-effect parameter bookkeeping remains in the config adapter in
  `effects.cpp`, outside the mapped target. Its score rose from **5.33% to
  65.52%**. Latest totals are **97.32% / 46.29% / 45.05%** full and
  **98.87% / 41.15% / 40.69%** game-only. `distrobox enter developer -- make
  reccmp-report` completed and `git diff --check` passed; tests were not run.

- The four objective-arm targets now map to direct record/global write bodies;
  their config adapters separately preserve the per-level objective mask. The
  shared result word is annotated at its recovered address `0x4ffbc0`, which
  removed the final operand mismatches. `ObjectiveCrystalOrOreArm`
  (`0x458580`) rose **4.65% → 100%**, `ObjectiveBlockArm` (`0x4585c0`)
  **6.06% → 100%**, `ObjectiveTimerArm` (`0x4585f0`) **11.76% → 100%**, and
  `ObjectiveConstructionArm` (`0x458620`) **31.25% → 100%**. The timer target
  uses the reference float argument and x87 store path. Latest totals:
  **97.32% / 46.44% / 45.20%** full and **98.87% / 41.32% / 40.85%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.

- HUD health, hunger, and bubble configuration now share the mapped state word
  `0x4ffe68` instead of publishing a second copy through setter/getter calls.
  `legacy_health_bar_config_load` (`0x459560`) moved from **4.10% to 4.24%**.
  Reccmp and `git diff --check` passed; current totals are **97.32% / 46.43% /
  45.19%** full and **98.87% / 41.31% / 40.84%** game-only. Tests were not run.

- The save-slot selector now maps directly to retail global `DAT_005574d0`.
  This resolved the global operand in `SavSetSlot` (`0x417ea0`), raising its
  match from **66.67% to 100%**; `SavGetSlot` (`0x417e90`) and
  `SaveSlotSelectionClear` (`0x415cf0`) also report **100%**. The full report
  is **97.32% / 46.50% / 45.25%** and game-only is **98.87% / 41.38% /
  40.91%**. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `GlobalStateFlagsBits6And9Set` (`0x426360`) now applies the bit-6 update to
  the shared word before testing and applying the bit-9 argument, with the
  reference branch direction. Its match rose **66.67% → 100%**. The latest
  report is **97.32% / 46.51% / 45.27%** full and **98.87% / 41.39% /
  40.93%** game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- Six level-field arithmetic targets now use the retail ABI and access the
  active level global directly; explicit-level adapters remain for portable
  callers. `GlobalObjectField8CIncrement` (`0x41f870`),
  `GlobalObjectField8CSubtract` (`0x41f880`), `GlobalObjectField98Add`
  (`0x41f8a0`), `LevelStateField9CAdd` (`0x41f920`),
  `GlobalObjectField94Increment` (`0x41f980`), and
  `GlobalObjectFieldA8Increment` (`0x41f990`) now each match at **100%**,
  from **66.67%**. The latest report is **97.32% / 46.55% / 45.30%** full and
  **98.87% / 41.44% / 40.97%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `WadReverseLowBits` (`0x49d000`) now branches on each source low bit like the
  reference and matches at **100%** (previously **70%**). The FLIC triplet
  transform (`0x484ba0`) now maps to its unchecked 256-entry body and matches
  at **100%** (previously **69.84%**); the public adapter retains its null
  guard for existing portable callers. The report is **97.32% / 46.57% /
  45.33%** full and **98.87% / 41.46% / 41.00%** game-only.
  `distrobox enter developer -- make reccmp-report` and `git diff --check`
  passed; tests were not run.

- `SetSystemStateFlags` (`0x456550`) and
  `GlobalValue_004ff9bcStoreAndReturn` (`0x4567c0`) now map to unchecked
  decompile-shaped bodies and match at **100%**, up from **71.43%** each.
  Their public APIs retain the null guards used by existing portable callers.
  The latest report is **97.32% / 46.60% / 45.35%** full and
  **98.87% / 41.49% / 41.02%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `UiTextUnderscoresToSpaces` (`0x4102b0`) now uses the retail 256-byte length
  gate and unchecked target body, while the public adapter retains null
  handling; its score rose **72.73% → 100%**. `DDiSceneAdvanceByTime`
  (`0x475120`) now maps the unchecked target path and rose **73.33% → 100%**
  after expressing the intermediate current and next times as local floats.
  Current totals are **97.32% / 46.66% / 45.41%** full and
  **98.87% / 41.56% / 41.09%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `EntityBuildPlacementCommandPairCreate` (`0x439340`) now calls the normal
  and alternate four-argument setup entry points, matching the retail call
  sequence instead of invoking the shared five-argument adapter twice. Its
  match rose **68.18% → 100%**. The normal and alternate setup targets remain
  low at **17.65%** and **14.68%** because their portable implementations
  include substantial runtime fallback logic. At that checkpoint, totals were
  **97.32% / 46.67% / 45.42%** full and **98.87% / 41.57% / 41.10%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- A direct-expression codegen experiment on the adjacent bounded terrain flag
  accessors (`0x432b00`, `0x432b50`, `0x432d90`, `0x432f90`, and `0x432bd0`)
  produced no target-score changes; it was reverted. Their current matches are
  **69.23%** for the four mask getters and **73.33%** for the walkability
  getter. The remaining differences are register allocation and address
  formation, so further work should use a different evidence-backed approach.

- `TextBufferReset` (`0x4859b0`) now maps to an unchecked retail-body entry
  while the public API retains null-record and null-buffer handling. The x86
  body follows the observed reset instruction order, including the final flag
  store after restoring `esi`; its score rose **73.33% → 100%**. Latest
  totals at that checkpoint were **97.32% / 46.66% / 45.42%** full and
  **98.87% / 41.56% / 41.09%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `MapFileDimensionsGet` (`0x4334c0`) now advances the resolved image pointer
  to its `+0x08` header before reading width and height, matching the target's
  `add eax, 8` then `[eax+4]` sequence. Its match rose **76.19% → 100%**.
  Latest totals are **97.32% / 46.67% / 45.43%** full and
  **98.87% / 41.57% / 41.11%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `RequestRecordReadyFlag4` (`0x45ab30`) now has an unchecked target body with
  the retail 0x30-byte stride, immediate request-record base, and flag gates;
  the public API retains its 18-entry bounds check. Its match rose
  **73.33% → 100%**. Latest totals are **97.32% / 46.69% / 45.45%** full and
  **98.87% / 41.60% / 41.13%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `ObjectStatePairGet` (`0x485a00`) now maps to its unchecked two-load/two-store
  body, while the public helper retains its null-record guard. Its match rose
  **76.19% → 100%**. Latest totals are **97.32% / 46.71% / 45.46%** full and
  **98.87% / 41.61% / 41.14%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `ObjectField38Release` (`0x489d70`) now maps to an unchecked target body,
  retaining the portable API's null-record guard in its wrapper. Its match
  rose **72.73% → 100%**. Latest totals are **97.32% / 46.71% / 45.46%** full
  and **98.87% / 41.62% / 41.15%** game-only. `distrobox enter developer --
  make reccmp-report` and `git diff --check` passed; tests were not run.

- `RequestRecordReadyFlag2` (`0x45ab60`) now uses a retail-shaped target body
  with the unchecked 0x30-byte record stride and bit-2/bit-3 gates; its match
  rose **78.26% → 100%**. Latest totals are **97.32% / 46.72% / 45.47%** full
  and **98.87% / 41.62% / 41.15%** game-only. `distrobox enter developer --
  make reccmp-report` and `git diff --check` passed; tests were not run.

- `RequestRecordFlag8IsSet` (`0x45ab90`) now maps to the unchecked retail
  address calculation and bit-8 mask; the public API retains its 18-entry
  bounds check. Its match rose **50.00% → 100%**. Rounded totals remain
  **97.32% / 46.72% / 45.47%** full and **98.87% / 41.62% / 41.15%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `TerrainCellPredugEdgeWallFlagSet` (`0x432e00`) now maps to a target body
  with the retail set/clear paths, including the direct memory OR in the set
  branch. Its match rose **70.00% → 100%**. Latest totals are
  **97.32% / 46.75% / 45.50%** full and **98.87% / 41.66% / 41.19%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `UITextOutputObjectRelease` (`0x45a620`) now maps to the unchecked target
  body and calls `TextBufferReset`'s mapped target entry. The public API still
  guards a null holder or record. Its match rose **71.43% → 100%**. Latest
  totals are **97.32% / 46.78% / 45.53%** full and
  **98.87% / 41.69% / 41.22%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `TextBufferDestroy` (`0x485190`) now maps to a target body that only frees
  the `+0x14` buffer and then the record, matching the retail sequence. The
  public wrapper removes the widened-pointer font sidecar first and retains
  its null guard. Its match rose **75.86% → 100%**. The adjacent
  `EntityTypeFlagBit1Get` (`0x46d330`) and `SceneObjectFlagBit3Get`
  (`0x473ba0`) also now use unchecked target bodies that preserve the retail
  low-byte mask as a distinct x86 instruction; their public accessors remain
  null-safe, and both matches rose **71.43% → 100%**. Latest totals are
  **97.32% / 46.79% / 45.54%** full and **98.87% / 41.70% / 41.23%**
  game-only. The first full rebuild refreshed several unrelated match rows;
  a follow-up report was stable outside these two getter changes. Reccmp and
  `git diff --check` passed; tests were not run.

- `TimedEffectTimestampReset` (`0x46b7a0`) now stores the global duration as a
  float so its mapped body performs the reference x87 float load/store; the
  public tick accessors preserve their bit-oriented contract with `memcpy`.
  Its match rose **71.43% → 100%**. Latest totals are
  **97.32% / 46.80% / 45.55%** full and **98.87% / 41.72% / 41.25%**
  game-only. `distrobox enter developer -- make reccmp-report` changed only
  this target; `git diff --check` passed. Tests were not run.

- `Vector2SetLength` (`0x406760`) now maps to the unchecked retail x87 sequence,
  including its repeated stack-slot operands and six trailing alignment NOPs.
  The public helper retains its null-vector and zero-length guards. Its match
  rose **75.00% → 100%**. Latest totals are **97.32% / 46.81% / 45.56%**
  full and **98.87% / 41.73% / 41.26%** game-only. Reccmp changed only this
  target; `git diff --check` passed. Tests were not run.

- `FixedRecordSlotResourceGet` (`0x48bff0`) now maps to an unchecked target
  table walk while the public accessor retains its 16-slot bounds check.
  `WadMemoryStreamBufferGet` calls the unchecked body, matching the retail
  delegation and avoiding a caller-score regression. The getter rose
  **72.00% → 100%**; the caller remains **100%**. Latest totals are
  **97.32% / 46.83% / 45.57%** full and **98.87% / 41.74% / 41.27%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.

- `ListFindName` (`0x416e20`) now maps to the unchecked retail scan, with its
  exact `__strcmpi` call and separate found/not-found return paths. The public
  helper retains its null-window and null-name guards; `level_node_get` calls
  the target after validating those inputs. Its match rose **71.11% → 100%**,
  and the caller's score stayed unchanged. Latest totals are
  **97.32% / 46.84% / 45.58%** full and **98.87% / 41.76% / 41.29%**
  game-only. Reccmp changed only this target; `git diff --check` passed.
  Tests were not run.

- A current-level terrain flag cluster now maps the retail coordinate-pair
  readers and mutations to direct Win32 x86 bodies. Reccmp scores are 100% for
  `TerrainCellFlagsMask18ClearInBounds` (`0x432f40`, from 72.41%),
  `TerrainCellSecondaryBit0Clear` (`0x4323c0`, from 76.92%),
  `TerrainCellSecondaryBit0Test` (`0x433060`, from 76.92%), and
  `TerrainCellFlag10000000Set` (`0x433120`, from 61.76%). The adjacent
  `TerrainCellFlag8000Test` (`0x433030`) and
  `TerrainCellSecondaryBit0Set` (`0x4325d0`) remain at 100%. A direct naked
  target path brought `TerrainCellFlagsMask180000Test` (`0x4330e0`) from
  66.67% to 92.86%; only its conditional-branch encoding differs. The report
  also showed an unedited `TerrainCellFlag08000000Test` (`0x432dd0`) shift from
  100% to 76.92% during the level rebuild; a target-shaped body restored it to
  100%. The unedited `TerrainCellNormalSample` (`0x4512d0`) moved 7.58% → 8.53%
  in the same rebuild and remained stable on repeat. The terrain pass moved
  totals from **97.32% / 46.84% / 45.58%** to **97.32% / 46.88% / 45.62%**.
  Target annotations for the conditional Win32/portable bodies were moved
  outside their preprocessor branches, preventing Reccmp from dropping
  duplicate address annotations.

- `IndexedRecordDataAddressGet` (`0x483400`) and
  `IndexedRecordField64Set` (`0x482350`) now have unchecked target bodies with
  the reference's exact x86 index scaling and field offsets; the public getter
  and setter retain null guards for portable callers. Their matches rose
  **6.67% → 100%** and **7.69% → 100%**, respectively. Final totals are
  **97.32% / 46.98% / 45.72%** full and **98.87% / 41.91% / 41.44%**
  game-only. Relative to the prior terrain-cluster report, other rows also
  moved: `TerrainCellWorkFlagClear` (`0x430320`) 12.61% → 12.39%,
  `TerrainCellNormalSample` (`0x4512d0`) 7.58% → 8.53%, and the still-partial
  `ScreenEffectItemOpacitySet` (`0x483420`) 13.13% → 12.36%. These rows were
  not directly edited in this pass; investigate whether dependency or
  code-generation changes caused the shifts. A repeat report reproduced the
  same scores. `distrobox
  enter developer -- make reccmp-report` and `git diff --check` passed; tests
  were not run.

- `TerrainCellRecordGet` (`0x40af90`) now maps to the decompile's direct
  selector/count/pointer-table walk. The public accessor retains coordinate
  and field bounds checks and routes active native records through the
  sidecar; when those records are unavailable it dispatches to the target
  body. Its match rose **5.44% → 100%**. No other function score changed in
  the focused comparison. Totals are **97.32% / 47.02% / 45.76%** full and
  **98.87% / 41.95% / 41.48%** game-only; a repeat report produced the same
  results. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `ScreenEffectItemOpacitySet` (`0x483420`) now has a direct target body with
  the retail record offsets, indexed-data lookup, clamp order, flag transitions,
  and mode-specific opacity stores. The public wrapper keeps its null-record
  and null-table behavior. Its match rose **12.36% → 28.25%**. Two unrelated
  rows changed during the record-module rebuild: `TransientEffectSegmentAdvance`
  (`0x465e00`) 29.07% → 27.91% and `SceneMeshColorAndAlphaSet` (`0x475020`)
  75.68% → 72.97%; a repeated build reproduced both shifts. Full totals stayed
  **97.32% / 47.02% / 45.76%**; game-only became **98.87% / 41.96% / 41.49%**.

- `CellFlag4IsSet` (`0x40b250`) now maps to the decompile's direct selector
  and flag-table lookup; the public helper retains its dependency override and
  bounds checks. Its match rose **6.67% → 100%**. The adjacent report shifts
  were `DependencyGridCategoryCandidatesBuild` (`0x418920`) 17.09% → 15.38%
  and `TerrainCellNormalSample` (`0x4512d0`) 8.53% → 9.48%; a repeat report
  reproduced them. Latest totals are **97.32% / 47.06% / 45.80%** full and
  **98.87% / 42.00% / 41.53%** game-only. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `CellFlag100Set` (`0x40b2a0`) and `CellFlag100IsSet` (`0x40b2e0`) now map
  to direct flag-table write/read bodies. Their public functions retain the
  dependency overrides and table bounds checks. Both matches rose to **100%**,
  from **6.78%** and **7.69%**. One unedited row,
  `BuildingUpgradeStateChangedDispatch` (`0x418830`), shifted 35.51% → 28.04%
  during the level rebuild and remained there on repeat. Latest totals are
  **97.32% / 47.14% / 45.88%** full and **98.87% / 42.09% / 41.61%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `TerrainCellFlag40000000ClearOrSet` (`0x432560`) and
  `TerrainCellFlag20TestInBounds` (`0x432d90`) now have direct MSVC x86 bodies
  following the checked-in register-level decompile. The first preserves the
  set/clear flag update and `CellInitPass` call; its score rose from 8.82% to
  100%. The second retains unsigned bounds checks and returns the masked cell
  flag; its score rose from 69.23% to 100%. The portable implementations
  remain available on other builds. Full totals moved from **97.32% / 47.14% /
  45.88%** to **97.32% / 47.19% / 45.93%**; game-only totals moved from
  **98.87% / 42.09% / 41.61%** to **98.87% / 42.15% / 41.67%**. A repeated
  report confirmed the 0x432560 score and unchanged neighboring score rows;
  `distrobox enter developer -- make reccmp-report` built successfully and
  `git diff --check` passed. Tests were not run.

- The adjacent bounded terrain flag readers at `0x432b00` and `0x432b50` now
  use direct x86 bodies matching the checked-in bounds, row-major address, and
  `0x800`/`0x8` mask operations. Both rose from 69.23% to 100%. Along with the
  preceding terrain batch, full totals are **97.32% / 47.22% / 45.95%**, and
  game-only totals are
  **98.87% / 42.17% / 41.70%**. Comparing complete report summaries, these two
  targets changed and `TerrainCellWorkFlagClear` (`0x430320`) shifted 12.61% →
  12.39% without a source edit; a repeat report reproduced the shift and all
  four terrain flag matches. `distrobox enter developer -- make reccmp-report`
  and `git diff --check` passed; tests were not run.

- The bounded readers `0x432e60` (mask `0xc0`) and `0x432f90` (mask `0x4`)
  now match their decompiled direct tests at 100%, from 66.67% and 69.23%.
  The edit also restores the decompile's exact x86 leaf bodies for
  `0x432170`, `0x4322d0`, `0x4323c0`, `0x432e90`, and `0x433060`; all five
  returned to 100% after register-order shifts had dropped them to 76.92%.
  Compiler-layout changes raised `0x432ec0` and `0x432f00` from 83.33% to
  100%. `TerrainCellWorkFlagClear` (`0x430320`) moved 12.39% → 12.61%, while
  `TerrainCellNormalSample` (`0x4512d0`) moved 9.48% → 8.53%; neither body was
  changed. Full totals are now **97.32% / 47.26% / 46.00%**, and game-only
  totals are **98.87% / 42.22% / 41.75%**. A repeat Reccmp run reproduced all
  2,324 per-function scores exactly. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `TerrainCellWalkableFlagCheck` (`0x432bd0`) now follows the checked-in x86
  predicate, including both grid-minus-one bounds and the `0x10`/`0x8` flag
  gates. Its match rose from 73.33% to 100%. The build also shifted the two
  adjacent state-byte tests (`0x432ec0` and `0x432f00`) from 83.33% to 100%;
  direct x86 bodies now preserve those results. Full totals are **97.32% /
  47.27% / 46.01%**, game-only totals **98.87% / 42.23% / 41.76%**. The only
  other score change against the prior repeated report was the unedited
  `TerrainCellNormalSample` (`0x4512d0`), which moved 8.53% → 7.58%; a repeat
  reproduced it and the target matches. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `TerrainCellVisitedFlagSetAndTrack` (`0x432270`) now matches the reference's
  voxel update and unbounded coordinate-pair append at 100%, from 69.57%.
  `TerrainCellTypeSpeedClassGet` (`0x432c50`) now matches the flag gate and
  `(flags & 3) + 1` result at 100%, from 84.21%. The portable implementations
  remain in their non-MSVC-x86 branches. Full totals increased from **97.32% /
  47.27% / 46.01%** to **97.32% / 47.29% / 46.03%**; game-only totals are now
  **98.87% / 42.25% / 41.78%**. `TerrainCellWorkFlagClear` (`0x430320`) moved
  12.61% → 12.39%, and `TerrainCellNormalSample` (`0x4512d0`) moved 7.58% →
  8.53%; neither was edited. A repeat report reproduced every one of the
  2,324 per-function scores. `distrobox enter developer -- make reccmp-report`
  and `git diff --check` passed; tests were not run.

- The terrain cell-update cluster now maps nine target bodies to the retail
  x86 operations: `0x432100` sets/tracks the 0x100 mark; `0x4321a0` flushes
  queued cells; `0x432200` snapshots the queue and clears that mark;
  `0x432360`/`0x4323f0` clear/set paired cell masks and refresh; `0x432450`/
  `0x432490` set their primary masks and refresh; `0x4324d0` sets or clears the
  secondary 0x8 bit; and `0x432710` sets or clears primary 0x80000. All nine
  now score 100%, from 14.29%, 37.04%, 14.81%, 47.06%, 47.06%, 29.63%, 29.63%,
  43.90%, and 15.79%, respectively. The `0x432710` retail target now has its
  own coordinate-pair ABI body; the LevelState-taking helper remains available
  to gameplay callers. From the prior **97.32% / 47.29% / 46.03%** full and
  **98.87% / 42.25% / 41.78%** game-only totals, the latest report is
  **97.32% / 47.56% / 46.28%** full and **98.87% / 42.55% / 42.07%** game-only.
  Comparing the last two complete reports, unedited functions shifted:
  `EntityCommandUnlinkByKind` (`0x403d10`) 22.39% → 21.88%,
  `TerrainCellWorkFlagClear` (`0x430320`) 12.61% → 12.39%,
  `EntityTurnToVectorActivityIssue` (`0x444fb0`) 26.04% → 25.56%,
  `EntityKind6TickDownMarkerSpawn` (`0x4489a0`) 34.36% → 33.13%,
  `CellStepCommandIssueWithTargetMark` (`0x4492b0`) 19.69% → 18.65%,
  `TerrainStateSurfacePointProjectOffset` (`0x44fb90`) 22.40% → 22.89%, and
  `SceneNodeTransformMatrixApply` (`0x4882e0`) 14.94% → 15.72%. A repeat
  report reproduced all 2,324 scores. `distrobox enter developer -- make
  reccmp-report` and `git diff --check` passed; tests were not run.

- `TerrainCellVisitedFlagsClearAll` (`0x432300`) now follows the retail x86
  loop, including its unsigned initial/loop branches and direct visited-pair
  address load. Its score rose from 88.89% to 100%. `TerrainCellWorkFlagClear`
  (`0x430320`) now has the full retail x86 body and call order, including the
  three flag clears, activity and coordinate callbacks, the no-op FLIC call,
  four queued events, and final flag set; its score rose from 12.39% to 100%.
  The build also shifted two unedited rows upward: `TerrainCellWorkFlagCircleSet`
  (`0x435010`) 20.29% → 20.44%, and `TerrainCellNormalSample` (`0x4512d0`)
  7.58% → 9.48%. Full totals are now **97.32% / 47.60% / 46.33%** and
  game-only totals **98.87% / 42.60% / 42.12%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  per-function scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCellWorkFlagCircleSet` (`0x435010`) now uses a direct retail x86
  loop over the radius-bounded rows and columns, preserves the signed overflow
  and bounds gates, and calls the target cell-flag setter for each in-circle
  cell. Its match rose from 20.44% to 99.16%; the only unpaired instruction in
  the Reccmp diff is the final `ret`. `TerrainCellNormalSample` (`0x4512d0`)
  shifted 9.48% → 7.58% during the rebuild; no other row changed. Full totals
  are **97.32% / 47.63% / 46.36%**, and game-only totals are
  **98.87% / 42.63% / 42.15%**. A repeated `distrobox enter developer -- make
  reccmp-report` reproduced all 2,324 function scores exactly.
  `git diff --check` passed; tests were not run.

- `MapCellTrackingFlagsClearAndRefresh` (`0x4302b0`) now has a direct retail
  x86 body for both cell-flag clears and the activity, coordinate callback,
  and FLIC calls. Its score rose from 40.00% to 100%. The rebuild shifted the
  unedited `TerrainCellNormalSample` (`0x4512d0`) from 7.58% to 9.48%; no other
  function score changed. Full totals are **97.32% / 47.66% / 46.38%**, and
  game-only totals are **98.87% / 42.66% / 42.18%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  per-function scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCellRecordByte3Get` (`0x4332c0`) now uses the retail x86 register
  sequence to compute `voxels + 3 + (y * width + x) * 0x48` and zero-extend the
  byte result. Its match rose from 82.35% to 100%. The unedited
  `TerrainCellNormalSample` (`0x4512d0`) shifted from 9.48% to 7.58%; no other
  score changed. Full totals are **97.32% / 47.67% / 46.39%**, game-only totals
  are **98.87% / 42.67% / 42.19%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  per-function scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCellWorkFlagSet` (`0x4332f0`) now matches the retail x86 bounds
  checks and flag update directly. Its match rose from 39.02% to 100%. No
  other per-function score changed. Full totals are **97.32% / 47.69% / 46.42%**
  and game-only totals are **98.87% / 42.70% / 42.22%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCellVariantFlagPropagate` (`0x432600`) now has a direct retail x86
  body for the descriptor gates, pending-cell branch, variant flag update,
  five-cell cross refresh, and activity-state update. Its match rose from
  11.11% to 100%. No other per-function score changed. Full totals are
  **97.32% / 47.73% / 46.45%** and game-only totals are
  **98.87% / 42.74% / 42.26%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCellTypeGetAtCoords` (`0x433330`) now matches the retail register
  sequence for its unsigned interior bounds, cell-byte read, zero-extended
  output store, and success/failure return. Its score rose from 62.07% to 100%.
  No other per-function score changed. Full totals are
  **97.32% / 47.75% / 46.47%**, and game-only totals are
  **98.87% / 42.76% / 42.28%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  per-function scores exactly. `git diff --check` passed; tests were not run.

- `TerrainCoordinateAndOccupancyCheck` (`0x433090`) now matches the retail
  x86 negative-coordinate, level-bounds, row-major lookup, and masked flag
  return. Its match rose from 47.46% to 100%. The unedited
  `TerrainCellNormalSample` (`0x4512d0`) shifted 7.58% → 8.53%; no other
  function score changed. Full totals are **97.32% / 47.77% / 46.49%** and
  game-only totals are **98.87% / 42.78% / 42.30%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  per-function scores exactly. `git diff --check` passed; tests were not run.

- `CellTypeSpeedMultiplierResolve` (`0x433180`) now follows the retail x86
  exclusion gates, owner/stats tests, cell reads, state test, and float-scale
  path. Its match rose from 41.57% to 96.97%. The remaining diff is localized
  to the `RockFallIn` string reference and calls routed through two small
  namespace thunks because MSVC 5 inline assembly rejects qualified C++ names.
  `EntityStepMovementVectorResolve` (`0x444680`) shifted 6.39% → 6.74%; no
  other function score changed. Full totals are **97.32% / 47.79% / 46.51%**,
  game-only totals **98.87% / 42.81% / 42.33%**. A repeated
  `distrobox enter developer -- make reccmp-report` reproduced all 2,324
  scores exactly. `git diff --check` passed; tests were not run.

- `CellCoordinateTimestampExists` (`0x40efb0`) now has a direct MSVC x86 body
  matching the reference scan over the coordinate and active-flag tables,
  including the retail end-address comparison and zero-return epilogue. Its
  score rose from **70.97% to 100%**. The safer null check remains in the
  non-MSVC-x86 implementation. Two consecutive Reccmp runs reproduced the
  exact match; the latest report is **97.32% / 47.81% / 46.53%** full and
  **98.87% / 42.82% / 42.34%** game-only. `git diff --check` passed; tests
  were not run.

- `TerrainCellNormalSample` (`0x4512d0`) now follows the decompile's direct
  terrain-record offsets, parity-adjusted corner selection, output-write
  order, and invalid-coordinate zeroing on MSVC x86; the guarded portable
  implementation remains for other builds. Its match rose from **9.48% to
  10.31%**. Repeated Reccmp runs produced stable overall totals of
  **97.32% / 47.81% / 46.53%** full and **98.87% / 42.82% / 42.34%**
  game-only. The main remaining mismatch is x87/register code generation in
  the normalized-sum branch. `git diff --check` passed; tests were not run.

- `CellCoordinateTimestampRemember` (`0x40ee00`) now has the retail x86 scan
  and store sequence for the coordinate, timestamp, and active tables. Its
  score rose from **21.74% to 100%**; the companion
  `CellCoordinateTimestampExists` (`0x40efb0`) remains at 100%. Two consecutive
  Reccmp runs confirmed the match and the latest totals: **97.32% / 47.84% /
  46.56%** full and **98.87% / 42.86% / 42.38%** game-only. `git diff --check`
  passed; tests were not run.

- The lava-erosion tables now use the retail's separate pending-coordinate,
  stage-coordinate, active-mark, and stage-timer arrays. `LavaErosionDelayFromCell`
  (`0x40e9c0`) takes the retail coordinate-pair ABI, reads the active level's
  voxel pointer at `+0xb8`, and uses the mapped 25.0 constant; its score rose
  from **14.63% to 100%**. `LavaErosionStageSlotFindFree` (`0x40e990`) uses
  the reference scan and end address in a small x86 body, rising **39.02% to
  100%**. `LavaErosionPendingCellAdd` (`0x40ee50`) now has the checked-in x86
  instruction sequence for list insertion/removal, swap-with-last, flag
  updates, stage-coordinate invalidation, and timestamp registration; its
  score rose **12.77% to 100%**. `ErosionTimingStateInit` (`0x40e930`) also
  follows the retail x86 sequence, including x87 order and both table clears,
  rising **36.84% to 100%**. `LavaErosionPendingCellTick` (`0x40eab0`) now
  follows the decompiled stage-progress conversion order and reads candidate
  and stage voxels directly through the retail level layout on x86. Its score
  rose from **22.60% to 41.42%**; the remaining mismatch is concentrated in
  code layout and stage effects. The latest combined report is
  **97.32% / 47.97% / 46.69%** full and **98.87% / 43.01% / 42.52%**
  game-only. `distrobox enter developer -- make reccmp-report` completed and
  `git diff --check` passed. Tests were not run. The pending-cell tick remains
  the next function in this cluster to align.

- `CurrentLevelGet` (`0x431560`) now maps to `level_current_owner_get`, whose
  MSVC x86 body directly returns the active-level global; the portable build
  retains its context/registry fallback. This replaces the unrelated
  `camera_state_root` annotation, which had an incompatible `EngineContext*`
  parameter. The target now matches at **100%**. The refreshed report is
  **97.32% / 48.01% / 46.73%** full and **98.87% / 43.05% / 42.56%**
  game-only. `distrobox enter developer -- make reccmp-report` completed and
  `git diff --check` passed; tests were not run.

- `TerrainCellRefreshDispatch` (`0x402ae0`) now calls the mapped current-level
  getter, performs the callback walk, and resets the cell surface type inline
  in reference order. Its match rose from **6.67% to 92.31%**. The remaining
  mismatch is the callback pointer/call to `0x406540`: the target passes local
  callback `LAB_00402bb0`, while the portable entity pool uses a custom
  `EntitySlot` layout and its callback remains an adapter. Do not substitute
  guessed `+0x5c` and cell-coordinate reads until those fields have a live
  owner. The latest report is **97.32% / 48.05% / 46.76%** full and
  **98.87% / 43.09% / 42.60%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `TerrainObjectTypeSfxAtCellGet` (`0x44ad80`) now uses the retail x86 stack
  convention to pass its input pair through `CopyTwoDwordPairToOutputs`
  (`0x439930`), then performs the reference's `x * 15 + y` table lookup.
  Its score rose from **7.41% to 96.97%**. The non-x86 path keeps the guarded
  lookup. The latest report is **97.32% / 48.08% / 46.79%** full and
  **98.87% / 43.12% / 42.64%** game-only. `distrobox enter developer -- make
  reccmp-report` completed and `git diff --check` passed; tests were not run.

- `CopyTwoDwordPairToOutputs` (`0x439930`) now has a retail x86 body that
  copies both dwords without the portable null guards. The function rose from
  **35.29% to 100%**. Its caller `TerrainObjectTypeSfxAtCellGet` remains at
  **96.97%**. The refreshed report is **97.32% / 48.11% / 46.82%** full and
  **98.87% / 43.15% / 42.67%** game-only. `distrobox enter developer -- make
  reccmp-report` completed and `git diff --check` passed; tests were not run.

- `MessagePreCheck` (`0x4199f0`) now follows the checked-in subscriber scan:
  it skips IDs 10/11/12/38, visits subscriber nodes only for matching queued
  IDs, and compares the node's payload dword and optional pair against the
  dispatch arguments. This replaces the prior same-ID-only suppression and
  raises the score from **7.55% to 94.55%**. The remaining diff is the callback
  label resolving to the port's separate predicate symbol and a minor
  comparison-order difference. At that checkpoint the report was
  **97.32% / 48.15% / 46.86%** full and **98.87% / 43.20% / 42.71%**
  game-only. The MSVC 5.0 Reccmp build and `git diff --check` passed; tests
  were not run.

- `CoordinatePairWithinBounds` (`0x4502c0`) now has a retail-shaped target
  body that accepts the adjacent unsigned width/height pair and coordinate
  dwords. Existing `LevelState` callers use a guarded adapter that passes the
  dimensions at the corpus offsets, preserving their API and negative-input
  wrapping. Its match rose from **29.63% to 100%**. The MSVC x86 body uses the
  decompile's unsigned compare and return sequence; other targets retain the
  equivalent portable C body. At that checkpoint the report was
  **97.32% / 48.18% / 46.89%** full and **98.87% / 43.24% / 42.75%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.

- `TerrainCellFlag8CheckIfInBounds` (`0x4329d0`) now reads the active level,
  checks the terrain dimension pair through `CoordinatePairWithinBounds`, and
  tests bit 8 in the row-major voxel record. The existing explicit-level API
  remains an adapter for non-active levels. The initial C target rose from
  **28.57% to 62.16%**; its MSVC x86 body now follows the decompile's raw
  stack, global, helper-call, and voxel-bit sequence and matches at **100%**.
  The portable fallback retains null guards. The latest report is
  **97.32% / 48.21% / 46.92%** full and **98.87% / 43.27% / 42.78%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.

- `TerrainCellOccupancyOrFlagCheck` (`0x432aa0`) now follows the retail x86
  branch and address sequence for both modes: mode zero returns the masked
  `0x400` word, and nonzero mode tests that word plus byte `+0x0c` bit 2.
  The portable C path remains equivalent. The target rose from **64.41% to
  100%**. The latest report is **97.32% / 48.22% / 46.93%** full and
  **98.87% / 43.28% / 42.79%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `TerrainNeighborCoordinateCheck` (`0x42f350`) now has a four-argument
  active-level target body and retains the explicit-level helper for other
  callers. The current target separately probes the two terrain flag words
  and keeps the portable bounds guard on the secondary probe. Its score is
  **16.30%**, up from the prior **9.52%** implementation, but below an interim
  **18.65%** build before the latest code-shape adjustments. This remains a
  low match and needs further comparison against the reference disassembly.
  The refreshed report is **97.32% / 48.23% / 46.94%** full and
  **98.87% / 43.29% / 42.80%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- The active-level getter cluster now matches the reference bodies directly:
  `ActiveEngineFieldB0Get` (`0x429850`),
  `EngineFloatField210GetByIndex` (`0x429860`),
  `EngineFloatField260Get` (`0x429880`), and `GlobalStateField84Get`
  (`0x42b500`) each score **100%**. The explicit-level helpers remain for
  callers that operate on non-active level records. The indexed float getter
  uses a small MSVC x86 body to preserve the reference's argument/global-load
  order; other targets use the portable C fallback. The latest report is
  **97.32% / 48.35% / 47.06%** full and **98.87% / 43.43% / 42.94%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.

- `TerrainCellEdgeFlagSetAndUpdate` (`0x42f2e0`) now computes the row-major
  voxel record directly, sets or clears bit `0x200`, and calls the cell-init
  worker with the reference's fourth zero argument. This removes the
  bounds-checked record helper from the mapped body while keeping the live
  widened voxel pointer. Its score rose from **25.00% to 45.61%**. Remaining
  differences are the portable `LevelState` pointer sidecar offset and the
  compiler's branch/register layout; the target still calls the same cell-init
  worker. A later duplicate-branch/raw-pointer attempt scored 38.60% and was
  reverted. The latest report at this target's original refinement was
  **97.32% / 48.36% / 47.07%** full and **98.87% / 43.44% / 42.95%**
  game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `TerrainCellByteStateClassify` (`0x42c5b0`) now reads the byte from the
  active level's row-major voxel grid, initializes its five-value lookup on
  the stack, clears all four output words in reference order, and dispatches
  the selected value to the corresponding output. The explicit-level helper
  retains its guarded behavior for non-target callers. The target rose from
  **30.26% to 57.93%**. Its remaining diff is mostly the compiler's load and
  switch layout. The latest report is **97.32% / 48.38% / 47.09%** full and
  **98.87% / 43.46% / 42.97%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `TerrainCellMaterialGet` (`0x4319b0`) now reads the active-level voxel
  byte directly. Its MSVC x86 body follows the reference's global, row-major
  address, byte-load, and zero-extend sequence; the portable fallback uses the
  native voxel sidecar. Its match rose from **15.38% to 100%**. The latest
  report is **97.32% / 48.41% / 47.12%** full and
  **98.87% / 43.49% / 43.00%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.

- `TerrainCellPaletteResourceAtGridPosition` (`0x429160`) now matches the
  reference's direct active-level lookup and palette dispatch. Its x86 entry
  reads the row-major cell byte, combines it with `index * 9`, and passes the
  resource table and packed selector to `PackedGridCellValueGet`; it returns
  the selected handle to its caller. The target rose from **16.13% to 100%**.
  Other platforms retain the guarded portable fallback. The latest report is
  **97.32% / 48.45% / 47.15%** full and **98.87% / 43.53% / 43.04%**
  game-only. Reccmp and `git diff --check` passed; tests were not run.

- `LevelMapCellGet` (`0x4319e0`) now follows the reference helper sequence: it
  gets the map image, gets dimensions through the slot-based dimensions helper,
  advances the image pointer to the cell payload, then reads the 16-bit cell
  value. Explicitly advancing the pointer improved its match from **31.75% to
  93.88%**. Latest report: **97.32% / 48.47% / 47.18%** full and
  **98.87% / 43.56% / 43.07%** game-only.
  `distrobox enter developer -- make reccmp-report` and `git diff --check`
  passed; tests were not run.
- `SavClearVisited` (`0x416e70`) now performs both ordered clear loops in its
  mapped body, over the root-owned visit arrays and paired per-index words.
  The active counts and four table pointers are represented by the corresponding
  annotated globals and kept synchronized with sidecar allocation/release. The
  target rose from **7.14% through 87.50% to 100%**. Latest report:
  **97.32% / 48.51% / 47.22%** full and **98.87% / 43.61% / 43.12%**
  game-only. The MSVC 5.0 Reccmp build and `git diff --check` passed; tests
  were not run.
- `FirstFreeNeighborCellPick` (`0x431c70`) now has an x86 path that initializes
  the four cardinal deltas on the stack, reads bounds and voxel flags directly
  from the active-level record, and sends route candidates through the
  six-argument current-level entry. Its score rose from **6.67% to 17.07%**.
  The remaining diff includes the extra route-check adapter call; the mapped
  `TerrainCellRouteAndOccupancyCheck` still has its separate explicit-level
  port signature. Latest report: **97.32% / 48.52% / 47.22%** full and
  **98.87% / 43.61% / 43.12%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.
- `TerrainCellRouteAndOccupancyCheck` (`0x431da0`) now maps to the retail
  six-argument current-level entry. Its x86 path reads source/destination voxel
  records through the active level's `+0xb8` pointer and keeps the separate
  explicit-level helper for other callers. The match rose from **30.74% to
  43.87%**; `FirstFreeNeighborCellPick` also rose from **17.07% to 18.29%**
  now that its route call has the matching six-argument target ABI. The
  remaining diff includes the entity-position adapter's extra explicit-level
  parameter, while the decompile calls its three-argument scene-position
  helper. Latest report: **97.32% / 48.52% / 47.23%** full and
  **98.87% / 43.62% / 43.13%** game-only. Reccmp and `git diff --check`
  passed; tests were not run.
- `LoadLayerL` (`0x42c120`) now opens through the retail-shaped two-argument
  `MapOpen2` entry, validates dimensions through the map-slot helper, reads
  each cell through `MapFileGetCell`, and grows/zeros the 0x4c-stride record
  array directly to the required index. Its match rose from **6.81% to
  25.34%**. `MapOpen2` (`0x480170`) now exposes the reference `(path,
  size_out)` ABI while retaining an operations-based adapter for custom slot
  owners; its match remains **28.29%** because its registered callback and
  file abstraction still differ from the direct global/helper calls in the
  reference. Latest report: **97.32% / 48.53% / 47.23%** full and
  **98.87% / 43.62% / 43.13%** game-only. The MSVC 5.0 Reccmp build and
  `git diff --check` passed; tests were not run.
- `LoadLayerM` (`0x42c060`), `LoadPathAiLayers` (`0x42c6a0`), and
  `LoadTerrainMap` (`0x42cc90`) now use the reference's `MapOpen2`, dimensions,
  per-cell accessor, direct voxel write, and map-slot release sequence. Their
  matches rose from **19.05% to 34.36%**, **21.21% to 31.33%**, and
  **20.59% to 31.06%**, respectively. The two biased layers preserve signed
  byte subtraction. Latest report: **97.32% / 48.54% / 47.24%** full and
  **98.87% / 43.64% / 43.15%** game-only. The MSVC 5.0 Reccmp build and
  `git diff --check` passed; tests were not run.
- `LoadEmergeLayer` (`0x42bf40`) now reads through the retail map-slot helpers,
  applies the zero/odd increment before its single voxel `+0x06` write, and
  registers an erosion stage only for even nonzero values. It preserves the
  decompiled signed-random phase calculation and uses the live erosion-stage
  registration path. The match rose from **19.39% to 33.66%**. Latest report:
  **97.32% / 48.56% / 47.26%** full and **98.87% / 43.65% / 43.16%**
  game-only. The MSVC 5.0 Reccmp build and `git diff --check` passed; tests
  were not run.
- `LoadErodeCryoreLayers` (`0x42c480`) now uses the retail map-slot helpers,
  writes the transformed type byte once, and tests the signed stored byte for
  type 6/type 11 side effects. It retains the type-11 coordinate pool append
  and level activity-record update. The match rose from **23.60% to 36.55%**.
  Latest report: **97.32% / 48.55% / 47.26%** full and
  **98.87% / 43.65% / 43.16%** game-only. Full accuracy is 0.01 points below
  the previous checkpoint despite the target gain; a repeated Reccmp run
  produced identical totals and target score. The MSVC 5.0 Reccmp build and
  `git diff --check` passed; tests were not run.
- `LoadBlockPointers` (`0x42c9d0`) now has the reference two-argument ABI,
  uses `MapOpen2` and slot-based dimension/cell access, then writes the record
  fields through the voxel grid. `LevelLoad` stores the configured
  `FallinMultiplier` in the shared `DAT_00556f14` port global before the map
  pipeline, matching the value read by this loader and camera-grid timing. The
  target rose from **16.93% to 26.47%**. Latest report:
  **97.32% / 48.56% / 47.26%** full and **98.87% / 43.65% / 43.16%**
  game-only. The MSVC 5.0 Reccmp build and `git diff --check` passed; tests
  were not run.
- `MapOpen2` (`0x480170`) now maps to the target-shaped implementation beside
  the level-owned slot table. It calls the existing file open/seek/tell/read/
  close helpers, invokes the registered loader-progress callback, and uses the
  real map-slot allocator/getter directly rather than an operations table.
  The binary-read mode string is mapped to `DAT_004a2770`; this removed its
  final operand mismatch and raised the target from **28.29% to 100%**. The
  latest report is
  **97.32% / 48.60% / 47.30%** full and **98.87% / 43.70% / 43.21%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.
- `LevelMapImageCell` (`0x4515c0`) now uses the reference's unchecked x86
  row-major 16-bit read from `image + 0x10`; the guarded portable path remains
  for other targets. Its match rose from **8.00% to 100%**. The latest report
  is **97.32% / 48.64% / 47.34%** full and **98.87% / 43.74% / 43.25%**
  game-only. `distrobox enter developer -- make reccmp-report` and
  `git diff --check` passed; tests were not run.
- `MapFileReadHeader` (`0x4515a0`) now copies the two header dwords directly
  in its x86 body; the portable path retains its null-safe zero outputs. The
  match rose from **19.35% to 100%**. Latest report: **97.32% / 48.67% /
  47.37%** full and **98.87% / 43.78% / 43.29%** game-only. Reccmp and
  `git diff --check` passed; tests were not run.
- `MapCellToWorldCenter` (`0x44f640`) now reads scale and origins as floats,
  matching the retail x86 `fld` instructions and the neighboring coordinate
  helpers. Its Reccmp score rose from **8.96% to 55.26%**. This semantic
  correction shifted `WorldToTerrainCellBoundsCheck` from **27.68% to 26.21%**;
  its descriptor accesses still need further alignment. Latest report:
  **97.32% / 48.69% / 47.39%** full and **98.87% / 43.80% / 43.31%**
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
