# Finding 258 — Frame Overlay, Interface-Method Wrappers, and Deferred-Set Relays

Nineteen more anonymous procedures now carry names grounded in their own
instruction sequences plus static call sites inside named/ported procedures —
including every member of the deferred carry-forward sets from findings 242–257
that `procedure_callers` had misreported as zero-caller. Grounding roots:
`EngineTick`, `GameConfigInit`, `BrightnessSet`, `MainSimStep`,
`SceneLightsBuildFromConfig`, `RayHitScanAndEffectApply`,
`EntityMovementTickScaleAndDeltaGet`, `EntityFlagWordBehaviorEvaluate`,
`VelocitySlotSteerDispatchAndSound`, `ObjectiveVoiceAliasPrepare`,
`RenderStateFlagsApply`, `TerrainViewTileGeometryBuild`,
`ObjectHierarchyRecursiveClone`, `BuildingUpgradeStateChangedEventDispatch`,
plus documented anonymous relays `FUN_0044cfa0` (EngineTick site `0x423688`),
`FUN_00418920` (`BuildingUpgradeStateChangedEventDispatch` site `0x4188ec`),
and the `EngineTick` → `FUN_0043b850` (sites `0x4237c6`/`0x4238d5`) →
`FUN_00406c00`/`FUN_0046ced0` → `FUN_00451bc0` chain. Named/ported callees
include `TransitionControl`, `SystemStateFlags`, `TextBufferAppendFormat`,
`TextBufferDrawWrapped`, `DDiSceneObjectAllocate`, `DDiSceneObjectDestroy`,
`DDiSceneObjectPositionSet`, `DDiSceneObjectPoseMethodDispatch`,
`DDiSceneObjectAttachOrUnlink`, `TerrainGridPick`, `TerrainCellRayHitTest`,
`SurfaceBlitRect`, `RandomFloatRange`, `LavaEffectSelectAndPlace`,
`TypeMotionScaleApplyDispatch`, and `CachedRenderResourceInstanceCreate`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x418a00 | OverlayTextBufferRefresh | Frame housekeeping: re-arms via `FUN_004189d0` when the `FUN_004191f0` handler is dead, returns unless `DAT_004dc650` bits `1`/`2` are set and `SystemStateFlags` lacks bit `0x800`, runs `TransitionControl(0, 1)`, blits `DAT_004db8f8` via `SurfaceBlitRect`, converts/queries the draw buffer (`FUN_00485390`, `FUN_00485230`), then walks the `DAT_004dc4d4` record array to compose the overlay draw. Call site `0x423c16` inside `EngineTick`. |
| 0x45c0d0 | PairCountHistogramBuild | Shifts the `DAT_00500868` slot table down by six pairs, stores `param_1`/`param_2` into `_DAT_0050087c`/`_DAT_00500894`, then builds a six-row pair/count histogram — appending `(value, 1)` entries for unmatched slots and incrementing counts for matches — over the combined table. Call site `0x423ba0` inside `EngineTick`. |
| 0x435b20 | SceneObjectChainRecordCreate | Allocates a `0xbc`-byte record, zeroes its float block, takes the `DAT_00472950` handle, sets its position via `DDiSceneObjectPositionSet` and pose via `DDiSceneObjectPoseMethodDispatch`, and for `param_2 != 3` chains up to four more `DDiSceneObjectAllocate` handles attached through `DDiSceneObjectAttachOrUnlink` (third with z `40.0`), unwinding with `DDiSceneObjectDestroy`/`FUN_00402210` on any failure. Call sites `0x420626`/`0x420649`/`0x42066b` inside `GameConfigInit`. |
| 0x469cb0 | EntityComponentFloatAt20 | Returns the float at component-handle offset `+0x20` (`*(param_1 + 0x334)` dereference). Call sites `0x4466b8`/`0x4466d9`/`0x4466f1` inside `EntityMovementTickScaleAndDeltaGet`. |
| 0x469ce0 | EntityComponentFloatAtD4 | Returns the component float at offset `+0xd4`. Call site `0x446799` inside `EntityMovementTickScaleAndDeltaGet`. |
| 0x469d20 | EntityComponentFloatAtD8 | Returns the component float at offset `+0xd8`. Call site `0x4467c7` inside `EntityMovementTickScaleAndDeltaGet`. |
| 0x46a0a0 | EntitySpeedComponentGet | Returns the component float at offset `+0xf8` — the entity speed value consumed with a 1.0 fallback by `VelocitySlotSteerDispatchAndSound`'s spatial-sound dispatch (finding 256). Call sites `0x43db1c` inside `EntityFlagWordBehaviorEvaluate`, `0x44687c` inside `EntityMovementTickScaleAndDeltaGet`, and `0x471e1a` inside `VelocitySlotSteerDispatchAndSound`. |
| 0x46f770 | EffectApplyByKindBranch | Runs `FUN_004436f0(param_1)` first, then branches on `param_5`: `1` — `FUN_0046a2d0` + `FUN_0046f380` under flag `0x2000000` and `LavaEffectSelectAndPlace(0, param_3, 0)`; `2` — position pair `* 1.5`, `FUN_0046a290`/`FUN_0046a2b0`/`FUN_0046f380`/`FUN_0044c150` under flag `0x4000000` and `LavaEffectSelectAndPlace(1, ...)`; `3` — scale `1.25`/`FUN_0046a310`-gated `FUN_0046a2f0`/`FUN_0044c030` under flag `0x1000000` and `LavaEffectSelectAndPlace(2, ...)`; otherwise `TypeMotionScaleApplyDispatch(param_1, param_4, 1, 0, pos)`. Call site `0x46f541` inside `RayHitScanAndEffectApply`. |
| 0x471630 | RayHitCellDistanceMinimizeSweep | Offsets the `param_1[2]`/`param_1[3]` endpoint pair, `TerrainGridPick`s both into clamped cell coordinates, orders the corners, and walks every cell in the box: `FUN_00431570(cell, ..., param_1[2], param_1[3], &out)` + `TerrainCellRayHitTest` hits recompute the center-to-endpoint Euclidean distance and tighten the sweep record when under `param_1[1]`. Call sites `0x46f4be` inside `RayHitScanAndEffectApply` and `0x4718a7`/`0x471a0d` inside relays `FUN_00471860`/`FUN_00471960`. |
| 0x473460 | LightInterfaceCallSlot40 | Loads the light interface from `*(param_1 + 0xc) + 4`, invokes vtable slot `+0x40` with `param_2`, and answers success when the result is zero. Call sites `0x414da5` (`BrightnessSet`), `0x425a03`/`0x425a59` (`MainSimStep`), `0x4337ea`/`0x433b06` (`SceneLightsBuildFromConfig`). |
| 0x473480 | LightInterfaceCallSlot3C | Same wrapper on vtable slot `+0x3c`. Call sites `0x414dc7` (`BrightnessSet`), `0x425a89` (`MainSimStep`), `0x4337fe`/`0x433b17` (`SceneLightsBuildFromConfig`). |
| 0x4734a0 | LightInterfaceCallSlot38 | Same wrapper on vtable slot `+0x38`. Call sites `0x433ba2`/`0x433bb6`/`0x433bca` inside `SceneLightsBuildFromConfig`. |
| 0x47e300 | InterfaceMethod18StructApply | Zeroes a `0x7c`-size method parameter block (`size 0x7c`, flags `7`, copies `param_2`/`param_3`, flag `0x840`), dispatches device-interface `DAT_0076ac84` vtable slot `+0x18`, reports the `..._Dev_SourceSafe_gods98_dx6...` error string at line `0x474` on failure, then runs the `+0x14` method on `param_3` into a handle and hands it to `FUN_0047e3e0`. Sole call site `0x45876b` inside `ObjectiveVoiceAliasPrepare`. |
| 0x483690 | MatrixMethodPairApply | Copies 17 floats of `param_2`, queries interface `DAT_0050591c` method slot `+0x68`, and when any matrix float differs from the copy invokes slot `+100` with `param_2`, answering failure; unchanged matrices answer success. Sole call site `0x482c59` inside `RenderStateFlagsApply`. |
| 0x485ea0 | InterfaceSlot58PairStoreClamp | Clears `_DAT_005407ec/f0`, adopts positive `param_1` x/y, queries interface `DAT_0076ac8c` slot `+0x58` through a zeroed `0x7c` parameter block, stores the returned pair into `_DAT_005407f4/f8`, and takes the minimum with `param_1[2] + x` / `param_1[3] + y`. Call sites `0x45db3a`/`0x45dba7` inside relay `FUN_0045db00` (reached from `FUN_0043c2b0` site `0x43c3d0`) and `0x45dc8f`/`0x45e3ea` inside `TerrainViewTileGeometryBuild`. |
| 0x489b30 | ObjectInterfaceArrayResourceCreate | Calls the object's vtable slot `+0x80` for the interface-array descriptor, then per entry calls `+0x80` again, `GetInterface(&DAT_0049fb48)` on the object, reads slot `+0x1c`, and feeds each result to `CachedRenderResourceInstanceCreate`. Sole call site `0x489a09` inside `ObjectHierarchyRecursiveClone`. |
| 0x44d280 | RandomWaypointTimerStepApply | Timer-driven waypoint walk on flag `DAT_004eaec8` (only when bit `4` set and `8` clear): countdown `_DAT_004eae3c` draws the next random `RandomFloatRange(DAT_004eae40, DAT_004eae44)` target triple plus a random `0–1` lerp factor into `DAT_004eae18..20`, calls `FUN_0044d710`, and advances `+12`/`+16` velocities either toward (subtract mode, bit `0x10` clear) or through (bit `0x10` set) the targets, cancelling state bits `0x08`/`0x10` on endpoint crossings. Call site `0x44cfc2` inside relay `FUN_0044cfa0`, dispatched by `EngineTick` site `0x423688`. |
| 0x40b000 | TileMatchingCellsCollect | Walks the `DAT_004b8c48` tile-record array (`0x31` stride, 0xf slots per record) and appends every entry matching the `(param_1, param_2)` cell coordinates — with `param_3` matching a non-zero third key field — into the `param_4` result record's `+0x200`-counted slot list, recording record index at `+0`, `+0x80`, `+0x100` and the slot index at `+0x180`. Call site `0x418946` inside relay `FUN_00418920`, reached from `BuildingUpgradeStateChangedEventDispatch` site `0x4188ec`. |
| 0x474760 | InterfaceArrayForwardSync | Picks interface `param_1[0]` (record type `6`) or `param_1[1]`, uses slot `+0x80` to enumerate a handle's object array (`DAT_0076ae24`), forwards each entry through slot `+0xa0` (with `param_3` as count), mirrors the pair onto `param_1[2]`'s child via slots `+0x48`/`+0xa0`, and finally clears or raises `param_1[5]`'s `4` bit — the null path instead re-enumerates and copies `*( *(unaff_EBX + 0xc) + 8 )` selectors through slot `+0x48`. Sole call site `0x451c10` inside relay `FUN_00451bc0` — reached from `EngineTick` through relays `FUN_0043b850` (sites `0x4237c6`/`0x4238d5`), `FUN_00406c00`, and `FUN_0046ced0`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-30.bundle` —
batch decompile record
`ev_34b2943ae4eec65ce70d2ab6788f10b428d3020b5a9215af77e94d265fafa8dd`
(19 addresses) plus one `procedure_callers` record per named address:
(`0x40b000` `ev_16a0a12e…`, `0x418a00` `ev_a05033cc…`, `0x435b20` `ev_e50e75e2…`, `0x44d280` `ev_12f05dc9…`, `0x45c0d0` `ev_c8d3b256…`, `0x469cb0` `ev_032ac613…`, `0x469ce0` `ev_ec36b7f0…`, `0x469d20` `ev_250c1493…`, `0x46a0a0` `ev_643cadac…`, `0x46f770` `ev_0402425e…`, `0x471630` `ev_9f54fae2…`, `0x473460` `ev_8b1ccf6e…`, `0x473480` `ev_a089c790…`, `0x4734a0` `ev_8e5c560a…`, `0x474760` `ev_4e8ec368…`, `0x47e300` `ev_2232736d…`, `0x483690` `ev_8aa9de83…`, `0x485ea0` `ev_48d29d6f…`, `0x489b30` `ev_8dd01e96…`). Grounding cross-references come from
`procedure_references` (direction: incoming), which surfaced the
`UNCONDITIONAL_CALL` sites inside named procedures that `procedure_callers`
alone had not. All investigations used shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x43fc20` stays deferred: its caller `FUN_0041f6c0` is
only reachable through a data reference at `0x41e067` inside
`UiActionDispatch` (indirect dispatch, evidence pending). `0x44fab0` stays
deferred: sole entry is relay `FUN_0044b7a0`'s self-entry. The
explicitly-unresolved `0x4802d0`/`0x480390`/`0x480570`, the CRT-region helpers
(`0x48e280`/`0x48e680`/`0x48e0ad`/`0x48e8e0`/`0x48e4f0`/`0x48ed30`/
`0x465c50`/`0x473560`/`0x489660`/`0x476e50`/`0x4887c0` plus the
0x493–0x49Bxxx cluster), and the data-reference-evidence group stand as before.
