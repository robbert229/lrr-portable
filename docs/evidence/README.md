# Evidence Bundle

Two REA evidence bundles (JSON) exported from Ghidra analysis sessions of
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