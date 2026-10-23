# Evidence Bundle

Several REA evidence bundles (JSON) exported from Ghidra analysis sessions of
`LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`):

## `legorr-osf-restore.bundle`

Exported 2026-10-21 from the `.osf` load-side restore session (issue #67 /
LPT-16): the full `OsfRead @ 0x459470` body with its `OsfClear @ 0x4592a0`
pre-step and the exact `8 + count*0x14` size / `0x534f5252` magic checks,
`OsfDirtyGet @ 0x4593d0` (returns `DAT_004ffe64`), the read primitives
(`FUN_0047f980`/`FUN_0047fca0` — the shared fread/fsize used by the `.sav`
reader too), the shadow rollback `FUN_00459360` (copy `DAT_004ffe5c` →
`DAT_004ffe50`, called from `FrontEndStartLevel @ 0x415220` and
`ObjectiveTick @ 0x458be0`), the `OsfSnapshot @ 0x4592e0` caller set, and the
caller proofs that `LevelLoad @ 0x429890` is the *only* entry point for both
`FUN_004593d0` and `FUN_00459470` (the gate: clean table AND active slot).
14 records. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-menu-wipe.bundle`

Exported 2026-10-21 from the finding-24 session (menu wipe / front-end
transition). 18 recorded observations: the `LoadMenuBuild @ 0x4161f0`
pseudocode with the `MenuWipe`/`!MenuWipe` config reads and the
`-reduceanimation`/`"Null"` gates, the string constants (`"MenuWipe"`
`0x4a26d0`, `"!MenuWipe"` `0x4a26c4`, `"Menu"` `0x4a2544`, `"Null"` `0x4a23f4`,
`"LWS"` `0x4a13e0`), the `DAT_005574ec`/`DAT_005574f0` xref sweeps, the wipe
start (`FUN_004103f0`), end (`FUN_00410440`), the `MenuRunLoop @ 0x413b80` /
`MenuTick @ 0x412c00` drives, the input-freeze gate (`FUN_00411a00`), and the
factory/dispatch (`FUN_00472cd0`, `FUN_00475930`, `FUN_00472710`,
`FUN_004765c0`) plus `MenuEnter @ 0x416c80`. Import with `import_evidence_bundle`
or inspect `records` directly.

## `legorr-scene-done.bundle`

Exported 2026-10-21 from the finding-27 session (the scene-done flag
consumers in the entity tick). 42 recorded observations: the two `+0x3e4 &
0x10000` reader sites inside the finding-10 flag-evaluator blob (assembly
windows `0x43d8f0–0x43ddc0`), reader A's SFX dispatch `FUN_00465050` with bank
indices `0x23/0x24/0x25` by target type, reader B's `FUN_004475c0` probe, the
`FUN_00444a200` work-object resolver / `FUN_00475300`/`FUN_004754c0`
scene helpers, the SFX-bank slot-order verification for `SFX_Place*`, and the
`+0x3e4` bit-map reads (`0x80000`, `0x100000`, `0x40000000`). Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-menu-background-audio.bundle`

Exported 2026-10-22 for findings 23, 141, and 144. The bundle records the random
front-end FLIC selection, overlay SFX alias resolution, and the `FUN_00464f60`
queue / `FUN_00464fa0` submit helpers. Findings 141 and 144 follow the menu
tick's deferred-list rotation and music-loop queue values in the full
decompile to show selector-zero overlay sounds and selector-one music streams.
Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-ddscene-owner-link.bundle`

Exported 2026-10-21 from the finding-28 owner-link trace. 20 recorded
observations: `FUN_00473da0` setter and `FUN_00473db0` getter for object
`+0x20`; the `FUN_00437d00` entity-spawn branch that stores a scene object at
entity `+0x1c` and assigns the entity as owner; type-1 model propagation via
`FUN_0046d140`; type-2/3/4 model wrapper `FUN_00408380`; and the scene-object
constructor `FUN_00472950` clearing the pooled record. The bundle also retains
the relevant caller/reference inventory. Import with `import_evidence_bundle`
or inspect `records` directly.

## `legorr-ddscene-lifecycle.bundle`

Exported 2026-10-21 from the finding-29 teardown trace. Eight recorded
observations: `FUN_00437540` entity teardown and its DDiScene-object handoff;
`FUN_00472a40`/`FUN_00472a50` object disposal, including the kind-3/4
mode-0 early return and the `+0x28` free-list insertion; the
`FUN_00475710(object, 0)` scene-interface path; and `FUN_00472800` pool
shutdown. The bundle preserves the decompiler bodies and exact caller
references. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-ddscene-reuse.bundle`

