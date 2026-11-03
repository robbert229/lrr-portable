# Independent source review: prepared pause/resume patch

2026-11-02. **Verdict: two required input-boundary corrections (P2).** The
production menu selection is corrected, but the full pause/resume input contract
does not yet pass source review. Build, Reccmp and candidate runtime acceptance
are unverified. This review does not advance any runtime goal gate.

## Scope and identity

Reviewed the prepared, unintegrated patch and its modified source in
`/home/johnrowl/.t3/worktrees/lrr-portable/t3-53d4faff` (called **prepared** below),
against HEAD `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`.
All `src/...` line numbers below refer to that prepared checkout, including its
patch. Patch line numbers refer to `hack/pause-resume-fix.patch` itself, not diff
hunk headers. Parent checkout source remains unmodified by this reviewer.

- Patch SHA256: `e48e0b494bdbd03f179fb67a8665c3f013e5cdb93c190675001eb98bfd4238bb`.
- Handoff SHA256: `95bc32a483a0e95b51d105925299633c172766af96891c007b92c9aaf4aee47c`.
- Prepared `src/gods98/frameloop.cpp` SHA256:
  `2866c5237e3bfa9beb055b3b89eb4a51ed3e811eb1aada5d9fc5f461280c329f`.
- The checkout's source diff matches the supplied patch on inspection.

Read repository `AGENTS.md`, the chief-of-staff and code-review skills, parent
`hack/wine-runtime-goal.md`, `hack/wine-runtime-flow-audit.md`,
`hack/wine-runtime-baseline.md`, the current baseline section of
`hack/port-loop-2.md`, the patch/handoff, and relevant frontend, input, WinAPI,
movie, reward, pointer-dispatch and canonical-decompile contracts. The user's
explicit independent/no-delegation restriction overrides skill delegation.
Only this new parent note is owned by this reviewer. No source edits, patch
application, builds, tests, reports, game/desktop/launch operations, commits,
pushes or PRs were performed. Scope is PE32/MSVC5 Wine; native64 is excluded.

## Required corrections, ordered by severity

### 1. P2 — Consume the entire Continue mouse gesture before gameplay dispatch

**Location:** patch lines 93–103, specifically the existing return boundary
retained at lines 101–103; prepared `src/gods98/frameloop.cpp:3492–3494` and
`:3536–3538`. This boundary is pre-existing code, but the patch makes the real
PausedMenu Continue route reachable and relies on this boundary to consume it
(handoff lines 39–40, 100–103).

`input_begin_frame` clears `mouse_press`, but retains `mouse_down`
(`src/gods98/input.cpp:307–317`). Continue closes on the **down** click through
`frontend_tick` (`src/game/frontend.cpp:3062–3065`) and returns `FE_EVT_RESUME`
(`:2870–2875`). A normal held click can therefore return to gameplay while the
button is still down. The gameplay pointer adapter independently uses held and
released state, rather than `mouse_press` (`frameloop.cpp:3717–3728`).

Concrete source sequence with the pointer over terrain/entity and gameplay's
last held state zero:

1. Click Continue and keep the button down through one resumed gameplay frame.
   The return boundary clears the click edge, but the next dispatch receives
   `active=1`, `suppress_initial=0` and arms the world pointer action.
2. Release over the same cell. The next dispatch gets `count_action=1` and the
   matching pointer action completes (`frontend.cpp:7695–7744`,
   `:11559–11567`). The terrain/entity fallback supplies action `0x49`
   (`frameloop.cpp:976–992`).
3. `frame_ui_action_complete` records a tutorial release and dispatches the
   world action (`frameloop.cpp:917–939`). This changes the tutorial pointer
   release counter (`tutorial_hud.cpp:274–279`), and action `0x49` can enqueue a
   cursor request or clear selection (`frontend.cpp:10324–10336`).

**Acceptance impact:** the edge-based selection/move block at
`frameloop.cpp:3748` is protected, but Continue's physical click can still become
a gameplay action on release. The claimed consumed-click boundary and preserved
selection/input acceptance are incomplete. A quick click whose up event arrives
before the first resumed frame may hide this defect; the baseline explicitly
uses held clicks, so inspect that ordinary case.

