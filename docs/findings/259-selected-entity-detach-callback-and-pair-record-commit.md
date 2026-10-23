# Finding 259 — Selected-Entity Detach Callback and Pair-Record Commit

Two final procedures from the deferred sets now carry names grounded in static
function-pointer evidence inside `UiActionDispatch` and their own instruction
sequences. `UiActionDispatch` cases `0x16`/`0x17` register `FUN_0041f6c0` as a
callback — passed literally to `SelectedEntitiesAnyMatch`, the selection sweep
that invokes it per entity — which resolves the `FUN_0041f6c0` zero-caller
root that kept both addresses deferred in findings 256–258. Named/ported
callees: `EntityCapabilityFlagsGet`, `EntityAttachedObjectListDetach`,
`EntityVisualResourceGetOrCreate`, `DDiSceneMoveOrPose`,
`CurrentLevelHeightGet`, `TerrainGridPick`, `TerrainCellRouteAndOccupancyCheck`,
`EntityTypeComponentGet`, `DDiSceneObjectPositionSet`,
`DDiSceneObjectPoseMethodDispatch`, `EntityActivityToggle`,
`EntityActivityListRegister`, `EntityActivityRequestSet`, and
`SceneActivityUpdateDispatch`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x41f6c0 | SelectedEntityDetachCallback | Per-entity selection callback: when `EntityCapabilityFlagsGet` bit 1 is clear it runs `EntityAttachedObjectListDetach(entity, 0)` unless the `+0x3e0` bit `0x400` holds *and* the `+0x308` record's first word reads `9`; when bit 1 is set instead, the same `+0x3e0` bit plus a non-null `+0x308` pointer dispatches the record word to `RecordPairPoseCopyAndActivityRaise`. Grounded statically as the callback argument of `SelectedEntitiesAnyMatch` in `UiActionDispatch` cases `0x16`/`0x17` (the data reference at `0x41e067`). |
| 0x43fc20 | RecordPairPoseCopyAndActivityRaise | Pair-record commit: takes the paired entity handle at record `+0x304`, resolves its visual through `EntityVisualResourceGetOrCreate` + `DDiSceneMoveOrPose`, converts the position through `CurrentLevelHeightGet`/`TerrainGridPick`, validates the cell pair with `TerrainCellRouteAndOccupancyCheck`, and requires live `EntityTypeComponentGet` components for both entities. Success clears record `+0x304`, target `+0x308`/`+0x324`, and target `+0x3e0` bit `0x400`; copies the target component's position (`DDiSceneObjectPositionSet`) and pose (`DDiSceneObjectPoseMethodDispatch`) onto the requesting entity; runs `EntityActivityToggle(target, 1)` and `EntityActivityListRegister(target)`; dispatches `EntityActivityRequestSet(entity, 0x34, 0)` + `SceneActivityUpdateDispatch(entity)` + `EntityActivityListRegister(entity)`; and records the target at `+0x2fc` with the `+0x3ec` bit `0x8000` set. Sole static caller `SelectedEntityDetachCallback`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-31.bundle` —
decompile records
`ev_4ae9259f…` (`0x41f6c0` with `0x44b680`) and
`ev_1bbb72e4…` (`0x43fc20` with `0x41f420`), plus `procedure_callers`
records `ev_b9bd6f3e…` (`0x41f6c0`, empty as expected for an indirect-entry
callback) and `ev_4ac0611a…` (`0x43fc20`, sole caller `0x41f6c0`). The
function-pointer literal in `UiActionDispatch` is quoted above. All
investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x44fab0` remains deferred — its sole entry is relay
`FUN_0044b7a0`, reachable only from an instruction address inside itself
(`0x44b879`); no named procedure or function-pointer reference reaches it.
`0x4802d0`/`0x480390`/`0x480570` stay explicitly unresolved. The CRT-region
helpers (`0x48e280`/`0x48e680`/`0x48e0ad`/`0x48e8e0`/`0x48e4f0`/`0x48ed30`/
`0x465c50`/`0x473560`/`0x489660`/`0x476e50`/`0x4887c0`, plus the
0x493–0x49Bxxx cluster) are runtime/CRT support, not game logic. Every
remaining 900–6900-byte anonymous procedure is accounted for in these
deferred sets.
