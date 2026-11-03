# Win32 Reccmp runtime goal

**PAUSED by user request.** All three active agents were stopped. No agent
retains source/build/runtime/desktop ownership. Commit/rebase maintenance is
authorized; do not resume the gameplay goal without the user.

Active user-authorized outcome: `build-reccmp/LegoRR.exe`, the 32-bit MSVC 5.0
candidate, completes the real settings → main menu → tutorial → interaction →
pause/resume → normal exit flow. Each gate requires current-revision runtime
evidence; the observations below do not establish the complete flow.

Scope: run under the same runner, compatdata, environment, and reference cwd as
`make reference-run` and `make reccmp-run`. Native64/SDL correctness, including
the known64 terrain bridge defect, is OUT OF SCOPE unless evidence shows it
directly blocks this Win32 candidate. Record that dependency before expanding work.

## Evidence pointers and initial state

- [Runtime baseline](wine-runtime-baseline.md): completed evidence packet. Current
  artifacts are in `build-reccmp/runtime-evidence/20261102T202607Z-msvc5-baseline/`:
  `build-result.json`, launch identities, screenshots, logs and `events.jsonl`.
  The MSVC5 build passed at `dc7f167b`. Reference settings/menu/tutorial/pause
  and normal quit were observed; reference exit was 0 before candidate launch.
  Ordinary user-commanded movement remains unverified on the reference.
- Preservation limitation: reference runtime changed `Data/Saves/5.sav` without
  a pre-run byte backup. Its original hash and changed bytes are retained;
  [bounded recovery](wine-save-preservation.md) found no exact original copy.
  Current save bytes remain intact and are backed up; no guessed restoration
  was applied. Before any further
  launch, snapshot mutable saves/settings and verify restoration afterward.
  Reference EXE/DLL/WAD hashes were unchanged in the completed baseline.
- [Source flow audit](wine-runtime-flow-audit.md): completed source inspection
  at `dc7f167b`. It documents the actual surface creation boundary, conflicting
  spatial/movement payload contracts, frame-time units, and pause-menu caller.
  These are diagnostic inputs; none establishes a candidate runtime gate.
- [Port handoff](port-loop-2.md): historical build/report and implementation
  context; its “Current verified baseline” does not verify this gameplay flow.
- [Repository rules](../AGENTS.md) and [launch recipes](../Makefile): keep MSVC
  5.0 compatibility; both recipes share Proton `runinprefix`, compatdata, and
  `reference/` cwd. The recipes alone prove neither launch parity nor exclusivity.
- Inspection HEAD: `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`;
  `3rdparty/isle-portable` was dirty. This identifies the inspection, not a tested
  executable. Preserve concurrent changes; HEAD alone cannot identify a build.

## Current disposition: terrain checkpoint closed; NEXT/pointer integration next

Latest completed packet: `build-reccmp/runtime-evidence/20261103T001147Z-sceneheight-integration/`;
[full handoff](wine-sceneheight-integration.md), checkpoint-result.json,
final-frozen-identity.json and final-restoration.json. HEADdc7f167b remains dirty.
Production EXE`c7e28d4b19a193ba765af3db8ded5f72a3d557e09daadb6a91e8890ea1957788`,
PDB`fe41960370d2254cc4f377bc0ebbe81319366491c507170867fb86b09d6154c5`.
Exactly five height files changed from the prior63cc production; every temporary
probe is removed. Earlier startup/Stats/input/movement/pause/objective and typed
NERP text-pointer repairs remain. Dirty miniwin preserved; no commits/pushes.

| Gate | Current evidence |
| --- | --- |
| Build/fidelity | MSVC5/report passes,2332full/2113game mappings, none lost. Full97.33%/52.82%/51.41%, game98.88%/48.37%/47.82%. Seven score changes, including unedited matrix16.15%→12.15% real x87 schedule/spill changes, disclosed. |
| Height/picking boundaries | Actual MoveTuto_01 40/6 loaded-data CPU-vs-scene red3547failures→green0, before/after real four-corner mutation. Physical edges/diagonals/nonzero offsets and near-plane rendering/picking fixtures pass. These are disposable PE32 diagnostics. |
| Ordinary candidate flow | Settings/menu/Moving/two authored briefing pages,35.0106s unpaused dwell, Continue and fresh-Escape pause/resume, WM close0, no terminating signal. Earlier wrong-Vehicles attempt remains failed. |
| Visible target | Actual Object3/4/5 reach render_raider diagnostically; pixel writes1579/0/0. Object3 visible helmet maps near274,175 and actual picker returns Object3. Object4/5 obscured by identified nearer wall triangles. Diagnostic correspondence is not ordinary selection; no invisible-marker clicks or full authored-model requirement. |
| NEXT/selection/movement | OPEN: no ordinary permitted actor selection, persistent selected pointer, two commanded destinations/arrivals or walking pause. Corrected NEXT and physical ingress are still unintegrated. |
| Reference | Same harness/environment/cwd, real menu reached. WM close hung20s, exact owned PID SIGTERM, make2. No new reference tutorial/NEXT/movement or clean-exit credit. Full paired flow remains open. |
| Preservation | All484reference/2218prefix entries restored with zero byte/type/symlink/restorable-metadata differences, no owned writers; old protected Wine device PIDs untouched. Original save bytes remain unrecovered; designated current save preserved. |

