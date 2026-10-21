# Evidence Bundle

Two REA evidence bundles (JSON) exported from Ghidra analysis sessions of
`LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`):

## `legorr-rev.bundle`

The canonical findings bundle. 236 recorded observations — function
decompilations, address lookups, xref queries, and byte reads — each tied to
the target digest, so findings 01–22 can be re-verified: import the bundle into
a session (`import_evidence_bundle`) or inspect the `records` array directly.
No game asset bytes are included; entries reference offsets and produce
analysis text only.

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