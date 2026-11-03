# Frozen MSVC5 Wine runtime baseline — 2026-11-02

Authoritative packet: `build-reccmp/runtime-evidence/20261102T202607Z-msvc5-baseline/manifest.json`.
All artifact paths below are relative to that packet directory. No application
source changes, commits, pushes or PRs were made. No native64/SDL run was used.

The first functional failure is **candidate settings accepted → main menu**.
Reference reached its main menu, selected Training Missions → **1. Moving Rock
Raiders**, loaded the briefing and live tutorial, displayed Paused, resumed,
and exited through the normal Quit Game → Yes flow (exit 0). Candidate accepted
Full Screen / 640×480×16 but exited before the main menu. A second functional
attempt with Wine diagnostics reproduced a null read at **candidate 0x004A314D**,
inside `scene_object_arguments_normalize` (candidate entry 0x004A3130, reference
0x00475900). Candidate tutorial, selection/movement and pause/resume are BLOCKED
by this earlier failure. User-commanded movement remains unverified for reference;
its recorded movement was the tutorial's scripted race.

| Gate | Reference | Candidate |
| --- | --- | --- |
| Visible settings dialog | PASS | PASS; duplicate labels differ |
| Accept available Full Screen 640×480×16 | PASS | PASS; logged accepted |
| Visible responsive main menu | PASS | FAIL before menu; null read |
| Tutorial selected via real menu and loaded | PASS: Moving Rock Raiders | BLOCKED |
| User selection and commanded movement | UNKNOWN; scripted movement observed | BLOCKED |
| Pause/resume | PASS: Paused menu, Escape removes menu, pause again works | BLOCKED |
| Normal game shutdown | PASS: menu Quit Game → Yes; exit 0 | UNKNOWN; crash exit |

## Reproduction and identity

Source `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`. Initially only the unrelated
`3rdparty/isle-portable` submodule was dirty (three miniwin source files).
`initial-status.txt`, `initial-diff.patch`, `initial-submodule-diff.patch` and
`frozen-inputs.json` preserve that state. Source hashes remained unchanged.
Root/investigator concurrently added their own `hack/wine-runtime-goal.md` and
`hack/wine-runtime-flow-audit.md`; these are outside this integrator's ownership.

Required build, successfully run once:

```sh
distrobox enter developer -- make reccmp-report
```

The incremental build reused the existing executable and objects; reports were
freshly regenerated. Initial and fresh candidate EXE SHA256 are both
`44622ef1b3b4289e9259292efc5f6ae53904891e63997cb74c2c7ecc40921e43`.
Reference SHA256 is
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Both are PE32 i386. EXE/PDB copies and metadata/hashes are in `prior-*`, `fresh-*`,
`initial-files.json` and `fresh-files.json`. Build exit/times: `build-result.json`;
complete output: `build.log`.

Full report: 2,332/2,396, 97.33% implemented, 52.85% accuracy, 51.44% progress.
Game-only: 2,113/2,137, 98.88%, 48.40%, 47.86%. No matching-score changes versus
preserved prior reports. `prior-report/` and `fresh-report/` each contain both
JSON and HTML reports; comparisons are `results.json.comparison.json` and
`game-only.json.comparison.json`.

Run sequentially from repository root, exiting reference before candidate:

```sh
distrobox enter developer -- make reference-run
distrobox enter developer -- make reccmp-run
```

Both existing Make targets discover
`/home/johnrowl/.local/share/lutris/runners/wine/dw-proton-x86_64/proton`
(version `1790724403 dwproton-11.0-14`) and use `runinprefix`, the same
`/home/johnrowl/.local/share/rock-raider-a2/compatdata`, client path
`/home/johnrowl`, and working directory
`/var/home/johnrowl/source/lrr-portable/reference`. No game arguments, config/WAD
patches, environment startup overrides, autostop or direct level selection were
used. Reference executable argument is `./LegoRR.exe`; candidate is its absolute
build path as specified by Make. Captured runtime environment allowlists match
exactly between reference-1 and candidate-1/2; default effective `WINEDEBUG=-all`.
See `make-launch-dry-run.txt`, `runner-identity.json`, `container-identity.txt`,
and `*-game-identity.json`. Only an allowlist of environment values was recorded;
no authentication cookie or secret environment values were exported.