The NEXT/pointer integration agent and two read-only audits were interrupted
at the pause. Current source DOES contain the pointer ingress, corrected NEXT,
review fixes and minimum-glyph-advance change. At the user's explicit request the diagnostic hooks remain, with versioned
probe fragments in tools/diagnostics and repository-relative includes. The
invocation is opt-in through LRR_MESSAGE_UI_PROBE and is not ordinary acceptance. No current
ordinary Wine acceptance exists for this later worktree; c7e28 remains the last
completed production runtime checkpoint. Resume from the current source and
saved next-phase packet, not by blindly reapplying prepared patches. See
[wine-pause-checkpoint.md](wine-pause-checkpoint.md).

Original integration recipe and provenance (now applied in the paused worktree;
validation is incomplete):

1. Physical ingress patch bff2e80feb1242226013573f37db57203eba0cd67e810e276a2323f2aaf1d49c
   and pause composition5ba2a5191be68273dff9e012a051e79dc284a9c5b18b1d9308bf84f09b3319d7
   in t3-adfb4cdd/hack. These include root's two cancellation corrections.
2. `/var/tmp/wine-next-root-corrections-20261103/after-pointer/tutorial-next.patch`,
   SHA256d9505307cc8ecdb3e1fe0277bf2f073bbdc9bacdaef6e71778cd5e11e4a01663.
   Read HANDOFF.md, manifest/composition, original author handoff and independent
   review. Root fixed overflow/line-layout and reward-owner findings. Its baseline
   already contains typed-text fix; do not reapply cumulative prerequisites.
3. Optional single-hunk font-minimum patch13fac5d0b54d44b5a841a86e1f8aae4a8680780f7657bad4dda455173761e749,
   `/var/tmp/wine-next-font-metrics-audit/`, conditional on actual PE32 blank-space
   advance failure. Archived font/source/reference loop predict zero-vs-minimum1.
   Do not claim briefing TextureSurface spacing is fixed by this different API.

The corrected NEXT directory contains an opt-in real-font/UI PE32 probe,
PROBE-RUNBOOK.md and removable disposable hook. None was built/run. Tests use
real tokens/font/resources and disclose fixture mutations; they cannot close
ordinary GUI gates. Host /var/tmp is not visible at that path inside developer:
compiler includes/logs must use shared /home external paths. Keep all raw .cpp
snapshots/probes OUTSIDE repo scans. At the InputState layout boundary preserve
baseline, clean ONLY build-reccmp once, then run required make reccmp-report;
current depend.make tracks37objects across four targets. Serialize builds,
restoration and launches. Workspace ~3.8GiB and /var~216MiB at closure: reuse
verified frozen evidence safely, no live prefix/reference hardlinks or unrelated
deletions. No further broad terrain/model/score pass unless actual ordinary
interaction demonstrates its dependency. Continue toward the full user flow.

## Historical checkpoints and gates (superseded by current disposition)

Last completed runtime packet:
`build-reccmp/runtime-evidence/20261102T223410Z-movement-integration/`;
[handoff](wine-movement-pause-integration.md). Last completed candidate SHA256
`a16f672d00622daa308054e3a4be03483bfa2b06b17890262b465c276ec723ed`,
PDB `35f9d48a3e8ad28053432dcc5e8b4fa09ea7450184a70a400d521dfd086cb797`.
Startup, Stats, input, movement, and pause/quit source deltas were integrated in
that checkpoint. The objective repair is now integrated for active validation;
the refreshed pointer patch remains unintegrated.
MSVC5 movement and combined pause builds/report generation both passed.
`final-frozen-identity.json` matches all176 source hashes and frozen artifacts.

| Gate / acceptance criterion | Last completed runtime status on a16f672d | Next action |
| --- | --- | --- |
| R: Equivalent shared harness, sequential launches | VERIFIED for this pair; reference exited before candidate launch | Preserve single execution ownership and repeat after integration. |
| S: Real settings accepted, graphics initialization succeeds | VERIFIED; normal settings input proceeds into menu and tutorial briefing | Preserve on next candidate. |
| M: Visible interactive main menu | VERIFIED for Training/Moving tutorial navigation | Preserve normal input and repeat on final candidate. |
| T: Menu-selected tutorial reaches a playable scene | PARTIAL: real OList actors have valid Stats, but no recognizable Raiders are visible | Repair demonstrated briefing/objective ownership failure before further actor interaction. |
| E: Ordinary entity selection and commanded movement | NOT VERIFIED; no invisible-actor clicks credited; movement probe compile-only | Establish visible actors and reference-permitted tutorial interaction, then verify selection persistence and two destinations. |
| P: Pause and resume preserve simulation and input | PARTIAL: real PausedMenu opens; actor/mission/queue/selection state and gameplay tickers remain unchanged for289 seconds in idle loaded state. Continue/Escape do not resume | Repair objective gate; verify resumed simulation, actual first post-resume delta, closing gesture, and pause while moving. |
| Q: Clean shutdown | PARTIAL: ordinary window close exits0; pause-menu Quit callbacks remain gated | Verify menu quit and mode-specific lifecycle; window-close success alone does not establish full reference parity. |