Exported 2026-10-21 for findings 30–32. Five focused pseudocode observations:
`FUN_00473690` (kind-3/4 inactive-clone scan, scene reattach, time-zero
draw-step, and copy path), `FUN_00472a50` (mode-0 auxiliary-slot parking),
`FUN_00437d00` (entity owner assignment after clone factory return),
`FUN_00475140` (callback gates and scene-end threshold), and `FUN_00472800`
(pool shutdown and global gate reset). Import with `import_evidence_bundle` or
inspect `records` directly.

## `legorr-ddscene-hooks.bundle`

Exported 2026-10-21 from the finding-26 session (the DDiScene animation hooks).
46 recorded observations: the boot scene-end hook registration
(`FUN_00472900`/`FUN_00472920`, `DAT_0076cd78/80`, `DAT_0076cd94 |= 0x40`),
the two hook stubs (`LAB_0043f130` decode: `FUN_00473db0` → `entity+0x3e4 |=
0x10000`; `LAB_00464c20` decode: `FUN_00464c70` SFX-name-hash lookup →
`FUN_00465050` → `FUN_0047ad70`), the 45-slot SFX-name-hash bank init
`FUN_00464740` (`DAT_00502a20`, `SFX_RockWipe` at slot `0x27`), the
`FUN_00464f60` pending-SFX queue submit, the `FUN_00475140` draw-step call
sites (`0x47519b` per-frame / `0x475294` scene-end gates), and the
`FUN_00473680`/`FUN_00465090` positional-sound path. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-ddscene.bundle`

Exported 2026-10-21 from the finding-25 session (the DDiScene object runtime
behind MenuWipe and all LWS/3D animated objects). 64 recorded observations:
the 9-slot kind-string table (`DAT_0076add8` slots → `NULL MESH FRAME ANIM
ACT - - LWS LWO`), the object allocator `FUN_00472950` and pool grow
`FUN_004759f0` (free list `DAT_0076add0`, `1<<DAT_0076cd90` doubling blocks,
`0x2c`-byte records), the destroy path `FUN_00472a50`, the property/drawing
accessor family (`FUN_00476160/180/1a0/1e0/200`, `FUN_00475f70`), the
draw/step API (`FUN_00475140` draw-at-time, `FUN_00475710` step,
`FUN_00475390/3f0` length/frame read, `FUN_00477010`/`FUN_00477150` begin/end
scene with the `gods98/dx6` source string, `FUN_00477240`/`FUN_00473b40`
flags), the animation hooks `DAT_0076cd78/80` and their `FUN_00472900`
registration, the scene loader `FUN_004765c0` and file-open path `FUN_004800a0`,
the `DDiScene` class string (`0x4aab98`) and its read site `FUN_00476770`, the
`DAT_00505914` vtable system xrefs, and the per-kind loader branches
(`FUN_004767e0` FRAME, `FUN_00476850` MESH, `FUN_004809e0` LWO). Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-tasktable.bundle`

Exported 2026-10-21 from the task-table block session (issue #64 / LPT-19): the
complete `+0x1CE8` consumer set (the `e81c0000` deref search), `TaskBlockInit` /
`TaskBlockFinish` / `CheckpointBlobBuild` / `CheckpointBlobFree` assembly with the
blob-vs-block allocation aliases (`DAT_00552990` live vs `DAT_00552994` per-frame
blob), the six field loaders' assembly (the `Reward Text.<name>` key at
`DAT_004a26b0`, the `_`→space map, the `0x31` flag OR), the `0x460360` blob pass
(record `+0x28→+0x30` derive, the `0..100` `+0x38` clamp loop `0x460893-0x4608e5`,
and the `record_base+0x868` Level03 dword force with the `"Levels::Level03"`
case-folded compare), the display-tick record reads (`param_1[0x80]`/`[0x81]`/
`[0x83]`, `+0xD4` staging string), and the negative imm-scan proving no task-domain
function touches the unattributed ranges. 113 records.

## `legorr-rev.bundle`

The canonical findings bundle. 403 recorded observations — function
decompilations, address lookups, xref queries, and byte reads — each tied to
the target digest, so findings 01–27 can be re-verified: import the bundle into
a session (`import_evidence_bundle`) or inspect the `records` array directly.
No game asset bytes are included; entries reference offsets and produce
analysis text only.

The last export (2026-10-21, entity-ae session) adds the `.ae` assembly
evidence: the full kind-dispatch body (`FUN_00475930`), the kind-table init
(`FUN_00472710` at `0x472710`), the token splitter (`FUN_00477440`), the
activity-loop assembly around the `!`-gate (proving bit `0x400000` of
`DAT_00505f84` is the `-reduceanimation` feature — set, `!`-activities are
skipped; clear, processed with `!` stripped), the startup feature-word
writes, and the string vocabulary (`"::"` joiner `DAT_004aab18`,
`-reduceanimation` at `0x4aad04`, kind strings, activity keys).