Both dialogs were ultimately accepted with Full Screen, 640×480×16. Reference
HAL device is labeled Wine D3DDevice using WineD3D/OpenGL; candidate hardcodes
DirectDraw HAL. Equivalent selected settings do not prove identical internal
rendering paths. Reference windowed HAL and RGB emulation each visibly report
**No supported 32 bit modes found**, so windowed was abandoned after those two
checks. This is not a candidate failure. Candidate duplicated combo labels are
recorded as a UI discrepancy, not the functional blocker.

## Runtime observations and attempt history

`events.jsonl` records UTC timestamps, every automated interaction command,
command exit status and screenshot observations. `record.py` is the reusable
recorder; it invokes the existing Make targets and captures logs/statuses.
Each `*-launch.json` / `*-result.json` records start/end, PID, command and exit.
Screenshots were captured through KDE Spectacle and actually inspected.

- Reference-1 (20:27:04–20:28:48 UTC): settings visible. Window click triggered
  KDE Remote Control permission for Control input devices; delivery unconfirmed.
  Agent did not approve or bypass it. Root/user subsequently approved. Own
  settings dialog was cancelled with WM_DELETE_WINDOW, exit 0.
- Candidate-1 (20:28:53–20:30:08): settings visible while permission was pending;
  no settings acceptance attempted. Own dialog cancelled before reference-2.
  Make status 2 / child Error 3 is intentional cancellation, not the functional
  startup failure. Candidate and reference were never concurrent.
- Reference-2 (20:30:11–20:36:23): Window radio visibly changed after relayed
  approval; unsupported windowed modes checked, then HAL/fullscreen restored and
  OK accepted. Intro logo was visible; Escape advanced to main menu. Held
  Training Missions click → first cavern labeled Moving Rock Raiders → held
  click loaded mission brief. Space twice entered tutorial. Narration arrows
  advanced through race demo; moving figures were visible. Selection during
  guided demonstration was not established. Escape opened Paused, second Escape
  removed it, third opened it again. Quit → Yes returned to training menu; back
  arrow → Quit Game → Yes exited 0.
- Candidate-2 (20:36:38–20:36:55): default fullscreen 640×480×16 accepted through
  OK. Window disappeared; no menu. Stub delta contains mounted 2 WADs, mode
  selection accepted, surface_create rc=0. Make reports child Error 5; Make and
  distrobox return 2. No application failure was inferred from process survival.
- Candidate-3 diagnostic (20:37:24–20:37:41): second functional attempt, same
  Make target/runner/compatdata/cwd/settings/binary; **only diagnostic delta**:
  `distrobox enter developer -- env WINEDEBUG=+seh,+ddraw make reccmp-run`.
  Black LegoRR window captured, no menu. Wine reports read-null exception
  0xC0000005 at 0x004A314D, EAX=0. Wine's automatic debugger subsequently
  reports its own internal crash; child Error 84 and Make/distrobox status 2.
  This diagnostic environment is explicitly separate from the matched baseline.
  No further attempts, builds or launches followed.

Control observations: immediate clicks could be missed by polled reference input;
200-ms held clicks worked. In tutorial, absolute mouse warps left the cursor at
the center. Relative `xdotool mousemove_relative` moved the visible game cursor,
and held clicks advanced tutorial narration. These observation/input-delivery
issues were corrected before classifying an application gate. Initial ImageMagick
capture and Python Xlib authentication attempts failed; Spectacle captured actual
windows. No browser automation or synthetic runtime-success logs were used.

Key inspected screenshots:

- `reference-1-settings.png`, `reference-1-window-setting.png` (OS permission).
- `reference-2-window-selected.png`, `reference-2-window-rgb.png` (unsupported).
- `reference-2-final-settings.png`, `reference-2-after-settings.png` (intro),
  `reference-2-after-logo.png` (main menu).