The exact-PDB snapshot `candidate-objective-renderer-paused.json` establishes
reward_active1, objective0xb1, pending1/result0, private result word0xb1, briefing
inactive, and valid terrain/depth storage. The archived log repeatedly treats
that pending objective as resolved. Source/state imply the reward guard sends
rendering to the coarse map; that branch was not directly recorded executing.
The pause breakpoint independently establishes blocked1 from objective&7, which
prevents menu_tick despite received Escape input. Proper briefing acknowledgement
and terminal-result ownership are the immediate repair; no forced flag clear,
render bypass, or guessed actor is authorized as a substitute.

Window close needed no terminating cleanup signal. An earlier debugger SIGINT
interrupted the run during a failed resume trace; it is disclosed and supplies
no resume/timing/render-branch proof. app.frame_dt is stored before local delta
bounding and cannot prove catch-up behavior by itself.

During the new integration, root independently verified all176 source hashes:
only the three authorized objective-patch files changed and each matches the
final delivery manifest. Inspected EXE SHA256 is
`50cc51f7d5b3fb326a66fbfed6c1f5ec214fe288c53883312cb10a52734e55c2`.
Full/game reports retain every mapping. [Root integration review](wine-objective-root-review.md)
records the two changed scores and unedited normal-helper code-generation delta.
All runtime gates for this new binary remain NOT YET VERIFIED until the sole
integrator supplies its current launch/interaction/exit packet. The table above
is explicitly the last completed a16 run, not acceptance of the new candidate.

Root transferred sole shared integration/build/report/desktop ownership from the
completed prior integrator to task0545336d-d1dd-46d6-9d69-b188ab4f3453,
“Integrate and validate real objective briefing ownership under Wine.” It may
integrate only the delivered three-file objective/briefing patch and tightly
necessary compile corrections, then freeze source for sequential reference and
candidate validation. Root reviewed the source contract and final artifact
`1caaa0b8079f910d731c4187710f13f30e1dbe09bb46d99e72f264b42f7cc4cf`;
read-only application against the current parent passed. This author result is
not build/runtime acceptance. The pointer patch remains unintegrated and outside
this phase. No other task has shared mutation or execution rights.

A disjoint isolated worker (taskdcdae2cc-1fd0-4e04-ad0b-13402e323b6b) may
prepare only tutorial_hud.cpp/ports.hpp callback repairs against the current
parent capture; it has no parent/build/runtime rights. The [callback audit](wine-nerp-callback-audit.md)
identifies shipped M01.npl calls to missing GetSelectedRecordObject and
SetRecordObjectPointer handlers plus an incomplete AdvanceMessage wrapper.
Actual reference disassembly supports the contracts. These are source/static
prerequisites, not additional runtime observations or authorization to integrate
them into the objective validation freeze. Later crystal/pickup callbacks remain
outside that bounded task.

The shipped MoveTuto_01 ObjectiveText contains two authored pages (raw395 bytes;
page lengths236/157), verified from the immutable WAD backup with unchanged
metadata. This checks applicability of the prepared parser capacity, not GUI
presentation. Source/provenance is external at /var/tmp/wine-tutorial-gate-audit/.

All484 reference and2218 prefix entries
were restored with zero differences to the designated current baseline;
ctime/inode are not restorable. The original5.sav loss described above remains
unrecovered. No desktop permission setting, harness, reference EXE/DLL/WAD, or
protected pre-existing process was changed by this checkpoint.

[Permitted tutorial interaction audit](wine-tutorial-permitted-interaction.md)
identifies the shipped script stages that enable actor selection and map clicks.
The opening NEXT-gated actor rejection is not a reference gesture test; scripted
races are not user-commanded movement. This source evidence guides the next
ordinary runtime replay and does not establish interpreter or gameplay success.

The callback worker completed a two-leaf144/254 patch; it remains unintegrated.
Command200/NEXT needs a shared message-history/frontend/timer owner and is still
unimplemented. Root also prepared a [one-hunk SetMessage pointer correction](wine-nerp-message-pointer-fix.md):
current command16 passes a numeric token index to an API that dereferences it as
a char pointer. The existing typed API accepts the already-resolved real token
text. This source defect is not yet proven to cause the observed post-resume
Error5. Keep the current binary frozen through the authorized single debugger-free
replay; no code is integrated from these preparations during that run.

Actual procedural rendering of a real production OList actor may establish
visible entity identity and movement, with graphical differences disclosed.
scene_render.cpp does not require attached_scene to draw/project/pick that actor.
Do not add an authored-model requirement absent from the user's goal or count a
strict probe's missing scene precondition as proof that ordinary UI evidence is
impossible. Normal reference-permitted interaction and every flow outcome remain
required; no fake/injected actor or debug command is acceptable.

