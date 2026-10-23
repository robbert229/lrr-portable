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

**Follow-up.** `0x44fab0` now has the mechanical name
`TerrainCellSurfaceTriangleVerticesSelect` (finding 266). Its callback reaches
named entity/scene update paths, although the direct call remains inside a code
region without a Ghidra procedure boundary and the threshold's exact meaning
is not known. The sibling `0x44b7a0` remains deferred: its only direct call is
at `0x44b879` inside the same unregistered callback zone. The token/directory
helpers `0x4802d0`/`0x480390`/`0x480570` now have mechanical names in finding
265; `0x480570`'s result remains unused by its caller. The CRT helpers listed
above and the 0x493–0x49Bxxx cluster are runtime support rather than game
logic, with new behavioral names recorded in findings 260, 261, and 264.