**Minimal correction:** suppress gameplay mouse press/held/release action
dispatch for the menu-closing gesture until its real button-up has been consumed,
then rearm the pointer action/history for a fresh gameplay press. Keep simulation
running after resume. Reset/reconcile the pointer latch as part of this boundary;
merely clearing `mouse_press`, assigning `mouse_down=0`, or copying the held bit
into `pointer_dispatch_previous_down` does not reliably suppress the later
release or stale pre-pause action. Integrator acceptance should include holding
Continue through multiple resumed frames, then releasing over terrain, checking
selection and tutorial release/action state, followed by a fresh working click.

### 2. P2 — Keep the persistent Escape latch synchronized through modal pumps

**Location:** patch lines 17–32 (new helper), 47–51 and 71–76 (its only capture
sites); prepared `frameloop.cpp:177–191`, `:3266–3269`, `:3406–3410`.

The application-lifetime latch is correct across ordinary frame resets, but it
only sees events captured by gameplay and the nested pause loop. Two other
production pumps remove real WinAPI events without updating it:

- Restart invokes `frameloop_load_level` (`:3507–3515`), which can play the
  configured level video (`:2120–2124`). `s_movie_frame_quit` pumps an entire
  batch and handles skip keys, but ignores releases (`:1856–1877`). This is a
  real WinAPI implementation path, not a native64-only branch: the movie backend
  uses AVIFIL32 (`src/gods98/movie.cpp:946–951`, `:1131–1144`).
- The reward modal pump captures into the same `InputState` without touching
  `pause_escape_down` (`frameloop.cpp:401–405`), and runs from normal objective
  completion (`:3829–3861`). Its held-key table may be correct while the new
  private latch remains stale.

Concrete restart sequence when a configured video opens: pause with Escape
held, click Restart and Yes while still holding it, then release Escape during
the video. The movie consumes KEYUP, but the private latch stays 1. The first
fresh gameplay KEYDOWN is ignored at `:184`; only its later gameplay KEYUP
rearms the following press. An Escape-held resume followed by reward entry and
release inside its modal pump has the same loss. Conversely, an Escape pressed
to skip a movie while the latch is 0 can leave it unaware of a physical hold,
allowing a subsequent OS repeat to appear as a new gameplay press.

**Acceptance impact:** released/repressed Escape is not reliable across restart
and modal-frame ownership changes. This is a new synchronization obligation
introduced by the persistent latch, not the documented timing dependency.

**Minimal correction:** feed the Escape state tracker every event batch removed
by these pumps, ignoring its returned press when that mode owns the input.
Update the whole batch before movie skip/quit early returns so a later KEYUP
in the same already-removed batch is not lost. A common capture seam is also
valid if it covers movie batches that currently bypass `input_capture_events`.
Do not clear the latch every frame or unconditionally at restart: that converts
a continued hold/repeat into another fresh pause press. Integrator acceptance
should include release during restart video/reward and fresh Escape afterward,
as well as holding an Escape used to skip a video into gameplay.

## Optional cleanup / existing dependency

**P3, existing WinAPI arrow translation:** the new Left/Right assignments
(patch lines 42–43 and 84–85) initialize the fields safely, but do not establish
working arrow navigation on this backend. `src/platform/winapi/frameloop.cpp:271`
passes printable-range virtual key values through before checking VK arrows at
`:273–276`. VK_LEFT/UP/RIGHT/DOWN are `0x25–0x28`, so those later branches are
unreachable and events differ from canonical `0x80–0x83`
(`src/platform/platform.h:53–56`). This predates the patch and is not a blocker
for Escape/Continue source repair. Treat handoff lines 108–113 as unverified;
reorder the special-key translation before printable handling in a separately
owned correction if keyboard option navigation is part of acceptance. No broad
input refactor is needed. No additional style-only cleanup is requested.

## Source checks that pass

- **Actual menu selection:** patch lines 61–67, prepared
  `frameloop.cpp:3389–3394`, opens `FE_MENU_PAUSED` through `frontend_open` and
  enters the loop only on success. `frontend_build` parses `PausedMenu`
  (`frontend.cpp:2099`); the root API delegates to validated submenu zero and
  clears run flags/prepares live state (`:2449–2522`). The prior erroneous
  `menu_enter(&fe, 2)` call is removed.
- **Retail mode 2 retained:** `frontend.cpp:2560–2574` remains unchanged and
  still selects MainMenuFull submenu 2. Canonical MenuEnter agrees
  (`docs/decompiles/full/LEGORR.full.c:16429–16448`); canonical overlay mode 0
  instead selects the separate pause-menu set (`:15298–15301`). No menu API
  semantics are rewritten.