Expanded isolated taska85805a0-9a4d-48c1-a66a-1e3ce315675a now owns the
complete tutorial message-history/NEXT contract across tutorial_hud, frontend,
and narrowly scoped frame-loop UI hunks. It has no parent/build/runtime rights.
Its cumulative artifact must include the reviewed144/254 leaves and typed string
fix, so those must not later be applied twice. It may resolve the formerly
out-of-scope frontend sound/history/timer ownership and production NEXT caller;
it may not substitute auto-NEXT, register writes, fake UI, or broad renderer work.
The sole integrator retains the existing50cc freeze and the one authorized
uninstrumented replay; the isolated work does not authorize a new shared phase.

Refocusing found no configured topology/work node; no successful managed work
trace is claimed. The explicit user goal and actual source/runtime path above
remain authoritative, with no invented replacement node.

A complete gate requires current runtime evidence for the identified binary.
Build/report success, source inspection, hooks, old screenshots, or a reference
success cannot substitute. Retain old packets after each integration and rerun
the flow on the final candidate. No commits, pushes, or merges are authorized.

## Historical repair queue and observations

The following narrative records earlier checkpoints and task ownership. Its
in-progress statements are historical; the current checkpoint above controls.


Current checkpoint: startup/menu and the observed Toolstation Stats fault have
been repaired. The c869d315 candidate loads the tutorial and dismisses its
briefing; visible actors, selection/movement and normal gameplay shutdown remain
unverified or failed. The Stats handoff records focused PE32 and live actor
evidence plus zero final reference/prefix restoration differences.

The original Stats/input integrator completed its checkpoint while retaining
the saved reference settings session. Root transferred sole shared integration,
build, report and desktop ownership to "Resume saved Wine session and complete
Win32 input UI validation" (task `6c570616-a594-4ac5-8216-f843c0f8e7c3`). This
owner must legitimately recheck current control, continue the saved reference
session, then run the candidate sequentially. Old completed tasks have no renewed
shared execution rights from a late response to their saved question. The input
packet is
`build-reccmp/runtime-evidence/20261102T215407Z-input-integration/`; candidate
SHA256 is `3d9aca4a30e35efe24b0b35c782c21db820b97827c51f7be756834a7793a6ca9`.
Root independently matched all 176 recorded source hashes to the current tree,
and matched the current EXE, PDB, CMake cache and full/game-only JSON reports
byte-for-byte to that packet's build archive. This verifies the frozen build's
identity; it adds no GUI or gameplay observation.
MSVC5 `make reccmp-report` passed. A focused production-object PE32 probe passed
with actual Win32 mouse input, including polling/warps, logical coordinates at
three client sizes, press/release and canonical arrows. The first probe's
PostMessage-only held-button assertions failed; that failed attempt is retained.
All compared mappings remain; three edited input/window scores decreased and
no unedited row changed. Runtime correctness has not yet justified accepting
those fidelity tradeoffs. GUI comparison is underway. The integrator's 21:59 capture showed a fresh KDE
control-input prompt, but the user subsequently reported no dialog. Root's fresh
full-desktop capture likewise shows no approval dialog; the older screenshot
must not be treated as evidence of a still-visible prompt. The retained reference
PID/window were subsequently absent; the stopped run is archived with unknown
exit mechanism/status and no normal-quit credit. After verified zero-difference
reference/prefix prechecks, the new owner launched reference sequentially through
the same harness. Its 22:19:58 bounded settings click visibly advanced to the DDI
intro and main menu (PID 3907207). Root inspected the saved intro/menu captures;
no new permission prompt is shown. Legitimate input therefore works for this
session. No desktop permission setting was changed, and no permanent grant is
claimed by an agent. A subsequent read-only query now returns `true` for KDE's
`kwinrc / Xwayland / XwaylandEisNoPrompt` (previously `false`). No agent wrote
that setting; it is current user-environment state, not part of prefix restoration.
The active integrator was informed to record it and preserve it. Old completed
baseline and Stats/input approval questions have been resolved as historical,
with explicit instructions granting no renewed execution or desktop rights.
Candidate GUI comparison and final run restoration remain pending.
New current-candidate observations are in the same packet:
`candidate-menu-background.png` → `candidate-menu-after-down.png` show a fresh
Down press/release changing StartGame's highlight to LoadASavedGame;
`candidate-training-hover-current.png` shows the pointer aligned with the
highlighted TrainingMissions row. Root inspected these captures. The effective
context JSON identifies EXE 3d9aca4a, the same runner/cwd/environment as reference,
and the configured KDE value `true`; candidate prelaunch reference/prefix checks
were zero. These support menu input responsiveness, not yet a completed input
checkpoint, tutorial playability, movement, pause or normal gameplay shutdown.
Root authorized a separate movement integration phase for the same sole owner
only after the input UI checkpoint and preservation verification are coherent.
It may apply the corrected complete movement artifact, build/report and compare
all rows, then establish visible loaded actors and actual route/arrival behavior.
Pointer-ingress and pause patches remain excluded until separately reviewed and
authorized. Source preparation or direct-call probes cannot pass ordinary E.
Subsequent input checkpoint closure is documented in `wine-input-integration.md`
and its packet's `continuation-handoff.md`: current-candidate menu input passed,
tutorial remained partial, and post-gameplay closes again failed before recorded
owned-PID SIGTERM. `final-restoration.json` verifies zero differences across all
484 reference and 2218 prefix entries, no owned writers, designated save intact.
Root changed priority after this second forced cleanup: the same integrator is
now authorized to compose the reviewed pause/quit delta before the next gameplay
validation. An in-progress movement build may finish/archive first; no further
gameplay run should repeat the unchanged known quit path. Preserve each source
delta and report identity. This authorization supersedes the earlier exclusion
of the pause patch; pointer integration still requires separate review.

