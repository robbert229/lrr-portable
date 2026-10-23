# Finding 252 — Config-Band Texture/Name Loaders, Route Command Issues, and Surface/Menu Helpers

Seventeen more anonymous procedures now carry names grounded in their own
instruction sequences, each with at least one named or ported caller:
`GameConfigInit`, `EntitySceneObjectTreeAttach`, `VehicleTypeDefinitionParse`,
`TerrainCellMutation`, `EntityFenceSparkTerrainInteraction`,
`DependencyGridConfigLoad`, `BuildingUpgradeStateChangedEventDispatch`,
`SharedObjectAnimationAndTimedEventUpdate`, `MenuTick`, `PendingLevelName`,
`TextMessagesConfigLoad`, `StepMovementVectorResolve`,
`EntityAdjacentPlacementCellSearch`, `RouteStepCommandIssue`,
`EntityActivityRecordRemoveByType`, `KindNineNeighborEmitForFootprintCells`,
`SimulationPointerActionDispatch`, and `QuadMeshRenderRecordCreate`, plus
named callees `ConfigEntryFind`, `StringDuplicate`, `BitmapSurfaceLoad`,
`ImageSurfaceBlitTransformed`, `TerrainSurfaceHeightInterpolate`,
`MapCellToWorldCenter`, `CurrentLevelHeightGet`, `TerrainGridPick`,
`TerrainCellSurfaceVerticesGet`, `ParseIntegerWrapper`,
`TerrainCellWalkableFlagCheck`, `TerrainCellOccupancyOrFlagCheck`,
`TerrainNeighborPathCheck`, `EntityRouteSetupAndMarkerAssociate`,
`EntityNearestConnectedTerrainCellRouteSelect`, `CatalogCallbackByIdDispatch`,
`RandomFloatRange`, `AnimatedTextureFrameSurfaceSet`,
`RenderObjectColorVectorSet`, `SurfaceBlitRect`, `CmdLinkHead`,
`ObjectTypeNameIndexFind`, `EngineHeapAlloc`, `EngineHeapFree`,
`EngineHeapAllocateWithRetry`, `SharedObjectClone`, `SceneRenderLockToggle`,
`TerrainCellSurfaceVerticesGet`, and `SurfaceTriangleNormalCalculate`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x403750 | CommandKindRecordIssueAndLink | Allocates the kind-`7` command record (`FUN_00406440`), writes `param_1/param_2/param_3` plus the `*param_4` pair into `+0x20…+0x2c/+8/+c`, fills the `+0x30/+0x34` random-jitter pair with `RandomFloatRange` when `*param_6` is null, raises `+0x5c` bit `0x800` for nonzero `param_7`, links the record with `CmdLinkHead(rec, 5)`, raises bit `0x400`, and dispatches the `FUN_00403810(param_4, …)` follow-up. Callers `EntityActivityRecordRemoveByType` + `KindNineNeighborEmitForFootprintCells` + `FUN_00408e50`. |
| 0x407240 | BubbleConfigTextureLoad | Walks the `Bubbles` config children (`ConfigEntryFind(DAT_005560c4, …)` + `EntryFirst/Next`): drops entries flagged with the `!` prefix unless `DAT_00505f84` bit `0x2000000` suppresses them (the stripped name is then used), maps the entry name through the `FUN_00407300` slot index, loads `BitmapSurfaceLoad(entry+8)` into `DAT_00557864[slot]`, and initializes loaded surfaces via `ImageSurfaceBlitTransformed(handle, 0, 0, 0, 0, 0, 0)`. Sole caller `GameConfigInit`. |
| 0x40a5e0 | RecordCountDigitsFill | For the `FUN_0040a5c0` render record: sets the four quad-vertex color pairs through `RenderObjectColorVectorSet` — white/white for counts under 5, white/1.0 under 10, else the `0.5` pair — then computes the decimal digit count of `param_2` and assigns the `DAT_004b8a58[digits % 10]` digit-texture surfaces to slots 0..(digits−1) via `AnimatedTextureFrameSurfaceSet`, ending with the `FUN_004827a0(slot, 1)` marker per slot and `record + 8 = digits + 1`. Sole caller `QuadMeshRenderRecordCreate`. |
| 0x40b1b0 | DependencyGridOccupancyPass | Sweeps the dependency grid (`DAT_004b8c48/c4c`, 15×16 blocks per row group, 300 row groups): for each still-unswept cell (`+flag` bit 8 clear), when the slot-1 neighbor word is occupied or `FUN_0040aea0(x, y, z)` reports dependency, sets the cell's bit 4; when the dependency resolves from the bit-4 state instead, swaps to `clear 4 \| set 8`. Called by `DependencyGridConfigLoad` + `BuildingUpgradeStateChangedEventDispatch`. |
| 0x40e0c0 | FenceSparkLaunchRegister | Converts the cell `(param_3, param_4)` center through `MapCellToWorldCenter` + `TerrainSurfaceHeightInterpolate`, converts the `(param_2[0], param_2[1])` vector the same way, forms the cell delta, computes the launch velocity from the interpolated height minus half the mass (`FUN_00469d50`), pushes the trajectory through `FUN_0040c2f0(0, base, delta…)`, resolves the `FenceSpark` type index through `ObjectTypeNameIndexFind(s_FenceSpark, 1, 0, &delta)`, registers the object on the entity (`FUN_0046f270`), and refreshes the entity (`FUN_00442210`). Sole caller `EntityFenceSparkTerrainInteraction`. |
| 0x40f6c0 | SceneNodeRecordCreate | Allocates the 0xa0-byte node record via `EngineHeapAlloc`: zeroes the counter slots, pins slot 8 to `1.0`, copies the `param_2` position triple into `+0/+4/+8` (zeros when null) and the `param_1` triple into `+12/+16/+20`, duplicates `param_3…param_6` into the `+14/15`, `+16/17`… pairs, stores `param_7` at `+38`, and returns the record. Callers `EntitySceneObjectTreeAttach` + `FUN_0040f7d0`. |
| 0x412af0 | MenuSurfaceBlitScaled | Chooses the overlay surface from `record + 0x14` (flag clear) or `record + 0x10`; when `+0x34` bit 1 is set, blits it at the record's own `+0x38` origin, otherwise at the negated `DAT_005574dc/e0` screen offset with the fixed `160.0 × 480.0` scale — the scaled-menu blit path. Sole caller `MenuTick`. |
| 0x41a920 | LevelStateCountersClear | Level-start reset: zeroes `DAT_004de1f8`, the 74-entry `DAT_004dd9ac`/`DAT_004dd60c` tables, the `DAT_004ddad4/ddad8/dd9a4/dd9a8` arrays sized by the `DAT_0055627c`/`DAT_00556288` entity-list counts, and the 300-entry `DAT_004ddd34` table, then runs `FUN_0041a990`. Sole caller `PendingLevelName`. |
| 0x41c1c0 | AdvisorOverlayHitResolve | Maps the `(param_1, param_2)` screen point against the active overlay's transformed rectangle: takes the slot origin through `FUN_0041b3c0`/`FUN_0041b470` for `DAT_004ddccc`, checks both axes against the `+ DAT_004dce64/68` origins and the `DAT_004dcec4/c8` extents offset by `DAT_004ddcd0/d4`; inside the rect answers `*param_3 = 0`; otherwise runs `FUN_0041b680` with the `LAB_0041c310` callback and copies the resolved `(x, y, extra)` triple out. Sole caller `SimulationPointerActionDispatch`. |
| 0x427f80 | LevelUpgradeNameStringsLoad | Walks the `UpgradeNames` config children: for entries whose key case-insensitively starts with `Level`, parses the trailing integer with `ParseIntegerWrapper` and, when the value is under 16, duplicates the entry's path string (`StringDuplicate(entry + 8)`) into `DAT_00556f58[value]`. Sole caller `GameConfigInit`. |
| 0x4455a0 | EntityMoveVectorSurfaceProbe | Bails when the entity cell fails `TerrainCellWalkableFlagCheck`; normalizes the entity's velocity (`EntityTransformPositionGet` pair at `+0x2ac/+0x2b0`), scales it by `CurrentLevelHeightGet`-derived surface scale `×1/3`, adds it to the entity position, re-picks the surface (`TerrainGridPick`), and succeeds only if either the walkable or occupancy check passes at the new cell. Sole caller `StepMovementVectorResolve`. |
| 0x4473b0 | RouteToCellCommandIssue | For an entity carrying `+0x3e0` bit `0x400` and not busy (`FUN_0043baf0 == 0`): reads the target entity `param_4`'s cell, and when it differs from `(param_2, param_3)`, issues the straight route (`EntityRouteSetupAndMarkerAssociate`, kind 1) if the cells match, else runs the neighbor-route search (`TerrainNeighborPathCheck`) and issues from its endpoint; writes step byte `5` into the step array, records the target handle in the `+0x308` record's `+0x2fc` slot, and returns success. Sole caller `EntityAdjacentPlacementCellSearch`. |
| 0x449880 | BlockedCellApproachWeightWrite | After the `TerrainCellWalkableFlagCheck` / `EntityNearestConnectedTerrainCellRouteSelect` gate, converts `param_4` by the surface scale (`CurrentLevelHeightGet` + `FUN_00450890`), initializes both step-axis weights to `1.0`, and writes the scaled value into the `−8`/`−c` step fields (as `fVar4` or `1 − fVar4`) selected by the `param_3` edge index, ORs the step byte with `8`, and reports success. Callers `RouteStepCommandIssue` + four anonymous ones. |
| 0x46ab10 | TextSlotMessageAssign | Frees the message slot's current string (`EngineHeapFree`), heap-copies the new `param_2` with `_` → space and `\n` escapes decoded into newlines into `DAT_005031f8[slot]`, and invalidates the slot's bitmap-text surface (`ImageSurfaceRelease`, clear) at `DAT_00503260[slot]`. Callers `TextMessagesConfigLoad` + `FUN_0046abb0`. |
| 0x46e220 | CellSurfaceQuadHeightFill | Resolves the cell surface record (`FUN_0046e8a0`), clears `+0x424` bit 2 / sets bit 1, turns `SceneRenderLockToggle` off for the render object, activates the cell-indexed quad slot (`+0x324 + cell*0x18`): `+0x14 = 1`, `+0xc/+0x10 = 0`, resolves the surface vertices with `TerrainCellSurfaceVerticesGet(__ftol(...))`, and stores the minimum of the two indexed triangle corner heights (`local_50` pair index table) into the slot's `+8`. Sole caller `TerrainCellMutation`. |
| 0x474f10 | SceneObjectBoundsUnion | Reads the object's child array (vtable `+0x1c`) under the `+0x34` bounds call on the parent, then for each child takes its vtable `+0x34`-reported box (three low/high float triples) and tightens the six bounds floats in `param_2` to the child minima/maxima. Sole caller `VehicleTypeDefinitionParse`. |
| 0x488170 | AnimationTrackKeysInterpolate | Takes the keyframe record at `+0x18` with the key pair `(param_3 & 0xffff, +1)` (stride `0x28`): lerps the position triple, the three-component second triple (quaternion), and the third triple by `param_4`, and applies the composed `(position, ?, orientation)` triples through `FUN_004882e0`. Sole caller `SharedObjectAnimationAndTimedEventUpdate`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-24.bundle` —
batch decompile record
`ev_e959cb3ca6814716839bb8ec7d3d52040fb96fb1a4a1f11f7482fda4eb8cecfc`
(19 addresses: the 17 named picks plus the examined-and-deferred
`0x4887c0`/`0x48e280`), plus one `procedure_callers` record per named address
(`0x403750` `ev_dc5ca857…`, `0x407240` `ev_68a38ecb…`, `0x40a5e0`
`ev_8d23b924…`, `0x40b1b0` `ev_baeceb72…`, `0x40e0c0` `ev_be7f6748…`,
`0x40f6c0` `ev_12f25f50…`, `0x412af0` `ev_7f0b5f5c…`, `0x41a920`
`ev_f28098d3…`, `0x41c1c0` `ev_8171292f…`, `0x427f80` `ev_5d70d5cd…`,
`0x4455a0` `ev_40eabdcd…`, `0x4473b0` `ev_f1876083…`, `0x449880`
`ev_16e4f41b…`, `0x46ab10` `ev_536292e4…`, `0x46e220` `ev_31217350…`,
`0x474f10` `ev_1b1c7596…`, `0x488170` `ev_77cffa53…`). All investigations used
shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The command-region trio `0x43a120`, `0x448c90`, and
`0x4490a0` (static call sites still resolve `not_in_procedure`), and the
steering-slot pair `0x471f00`/`0x471d20` (instruction-level facts on record:
the slot-base accessor `0x4712c0` reads only its first argument — kind 1 →
`entity[3] + 0x138`, kind 4 → `entity[6] + 0xa0`, else 0). Newly examined and
deferred: `0x4887c0` (recursive scene-object-tree registration that reads the
device-interface record through the `DAT_00505914` vtable `+0x10`, conditionally
resets a transform stack through vtable `+0x38`, links the child handle into
the parent `+0x14` slot table, formats and character-class-sanitizes the
`"%s_%s_%d"` name into vtable `+0x20`, and recurses the `+0x30` child chain;
the vtable slots themselves are unidentified) and `0x48e280` (a
precision/FPU-control-word-dependent float↔double conversion that calls
`FUN_00493150/00493170/004931c0/00493a80` from the CRT math band; the
game-side callers `SurfaceBlitRect`/`ManagerTick` hit it as incidental
arithmetic). Zero-caller band addresses still need indirect-dispatch evidence
before naming: `0x476410`, `0x444460`, `0x463d20`, `0x44b500`, `0x41c800`,
`0x43aab0`, `0x439a20`, `0x4440f0`. These carry forward the unresolved set
from findings 242–251 (`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`,
`0x44d280`, `0x473460`/`0x473480`/`0x4734a0`, `0x40b000`, `0x474760`,
`0x483690`, `0x471630`, `0x46f770`, `0x435b20`, `0x465c50`/`0x473560`/`0x489660`,
the CRT cluster 0x493–0x49Bxxx, and the entity scalar accessors
`FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The anonymous-chain band entries
`0x40f920`/`0x43fc20`/`0x448eb0`/`0x40b880`/`0x44c7f0`/`0x410490`/`0x40ba00`/`0x40bb90`
and the remaining band addresses (roughly 77 after this batch) stay queued.
