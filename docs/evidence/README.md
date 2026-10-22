# Evidence Bundle

Several REA evidence bundles (JSON) exported from Ghidra analysis sessions of
`LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`):

## `legorr-rev.bundle`

The canonical findings bundle. 403 recorded observations — function
decompilations, address lookups, xref queries, and byte reads — each tied to
the target digest, so findings 01–23 can be re-verified: import the bundle into
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