Movement and the reviewed pause/quit delta are now applied serially in the shared
tree, recorded in `build-reccmp/runtime-evidence/20261102T223410Z-movement-integration/`.
Both `build-result.json` and `pause-build-result.json` record MSVC5/Reccmp exit 0.
The movement-only build is archived (EXE SHA256
`1dca5aaa2714fdef8af4114975f324c661e75f6af15c8ec316e811114d05bb8e`);
no movement-only gameplay run was made before composing pause/quit. Both report
comparisons retain all mapped rows. Root has not yet reviewed combined runtime
acceptance; these builds pass no gameplay gate. Pointer repair remains isolated.
Combined runtime build: EXE SHA256
`a16f672d00622daa308054e3a4be03483bfa2b06b17890262b465c276ec723ed`,
PDB `35f9d48a3e8ad28053432dcc5e8b4fa09ea7450184a70a400d521dfd086cb797`.
Root independently matched all 176 current source hashes and the current
EXE/PDB/cache/full+game report JSON to `combined-build/`. Runtime/debugging must
use this identity, not addresses/layouts from the older Stats or input binary.

Root reviewed the unedited erosion helper's exact-match decrease: its existing
loads are rescheduled and integer multiply operands swapped, with the same
remaining arithmetic. `level.cpp` hashes are identical across the input/movement
snapshots. Candidate-to-candidate terrain-normal differences use equivalent
address offsets/registers and a commutative floating-add order; the ray helper
likewise adds an FXCH before FADDP. No unrelated source or compiler-flag change
is justified from these diffs alone. Preserve all comparison rows/decreases;
this static code-shape analysis does not establish runtime correctness. Pause's
raw message-pump decrease follows its deliberate added quit-ingestion behavior.
Root's integration review confirms that WinMain currently supplies seconds
(`elapsed / 1000`), `options_scale_frame` multiplies them once by the game-speed
factor, and `ai_tick` receives that result before the EngineTick scheduler.
The corrected movement artifact's RouteSpeed query returns world distance per
reference tick, so its local factor of 25 belongs at this seconds boundary;
there is no additional terrain-block multiplier for ordinary walking. This is
source verification of the queued artifact's units, not executed movement or
reference timing parity. Keep this boundary explicit when rebasing the pause
clock, and verify displacement/arrival on the final candidate.
Root also checked the route activity boundary: `ai_set_activity` updates activity
fields only, and ordinary `AI_ACT_ROUTE` is index 1, explicitly excluded from
idle-chain insertion by `activity_register_bookkeeping`. The suspected immediate
idle-head replacement is therefore unsupported for this route transition; no
bookkeeping change is justified by that hypothesis. Later callback/cleanup effects
still require actual command-head/pose observations during movement validation.

An isolated implementation task is composing the prepared pause input repair
with the audited quit-owner corrections. Its partial delivery is
`/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-ecf54ee7/hack/wine-pause-quit-fix.patch`
(SHA256 `a93140e43c97bc92d1f34bb3005960cb9f33d5118c6f639abc2eaf6acd185c3d`),
with its handoff alongside. It changes only gods98 frameloop in the isolated
checkout: pause input, first-resumed-delta bounding, sticky close ingestion and
movie/raw-pump quit ordering. Static checks passed; builds/runtime NOT RUN.
Root reviewed the complete delivery delta: the movie pre-scan preserves a later
quit, raw WM_QUIT reaches the sticky latch, closing mouse input remains suppressed
through release, and the next delta is bounded by elapsed time since resume
rather than subtracting from an already clamped value. No additional source
finding was identified in those paths. This review does not validate compilation,
UI gating, simulation stability, delivered-close behavior or teardown under Wine.
Mission return remains unresolved because no complete release/reinitialization
contract exists for mission entity/command/level owners. The patch still exits
the application on mission Quit; it must not be credited with reference mission
return parity. No shared build/launch/desktop rights were used.
The completed independent world-pointer audit is
`/home/johnrowl/.t3/worktrees/lrr-portable/t3-dd266e71/hack/wine-world-pointer-contract-audit.md`:
`frame_pointer_overlay_hit` currently emits catalog action 0x49 for world hits,
then the direct press handler separately selects/moves an actor. Catalog action
completion can clear selection on a matching release under the audit's exact
source predicates. Actual runtime causality remains unobserved. The platform
also combines left/right button identity. The queued spatial/movement patch must
not be credited with repairing this separate ingress contract. A new isolated
implementation task (`51057f0e-76e7-4313-bb9e-91d89455771c`) owns the bounded
button/domain/consumption repair; it has no shared execution rights and must
retain correct reference catalog behavior. The current reference round did not
test an ordinary move/cancel gesture. Exact reference button policy is UNKNOWN;
earlier left-select/right-move assumptions are not acceptance evidence and must
not be hardwired without support.
The isolated pointer delivery is now prepared at
`/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-adfb4cdd/hack/wine-pointer-ingress.patch`
(corrected SHA256 `bff2e80feb1242226013573f37db57203eba0cd67e810e276a2323f2aaf1d49c`),
with its handoff and separate pause-composition patch alongside. An independent
read-only review (`6322f7f7-2571-4ee0-8b00-3ed5012ac519`) found two P2 cancellation
defects in the original c19b761a artifact. Root corrected them in the isolated
author tree: no fictitious held-button suppression on empty cancellation, and
same-pump cancellation coalesces invalidated pointer records before return while
preserving keyboard/quit order, including empty PeekMessage/capacity boundaries.
The original reviewed patch/manifest are retained; updated handoff, manifest and
review-fixes note identify the correction. All ten baseline round-trip hashes,
read-only current-parent applicability and external current-parent plus pause
composition checks passed. The composition patch SHA256 remains
`5ba2a5191be68273dff9e012a051e79dc284a9c5b18b1d9308bf84f09b3319d7`.
No independent re-review, build or runtime validation of these corrections is
claimed; no parent source was changed by this isolated correction.
Its existing either-button reconstruction policy and absent full catalog-hit
owner remain explicit limitations; removing world/catalog mixing does not itself
establish complete reference input policy. The latest reference attempt was
blocked at an instruction requesting NEXT; red-X rejection at that step does
not establish ordinary permitted selection or movement behavior.

