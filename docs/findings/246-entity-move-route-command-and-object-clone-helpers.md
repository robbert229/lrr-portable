# Finding 246 — Entity Move/Route Commands, Cell Queue, and Object Clones

Thirteen more anonymous procedures now carry names grounded in their own
instruction sequences. Eight were the entity/clone band deferred from
finding 245; five were surfaced by a deeper ledger band scan (0.9–2.2 KB
pseudocode) and have a ledger-named or ported caller directly: `MenuTick`,
`AdvisorOverlayUpdate`/`AdvisorCallbackScheduleGate`,
`PathStepAdvanceAndArrivalCheck`, `CellInitPass`, and the batch-6 surface
helpers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4124f0 | MenuSaveImageConfigRectBlit | Runs only for the focused window (`*DAT_00556fc4 + 0xc` or the secondary `*DAT_00556fcc`) and a nonzero image slot from `DAT_004db890[param_2]`; formats `"pos_%d"` per slot and reads the `"pos_%d"`, `"Width"`, `"Height"` keys of the `"Menu::SaveImage"` config section (`ConfigEntryFind` + `ConfigPathValueGet`, `DAT_004b8a14` fallback) split through `BlockTextTokenSplit`; with `param_4` nonzero the rect is retargeted against the view box `DAT_0055750c/510` using the `0.25`/`-0.5` factors, then blitted with `SurfaceBlitRect(image, 0, rect)`. Sole caller `MenuTick`. |
| 0x41b680 | CatalogCallbackByIdDispatch | Callback dispatch per catalog id: id `0x13` loops `DAT_00556288` entries calling `callback(0x49, 4, i, param_3)` until nonzero; ids `0x14`/`0x15` loop `DAT_0055627c` records gated by `FUN_00469ee0`/`FUN_00469f00` flag bits `0x1000`/`0x2000`/`0x800`/`8` calling `callback(0x49, 1, i, param_3)`; any other id iterates the id table `&DAT_004ddc04[param_1*2]` (size `&DAT_004ddc08[param_1*8]`), mapping ids `0x22`/`0x23` through `FUN_0041b9c0` and calling `callback(id, 0, 0, param_3)`. Callers `AdvisorOverlayUpdate`, `AdvisorCallbackScheduleGate`, and `FUN_0041c1c0`. |
| 0x433c10 | EntityNeighborBlockedCellStepProbe | With entity flag bit `0x1000` set, `+0x3e0` bit 1 clear, and `FUN_0043baf0` reporting no active sequence, every `1/random` tick reads the entity cell with `EntityPositionToTerrainCell` and tests the four cardinal neighbor cells (`(-1,0) (1,0) (0,-1) (0,1)`) against in-bounds cell records for flag `& 0x10` without bit `8`; a hit writes `-1.5` to `+0x340`, calls `FUN_00449700`, and returns 1. A second gated branch (`param_3` nonzero or flag bit `0x400`) calls `FUN_00444260`. Sole caller `PathStepAdvanceAndArrivalCheck`. |
| 0x441ed0 | EntityFaceTargetActionStart | Gates on range `+0x360 > 0` or `DAT_00556ed0 & 4`, a type-`3` target record (`*param_2 == 3`), and `FUN_0043baf0` reporting no active sequence; runs `EntityActionSequenceReset(entity, 0)`, reads both world poses through `EntityTypeComponentGet(...,0,&buf)` + `DDiSceneMoveOrPose`, lowers each end by `0.5 * FUN_00469d50`, normalizes target-minus-entity into a direction, and writes it via `FUN_00442300(entity, 0, dir)`; `param_3` 1/2/3 each fires `EntityActivityRequestSet(entity, 0x2c, 1)` + `SceneActivityUpdateDispatch` with flag `+0x3e4 |= 0x400000`/`0x800000`/`0x1000000`, then stores the target point (`+0x3a8/+0x3ac/+0x3b0`) and direction (`+0x39c/+0x3a0/+0x3a4`). Sole caller `FUN_0043c4c0`. |
| 0x446e40 | EntityNearestCellRouteCommit | After `TerrainCellWalkabilityQuery(entity, x, y, param_4, 1)` accepts, `EntityNearestConnectedTerrainCellRouteSelect` fills a 33-entry route table; for `param_4 != 0` the table is overwritten with fixed step/flag entries (`1/2/3` triples) and the `0.9/0.5/0.1` float constants, the final route entry is committed through `FUN_004468c0` (addressed via the table's link indices), and the scene-node cell coords (`[entity+0x24] + node*0x14`, offsets `-0xc/-8`) are written from table entries `0x11/0x12`; the target lands in `+0x2f4/+0x2f8` with flag `+0x3e8 |= 0x2000000`. For `param_4 == 0` it commits `FUN_004468c0(entity, x, y)` directly and jitters the two node coordinates with `RandomFloatRange(0.3, 0.7)` while clearing flag bit `0x2000000`. No static callers (command-table dispatch). |
| 0x44bc30 | EntityMotionToSceneNodeSync | Under entity flags `& 0x200000 && ! 0x400000` (with `FUN_00473ba0` falling back to `FUN_00449f50(entity, 1)`), adds `param_2` to the scene-node clock when node `+0x24` bit 1 is set, fires `EntityCommandUnlinkByKind(entity, 1, 0, 1)` once `FUN_0046a250(entity) <= dt` with flag bit `0x80` clear (clearing node bit 1), mirrors the entity pose into the node through `EntityTypeComponentGet`/`DDiSceneMoveOrPose` with the y term negated — writing via `FUN_00410020/00410070/004100b0` for a null `FUN_004897d0` node — dispatches `FUN_0044bb30(entity, 0/1)` from flag bit `0x80`, and finishes with the node writers `FUN_0040fdb0` (position), `FUN_0040ff50` (scale from `FUN_0046a190`), `FUN_0040fe00` (dt), and `FUN_0040fe90` callback registration at `LAB_0044b850`. No static callers. |
| 0x448a60 | EntityNeighborCellMoveCommandHandle | Handles command record kind `0xb` (`param_1[0xc2]`): requires `TerrainCellHasOccupiedOrFlaggedNeighbor(target)`; when the entity already stands on the target cell it calls `EntityRouteSetupAndMarkerAssociate(entity, 1, x, y, 0)`, otherwise `TerrainNeighborPathCheck` from the current cell (`EntityPositionToTerrainCell`) to the target and then `EntityRouteSetupAndMarkerAssociate`, freeing both path lists with `EngineHeapFree`; success marks the node byte at `param_1[9] - 3 + param_1[10]*0x14` with `9`, and for entity type 1 also jitters the two node coordinate slots via `RandomFloatRange(0.55, 0.75)` with the `0.5`/`1.0` corner table and node byte `| 8`, finally writing the target into `+0x2f4/+0x2f8`. No static callers. |
| 0x450960 | CellDirtyQueuePushAndClear | Cell dirty-queue at `+0x4a8a` (x) / `+0x4a88` (y) with count `+0x7198` and cell-record flag bit `0x10`: `param_4 != 0` appends `(x,y)` as shorts and raises bit `0x10`; `param_4 == 0` scans the queue for `(x,y)`, overwrites the found entry with the last queue entry, decrements the count, and clears bit `0x10` (`&= 0xef`). Cells without the flag are ignored. Sole caller `CellInitPass`. |
| 0x465f40 | InstanceQuadVerticesBuild | Per instance entry `param_1 + param_2*0x3c`: takes the plane of `param_3` (`FUN_00476f70`), derives its basis vectors (`FUN_00475500`), rotates the `(0,0,-1)` origin axes through `FUN_00479510(..., 0.7)` when the plane offset `local_30 > 0.3`, scales both bases by `entry[+0x38]/entry[+0x34] - 1.0` times `entry[+0x28]/[+0x2c]` offsets, builds four corners around the center `entry[+0x1c/+0x20/+0x24]`, and hands them to `FUN_00482500(record+0x290, index, 0, 4, verts, param_4, flags)` with the default 4×4 flag matrix. No static callers. |
| 0x471a60 | EntityVelocitySteerBlendUpdate | Stores the desired vector into the `+0x68 + slot*0xc` table when `param_4` is set, then for a live object (`FUN_00471a40`, `1e-05 < param_3`) reads the current position via `DDiSceneMoveOrPose`, normalizes target-minus-position, scales the existing velocity by `5.0 / dt` plus that unit vector, renormalizes, clamps the `+0x4c` component at the `+0x30` cap, and dispatches the new velocity with `DDiSceneObjectPoseMethodDispatch(obj, 0, vx, slot+6 entry, vz, 0, 0, -1.5)` — including the `FUN_00471a50` secondary object when present. Callers `FUN_00471d20`/`FUN_00471f00`. |
| 0x47d740 | SurfacePixelRowsFlipWithScratch | Flips the surface buffer vertically: computes the row bytes from `(param_1[+0x10] / 2) * 2`, keeps the last-row pointer, and for each of `rows/2` row pairs moves the first row to a row-sized `EngineHeapAlloc` scratch, the last row over it, and the scratch to the last row, ending with `EngineHeapFree`; the loop walks both pointers by the row size. Called by `Palette8To16BitSurfaceExpand` and `Bitmap24BitSurfaceConvert`. |
| 0x487a00 | SharedObjectClone | Allocates a fresh 14-dword record, zeroes it, copies the source record's 14 dwords (preferring `param_1[0xb]` when set), raises the clone count `+0x30` to 1, and chains the source into `+0x2c`; children are duplicated through `FUN_004887c0` walking the `+0x30` link, then per component entry the template/clone vtable slots `+0x5c`/`+0x74` and clone `+0x38` are invoked, and entries without flag bit 1 are rebuilt from the template array `param_1[5][idx]` (vtable `+0x80` queries) with the instance created through `CachedRenderResourceInstanceCreate`; the clone closes with `SharedObjectAnimationAndTimedEventUpdate` and the source's `+0x30` counter increment. Caller `FUN_00489660`. |
| 0x4899f0 | ObjectHierarchyRecursiveClone | Recursive clone: after `param_3`-gated bookkeeping through `FUN_00489b30`/`FUN_00489bc0`, copies the object state via the `(*obj + 0x24)()` reader into an allocated buffer and the `(*src + 0x20)()` writer pair, reads `(*obj + 0x5c)()`/`(*obj + 0x74)()`; when `(*obj + 0x4c)()` reports no live copy it enumerates the child list (`(*child_list + 0xc)()` count), duplicates each child (`+0x10` vtable call), spawns it with `DAT_0049f988` as context, adds it to the `DAT_00505914` registry (`(+0x10)`), and recurses `FUN_004899f0` with `DAT_0049f988`. Callers `FUN_00489660` and itself. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-18.bundle](../evidence/legorr-engine-helpers-expansion-18.bundle):
`ev_06f2b084f4040c8617a197dd5979daa53a7f1d1af8f3bb2d0ca28a61041b607d`
(batch decompile of all thirteen addresses). Caller observations are the
matching `procedure_callers` records: `ev_426f555b2ace577b913a7a8f0a877b67e8b16625321c0a6d4f46f97d8eb4fec0`
(0x4124f0), `ev_9fcfd0af64cd50d995cabb2b5cb708b7beaec4e5c9efbff6611a61bb810c5e85`
(0x41b680), `ev_84b23746271b1fdcbe6a835ff4e10b4299663de515d28bf00254ff9142c84e95`
(0x433c10), `ev_d2efd329e122bb519c2ee39029aaa839bdffbc36adc71ca0a63f8ff313cec6f6`
(0x441ed0), `ev_e4ae56e1ba685bd18bc3a30757156d46ddc8f048816d7260f09a525fc4f0af36`
(0x446e40), `ev_caa2e1dcc7d0562ed300e402999add2e35aa14f5767dffd6a768d5c3ed0ba7cc`
(0x448a60), `ev_ee9366816ace83f588c3c58afc256781035d8ddc6dade87d6c7bd6f69fb9d1ae`
(0x44bc30), `ev_a8aebca592bdbd8e4e917c2894ae708e81e43b529edb4199c69d64f3f84df7b7`
(0x450960), `ev_ff742eb4bc286fbe1569a684adab58b43df2bcc2c49a12451be6a1e1dc9ffa01`
(0x465f40), `ev_6e12959843d1437f973984e8cab97cff5452e026bc9e0acf6ade0610f9908299`
(0x471a60), `ev_d68982e4f922d4196c8306aeebbf93d18d93507b4b0736142fa000fc98d61341`
(0x47d740), `ev_23a0c3b1973f101a4d22221ff60072c56a2432a1df87524fc1babb254134f47a`
(0x487a00), and `ev_5bb12e2d3ee542e1c1cc6a766eb5a55623cdaba502a533a048f3310e69ce6a70`
(0x4899f0). Caller identities were read from each record's `raw_result.value`.
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x40b000` (sector-table positional record collect,
domain term not evidenced) and `0x474760` (pure vtable `+0x48/+0x80/+0xa0`
indirection) keep their status, as does the CRT cluster 0x493–0x49Bxxx, the
entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`, the pair-list
builders, `DAT_00544858`, and the unresolved set carried from findings
240–245 (`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`). The band scan surfaced thirteen further
candidates queued for the next batch by caller proximity to already-named
code: 0x4334f0, 0x45d8b0, 0x44dde0, and 0x41afd0 (called from
`GameConfigInit`); 0x44a5d0 (`LevelDebugInputCellMutatorDispatch`,
`EntityConditionalTypeFiveSpawn`); 0x409d50 (`ObjectiveSidecarAppendGate`);
0x453da0 (`TutorialScriptCommandDispatch`); 0x45bcd0
(`SimCursorInteractionTick`); 0x40c990 (`MiscObjectResourcesLoad`); 0x481ac0
(`BindModelTextures`); 0x40b470 (`TerrainProjectedCellStateRefresh`);
0x4915a0 (`EnginePoolAllocate`); and 0x471d20/0x471f00 (steering dispatch
pair calling `EntityVelocitySteerBlendUpdate`). Caller-less band entries
0x43a120, 0x440a10, 0x448c90, and 0x4490a0 remain queued behind them.
