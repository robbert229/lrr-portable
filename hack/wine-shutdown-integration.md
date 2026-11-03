# Wine shutdown / timed-message checkpoint

Bounded checkpoint completed 2026-11-03 UTC. **A concrete pre-close application
fault is captured and repaired; the final ordinary tutorial dwell, pause/resume,
and WM-close run exits 0.** The historical child Error 5 is not independently
identified as this same exception. Full gameplay acceptance remains incomplete.
Exclusive shared integration/build/report/runtime/desktop ownership is retained
pending explicit root transfer. No further phase starts here.

Packet: `build-reccmp/runtime-evidence/20261102T234005Z-shutdown-integration/`.
Read `checkpoint-result.json`, `manifest.json`, `ordinary-close-repro.py`,
`ordinary-acceptance/result.json`, and `final-restoration.json`.

## Actual fault and supported repair

The untouched entry candidate was EXE
`50cc51f7d5b3fb326a66fbfed6c1f5ec214fe288c53883312cb10a52734e55c2`, PDB
`8ccb11237fa75c526924776dc7147448f170c76f89ded4cb74a8f4056c0defd1`.
All 176 entry source hashes and the dirty miniwin diff matched the parent
objective packet. `before-build/` preserves its complete artifacts/reports;
raw C++ snapshots are outside repository scans at
`/var/tmp/lrr-shutdown-integration/`.

`exit-boundary/exact-script.py` was saved and executed against that candidate.
The exact invocation was:

```sh
python3 build-reccmp/runtime-evidence/20261102T234005Z-shutdown-integration/ordinary-close-repro.py exit-boundary tutorial seh
```

It used normal settings/menu/briefing input, pause/resume, and planned ordinary
close. Passive `WINEDEBUG=-all,+seh,+process,+timestamp` captured an **unhandled
`0xc0000005` read of `0x00000001` at EIP `0x004566bc`, EDI `1`**. No WM close had
been sent: `close_sent=false`. Wine's automatic postfault debugger then crashed;
make returned 2 / child Error 84. That diagnostic child status is distinct from
the historical Error 5. No agent GDB attach, ptrace, breakpoint, inferior call,
or application state/register write occurred.

`fault-pdb-map.json` freshly resolves the exact archived 50cc PDB:
`advisor_inline_token_render` begins at `0x456620`; the fault maps precisely to
`src/game/frontend.cpp:6582`, `while (source[i] != '\0')`. Disassembly shows
`mov al,BYTE PTR [edi]`. The only production caller of the legacy request API in
the entry source is command 16 in `tutorial_builtin_command`. Its compiled
instructions pass the numeric token index to `advisor_text_request_setup`,
which forwards it as a character pointer. `old-command-disassembly.txt` and
`old-advisor-call-disassembly.txt` retain that chain. This is source/call-site
evidence, **not a captured live caller stack or message-state snapshot**.
The observed pointer 1 is consistent with message ID 2's zero-based token index.

After the actual red signal, three ranked falsifiable hypotheses were disclosed:
numeric token-as-pointer predicts a small source pointer at the parser byte
read; ABI corruption predicts a valid caller pointer becoming 1 at the callee;
text lifetime failure predicts a formerly allocated text address. Exact PDB,
source and call instructions support the first. The close-only hypothesis is
excluded for this captured failure because close was never sent. The parent's
`wine-error5-active-time-hypothesis.md` independently explains the approximate
25-second active-time threshold; historical Error 5 attribution remains an
inference, not an observed exception identity.

The user's contingent authorization applied once the fault landed in this
parser. Applied **only** `hack/wine-nerp-message-pointer.patch`, SHA256
`c3288b9d98fa2a268822041c31c04b6fbfd95318c9dcccf1b821adc5c76c30aa`:

```cpp
advisor_text_request_setup_text(token_text, args[1]);
```

This replaces the numeric-index argument with the already resolved text through
the existing typed API. Final `tutorial_hud.cpp` SHA256 is
`8975555b09afef4f5720ab1f4dd8793061b4bc0658673da8bb00c1d751981d26`.
This is the sole retained source expansion beyond the assigned cleanup owners;
all other 175 source hashes and miniwin changes remain identical to entry.
No timer, register, pending flag, message queue clearing, NEXT callback, pointer
ingress, terrain or renderer repair was applied. Future NEXT composition must
include this parent one-hunk change; its old tutorial_hud baseline is obsolete.