New visibility prerequisite: the corrected scene renderer is compiled for MSVC5,
but `level_render_scene` falls back to the coarse map for `reward_active`, missing
terrain or allocation failure. Source shows the frame treating objective adapter
return 0 as terminal while `objective_tick_target` also returns 0 for the pending
briefing/result-word bit 1. Separate manual briefing dismissal may fail to
acknowledge that owner. This is a source-correlated hypothesis, not captured
causality. The sole integrator was asked to capture actual guards/result state on
the exact combined build. Isolated task `45c20f84-084f-4e3a-bd51-874ebc2582c5`
owns the bounded briefing/objective/reward transition repair or refutation. It
must preserve real terminal outcomes and reference-supported acknowledgment;
forcing reward off, bypassing the briefing or changing geometry is not a repair.
The completed isolated shutdown audit is at
`/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-832f49ca/hack/wine-gameplay-quit-audit.md`.
It identifies the unconsumed window-close latch, wrong pause-menu caller and
movie skip/quit batch loss. These are source defects; the exact failed close
path in the c869d315 run is still unknown. Its termination was recorded owned-PID
SIGTERM, not a newly observed application exception. A separate pause/quit
checkpoint must preserve normal exit(0), diagnose quit ownership and distinguish
mission return to Training from application exit.

The older investigation narrative below is retained as history.

- Primary: startup repair integrated from the captured patch in
  `build-reccmp/runtime-evidence/20261102T205420Z-startup-integration/captured/`.
  The MSVC5 build/report passed (`build-result.json`, exit 0). The first patched
  candidate accepted settings, then logged `scene initialization failed` and
  `application init callback failed`; launcher exit 0 does not establish a
  working menu or normal gameplay shutdown. The exact failing initialization
  stage remains under investigation. The original null-read is not yet proven
  resolved. Reference settings → responsive menu → normal quit passed in this
  round (`reference-1-result.json`, exit 0). Reference-tree and compatdata
  restoration were verified before candidate launch.
  The diagnostic second attempt (`candidate-2-stages.log`) identifies
  `SetSortMode` returning `0x80004001` (`E_NOTIMPL`). The reference initializer
  at `0x472710` calls this method without checking its result. The candidate's
  new fatal check therefore differs from the reference. Subsequent paired
  loader traces establish a runtime dependency difference:
  `reference-2-loader.log:159` loads the native DLL from `reference/`, while
  `candidate-3-loader.log:157` loads the Wine builtin system DLL. The latter
  logs its SetSortMode stub at line 321 and returns `E_NOTIMPL`. The next
  repair is application DLL resolution to the game's actual native library,
  followed by another build and normal settings-to-menu run. Keep the shared
  harness unchanged and required initialization failures visible.
  Initial report comparison retains all 2,332 full and 2,113 game-only rows,
  with 20 changed scores and 10 decreases. Root inspected the saved before/
  after disassembly for unedited `0x47ec60` (AVI frame index) and `0x482620`
  (mesh vertex fill): both show register-allocation differences with the same
  operations, rather than evidence of a changed gameplay contract. This narrow
  inspection does not clear the other decreases or establish runtime safety.
  The subsequent application loader correction resolves `D3DRM.DLL` against
  the launch working directory with checked full-path construction. In
  `candidate-4-native.log:324`, the native reference-directory DLL is loaded;
  this diagnostic launch reaches a visible main menu. Its live application
  log records `surface_create rc=0` and a nonzero initialized scene root.
  `candidate-4-native-after-ok.png` and `candidate-4-native-menu-response.png`
  establish visible rendering and cursor changes; these still images alone
  do not establish completed menu navigation, tutorial entry or clean exit.
  Preserve the run identity and final logs before restoration, and complete
  ordinary menu interaction before closing the menu gate.
  Integrator evidence additionally records candidate-4 Quit Game → visible
  confirmation → Yes → exit 0; normal candidate-5 navigates to the training
  picker and highlights Moving Rock Raiders before its distinct crash.
  Menu-only normal exit does not establish the post-gameplay shutdown gate.
  During menu validation, source inspection confirms that WinAPI arrow VKs
  (`0x25`–`0x28`) enter the printable-character branch before the translator's
  special-arrow cases. The frontend expects canonical `PLATFORM_KEY_*` values,
  so those cases are unreachable. The integrator owns that same platform file
  and is checking this input prerequisite. Raw client mouse coordinates versus
  stretched presentation remain an unverified secondary suspicion.
