# Finding 249 — Config-Band Loaders, Poly-Variant Build, Panel/Cell Registration, Layout, and Activity Helpers

Eighteen more anonymous procedures now carry names grounded in their own
instruction sequences. Two are the remaining `GameConfigInit` band candidates
(`GameConfigInit` is their sole caller, which grounds the config-loading
family while each name comes from the function's own section strings). The
rest are band candidates whose names and behavior are grounded by their
callers: every one of them is called only from procedures that already carry
a name or port (`BuildingPlacementPreviewUpdate`, `TerrainCellFlagCleanup`,
`TerrainCellNeighborFloodWalk`, `MenuBuild`, `HelpWindowResourcesInit`,
`AdvisorOverlayUpdate`, `TypeTableResourceRowLoad`, `VehicleTypeDefinitionParse`,
`SelectionCandidateFilter`, `StepMovementVectorResolve`,
`BuildingPlacementFootprintRenderAndValidate`,
`PendingCommandEntityAssignmentTick`, `RewardStatsPanelDraw`,
`PanelButtonLayoutConfigLoad`, `SoundStreamSelectorUpdate`,
`MeshBuildEightSidedSegment`, `MaterialPass`, and the ported tickers
`44cfa0`/`46f550`), plus named callees such as `CellInitPass`,
`EntityTeardown`, `EngineHeapAlloc`/`EngineHeapGrow`, `CurrentLevelHeightGet`,
`TerrainSurfaceHeightInterpolate`, `CampaignLevelFallbackLabel`,
`TerrainCellRouteAndOccupancyCheck`, `TerrainCellOccupancyOrFlagCheck`,
`MapCellToWorldCenter`, and `EntityActivityRequestSet`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x409790 | CellPathRecordRegisterApply | Registers a cell-path record: takes a registration slot from `FUN_00409350(param_2)`, stores `param_1`/`param_3`, allocates `param_5*8` bytes with `EngineHeapAlloc` and copies the `param_4` cell-pair array, runs the pair list through `CurrentLevelHeightGet` (`FUN_00431570`, count + `0x40800000` constant) into `TerrainCellListSurfaceHeightClampMutate`, then walks the pairs: for every cell record (`DAT_005560d4` map, `0x48` stride) raises flag `0x100000` and stores the record pointer at `+0x10`, raises `0x8000` as well on consecutive-duplicate pairs, and calls `CellInitPass` (`FUN_0042f6f0`), `FUN_00409940`, and `FUN_0041cfb0` per cell; returns the record's `+2` field. Caller `BuildingPlacementPreviewUpdate`. |
| 0x40bfd0 | RockFallStylesConfigLoad | For each `RockFallStyles` section entry (`ConfigEntryFind` + `ConfigNestedPathChildFind` + `SortedEntryNextEqualKey`): duplicates the resource string (`StringDuplicate`) into `DAT_004c7990[DAT_004c79a0]`, copies the entry value, tokenizes it at `DAT_004a13e4` (`BlockTextTokenSplit`), registers every token after the first with `FUN_0040c0d0(token, DAT_004c79a0, index-1, name)`, and bumps `DAT_004c79a0`. Sole caller `GameConfigInit`. |
| 0x40c3a0 | ResourceAvailabilitySweep | Calls the four `FUN_0040c830`/`c4d0`/`c520`/`cc40` reset helpers, walks three `0xa4`-stride slot groups (base index `DAT_004c79a4*3 + 0..2`, 4 slots each) of the `DAT_004c71e0` table: each unused slot whose `DDiSceneAdvanceByTime(id, param_1)` returns `> 0` is registered with `SceneRenderLockToggle(id, 1)`, marked used, its `+0x18` word zeroed, and its `(group, slot)` appended to the `DAT_004c7180`/`DAT_004c71b0` output lists, returning the count; then sweeps the `DAT_004c7c74`–`DAT_004c7e5c` `0x3d`-stride block table the same way (id at `-0x1e` before the used flag, 30 slots per window) without collecting. Callers `FUN_0040bdc0` and `TerrainCellFlagCleanup`. |
| 0x40dc20 | CellPairEdgeEffectApply | Classifies the `(param_3, param_4)` offset from `(param_1, param_2)` against a stack-built four-neighbor offset table to derive a 0/1 direction selector (0 when no match), collects edge parameters through `FUN_0040dd90`, converts both cell coordinates to world centers (`MapCellToWorldCenter`), samples the surface height (`TerrainSurfaceHeightInterpolate`), and issues `FUN_0040c2f0(direction, x, y, height, params)` for the matched edge. Sole caller `TerrainCellNeighborFloodWalk`. |
| 0x411100 | MenuSectionRecordCreate | Allocates a zeroed `0xa0` record with `EngineHeapAlloc`, resolves the two name strings to ids (`CampaignLevelFallbackLabel`; gives up on a zero id), stores `strlen(param_1)-1`, six integer parameters at `[3]`/`[0xb]`/`[0xc]`/`[0x11]`/`[0x12]`/`[0x16]`, the `0xf` grid constant, a `(param_9 != 0) ? 2 : 0` selector, a `0x3c`-byte sub-allocation (freeing the record via `FUN_00410f30` on failure), and, when `param_10` is set, tokenizes it at `DAT_004a1cd4` and stores two integer conversions (`ParseIntegerWrapper`) at `[0x14]`/`[0x15]`. Sole caller `MenuBuild`. |
| 0x4185f0 | HelpWindowButtonLayoutLoad | Reads the `HelpWindow/ContinueButton` and `HelpWindow/CancelFutureButton` config records (`ConfigEntryFind` + `ConfigStringCopy`), tokenizes each value at `DAT_004a13e4`, converts the first four tokens (`ParseIntegerWrapper`) into position globals (two with the `_DAT_004db8fc`/`_DAT_004db900` offsets added), loads two texture handles (`FUN_0047d9d0`) into `_DAT_004db914`/`_DAT_004db918` and `_DAT_004db930`/`DAT_004db934`, and passes the last token through `FUN_00477590` into `UiTextEntryLoad` slots 8 and 9. Sole caller `HelpWindowResourcesInit`. |
| 0x41eca0 | AdvisorDialogStackLayout | Detects level/position changes (`DAT_004ddcd8`/`DAT_004ddcdc` vs `DAT_004a2b5c`/`DAT_004dcd50`, reset via `FUN_0041ee60`), then when `DAT_004de1f8` flag `0x800` is set collects the dialog list (`FUN_0040af90` giving count and `(entity, cell)` pairs) and stacks the dialogs bottom-up: below the header text (`DAT_004de1e8`/`DAT_004de1e4` height or a fixed 20 px) each dialog's entity record (`FUN_004377b0`/`FUN_0040af40`) feeds its text record (`FUN_0041cb90(entity, cell, attached?)`, skipped 40 px when absent), positioned through `SurfaceBlitRect` (`FUN_0047dcb0`) minus the record height, with attached dialogs advancing the line counter `DAT_00556f58` by `__ftol(&DAT_004a1d60,…)+3` lines via `FUN_0047a180`. Sole caller `AdvisorOverlayUpdate`. |
| 0x431b20 | PlacementBlockedAdjacentCellScan | Single-cell placement test: passes the `FUN_004090a0` guard when the cell `+0xc` byte lacks bit `0x20`, `FUN_0040e460` reports nothing, and the `+8` flag bit `0x20000000` is clear; then, gated on the `param_3`/`param_4` conditions (`+2` char not `'\x06'`, not `'\t'` when `param_4` is set, flags `8|0x200000` set with `0x108400` clear, bit `0x8000` clear unless `param_3` is zero), walks the four cardinal neighbors and returns 1 when no neighbor cell carries the `+2` marker character `'\v'`; otherwise 0. Sole caller `BuildingPlacementFootprintRenderAndValidate`. |
| 0x431c70 | FirstFreeNeighborCellPick | Advances the global cursor `DAT_004de588` and tries the four cardinal neighbors of `(param_2, param_2+1)`: in `param_4` mode checks in-bounds plus the walkable flags (`+8` bit `0x10` set, bit `8` clear) directly; otherwise routes through `TerrainCellRouteAndOccupancyCheck` (`FUN_00431da0`). Writes the first accepted neighbor into `param_3` and returns 1; returns 0 after a full cursor cycle. Sole caller `PendingCommandEntityAssignmentTick`. |
| 0x4350a0 | SurfaceTypeDescriptionsConfigLoad | Loops the stride-dword table at `DAT_00556320` until the pointer passes `DAT_00556367`: looks up each `SurfaceTypeDescriptions` child with the path key from `DAT_00556290`, reads the string value (`ConfigPathValueGet`), tokenizes at `DAT_004a13e4`, resolves the first token through `SoundAliasLookup` into the table slot (zero when unmatched), and stores the second token through `FUN_00477590` into `DAT_005562D8 + i*4`. Sole caller `GameConfigInit`. |
| 0x444fb0 | EntityTurnToVectorActivityIssue | For kind-2 entities only: normalizes the entity `+0xab/+0xac` velocity vector and the `(param_2[0], param_2[1])` target vector, takes the target angle (`FUN_0048e090`), and inside `±45°` issues activity char `'\t'`; between `45°` and `135°` rotates the velocity vector ninety degrees (`FUN_00479510`, `0xbfc90fdb` constant) and issues `'\n'`; beyond `135°` negates the velocity and raises `+0xf8` bit `0x80`; when `+0xf8` bit `0x400` is set the char becomes `0x14`/`0x15` instead, and `+0xf8 |= 4`; issues via `EntityActivityRequestSet` and ticks `FUN_00447c40`. Sole caller `SelectionCandidateFilter`. |
| 0x4451e0 | EntityPathStepVectorNormalize | For entities with flag `0x20000000` and no in-progress action char `5/6/7/3`: walks the `+0x30`-count, `+0x34`-start `(x,y)` step list, normalizes each vector, and for vectors passing `TerrainCellOccupancyOrFlagCheck` re-blends them toward the cell center: takes the surface point (`CurrentLevelHeightGet` → `FUN_0044f6d0` `TerrainGridPick`), reads the `FUN_00450890` weight, and writes `weight*0.71*vector + center`. Sole caller `StepMovementVectorResolve`. |
| 0x44a7d0 | PolyVariantSectionModelBuild | For each of `param_5` levels of detail builds the section key `"FPPoly %d"` (`FormattedStringOutput`) for `param_4 == 3` or the `"HighPoly"`/`"MediumPoly"` constant for `2`/`1`, walks the entries (`ConfigNestedPathChildFind` + `SortedEntryNextEqualKey`), skips `!`-prefixed names unless `DAT_00505f84` bit `0x800000` is set, and for each entry either builds the primitive (`FUN_00451ab0`) when the value is the `DAT_004a13f4` placeholder or the mesh (`FUN_004519b0(name, param_3, value, lod)`) otherwise, keeping the first built model. Callers `TypeTableResourceRowLoad`, `VehicleTypeDefinitionParse`. |
| 0x44d760 | JitterStepApply | Flag-driven jitter over `DAT_004eaec8`: requires bit `0x40`; when bit `0x80` is clear counts `_DAT_004eae8c` down by `param_1` and on expiry rebuilds the step — three range-random offsets (`RandomFloatRange`), a random unit direction (`D3DRMVectorRandom`) normalized and scaled by the magnitude at `_DAT_004eae98` — and sets bit `0x80`; in the stepping phase reads the `DAT_004eadd8`/`DAT_004eaddc` object position (`DDiSceneMoveOrPose`), probes `+offset` then `-offset` (`FUN_0044d9a0`) and clears bit `0x80` (stop) when neither passes, otherwise writes the offset position (`DDiSceneObjectPositionSet`) and counts the intensity down, clearing bit `0x80` at zero. Sole caller `FUN_0044cfa0` (ported editor-steps ticker). |
| 0x45ac30 | PanelTextureSlotsAllocate | (Re)allocates the `param_2`-entry `0x2c`-stride draw-record block for panel `param_1` at `DAT_00500134[param_1*0xc]` with the count at `DAT_00500138[param_1*0xc]`, and per row copies the position/extent fields from the `param_4`/`param_8` layouts (position, size-sum, and the trailing fields) and probe-loads up to three texture handles (`FUN_0047d9d0` on ids from the `param_5`/`param_6`/`param_7` arrays) into `+4`/`+5`/`+6`, releasing each handle immediately after the load (`FUN_0047dbf0`). Sole caller `PanelButtonLayoutConfigLoad`. |
| 0x4629d0 | PanelEventEntryAppend | Appends to the panel context's growing entry list (`+0x1c` count, `+0x20` array grown by `EngineHeapGrow`): allocates a zeroed `0x20` record, stores `param_4`/`param_5` at `+4`/`+8`, formats `param_2` through `&DAT_004a1d60` (`FormattedStringOutput`) into the `+0x14` string buffer, sets entry kind 3, `param_3` as the `+0x18` length, ORs `param_6` into `+0x1c`, and returns the record. Sole caller `RewardStatsPanelDraw`. |
| 0x46f3b0 | RayHitScanAndEffectApply | When the object's `+0x2a4` tracked entity is set: releases it (`EntityTeardown`, clearing the slot) when it lies farther than the `FUN_0046f170(+0x2a8)` range from the `+0x274…+0x27c` origin; otherwise casts a ray from the origin toward the `+0x280…+0x288` position through the entity-list callback `LAB_00471370` (`EntityPoolFindByCallbackFiltered`) and the `FUN_00471630` cell-box scan, tears down the tracked entity on any hit, and when a different entity was found interpolates the hit point along the ray and hands it to `FUN_0046f770(entity, this+0x290, hit, +0x2a8, +0x2ac)` for the mode-switched effect response. Sole caller `FUN_0046f550` (ported). |
| 0x482220 | MeshRecordAllocateAndFill | Grows the per-record geometry pool (start 10 `0x70`-stride entries; when exhausted `EngineHeapGrow`s to `count*1.5`), zero-fills the new 28-dword slot, copies the `param_4*param_3` `ushort` index list (stride 2) from `param_5` into a new index array, zero-fills the `param_2` `0x20`-stride vertex array, stores the counts, clears the trailing `+0x6c` word, marks the slot through `FUN_00483170`, and returns the new slot index. Callers `FUN_0040a4b0`, `SoundStreamSelectorUpdate`, `MeshBuildEightSidedSegment`, `FUN_004748f0`, `MaterialPass`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-21.bundle` —
batch decompile record
`ev_8be789eb64a728282aa796516188e5a519d3dc59541bc1cf61784b75de63e8c0`
(20 addresses: the 18 named picks plus the still-deferred steering pair
`0x471f00`/`0x471d20`), plus one `procedure_callers` record per named address
(`0x409790` `ev_8d7e81b3…`, `0x40bfd0` `ev_26093996…`, `0x40c3a0`
`ev_3775806c…`, `0x40dc20` `ev_d32e007a…`, `0x411100` `ev_e92ea136…`,
`0x4185f0` `ev_17858065…`, `0x41eca0` `ev_19a7fd2a…`, `0x431b20`
`ev_efec3005…`, `0x431c70` `ev_0b68c4ad…`, `0x4350a0` `ev_c797ee6a…`,
`0x444fb0` `ev_4b249d1a…`, `0x4451e0` `ev_d27509a6…`, `0x44a7d0`
`ev_48e02a87…`, `0x44d760` `ev_b7e6b692…`, `0x45ac30` `ev_250c6ef0…`,
`0x4629d0` `ev_d9411c0a…`, `0x46f3b0` `ev_27c050d7…`, `0x482220`
`ev_6dacd155…`). All investigations used shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The three command-region handlers `0x43a120`, `0x448c90`,
and `0x4490a0` (each writes the entity command record's opcode/coordinate
slots with opcodes `8/9`, `0xf`, and `6`, but the three command kinds are
still not distinguished, and their static call sites resolve as
`not_in_procedure`), the steering-slot pair `0x471f00`/`0x471d20` (re-decoded
this batch: `0x471f00` reads the entity velocity component through
`EntityTypeComponentGet`/`FUN_00475500`, scales it by `10000.0`, adds the
`FUN_004754c0` pose delta, and feeds the displacement into the steering path
for `EntityVelocitySteerBlendUpdate` — but neither side's dispatch split is
established), and the carried deferred picks `0x483690` (renderer matrix
set-if-changed), `0x471630` (ray-vs-cell-box minimum-distance scan, now also
seen as `RayHitScanAndEffectApply`'s intersection stage), `0x46f770`
(mode-switched hit response invoked by `RayHitScanAndEffectApply`), and
`0x435b20` (188-byte display-object group record). These carry forward the
unresolved set from findings 240–248 (`0x4802d0`/`0x480390`/`0x480570`,
`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`,
`0x40b000`, `0x474760`, the CRT cluster 0x493–0x49Bxxx, and the entity scalar
accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The `900 B`-and-up
anonymous band scan continues to queue 216 candidates for the next batch.