## `legorr-ol.bundle`

Exported 2026-10-21 from the `.ol` object-list session: `FUN_0042e020`
(the `.ol` ObjectListLoad consumer) and `FUN_0042e8b0` (ObjectTypeByName —
the `.ol` `type` name → entity-code resolver), plus the registry
populators (`FUN_0042cda0` VehicleTypes, `FUN_0042d100` RockMonsterTypes,
`FUN_0040e490` encyclopedia), the shipped `Lego.cfg` `…Types` groups, and
the string reads that pinned the `.ol` field keys (`type`/`xpos`/`ypos`/
`heading`/`health`/`driving`) and the special-object names (Ore,
ProcessedOre, PowerCrystal, Boulder, Dynamite, Barrier, ElectricFence,
SpiderWeb, OohScary, Path, Pusher, Freezer, LaserShot, TVCamera). 21
records. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-frontend.bundle`

Exported 2026-10-21 from the finding-23 session (front-end & menus). 73
recorded observations: decompilations of `MenuBuild @ 0x4140c0`,
`MenuRunLoop @ 0x413b80`, the front-end gate cluster (`FrontEndActive @
0x416d00`, `MenuEnter`, `FrontEndStateChange`, `FrontEndSelectedAction`,
`QuitMenuProbe`), the level-chain / start-level entries (`LevelChainBuild`,
`FrontEndStartLevel`, `StartLevelPilots`, `StartLevelSelected`), the options /
movie callbacks (`AvisEnabled`, `MoviePlay*`, `BrightnessSet`, `VolumeSet`,
`GameSpeedSelect`, …), plus the `::` path-delimiter byte read
(`DAT_004aab18` = `"::"`) and the 50-address naming record. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-ai-receivers.bundle`

Exported 2026-10-21 from the AI-execution-receivers session (finding 10's
closed item). 51 recorded observations covering the entity-tick anatomy: the
scene-pass flag word and 125 s broadcast timer (`FUN_0043b270`), the entity
walker (`FUN_004377d0`), the post-step ticker (`FUN_0044bf00`), the activity
setter (`FUN_004409e0`) and bookkeeping (`FUN_00447c40`, `FUN_00403f30`), the
work-object resolver (`FUN_0044a200`), the idle/wake/cheat helpers
(`FUN_0043a960`, `FUN_0044c110`, `FUN_0044c530`, `FUN_0043c440`'s call
sites), the per-type creators (`FUN_004035c0`/`6c0`/`610`, `FUN_00402720`),
the `DAT_004decc4` activity-table initializer, the `DAT_004b32e8` xref
sweep (command-creation layer only — the negative result that no tick reads
record `+0x1c`), and the `test $0x41,%ah` zero-compare idiom. Import with
`import_evidence_bundle` or inspect `records` directly.

This bundle also contains the `FUN_0043f190` teleport-spawn worker
decompilation. Finding 223 connects it to the Teleport Man request and its
direct entity-callback call at `0x43ca38`; that callsite was checked against
the shipped-image disassembly.

Finding 223 also traces the vehicle `GetOut` unload path and Place-command
gate (`FUN_004401b0`, `FUN_00439e10`, `FUN_00439d50`, `FUN_00404fc0`,
`FUN_00432550`), edge descriptor creation (`FUN_0041d040`, `FUN_0041dca0`,
`FUN_0041ea50`, `FUN_00409300`, `FUN_00433090`), and the cell-flag producer
path (`FUN_00432c90` and the lava-neighbor helpers). These observations come
from `LEGORR.full.c` and disassembly of `reference/LegoRR.exe`; they are not
part of this REA bundle.

## `legorr-path-list-movement.bundle`

Exported 2026-10-21 from the path-list movement session — 67 retained
observations, a superset of `legorr-ai-receivers.bundle` (51). The added
records cover the move-branch decompiles: the tile-flag setter
(`FUN_00432e00`, bit `0x8000000` on the `0x48`-stride records behind
`DAT_005560d4+0xb8`), the walkability check (`FUN_00432770`), the
position/walkability wrapper (`FUN_0044a2a0`), and the record-list consumer
`FUN_00403d10` (the `+0xbc` kind switches behind the arrival unlink
`(ent, 8, 0, 1)`). The branch-body addresses themselves (`0x43cb40–0x43ce88`)
were read from the shipped-image disassembly, not recorded. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-tile-queries.bundle`

Exported 2026-10-21 from the tile-map slice — 68 retained observations, a
superset of `legorr-path-list-movement.bundle`. The added records cover the
tile-query decompiles: the walkability check (`FUN_00432770`, the flag
`0x10`/`0x40` + type `{1,7,11}` simple mode and the mode/neighborhood
branch), the state reader (`FUN_00469e60`, `+0x334+0x144`), the per-terrain
speed probe (`FUN_00469cc0`, `+0x334+0x24`), and the idle registration
(`FUN_0043a960`, the `'\t'` probe and `DAT_004eabcc` table). The occupancy
setter (`FUN_00432e00`) and the round-to-nearest converter (`FUN_0048e060`)
come from the earlier sessions' records and the shipped-image disassembly.
Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-ddscene-shutdown-reachability.bundle`

