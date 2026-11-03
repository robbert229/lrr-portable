Resume the Lego Rock Raiders Wine runtime effort on this machine.

Repository: https://github.com/robbert229/lrr-portable
Branch: feat/runnable-recomp
Starting checkpoint: 7ce3b7f41e8b349951e88064b25cca140ea4da67
PR: https://github.com/robbert229/lrr-portable/pull/373

This explicitly authorizes resuming here. PAUSED notices describe the previous
session. The branch was already committed, rebased onto main and force-pushed;
do not repeat that maintenance or reapply historical implementation patches.

Goal
Make the 32-bit MSVC 5.0 build-reccmp/LegoRR.exe reliably complete:
settings dialog → main menu → Moving Rock Raiders tutorial →
ordinary entity selection and movement → pause/resume → clean shutdown.

Focus exclusively on the MSVC5 candidate under Wine/Proton. Native64/SDL work
is outside scope unless it directly blocks this candidate.

Read first
- AGENTS.md
- hack/port-loop-2.md
- hack/wine-pause-checkpoint.md
- hack/wine-runtime-goal.md
- hack/wine-sceneheight-integration.md
- hack/wine-save-preservation.md
- tools/diagnostics/README.md

Current state
The committed source already contains startup/scene-root, Stats, input,
movement, pause/quit, objective, terrain-height, physical-pointer and
NEXT/message/history/timer repairs. The latest NEXT/pointer phase was
interrupted before final ordinary gameplay acceptance. Inspect and test the
current implementation; do not assume those features remain unimplemented.

MSVC5 make reccmp-report passed at the starting checkpoint on the old machine.
There is no ordinary gameplay acceptance for that exact checkpoint yet.

The previous completed production binary reached settings, menu, both Moving
briefing pages, a 35-second unpaused dwell, pause/resume and window-close exit0.
Loaded-data terrain-height and geometry regression probes passed.

Actual Object3/4/5 Raiders reached rendering diagnostically. Object3 had a
visible helmet/head/arms fragment and corresponding picker result; Object4/5
were completely occluded by identified wall triangles. The radius picker can
return hidden actors. Neither these diagnostics nor visible fragments establish
ordinary selection or movement.

A recent reference run reached its menu but hung after window-close and required
forced cleanup. Current reference tutorial/movement/shutdown parity remains open.

Machine-local setup
Establish the MSVC5 RTM toolchain, Reccmp, Wine/Proton, desktop input access and
licensed reference assets. Use the developer distrobox if available, or an
equivalent environment without changing compiler family.

Use existing Makefile variables to adapt machine-local paths:
REPROBIT_MSVC_5_0_RTM_ROOT, RECCMP_WINE_BIN, RECCMP_RECCMP,
RECCMP_PROTON_BIN and RECCMP_COMPATDATA.

Reference and candidate must use the SAME runner, comparison prefix,
environment, settings and reference/ working directory. Run them sequentially
through make reference-run and make reccmp-run, preserving the shared mutex.
Do not assume the previous machine’s prefix or display settings are portable.

Old build-reccmp/runtime-evidence directories, /var/tmp artifacts, caches and
worktrees are not in Git. Treat unavailable evidence as historical reports,
not current verification or a reason to fabricate results. Uncommitted miniwin
changes remained on the old machine and are outside this goal.

Diagnostic hooks
The user intentionally retained hooks and probe fragments under tools/diagnostics.
They compile for the MSVC5 x86 WinAPI backend.

LRR_MESSAGE_UI_PROBE enables disposable UI/NERP fixtures that mutate state.
Leave it UNSET for ordinary gameplay acceptance. Preserve separate identities
and results for diagnostic and ordinary runs. Never count fixture actions as
authored tutorial progress.

Work loop
1. Establish a fresh build and runtime baseline before choosing a repair.
   Run make reccmp-report in the configured compiler environment.
2. Select the earliest failing milestone and investigate its actual caller,
   owner, reference behavior and observable failure.
3. Implement a bounded correction and validate it under Wine. Keep MSVC5
   compatibility and the established /O2 /Ob1 profile.
4. Delegate bounded work when useful, with disjoint ownership and ONE integrator
   controlling shared edits, builds, reports and desktop launches.
   Use chief-of-staff if installed; otherwise follow this protocol directly.
5. After two attempts without new evidence, change approach or trace the missing
   dependency. Avoid repeated focus/navigation experiments or generic porting.
6. Record exact source/EXE/PDB identities, commands, settings, actual interactions,
   logs, screenshots, exit results and complete Reccmp regressions/mapping changes.

Acceptance
Follow real authored NEXT prompts and waits. Opening DisallowAll restrictions
are expected. Scripted NPC races are not user-commanded movement.

Select a genuinely visible Raider through ordinary input, verify persistent
selection, issue two distinct reachable destinations, and observe displacement
and arrival. Pause while walking, verify movement stops, then resumes and arrives.
Finish with ordinary shutdown, exit0 and no remaining game process.

Use actual reference evidence to repair any required camera, wall, picking or
movement contract. Do not force the camera, hide walls, weaken depth, click
invisible diagnostic coordinates, inject script stages, fabricate actors or
substitute a fake dialog. Authored-model/pixel parity is not an extra requirement.

Preservation and completion
Before launches, back up saves/settings and protected reference/prefix state;
restore and verify afterward with all owned writers stopped. Never restore a
live shared prefix or kill unrelated Wine processes. Historical original save
bytes were lost on the old machine; do not claim they were recovered.

Focused Win32 tests and ordinary GUI interaction are authorized. Preserve
unrelated work. Do not commit, push or merge unless requested. Honor pause
immediately.

Finish only when every milestone has current Wine/Proton evidence for the final
frozen candidate and a valid reference comparison. Builds, scores, probes and
historical successes cannot independently close gameplay milestones.