## Builds and complete report comparison

Two serialized `distrobox enter developer -- make reccmp-report` builds passed:
the repair with temporary shutdown/CRT markers, then the final build after
removing every marker. No redundant passing rebuild occurred. Both
`git diff --check` checks passed. `diagnostic-build*` and `production-build*`
retain commands, logs, source hashes, EXE/PDB/cache and complete HTML/JSON reports.

The diagnostic report changed only WinMain's score, 28.4884% → 28.5714%, from
the temporary atexit trace registration; no decrease or new/lost mapping.
The immediate probe-removal comparison reverses that diagnostic-only change,
28.5714% → 28.4884%, at WinMain alone; its complete row/diff is retained in
`probe-removal-*-comparison.json`. This returns to the entry score.
The final production report has **zero matching-score changes, zero decreases,
and zero new/lost mappings** versus entry, across all 2,332 full / 2,113 game rows.
There are 1,232 full / 1,013 game rows with changed recompilation addresses or
address-bearing disassembly diffs; every such row/field is enumerated in the
comparison JSON, with complete before/after rows available in archived reports.
Totals remain full **97.33% implemented / 52.81% accuracy / 51.40% progress**;
game **98.88% / 48.36% / 47.82%**.

Final production freeze:

- EXE `63cc5b0403c6f50a7cb82417d1ef8ec128ea30d65899b86ca4f7d9d7a1aa69de`.
- PDB `12679761cca0de76d4f6b0084fea3789d8eb46af1c3946573fbc74f8f8d3bc4f`.
- CMake cache remains `7473b486394bf5315db88c6da159c0bb91304bc18ee1120f45ab45221f741525`.

`production-build-frozen.json` and `final-frozen-identity.json` verify the whole
source/artifact association and no mutation during the ordinary replay.
`fault-pdb-map.json` belongs to the entry 50cc candidate, not the final PDB.

## Ordinary acceptance on final production binary

```sh
python3 build-reccmp/runtime-evidence/20261102T234005Z-shutdown-integration/ordinary-close-repro.py ordinary-acceptance tutorial ordinary
```

The script launched only through
`distrobox enter developer -- env WINEDEBUG=-all make reccmp-run`.
Same Proton/preloader/prefix/reference cwd and relevant environment as the parent;
no Makefile, runner, assets, display setting or backend flag was changed.
`ordinary-acceptance/effective-context.json` records the actual process context.
No reference was launched; no new paired-reference or reference-gesture credit.

PID 3992908, 23:57:08 → 23:58:44 UTC. Owned screenshots and exact events verify:

1. Real settings, main menu, Training Missions via ordinary Down/Enter navigation,
   then the visible first Moving tutorial (the picker initially selects Back).
2. Both real briefing pages accepted with fresh Space presses.
3. Scene remains unpaused for **35.0079 seconds**, 23:57:31.925727 →
   23:58:06.933654, before any pause or close.
4. Escape opens PausedMenu; Enter activates Continue and returns the scene.
   More than 11 seconds later another fresh Escape opens a second pause.
5. Fresh Escape returns the scene; owned capture and visible-state assertion
   confirm resume. WM-close request is recorded **before send** at
   23:58:44.353350, command completes 23:58:44.356786.
6. Launcher **exit 0**, game gone, no errors/signals. The unchanged Make recipe
   propagates the child success. `result.json` records `pass_exit_zero=true`.

This run has no debugger, ptrace, breakpoint, process-memory inspection, Wine
exception tracing, injected call/state, automatic acknowledgement, debug menu,
numeric selection or autostop. Full exact executed script is preserved at
`ordinary-acceptance/exact-script.py`; the current packet script is runnable
with a fresh label after its strict preservation and no-writer guards pass.
Navigation/assertion corrections between diagnostic attempts are preserved;
this is not a claim that all attempts used identical physical inputs.