- Pause: prepared patch and evidence at
  `/home/johnrowl/.t3/worktrees/lrr-portable/t3-53d4faff/hack/pause-resume-fix.patch`;
  [source review](wine-pause-review.md) found two input-boundary issues. Both
  were corrected in the isolated checkout; `pause-resume-review-fixes.md` beside
  the patch records the updated hash and acceptance checks. No new independent
  review or build/runtime validation is claimed.
- Input: prepared two-file patch at
  `/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-66d21480/hack/wine-input-contract.patch`
  (SHA256 `951a3108f86c2afc07e9e4498a8de5658ede3100dd528627b366818852761a8d`).
  It addresses demonstrated WinAPI arrow precedence, established letter case,
  and one logical-framebuffer transform for events, cursor sampling and inverse
  warps. Root source inspection and read-only apply check passed; no patch was
  applied and build/runtime validation is NOT RUN. The current integrator may
  validate this in a separate subsequent packet after a coherent Stats
  checkpoint, retaining sole desktop ownership. An unresolved earlier loading
  failure takes priority. Hover still overrides keyboard selection in the
  existing frontend, so keyboard acceptance must park the pointer off rows.
  The same integrator has now applied the scoped input changes in the separate
  `20261102T215407Z-input-integration/` phase. Earlier apply-check results were
  read-only; this is the first integrated input source state. Compilation,
  report comparison and runtime acceptance for that changed candidate remain
  pending. Startup/Stats checkpoints are preserved as history for comparison.
- Movement: the initial selection/command-only attempt made no source edits.
  Its handoff at
  `/home/johnrowl/.t3/worktrees/lrr-portable/t3-4bec38f0/hack/wine-selection-command-contract.txt`
  establishes competing integer-cell and float consumers of the same record.
  The replacement assignment prepared a complete command/AI/spatial patch at
  `/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-3ed2e09b/hack/wine-movement-contract.patch`,
  with its handoff and an opt-in Win32 check beside it. Independent review at
  `/home/johnrowl/.t3/worktrees/lrr-portable/t3-871ea38e/hack/wine-movement-review.md`
  found two P1 blockers: an adjacent route can end at the start cell yet retire
  the command, and ordinary walking uses loaded drill times instead of
  `RouteSpeed`. A corrected COMPLETE replacement is now prepared at
  `/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-f9c6df51/hack/wine-movement-contract.patch`
  (SHA256 `0335ba351f723f72fac4d38c0952c14bee6cf2fd654cb8cd0be93e6331c6157d`),
  with its corrected handoff and strengthened Win32 probe beside it. Apply
  only this complete replacement in a future movement round, never both
  artifacts. Root's isolated whitespace check and read-only apply check
  against the current shared diagnostic source passed; no patch was applied.
  Repeat the apply check after the Stats repair. Builds, probe execution,
  Reccmp and runtime validation remain NOT RUN for the corrected patch.
- The tutorial Stats integration task is the exclusive shared-source, build,
  report, launch and desktop owner. Pause and movement patches remain
  unintegrated; their preparation grants no shared validation rights.
