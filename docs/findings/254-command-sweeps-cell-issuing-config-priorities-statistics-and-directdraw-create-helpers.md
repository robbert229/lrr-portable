# Finding 254 — Command Sweeps, Cell Issuing, Config Priorities, Statistics, and DirectDraw Create Helpers

Nineteen more anonymous procedures now carry names grounded in their own
instruction sequences, each with at least one named or ported caller:
`SelectionDragAreaCommandCreate`, `UiActionDispatch`, `EntityStatsConfigLoad`,
`AdvisorOverlayUpdate`, `PositionTrackerUpdateAll`,
`SimulationPointerActionDispatch`, `TutorialPathMenuAction`,
`EntityCreateByType`, `TerrainProjectedCellStateRefresh`,
`PredugSpawnCoordinateSelectAndCreate`, `RouteStepCommandIssue`,
`TerrainCellMutation`, `EntityRouteSetupAndMarkerAssociate`, the ported
ticker `0x450f30`, `LevelLoad`, `EntityCommandStatusFeedbackDispatch`,
`FenceSparkLaunchRegister`, `EntityTypeBehaviorUpdate`,
`SceneEntitySlotTableRebuild`, `ResourceSlotRegisterName`,
`SceneChannelSlotsRebuild`, `PlaceCommandEligibilityCheck`,
`DirectDrawDeviceInitialize`, and `ObjectiveVoiceAliasPrepare`, plus named
callees `CmdFree`, `CmdKindByName`, `ObjectListEntityTypeResolve`,
`CurrentLevelGet`, `CurrentLevelHeightGet`, `MapCellToWorldCenter`,
`TerrainCellSurfaceVerticesGet`, `TerrainCellSurfaceTypeSet`,
`TerrainCellPredugEdgeWallFlagSet`, `TerrainCellWalkableFlagCheck`,
`TerrainCellOccupancyOrFlagCheck`, `SurfaceDrawPolyline`,
`EntityDamageApply`, `EntityDirectionalActivityStart`,
`CachedRenderResourceInstanceCreate`, `DirectXResultDiagnosticDispatch`,
`ConfigBooleanParse`, `EngineHeapAlloc`, `RandomFloatRange`, and
`EntityCreateWithPosition`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x404100 | CommandQueueSweepRelink | Walks the queue chain at `param_1 + 0x2f0`: records whose `+0x5c` bit `4` is raised are freed with `CmdFree`, records carrying bit `0x10` without bit `0x20` are relinked into the `DAT_004b3354` side list, records with bit `0x2000` without bit `0x20` into `DAT_004b3358`, and the remaining records are threaded back into the rebuilt head chain stored at `param_1 + 0x2f0`. Sole caller `SelectionDragAreaCommandCreate`. |
| 0x402d50 | KindNineCellCommandIssue | Skips when the cell byte (`+2` of the stride-`0x48` record) is `1`, `TerrainCellWalkableFlagCheck` fails, or `FUN_00432dd0(cell)` reports pending work; otherwise allocates the kind-`9` command (`FUN_00406440(9)`), writes the cell into `+8/+c`, `+20 = 8`, `+24 = −1`, `+28 = 0`, raises bit `0x400`, wakes with `FUN_00402600(rec, 1)` for `param_2 == 0`, links via `CmdLinkHead(rec, 3)`, dispatches `FUN_00403810(cell, 8, 0, 0)`, marks the cell through `TerrainCellPredugEdgeWallFlagSet`, and re-stamps the surface type with `TerrainCellSurfaceTypeSet(CurrentLevelHeightGet(cell, 2))`. Sole caller `UiActionDispatch`. |
| 0x408c80 | StringTableIndexLookup | Pairwise byte-walks each entry of the `DAT_004b8a3c` name table (count `DAT_004b8a50`, bail with the count when the table is empty), returning the index of the first entry that compares equal to `param_1` under the zero-terminating two-byte-per-step comparator. Sole caller `EntityStatsConfigLoad`. |
| 0x41cd30 | PositionTrackerBoxDraw | When `DAT_00556ecc` bit `0x100000` is clear, builds the eight corners of the box `(param_1 − 1)` around the given extents and strokes it through `SurfaceDrawPolyline(corners, center, 4, 0, 1.0, 0, 0)`, then repeats with a two-unit inset — the twice-outlined tracker rectangle. Callers `AdvisorOverlayUpdate` + `PositionTrackerUpdateAll`. |
| 0x41c6e0 | PointerActionStateDispatch | Kind-state machine over `DAT_004ddd00/04/08` (`0x4a` sentinel): `param_4 == 0` resets the state; from the sentinel a new kind starts (kind `0x49` dispatches `FUN_0041ca20(param_2, param_3)`), on a matching kind the stored coordinates decide the `FUN_0041ca20(…, 1)` re-issue, otherwise `FUN_0041c9f0(param_1, 1)` runs, and the tail answers whether the tracked kind/coordinate triple still matches. Sole caller `SimulationPointerActionDispatch`. |
| 0x41f740 | CellFourNeighborWalkCheck | With the `FUN_00432e60(cell)` result zero, fails if any of the four `(±1, 0)/(0, ±1)` neighbors fails `TerrainCellOccupancyOrFlagCheck`; when the cell itself is occupied, fails only if more than two of the four neighbors fail. Sole caller `TutorialPathMenuAction`. |
| 0x43c570 | KindFourPostCreateResolve | For kind-`4` records with `+0xfa` bit `0x80000000` clear: takes the cell via `FUN_004424f0`, and when neither flag bit `0x200` nor `0x400` is set consults `FUN_00409450(cell, FUN_00469b50(rec))` — on failure clearing bit `29` and running `FUN_00407410(rec)`; otherwise raises bit `29`, and with flag bit `0x200` set chains `FUN_004385b0(rec, 0)`, `FUN_0041f8e0(1)`, and `FUN_004093b0(cell)`. Callers `EntityCreateByType` + `FUN_0046a330`. |
| 0x434480 | CellSurfaceUpdateDispatch | When the cell flag word `+8` carries bits `0x18` and the type slot lookup (`param_1 + 0x80`, indexed through `+0x74`) resolves non-null: fills the surface vertex block via `TerrainCellSurfaceVerticesGet` and hands it with the cell record's identity bytes to `FUN_0040b880(handle, vertices, name byte, flag byte, param_4, param_5)`, reporting success. Sole caller `TerrainProjectedCellStateRefresh`. |
| 0x44a4f0 | NearestCellCoordinateWorldResolve | Reads the entity position through `FUN_00442260`, sweeps the `DAT_004e844c` cell-pair list (`DAT_004e853c` entries, `CurrentLevelHeightGet`-converted to world centers with `MapCellToWorldCenter`), keeps the nearest by Euclidean distance against a 10000-unit start, and writes the winning cell out. Callers `PredugSpawnCoordinateSelectAndCreate` + `RouteStepCommandIssue`. |
| 0x44b140 | SmallSpiderCellSwarmSpawn | Resolves the `SmallSpider` entity type (`ObjectListEntityTypeResolve(s_SmallSpider…)`), reads the cell bounds (`CurrentLevelHeightGet` + the vertex block), and creates `(param_3 & 3) + 2` entities at `RandomFloatRange`-scattered positions with `rand() % 7` rotations through `EntityCreateWithPosition`, raising each record's `+0x3e0` bit `0x10000`. Sole caller `TerrainCellMutation`. |
| 0x4508a0 | CellNeighborDirectionCode | Pure adjacent-cell delta-to-direction mapping: same column gives `0` (north step) or `2` (south), same row `1`/`3`, east-diagonal pairs `4`/`5`, west-diagonal pairs `6`/`7`, and anything else `−1`. Sole caller `EntityRouteSetupAndMarkerAssociate`. |
| 0x451180 | CellVariantEntryMatchCheck | After both cells pass the header bounds, loads each cell's variant byte at `+0x18` and walks the first cell's variant list (`+0x4034 + cell*0x80` count table, `0x80`-stride entries, match field `+0x34`), returning 1 when any entry equals the second cell's variant byte. Sole caller the ported ticker `0x450f30` (sibling of `TerrainCellSurfaceVerticesGet`-adjacent helpers). |
| 0x45cf50 | PrioritiesConfigLoad | For each `Priorities` config child whose name resolves through `CmdKindByName`: stores the kind into the parallel `DAT_0050111c`/`DAT_00501260` tables, installs the computed weight `index * −5 + 0x50` through `FUN_00402090(kind, weight)`, parses the entry's boolean flag (`ConfigBooleanParse`) through `FUN_004020a0(kind, …)` into the `DAT_00501338` table, and increments the `DAT_005013a4` counter. Sole caller `LevelLoad`. |
| 0x46b660 | CommandSampleStatsAccumulate | Per-kind statistic slot (stride `0x27c`): a null sample zeroes the window head/total; a sample slides the window (subtracting `+8 − +0` worth of the old total) when the stored total is below `sample + 8`, adds `sample + 4` to the running total, raises the `+12` high-water mark, and appends the sample pointer to the circular log at `+0x54c20c + index*4` with the index incremented. Callers `SimulationPointerActionDispatch` + `EntityCommandStatusFeedbackDispatch`. |
| 0x46f270 | TypeMotionScaleApplyDispatch | Computes the distance scale `FUN_0046f1d0(param_2, record)`; for records with `+0xfa` bit `0x40000` and a live object (`FUN_004385b0(record, 1)`), the stride-`0x4b68` type table at `DAT_00503878 + 0x4b00` selects the path: with `param_3` set, `EntityDamageApply(record, table * scale, 1, param_4)` plus the `+0xd7/+0xd6` handle pair and `EntityDirectionalActivityStart(record, param_5, 1)`, else `EntityDamageApply(record, scale * param_4, 1, param_4)`; without the table entry a scale-only `EntityDamageApply` (with the `*record == 3` fallback hook) followed by `EntityDirectionalActivityStart` when `param_3` is set. Callers `FenceSparkLaunchRegister` + `EntityTypeBehaviorUpdate`. |
| 0x473bc0 | ResourceNameRegisterOrQuery | Builds the `{name pointer, length, mode param_3, extra}` record and dispatches it through `FUN_00476330(*param_1, 0, FUN_00476410, &record)` — the first documented indirect use of the zero-caller `FUN_00476410` callback: for modes 0/2 it returns the resource handle through `FUN_00475880` when the table resolves one, mode 1 copies the resolved value back into `*param_4`. Callers `SceneEntitySlotTableRebuild`, `ResourceSlotRegisterName`, `SceneChannelSlotsRebuild`, `FUN_00406f50`, `FUN_0040c0d0`, `FUN_00451bc0`, `FUN_0046d260`, `FUN_0046d9f0`. |
| 0x47c2e0 | DirectDrawDeviceCreate | Clears `DAT_0076acb4`, runs `DirectDrawCreate(param_1)`, queries the `DAT_0049f648` interface on the created object (storing `DAT_0076aca4`), registers the `LAB_0047c380` notification through the interface `+8` call, releases the interfaces, and reports whether the create-and-query chain succeeded. Callers `DirectDrawDeviceInitialize` + `FUN_0049d780`. |
| 0x47c4b0 | DirectDrawEnumeratedDeviceCreate | For device descriptors carrying `+0x110` bit 1 (hardware flag): runs `DirectDrawCreate` with the descriptor minus bit `0x10`, queries the `DAT_0049f328` interface, installs the `LAB_0047c550` notification through the interface `+0x20` call, stores the interface at `DAT_0076acac`, and reports success. Callers `DirectDrawDeviceInitialize` + `FUN_0049d630`. |
| 0x471960 | SeeThroughWallsRayCheck | Resolves the `SeeThroughWalls` slot (`CurrentLevelGet` + config lookup, aborting when `FUN_004790d0(DAT_005053fc, slot)` is set), transforms both view arguments with `FUN_00471ca0` into an origin and the `(Δx, Δy, Δz)` delta pair set, seeds the ray record (`local_18 = 0`, `1.0`), dispatches it through `FUN_00471630(&ray)`, and answers from `local_8`. Sole caller `PlaceCommandEligibilityCheck`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-26.bundle` —
batch decompile record
`ev_21ab87f2b4a07bca172c27cdd54a68dc9987deef7ebd79f9cac37e58dbf0d77e`
(23 addresses: the 19 named picks plus the examined-and-deferred
`0x47e300`/`0x485ea0`/`0x489b30`/`0x48e680`), plus one `procedure_callers`
record per named address (`0x404100` `ev_c7155270…`, `0x402d50` `ev_7c8b381d…`,
`0x408c80` `ev_41ca59d0…`, `0x41cd30` `ev_7f1aaa48…`, `0x41c6e0`
`ev_4d4dbd15…`, `0x41f740` `ev_8a7a05c3…`, `0x43c570` `ev_5d24c497…`,
`0x434480` `ev_02112d8b…`, `0x44a4f0` `ev_22c76b4d…`, `0x44b140`
`ev_31d3b942…`, `0x4508a0` `ev_9b38c5a1…`, `0x451180` `ev_6196813b…`,
`0x45cf50` `ev_60af60fb…`, `0x46b660` `ev_7dbdfe89…`, `0x46f270`
`ev_126b1a16…`, `0x473bc0` `ev_a98ff56b…`, `0x47c2e0` `ev_6b398646…`,
`0x47c4b0` `ev_140f499f…`, `0x471960` `ev_cd89df33…`). All investigations used
shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** Newly examined and deferred: `0x47e300` (fills the
124-byte device request block, dispatches the `DAT_0076ac84` device-interface
vtable `+0x18`, logs the failure through `DirectXResultDiagnosticDispatch`
with the embedded `C:\Dev\SourceSafe\gods98\dx6\god…` source string, and
otherwise queries the `param_3` surface vtable `+0x14` into `FUN_0047e3e0` —
the vtable slot semantics are unidentified), `0x485ea0` (stores positive
float limits from `param_1`, dispatches the `DAT_0076ac8c` device-interface
vtable `+0x58` with the 124-byte block, and keeps the returned float pair
clamped against the extended parameter bounds), `0x489b30` (enumerates up to
three children through the `+0x80` vtable slot and GUID `DAT_0049fb48`
interface queries, creating a `CachedRenderResourceInstanceCreate` per child —
the GUID and slot semantics are unidentified), and `0x48e680` (float10
absolute-value helper with the `0x7ff0` exponent guard and `FUN_00493c10`/
`FUN_00493a80` CRT-math calls). The command-region trio `0x43a120`, `0x448c90`,
and `0x4490a0` (needs a call-site instruction dump) and the steering-slot pair
`0x471f00`/`0x471d20` (the slot-base accessor `0x4712c0` reads only its first
argument) continue to defer. Previously deferred `0x476e50`, `0x4887c0`, and
`0x48e280` stand, as do the zero-caller band addresses — `0x476410` is now
touched as the `FUN_00473bc0` dispatch callback, so its semantics come from
that table — while `0x444460`, `0x463d20`, `0x44b500`, `0x41c800`, `0x43aab0`,
`0x439a20`, and `0x4440f0` still need indirect-dispatch evidence. The
carry-forward set from findings 242–253 (`0x4802d0`/`0x480390`/`0x480570`,
`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`,
`0x40b000`, `0x474760`, `0x483690`, `0x471630`, `0x46f770`, `0x435b20`,
`0x465c50`/`0x473560`/`0x489660`, the CRT cluster 0x493–0x49Bxxx, and the
entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`) stands, together
with the anonymous-chain band entries `0x40f920`/`0x43fc20`/`0x448eb0`/
`0x40b880`/`0x44c7f0`/`0x410490`/`0x40ba00`/`0x40bb90`. The band scan leaves
43 further non-CRT candidates for the next batch.