Exported 2026-10-21 for findings 33–36 — 33 retained observations. The focused
records include `FUN_0041fb50` initialization and its shutdown/failure exit,
the direct caller inventory and exact incoming references for
`FUN_00472800` (including the second unowned call instruction at `0x42503f`),
and the animation-name dispatcher (`FUN_00447c40`), paired scene updater
(`FUN_0046c3f0`), name-set/presence helpers (`FUN_00473370`/`FUN_00473340`),
and shared draw-step (`FUN_00475140`). Ghidra did not assign the call at
`0x42503f` to a procedure, so its caller and reachability remain unknown.
Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-advisor-animation.bundle`

Exported 2026-10-21 for findings 37–43 — 65 retained observations from the
Advisor animation-controller and DDiScene hook pass. Records cover controller
setup and action labels (`FUN_00401000`), request admission/reset and resolved
request callers (`FUN_004018c0`, `FUN_00401930`), the transition worker and
per-slot transform (`FUN_00401990`, `FUN_00401b30`), action-window stepping
(`FUN_00401a70`), sound submission/queue helpers and the sound-name table, and
the shared scene step/action flag helpers (`FUN_00475140`, `FUN_00473370`,
`FUN_00473b40`). Caller coverage at `0x423c2c` is unassigned, and the
`FUN_00475120` decompiler return type conflicts with the caller's machine-level
x87 result use; the findings retain both limitations. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-pathfinder-build.bundle`

Exported 2026-10-21 from the pathfinder slice — 68 retained observations,
the same store as `legorr-tile-queries.bundle` (the session added the
wave-flood decompiles recorded there). The records the slice leans on are
the flood seed (`FUN_00430ee0`, target validation + flag `0x40000` spread),
the traceback probe (`FUN_00430530`, `flags & 8` gate, type-8/10 cells),
the grid-handle entry points (`FUN_0044f380`/`FUN_0044efb0`), and the tile
accessors (`FUN_00431560`, `FUN_004319b0`). The creator lifecycle — install
(`0x4418d0`), per-cell append with the `0.5f` marker (0x44693b), and the
finish reset (`0x441a50`) — was read from the shipped-image disassembly.
Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-spawn-dispatch-granters.bundle`

Exported 2026-10-21 from the minion task-state housekeeping session (issue
#80 / board task lrr-portable/6). 50 recorded observations covering the
entity spawn dispatcher `FUN_00437d00` — full decompile and assembly — the
`0x438150` task-state switch (bytes at `0x438140`, the type−8 selector
table at `0x4382ac` and the jump table at `0x438298`), the minion static
stats templates (`0x4a98f0`/`0x4a9a40`/`0x4a9b90`, stride `0x150`), the
stats accessors (`FUN_00469c10` state setter, `FUN_00469e60` +0x144,
`FUN_0046a060` +0xcc capacity, `FUN_00469e80` +0x148), the record creators
(`FUN_00402720`, `FUN_004036c0`, `FUN_00404030`, `FUN_00403610`,
`FUN_00402850`) and the generic record unlink `FUN_00403d10`, the appender
`FUN_0044b370` and its two callers (xrefs `0x437eb6`, `0x43da62` — the
deferred branch bytes at `0x43da60`), the walker `FUN_004377d0` and its
scene-pass caller `FUN_0043b270`, plus the `+0x3e4`-write census searches
that disproved the "switch raises the grant flag" claim. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-wake-riding.bundle`