- Tutorial crash diagnosis is a separate read-only isolated task. The game
  exception is captured, but Wine's automatic debugger crashed afterward;
  no reliable dynamic caller stack or actor identity has been obtained yet.
  Production stats loading/binding already exists and must be traced before
  selecting a repair. Source inspection must not be reported as observed
  entity identity or a demonstrated missing config row.
  The completed audit is
  `/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-a49214f2/hack/wine-tutorial-stats-audit.md`.
  It proves config-scope misses, an Object2/Toolstation creation-order path
  that calls the getter before OList installs row 0, and missing raw static
  image pointers for reference-proven special kinds. These remain separate
  hypotheses for the observed actor until runtime identity is captured.
  Real asset requirements: Toolstation kind 4/row 0/span 3; Pilot kind 2/row
  0/span 4 with RouteSpeed 1.1. Static templates cannot substitute for either.
  Runtime diagnosis now establishes the immediate actor/caller:
  `20261102T212003Z-tutorial-stats-integration/diagnostic-1-stats-probe.log`
  records Object2, kind 4, placeholder row 4, empty type_name, task_state -1,
  index 0, null selected record/table and span 0, in `MoveTuto_01`.
  Return address `0x416cb7` is inside the diagnostic binary's
  `entity_kind_four_post_create_resolve` at `0x416c20`. This confirms the
  creation-order hypothesis for the observed crash, rather than a special
  static-state actor. It does not establish that the subsequent repair works.
  The subsequent repair scopes config lookups under the actual root and
  supplies the configured type name to OList creation before callbacks,
  restoring the instance label afterward. Temporary getter probes are removed.
  `20261102T212003Z-tutorial-stats-integration/repair-build/` records a successful
  MSVC5/Reccmp build with executable SHA256
  `c869d3155092b78aa1180d749f7b2db429d1a447b35a56c5bcbec0b9e3ed819c`,
  independently matched by root. Complete report comparison has no missing
  mappings and no decreases; only OList's score changes. A paired runtime
  comparison is underway; this build result does not close the tutorial gate.
  Normal repaired-candidate observations now pass the previous Toolstation
  fault and show the real briefing (`candidate-pair-tutorial-entry.png`) then
  a continuing post-brief loop (`candidate-pair-after-brief.png`). The latter
  is a mostly dark scene without clearly visible Raiders; playable tutorial
  acceptance remains PARTIAL. Actor visibility/picking and subsequent commands
  are unverified. The prepared movement patch contains a source-backed world/
  grid renderer boundary correction, but its runtime effect is not yet tested.
  The completed Stats handoff and packet additionally verify a focused PE32
  real-WAD production-boundary test (exit 0, failures 0): Toolstation row 0,
  span 3, flags `0x800400`; Pilot row 0, span 4, RouteSpeed 1.1. Read-only live
  snapshots confirm these bindings on the actual four OList actors. Required
  type-renderer ops remain uninstalled, and renderer input still treats their
  captured world coordinates as grid values. Neither is sole-cause proof for
  the dark frame. `candidate-forced-cleanup.json` establishes owned-PID SIGTERM
  after unsuccessful UI closes; exit 241 is that cleanup outcome, not a newly
  captured exception. Final restoration reports zero differences. Use archived
  logs for inspection to preserve restored reference metadata.

## Runtime packet contract

The baseline owner records one run ID, time, observer, and reference/candidate
identity per launch. Include HEAD plus dirty diff/submodule state, exact binary
path and hash, PE32/x86 identity, and the build/source snapshot association.
Record the resolved Proton path/version, compatdata path, relevant environment
and overrides, exact command/arguments, absolute `reference/` cwd, and settings/
asset identity or changes. Use matching inputs for the sequential comparison;
document settings mutations so sharing a prefix does not hide a changed context.

Attach actual screenshot/video paths with timestamps or timecodes, action steps,
expected versus observed result, complete stdout/stderr logs, and exit/process
observations. Correlate input with visible behavior; a still image alone cannot
prove movement or pause/resume. Link packets from the gate rows when available.
Preserve crashes, hangs, error text, and failed attempts as evidence. Forced
termination is a failed clean-shutdown gate, even if needed to release the seat.
Do not fake dialogs, suppress failures, bypass settings/errors, inject a level,
or use a direct-start option as proof of the menu/tutorial acceptance path.

## Ownership and next loop

- Root supervises background agents, keeps disjoint assignments, and checks
  progress against the earliest unverified gate. Keep the root responsive to
  user steering; agents report through baseline/audit pointers and brief findings.
- Startup integration has finished with a matching binary manifest and verified
  reference/prefix restoration. The agent assigned "Diagnose and repair the
  Win32 tutorial Stats binding crash" completed the Stats/input checkpoint.
  Root explicitly transferred shared source/build/config and desktop/launches
  to "Resume saved Wine session and complete Win32 input UI validation".
  Isolated movement, pause and pointer-audit work has no shared execution rights.
- Only the designated owner changes shared build/config state. Serialize builds
  against source edits; no agent races `build-reccmp`, reports, runner/prefix,
  settings, or the desktop. Transfer ownership explicitly through root.
- Desktop owner launches reference and candidate sequentially, checks process/
  window state before and after each, and preserves existing single-instance
  semantics. Never disable the instance guard or overlap the two executables.
- First establish R and the reference flow, then attempt S onward on the current
  candidate. If blocked, combine the failed packet with the source audit to choose
  one concrete blocker; fix its real owner/caller and rerun the affected flow.
- Build owner preserves complete before/after Reccmp reports (`index.html`,
  `results.json`, full and game-only outputs) and build logs under distinct run
  paths before regeneration. Record source/binary hashes, toolchain/config,
  commands, timestamps, totals, every changed row, and whole-report regressions.
  Validate authorized source fixes with `make reccmp-report` when available;
  report build, score, and runtime regressions separately. Scores never close gates.
- After two attempts on a blocker yield no new evidence, stop repeating them.
  Root records attempts and uncertainty, then reprioritizes toward a different
  evidence source or upstream blocker with an explicit next observation.
- Preserve unrelated work, environment, and reference assets. The user explicitly
  authorizes focused tests relevant to the 32-bit candidate and its harness;
  native64 testing is outside this goal. No commits, pushes, or PRs unless
  requested; agents never merge,
  squash-merge, or auto-merge (human responsibility under AGENTS.md).
- On user pause/stop, root immediately pauses its work and all background agents,
  prevents further builds/launches/edits, and records the last evidence and live
  process state. Resume only on user instruction.

Done means every gate is VERIFIED by current candidate evidence, with the full
before/after report provenance and any regressions disclosed to the user.