- **Opening event consumption:** `input_begin_frame` before nested capture
  clears the opening frame's key current bytes and mouse press. The new
  Escape latch ignores opening-hold repeats, including repeats after empty
  frames. No captured opening click is passed as `fi.click`. The root has no
  slider requiring consumption of an opening held drag. The closing held-click
  issue is finding 1 above.
- **All ten FeInput fields:** boot `frameloop.cpp:2575–2585`, pause
  `:3438–3449`, and existing reward-save `:3324–3334` assign up/down,
  left/right, enter/esc, mouse_x/y, click and mdown. The new assignments remove
  the boot/pause uninitialized Left/Right reads.
- **Ordinary Escape sequence:** with all captures staying in gameplay/pause,
  `input_begin_frame` does not reset the application latch. Down opens once;
  repeated downs and empty batches leave it held; KEYUP rearms; a fresh down
  at the pause root resumes; the held closing Escape cannot immediately reopen
  gameplay; its release and later fresh press open again. Down/up in one batch
  still yields one press with final held state zero. This reasoning uses the
  actual WinAPI repeated-key message mapping (`platform/winapi/frameloop.cpp:
  246–284`) and cur-to-prev/clear input semantics, not an assumed held `cur`.
- **Lifetime/reset/restart:** static app storage and initialization memset
  (`frameloop.cpp:173`, `:2673–2675`) initialize the added integer. No normal
  input frame reset clears it. A restart does not rerun app init or gameplay
  preparation; retaining held state is desirable. KEYUP observed in the usual
  next gameplay capture correctly rearms it. Missing modal captures, not an
  incorrect per-frame reset, cause finding 2. Teardown's existing memset at
  `:4070` would clear it on a returning shutdown/reinitialization path; normal
  callback shutdown exits the process before that point.
- **Continue and root Escape control flow:** parsed Continue and root Escape
  close the frontend and return `FE_EVT_RESUME` (`frontend.cpp:2870–2875`,
  `:3069–3076`); the pause handler breaks and returns before simulation/render
  for that invocation (`frameloop.cpp:3492–3494`, `:3536–3538`). Escape inside a
  submenu returns to root first. No AI/objective/registered engine gameplay
  tick is called by the nested pause body. The next gameplay frame reaches
  the existing simulation path.
- **Shutdown untouched:** Quit still sets `quit`/stop reason (`:3487–3490`),
  the application returns to the outer shutdown path, and
  `engine_callback_triplet_shutdown_dispatch` remains before explicit cleanup
  (`:4051`). Normal `EngineShutdown` still disables the watchdog and calls
  `exit(0)` (`src/gods98/engine.cpp:460–477`). Later cleanup markers are not a
  shutdown acceptance requirement.
- **MSVC5 source compatibility:** the additions use existing types, integer
  storage, ordinary C++ declarations/loops and existing APIs; no new language
  or CRT requirement was identified. This is source inspection, not compiler
  validation. Existing SPDX headers and frontend semantics are retained.

## Separate timing dependency and evidence limits

The patch does not alter the outer clock, delta units, clamps, or tick consumers.
`src/platform/winapi/entry.cpp:160–174` still records previous_time before the
blocking tick, so the next delta includes paused wall time. The global 25-Hz
tick/seconds inconsistency likewise remains. Opening the intended pause menu
makes that existing dependency observable through the correct UI, but no added
timing mutation or distinct worsening was found. Neither finding above requires
a broad timing rewrite. This source review cannot establish resume without
catch-up; that remains a separate bounded task/acceptance observation.

Inspected the existing reference screenshot
`build-reccmp/runtime-evidence/20261102T202607Z-msvc5-baseline/reference-2-pause.png`
in the parent checkout: Paused, Continue Game, Options, Advanced Options, Restart
Mission and Quit are visible. Read the baseline narrative and
`events.jsonl:79–83,102` for its Escape pause/resume sequence. Those observations
identify the reference surface and input history only. The baseline candidate
failed before its main menu; no candidate pause result is supplied by that
packet. No claim of completed candidate flow follows from reference success,
this patch's menu identity, or this static review.

Integrator retains exclusive ownership of all corrections, combined-patch
validation, MSVC5 build/Reccmp reports and ordinary-input Wine acceptance.