Exported 2026-10-21 from the wake/riding slice — 72 retained observations,
the pathfinder bundle's store plus the four probe/wake decompiles: the
attached wake writer (`FUN_0044c110`: `+0x340 = 100.0`, `+0x3e8 |=
0x800000`, gated by the target's `+0x3e0` bit `0x200000`), the can-do probe
(`FUN_0043c440` over activities 4/5/7, `FUN_0044c530` wrapping it behind
the type-2 gate), and the list scanner (`FUN_0044b480`, `+0x370` slots with
count `+0x384`). The spawn-dispatch switch tables (`0x438298`/`0x4382ac`)
and stats-template bytes came from the shipped-image disassembly. Import
with `import_evidence_bundle` or inspect `records` directly.

## `legorr-campaign-expansion.bundle`

Exported 2026-10-22 for findings 44–47 and 140 — 25 retained observations. The
bundle covers reward-button save/advance dispatch, timer-objective evaluation
order, objective-sidecar append gates and serialized fields, entity-removal
event messages, and objective-result voice-alias construction/submission.
Static control flow does not establish every user-visible outcome or the
runtime conditions that populate each optional message field. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-engine-expansion.bundle`

Exported 2026-10-22 for findings 48–50 — 25 retained observations. Records
cover DirectDraw setup branches and the D3DRM/device bridge, model-render pass
dispatch, and tile-texture refresh order. The fullscreen/windowed labels in
finding 48 remain inferences from the observed calls; the renderer findings
describe static dispatch, not a captured rendered frame. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-command-hotkey-expansion.bundle`

Exported 2026-10-22 for finding 51 and the finding-10 update — 38 retained
observations. Records cover command-queue key polling and enqueue behavior,
the key-registration producer and its call arguments, queue capacity, and
command-record `+0x14` initialization/link writes. The meanings and units of
that record field and the human-facing labels for key-state indices remain
unknown. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-campaign-transition-agent.bundle`

Exported 2026-10-22 for findings 52–53 — 40 retained observations. Records
cover reward Advance's campaign-row marking gates and startup construction of
normal/tutorial level chains from `NextLevel` references. The meaning of the
completion status and low row bits, the configured chain order, and validation
of cycles or missing references remain open. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-terrain-geometry-expansion.bundle`

Exported 2026-10-22 for findings 54–55 — 27 retained observations. Records
cover per-cell four-vertex construction and indexed buffer updates, plus
material replacement and its bounded ten-record path. Vertex-field meanings,
visual effects, and the timing semantics of the `0.05` field are not
established. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-incident-message-rendering.bundle`

Exported 2026-10-22 for findings 56–57 — 51 retained observations. Records
cover config-time incident template compilation, token lookup and formatting,
message-queue admission, and the scrolling text-widget consumer. Token meanings
and exact runtime scheduling/display remain unresolved; one incoming reference
to the consumer is omitted from the available caller inventory. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-win32-input-path.bundle`

Exported 2026-10-22 for findings 58–59 — 87 retained observations. Records
cover the Win32 message-pump filter and dispatch order, the DirectInput device
setup and 256-byte keyboard-state polling/reacquire path, and the prior/current
key-state conversion used by text entry. The mode bit meaning, exact window
procedure target, symbolic DirectInput format, and correspondence between raw
table indices and Win32 virtual keys remain unresolved. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-cursor-routing-expansion.bundle`

Exported 2026-10-22 for findings 60–61 — 31 retained observations. Records
cover Win32 cursor sampling, client-relative coordinate clamping and deltas,
plus the gameplay screen-to-ray camera route. The bundle supports static
control-flow claims; it does not establish runtime camera behavior. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-sound-instance-spatial-expansion.bundle`

Exported 2026-10-22 for findings 62–63 — 70 retained observations. Records
cover DirectSound distance and rolloff controls, sound-instance buffer
rotation, stream-mode routing, deferred positional requests and stop cleanup.
The audible result of these paths was not runtime-tested. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-wad-stream-contract.bundle`

Exported 2026-10-22 for findings 64–65 — 26 retained observations. Records
cover WAD open/read/seek/close behavior, per-open materialized buffers, clipped
item reads and end-relative seek arithmetic. The boundary behavior is derived
from static code and was not exercised at runtime. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-options-context-expansion.bundle`

Exported 2026-10-22 for findings 66–67 — 44 retained observations. Records
cover option globals across six save contexts and option data included in
save-slot writes. The semantics of individual fields and the `.osf` file role
remain uncertain. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-avi-playback-expansion.bundle`

Exported 2026-10-22 for findings 68–69 — 47 retained observations. Records
cover AVI frame-index timebase arithmetic, stream setup stages, failure state
and teardown. Playback cadence and user-visible failure outcomes were not
runtime-tested. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-activity-state-transitions.bundle`

Exported 2026-10-22 for finding 70 — 14 retained observations. Records cover
activity-name change dispatch, kind-specific command-chain runners, attached
object follow-up, and timer/pointer/bit updates. The evaluator entry remains
unassigned in Ghidra. Import with `import_evidence_bundle` or inspect
`records` directly.

## `legorr-checkpoint-reader-expansion.bundle`

Exported 2026-10-22 for findings 71–72 — 68 retained observations. Records
cover save-slot level-marker restoration and `.osf` reader validation and
return behavior. The findings describe observed static paths, not a complete
runtime load result. Import with `import_evidence_bundle` or inspect
`records` directly.

## `legorr-flh-playback-state-machine.bundle`

Exported 2026-10-22 for findings 73–74 — 31 retained observations. Records
cover caller-gated FLIC stepping and drawing, the reward animation's terminal
frame behavior, and per-task reward contexts across transitions and exit.
Runtime playback cadence remains unknown. Import with `import_evidence_bundle`
or inspect `records` directly.

## `legorr-olistfile-focused-expansion.bundle`

Exported 2026-10-22 for findings 75 and 83 — 6 retained observations selected
from the `.ol` loader pass. Records cover required entity-entry placement and
the second-pass resolution of optional `driving` names. No `.ol` files were
replayed at runtime. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-entity-destruction-chain-and-payload-expansion.bundle`

Exported 2026-10-22 for findings 76 and 84 — 16 retained observations. Records
cover teardown's flag-dependent command-chain disposition and type-dispatched
cleanup of entity payload slots. Static evidence does not resolve allocator
ownership beyond the observed call paths. Import with `import_evidence_bundle`
or inspect `records` directly.

## `legorr-selection-clear-expansion.bundle`

Exported 2026-10-22 for finding 77 — 60 retained observations. The cited
records follow a zero-mode selection action through queued opcode 3, active
selection clearing, and transfer of a per-entity marker. Runtime input context
remains unknown. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-energy-low-target-routing.bundle`

Exported 2026-10-22 for findings 78 and 86 — 59 retained observations. Records cover
energy-decay setup and the low-resource target-search path that can queue a
type-`0x11` route command. Candidate identity and any recharge outcome remain
unresolved. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-terrain-mutation-focused-expansion.bundle`

Exported 2026-10-22 for findings 79, 87, and 88 — 3 retained observations
selected from the terrain-mutation pass. Records cover voxel-state progression,
a distinct neighborhood flag update, and a neighbor scan that consumes a work
flag through mutation mode 1. Field meanings and runtime effects are not
established. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-object-state-and-health-ui-expansion.bundle`

Exported 2026-10-22 for findings 80, 82, and 89 — 6 retained observations. Records
cover the gated kind-4 `Activity_Deposit` transition and health-bar config
parsing, including derived/clamped border colors. Resource transfer and
rendered appearance were not runtime-tested. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-lava-erosion-focused-expansion.bundle`

Exported 2026-10-22 for findings 81 and 90 — 8 retained observations selected
from the `EmergeMap` pass. Records cover pending-cell seeding and the separate
timed ticker path through terrain stages and terminal event dispatch. Stage
names and runtime cadence remain unknown. Import with `import_evidence_bundle`
or inspect `records` directly.

## `legorr-selection-eligibility-expansion.bundle`

Exported 2026-10-22 for finding 85 — 62 retained observations. The cited
records describe selection-candidate flag, kind, and float gates. They do not
establish which candidates occur in a live scenario. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-objective-advisor-inline-markup.bundle`

Exported 2026-10-22 for findings 93–94 — 5 retained observations. Records cover
objective/advisor stream token table construction and the inline resource,
sound, and text consumer. Specific level stream contents and audible/displayed
results remain unverified. Import with `import_evidence_bundle` or inspect
`records` directly.

## `legorr-config-and-read-buffer-lifecycle.bundle`

Exported 2026-10-22 for findings 95–96 — 10 retained observations. Records
cover the shared block-text record arena and the whole-file stream-to-buffer
adapter. Callback effects, short-read behavior, and document reachability at
shutdown remain unresolved. Import with `import_evidence_bundle` or inspect
`records` directly.

## `legorr-entity-target-route-markers.bundle`

Exported 2026-10-22 for findings 97–98 — 4 retained observations. Records
cover two entity target-route setup paths, their route checks, stored pointers,
and final path markers. Callsite conditions, target identities, and gameplay
purpose remain unknown. Import with `import_evidence_bundle` or inspect
`records` directly.

## `upgrade-transition-focused.bundle`

Exported 2026-10-22 for findings 99–100 — 9 retained observations. Records
cover upgrade admission/payment alternatives and the shared commit path from a
command case and worker tick. Resource-slot identity and any `Activity_Upgrade`
animation selection remain unresolved. Import with `import_evidence_bundle` or
inspect `records` directly.

## `legorr-camera-control-focused.bundle`

Exported 2026-10-22 for findings 101–102 — 13 retained observations selected
from the camera-control analysis. Records cover gated held-key adjustments,
config-backed pointer-pan speed/acceleration, and follow-mode bounds. The key
byte mappings and runtime camera feel remain unverified. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-olistfile-health-field.bundle`

Exported 2026-10-22 for finding 91 — 5 retained observations. Records cover
the optional `.ol` health lookup, zero-to-100 default, and entity-field write.
No range clamp for nonzero values or runtime entity health behavior is
established. Import with `import_evidence_bundle` or inspect `records`
directly.

## `legorr-save-write-results-and-count-reset.bundle`

Exported 2026-10-22 for findings 105–106 — 18 retained observations. Records
cover the save writer's unchecked lower-level write results and the reader's
record-count mismatch reset/retry path. No short-write fault was forced, and
successful reinitialization is not established. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-pointer-cursor-config-and-override.bundle`

Exported 2026-10-22 for findings 103–104 — 10 retained observations. Records
cover the fixed cursor-name registry, config decoding for bitmap and animated
slots, timed selection overrides, and the reward-pump caller. Exact cursor
assets and elapsed-time duration in that loop remain unverified. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-samples-config.bundle`

Exported 2026-10-22 for findings 107, 139, and 147 — 33 retained observations.
Records cover the `Samples` config group, alias registration,
comma-separated variants, loop/stream markers, variant selection, the
deferred submission queue's selector/route-bit forwarding, and the signed
volume marker path, including the `#...#` token-buffer termination/bounds
limitation, and the streamed branch's bypass of the `*` duplicate request. It
also records how `-reducesamples` gates `!`-prefixed aliases before
resolution; the shipped config has 242 such aliases, 37 with streamed paths.
Finding 147 traces the shared generator and the LevelLoad seed source. The
`#...#` prefix has no shipped example. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-streamed-wav-format.bundle`

Exported 2026-10-22 for finding 108 — 5 focused observations. Records cover
non-PCM WAVEFORMATEX extension preservation, DirectSound buffer creation, and
the streamed playback caller. API acceptance and asset presence are
unverified. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-emergemap-layerl.bundle`

Exported 2026-10-22 for finding 109 — 10 retained observations. Records cover
LevelLoad's EmergeMap argument, the map pipeline, and LayerL's six-slot record
encoding. Consumers and gameplay meaning of the appended records remain
unknown. Finding 223 separately scans all configured EmergeMap and ErodeMap
assets for top/left border values; that asset scan is not retained in this
bundle. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-objective-entity-paired-predicate.bundle`

Exported 2026-10-22 for finding 110 — 30 retained observations. Records cover
the delegated objective predicate, its registered-entity iterator, callback
branches, field comparisons, and helper outputs. The compared fields' semantic
meanings and live level reachability remain unknown. Import with
`import_evidence_bundle` or inspect `records` directly.

## `legorr-graphics-mode-surfaces.bundle`

Exported 2026-10-22 for finding 111 — 5 retained observations. Records cover
DirectDraw windowed/exclusive setup, presentation-surface descriptor writes,
selected-mode probing, and the device setup path. Surface behavior is not
runtime-tested. Import with `import_evidence_bundle` or inspect `records` directly.

## `legorr-registry-accessor.bundle`

Exported 2026-10-22 for finding 112 — 8 retained observations. Records cover
the recursive subkey path splitter/accessor, RegCreateKeyA/RegQueryValueExA
use, and startup callers. Runtime registry contents remain unobserved. Import
with `import_evidence_bundle` or inspect `records` directly.

## `legorr-window-activation-key-poll.bundle`

Exported 2026-10-22 for finding 113 — 7 retained observations. Records cover
WM_ACTIVATEAPP handling, the keyboard-sampling gate, the frame loop, cursor
update, and the alternate-mode message filter. No runtime capture or media
advancement claim is included. Import with `import_evidence_bundle` or
inspect `records` directly.

## legorr-terraincreate-fields.bundle

Exported for finding 114. Eleven records cover TerrainCreate's 0x28-byte cell record, map-height byte assignment, mesh vertex use, diagonal selection/orientation fix-up, normal recalculation, and dirty-flag use.

## legorr-ptl-consumer.bundle

Exported for finding 115. Five records cover the LevelLoad PTLFile call chain, ordered block-child iteration, Message-name translation, and numeric-pair storage. It establishes table construction but does not identify a downstream consumer.

## legorr-reward-widget.bundle

Exported for finding 116. Nine records cover reward-button tuple initialization, the six-zero helper calls, RGB packing and surface-format conversion, and RewardInit ordering. The DirectDraw API mapping remains an inference.

## legorr-loader-progress-read-tap.bundle

Exported for finding 118. Twenty records cover loader-profile setup and selection, read-tap installation, both whole-file loader call sites, measured-size accumulation, and progress UI updates.

## legorr-directsound3d-init.bundle

Exported for finding 119. Five records cover DirectSound initialization, the queried GUID bytes and xref, the listener-interface acquisition branch, and subsequent initialization. One exploratory WAV-loader dossier is included but not cited by the finding.

## `legorr-sound-buffer-duplication.bundle`

Exported for finding 120. Two function records cover sound-buffer creation, the optional duplicate-buffer branch, and the caller forwarding the branch argument.

## `legorr-sound-buffer-fill.bundle`

Exported for finding 120. One function record covers locking the primary buffer, copying the loaded sample into it, unlocking, and querying buffer state.

## `legorr-sound3d-per-source.bundle`

Exported for finding 121. Four function records cover the DirectSound buffer descriptor, sound-record buffer creation, the tracked-buffer list, and configured distance setter calls.

## `legorr-wav-handoff-memory-copy.bundle`

Exported for finding 122. Three function records cover `FUN_0047b580` allocation and call order, WAV data-chunk seeking, and copying from the MMIO read window into the allocated memory buffer.

## `legorr-avi-frame-row-copy.bundle`

Exported for finding 123. Two function records cover `AVIStreamGetFrame` dispatch and the helper that copies DIB rows into the destination pixel buffer.

## `legorr-screen-shake-raiser.bundle`

Exported for finding 125. Two observations cover the negative-result branch in `FUN_004475c0`, which calls `FUN_0043f6a0` and raises entity `+0x3e4` bit `0x40000000`, plus the incoming-reference inventory for `DAT_004a48b0`. The global's consumer semantics remain unresolved.

## `legorr-screen-space-effect-lane.bundle`

Exported for finding 126. Three function records cover the ten-entry effect-table dispatcher, its 15-second timer/fade helper, and indexed per-item alpha/flag updates. Screen-space coordinates and the later draw consumer are not established.

## `legorr-permitted-activity-capacity.bundle`

Exported for finding 128. The 53-record bundle retains the canonical entity-activity evidence and adds fresh function records for `FUN_0044b370` (activity-list installation) and `FUN_0046a060` (capacity getter). It supports the getter's read from entity stats `+0xcc`, the installer's zero-store to `+0x3f4`, and the list mutation branches; the fields' broader semantics remain unresolved.

Finding 129 reuses the `FUN_00483420` record in `legorr-screen-space-effect-lane.bundle` and the `FUN_00482d60` record in `legorr-engine-expansion.bundle`; no duplicate bundle was added.

## `legorr-activity-3f4-reader-census.bundle`

Exported for finding 130. The 58-record bundle retains activity-capacity evidence and adds a complete initialized-image search for displacement `0x3f4` plus instruction decodes of every non-writer match. The search finds no other direct entity-field reference; derived pointer arithmetic remains outside its scope.

## `legorr-stats-tools-capacity.bundle`

Exported for finding 131. Seventy-five records cover the Stats loader reading `NumOfToolsCanCarry`, parsing per-level values into `+0xcc` in `0x150`-byte records, the state selector's per-level record pointer, and the activity-capacity getter.

## `legorr-sound-duplicate-selection.bundle`

Exported for finding 133. Five records cover the optional duplicate-buffer argument, bank-entry flag setup, and playback's single-buffer versus three-buffer rotation, together with the canonical buffer-creation record from finding 120.

Finding 134 reuses the `FUN_00464d00` parser record in `legorr-samples-config.bundle` and the loader/flag/playback records in `legorr-sound-duplicate-selection.bundle` to trace the `Samples` value's leading `*` through to three-buffer rotation. Its 13-entry inventory comes from `Lego.cfg` in `reference/LegoRR1.wad`; the finding records both source and extracted-config SHA-256 values.

Finding 135 reuses `FUN_00464d00` in `legorr-samples-config.bundle` to document the stream-mode local's lifetime across the comma-item loop. The shipped config check found no comma-separated entry containing `@`; this constrains the observed use, not the parser's behavior for other inputs.

Finding 136 uses `FUN_0047b970` and `FUN_0047ba20` from `legorr-sound-instance-spatial-expansion.bundle` to trace replacement and cleanup for a selected streamed-playback state. The argument path from `FUN_0047ad70` is cross-checked against the assembly retained in `legorr-sound-duplicate-selection.bundle`.

Finding 137 combines the level-name handler records in `legorr-frontend.bundle` with the sound-instance updater and wrapper records in `legorr-engine-expansion.bundle` and `legorr-sound-instance-spatial-expansion.bundle`. It traces selector values 0 and 1 to `FUN_0047b970`, while leaving actual streamed use of the dynamic selector-1 alias unresolved.