The fresh game-log append, `ordinary-acceptance-after-new-game-log.txt`, records
actual MoveTuto_01 load, both briefing actions, two pauses, then
`1361 frames ... tickers=23760, exit=closed`. Diagnostic persisted logs reached
engine-shutdown dispatch. The existing normal-play EngineShutdown branch releases
the scene and exits through the CRT; a separate diagnostic menu-quit run captured
the orderly CRT callback and Wine process exit 0. That existing branch does not
return to the later portable cleanup tail. **No new early exit, success handler,
exception suppression, cleanup skip or teardown reordering was introduced.**
Loader-mode/full-resource cleanup completeness is not verified here.

The script is the real runtime regression seam. No shallow substitute test or
native64/SDL suite was used. No live message-counter/timer sample was captured;
surviving the long dwell is not independent proof of complete NEXT delivery.
Pause/resume UI and responsiveness are verified, not walking pause or a measured
post-resume simulation-state delta.

## Failed attempts retained

- `red-menu`: first script expected the wrong settings title and timed out.
  Guarded Enter continued the same launch to main menu; ordinary close removed
  it. Original launcher status was lost. No pass/red cause credit.
- `red-tutorial`: unchanged 50cc, make 0. Screens show Continue had not yet
  resumed at capture, and final close was paused; active time was too short.
  This is not evidence against the later timed-message fault.
- `exit-boundary`: concrete pre-close parser fault above. ImageMagick later
  blocked on the vanished window. **SIGTERM to owned capture helper 3983151**
  released the script; no signal to any game/Wine process. Automatic winedbg's
  own crash and child Error 84 remain disclosed.
- `fixed-diagnostic`: a physical pointer/menu mismatch opened Quit confirmation,
  then ordinary input exited through menu quit. Script verdict remained red
  despite make 0; no tutorial acceptance.
- `fixed-diagnostic-keyboard`: assertion correctly caught Enter on the picker
  Back row returning to the main menu. Script stopped while the game was live.
  `continue-diagnostic.py` corrected navigation on **that same launch**, completed
  a 35-second dwell, both resume gestures and WM close. Persisted log reached
  `exit=closed` and shutdown dispatch; launcher status and late stderr were lost
  when the first supervisor ended. No overall launcher-pass credit from it.
- `final-ordinary`: preservation preflight only; no game launch. ENOSPC interrupted
  post-build inventory serialization. The guard refused runtime on the mismatch.
  Failed empty inventory and recovery verification are retained.

No exact Error 5 ordinary-close red was reproduced in this phase. Instead the
new diagnostic fault demonstrated failure without close. Do not retroactively
assign that address to either historical failed run or blame GDB for them.

## Preservation, capacity and closure

Made a new byte-readback-verified O_NOATIME backup of all 484 reference / 2,218
prefix entries before launching. Each completed run's afterstate was archived,
registry changes reviewed offline, owned writers checked gone, exact afterstate
rechecked, then restored and verified before the next launch. Only six generated
HID/WINEBUS ContainerIds changed per run; no game/user registry values changed.
Build-only reference-directory atimes were separately archived and restored.

ENOSPC recovery deduplicated **only this task's preservation copies**, retaining
all paths and bytes through internal hardlinks plus original metadata inventories;
5,213,104,716 redundant bytes removed. No live prefix/reference path, old packet,
active backup path or user file was deleted or hardlinked. Recovery fully hashed
the existing after-copies against stable live inventories before completing the
interrupted inventory and restoration. `evidence-deduplication.json` and
`enospc-recovery.json` record this. No inference about the separate Git worktree
failure is made. Capacity must be checked before future large archives.

`final-restoration.json` verifies **zero differences** in all bytes, types,
symlinks and restorable metadata for all 484 / 2,218 entries. Nonrestorable
ctime/inode are not claimed restored. Current designated save remains
`41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`;
lost original `e33adbaa…` remains unrecovered. Protected PIDs 2821989 / 2822012
remain alive and untouched. No owned game/debugger/wineserver remains.
XwaylandEisNoPrompt remains observed true; no desktop permissions/settings edit.

First unresolved full-flow blocker remains **T/E: recognizable visible loaded
Raiders and ordinary selection/movement**. Walls and partial figures remain in
current screenshots. NEXT/message ownership remains incomplete. Isolated NEXT
and scene-height artifacts, their reviews and the terrain audit are untouched;
no optional terrain sample or extra T launch was performed. No commits, pushes,
PRs, merge, native64/SDL work or delegates. Retain sole ownership until root
explicitly transfers it.