- `reference-2-first-training-label.png`, `reference-2-tutorial-start.png`,
  `reference-2-tutorial-live.png`, `reference-2-moving-visible.png`.
- `reference-2-pause.png`, `reference-2-resumed.png`,
  `reference-2-quit-confirm.png`, `reference-2-main-menu-return.png`,
  `reference-2-shutdown-confirm.png`.
- `candidate-2-settings.png`, `candidate-2-after-ok.png` (desktop after exit),
  `candidate-3-settings.png`, `candidate-3-after-ok.png` (black window).

## Crash evidence versus source interpretation

**Observed runtime:** `candidate-3-diagnostic.log:675` onward shows selected-format
16-bit probe; lock pitch 1280. Later application surface is 640×480, **32-bit RGB,
pitch 2560**, with successful lock description and unlock. Cooperative-level
calls report DD_OK. `candidate-2-stub-delta.log` reports surface_create rc=0.
The first failure is therefore not an observed surface-create HRESULT error.
Exception starts at diagnostic log line 779; null read and EIP at lines 780–785.
`diagnostic-tail.txt` preserves this extract with original line numbers.

**Binary correlation:** `fault-symbol.json` records the fresh report row mapping
candidate 0x4A3130 to reference 0x475900. `fault-disassembly.txt` shows:
`mov eax,[0x0093eb00]` at 0x4A3144, then `mov ecx,[eax]` at **0x4A314D**.
The exception reports EAX=0 and read address 0.

**Source-correlated hypothesis, not a captured call stack:**
`src/gods98/view.cpp:7689` implements `scene_object_arguments_normalize`; x86 line
7695 blindly dereferences `s_scene_root` when the alternate frame is NULL.
`s_scene_root` is defined at line 6709 and assigned by `scene_root_install` at
6918; repository search found only tests invoking that installer. Startup
`src/gods98/frameloop.cpp:2828` dispatches the init callback after surface creation;
callback `:2477` loads game/type/HUD resources. `view.cpp:7425` / `:7455` pose
entry points call normalization; resource loaders are possible callers. The
specific startup caller was not captured because Wine's automatic debugger
crashed. See investigator-owned `hack/wine-runtime-flow-audit.md` for independent
source traces; its movement, coordinate, timer and pause findings remain
unobserved in candidate because the menu was not reached.

Suggested next bounded repair: establish the real owned scene/root/frame contract
before startup resource pose calls and trace the concrete caller. Do not replace
it with a dummy root, initialization bypass or wrapper-only success. Rebuild and
repeat settings → visibly responsive menu first, then the same tutorial. Only
then validate selection, numeric movement destinations, pause/resume and UI quit.

## Cleanup and preservation limitation

`cleanup.json`, `final-processes.txt`, `final-windows.txt`: no owned LegoRR or
Wine debugger remains. Initial Wine device PIDs 2821989/2822012 remain alive;
no pre-existing process or shared wineserver was terminated. Reference dialogs
were cancelled gracefully and reference-2 quit normally; candidate functional
runs exited on their own. No game termination signals were sent.

Reference EXE, DLL, WADs and all frozen source/build inputs retain initial hashes.
The pre-existing reference stub log was copied before launch; complete resulting
log and appended-only delta are preserved, and its original bytes/metadata were
restored. **One preservation limitation:** normal reference runtime changed
`reference/Data/Saves/5.sav`. Its initial hash/size/mtime were captured, but its
initial bytes were not backed up. I should have backed up autosave bytes before
launching. The after-runtime file is preserved as
`reference-Data-Saves-5-after-runtime.sav`; no guessed restoration was applied.
Initial SHA256 `e33adbaa64b671252402f4319e1c307be5bdd79319272d128ccee73a3da889e7`;
after `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`.
All other reference files match their initial hashes; no new reference files.
This mutable save-state difference is disclosed; candidate failed before its
logged save-slot-load step, but that is not proof save state can never matter.

Build/report/launch ownership is now relinquished to root for the next expressly
assigned integration stage. This packet is a failed baseline, not a completed
runtime acceptance or a validated repair.
