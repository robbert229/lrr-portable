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
the target digest, so findings 01–25 can be re-verified: import the bundle into
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