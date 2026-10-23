# Finding 242 — Path-Step, Terrain, and Pending-Record Helpers

Twelve anonymous procedures now carry names grounded in their own instruction
sequences and in the named/ported procedures that call or are called by them.
Caller context comes from `CmdTickGlobal`, `GameConfigInit`,
`LevelDebugCellRetargetDispatch`, `TerrainCellQuadGeometryEmit`, and the
entity-simulation cluster around `PathStepAdvanceAndArrivalCheck`; none of the
names below depends on feature-level inference.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4438b0 | PathStepAdvanceAndArrivalCheck | Walks a mover entity's step table (0x14-stride records indexed by field 0xb, count field 10): dispatches a switch on the step-kind byte at record+0x11 setting one distinct status bit in state word f8/f9 per kind, advances the step index when the current record's exhausted flag (byte+0x10 bit 4) is set, gates on `tick % DAT_0049e3d4`, resolves the linked work object through `EntityWorkObjectPositionGet`, checks the step cell via `TerrainCellWalkableFlagCheck`/`TerrainCellWalkabilityQuery`, and runs per-kind arrival tests (`SQRT(dx²+dy²) <= 6.6`, `<= 6.0`, or within the object's `FUN_00469d40` radius) that set completion bits and call `EntityActionSequenceReset`; `StepMovementVectorResolve` refines the vector mid-flight. No static callers (indirect/ticker entry). |
| 0x444680 | StepMovementVectorResolve | Sole callee-pair of the step machinery above: converts the step record's flag byte (`& 3`) into the four cardinal unit vectors, reads terrain heights for the current and next step cell (`CurrentLevelHeightGet`/`MapCellToWorldCenter`), projects onto the surface when the step kind is not 1, normalizes three direction vectors, dot-tests them against ±0.7/±0.9/0.1 thresholds, issues `EntityActivityRequestSet` codes, scales the resulting displacement (×0.2 snap under 0.1, ×1.6 otherwise), and writes the movement vector fields 0xa3/0xa6/0xa7 back to the entity. Sole caller `PathStepAdvanceAndArrivalCheck`. |
| 0x4469c0 | EntityNearestConnectedTerrainCellRouteSelect | Probes the 4 (or 8, when a mode flag is set) cardinal/diagonal unit corner offsets (±1.0/±2.0 immediates) around the entity's cell: the corner mapping back to the entity's own grid point short-circuits a direct route (`EntityRouteSetupAndMarkerAssociate` to the cell), otherwise the non-matching corners run `TerrainNeighborPathCheck` + `TerrainCellWalkableFlagCheck`, the cheapest hit route is installed through `EntityRouteSetupAndMarkerAssociate`, and per-corner heap paths are freed. Called from `LevelDebugCellRetargetDispatch` and five anonymous neighbors. |
| 0x442f80 | EntityPoseOrientationDispatch | Type-dispatched per-entity maintenance: type 1 updates an attached model's orientation (`AttachedModelOrientationUpdate` with a height callback), types 3/4 the same through per-type accessors, type 2 projects onto the terrain surface when the walkable/feature byte disagrees (`TerrainCellWalkableFlagCheck`, `TerrainSurfacePointProjectOffset`, displacement clamped to `param_2*0.5`), and types 6–0xd follow the ground: `TerrainSurfaceHeightInterpolate` minus a fixed 1.2285 height offset, written through `DDiSceneObjectPositionSet`/`DDiSceneObjectPoseMethodDispatch` with a smooth-approach fallback decaying by `param_2*0.05` within ±0.5. No static callers. |
| 0x442b10 | EntityMovementAccumulatorApply | Drains the entity movement accumulators 0xaf/0xb0/0xb1: applies `FUN_00475580` with the sign selected by the type word and `FUN_004755e0`'s normalized x/z offset, validates the destination cell through `TerrainCellRouteAndOccupancyCheck`/`TerrainCellWalkabilityQuery`/`EntityPermittedActivityCheck`, commits the scene pose on success, projects onto the terrain surface plus per-type model update on failure, clears the accumulators, and returns a status code (3 walkability, 4 stats-bit gated). No static callers. |
| 0x44ead0 | TerrainCellListSurfaceHeightClampMutate | Takes a cell-coordinate pair list: deduplicates consecutive pairs into the `DAT_004eaed0` staging array, samples each cell's four corner heights via `TerrainCellSurfaceVerticesGet`, computes the corner mean with the processed-cell bit-8 min/max clamped to `avg ± tolerance*0.5` (min init 10000.0, max init −25000.0f), then moves every unprocessed corner toward the clamped average, stopping `tolerance` short, through `TerrainVoxelNeighborsMutate`, setting the processed bit. Callers `FUN_00409790`/`FUN_00409a90` (both pair-list builders). |
| 0x463230 | TerrainDirtyCellQuadNormalEmitPass | Consume-pass over the quad dirty flag `DAT_00552190` raised by `TerrainCellQuadGeometryEmit`: for each cell whose record flag bit 2 is set, re-reads the four quad vertices (`LwoLayerPolygonVertexDataRead`), accumulates the four neighbor surface normals per corner (winding order selected by bit 4, matching the corner-order flag in the emit path), normalizes each, and re-emits the vertex record through `TerrainCellQuadVertexEmit`, clearing bit 2. No static callers (registered tick). |
| 0x463590 | TerrainCellSurfaceNormalGet | Bounds-checked per-cell helper: reads the cell's quad vertices via `LwoLayerPolygonVertexDataRead`, sums the two triangle normals from `SurfaceTriangleNormalCalculate`, normalizes to a unit vector and returns 1 (0 if the cell record is dirty/unreadable). Sole caller `TerrainDirtyCellQuadNormalEmitPass`. |
| 0x40d850 | TerrainCellNeighborFloodWalk | Recursive flood over the terrain cell grid (`DAT_004c7df8` 12-byte cell records, `DAT_004c7dfc` grid descriptor): marks cells with visited bit 0x100, walks the four cardinal neighbor offsets with bounds test (`FUN_004502c0`) and `TerrainCellSpecialStatePredicate` queries, gates continuation through `RandomNext` against `min(param_4, 200.0)`, records per-direction flag bits (`1 << ((dir-2)&3)`), spawns a directional per-edge effect via `FUN_0040dc20` (2×16-slot effect list `DAT_004c7c74`), and recurses into unvisited neighbors. Callers `FUN_0040d4f0` (from `EntityFenceSparkTerrainInteraction`) and itself. |
| 0x405950 | PendingCommandTimerFireScan | Timer pass over the pending-record list `DAT_004b3358` (same record layout as `PendingCommandEntityAssignmentTick`): when a record's float timer expires, evaluates the per-type entity predicate (types 0/3/4/10/25/29/24 — moved-coordinate check, distance tests re-arming the timer to 1500.0 or 250.0, stats-bit checks), decrements the live count `DAT_004b34f0` with swap-removal from the cell registry, unlinks the record from the entity chain at +0x2f0, and duplicates it into an entity slot via `FUN_00406400` under flag 0x40. Sole caller `CmdTickGlobal`. |
| 0x407a10 | PendingTypeValueSlotSelect | Reads the entity's pending-record chain (+0x2f0, skipping trailing type-0xE records) and runs a switch over the record type code selecting one of ~30 global value slots in `DAT_00557864`–`DAT_005578FC` (zero-filled BSS, written at runtime) into the output pointer. The selected slot is cached per entity and consumed by a countdown timer called from `TickEntityScreenEffects`. No callees; sole grounding is the dataflow into the cached selector. |
| 0x40ab70 | DependencyGridConfigLoad | Loads the `Dependencies` config key: splits each entry name into three tokens (`BlockTextTokenSplit`), resolves an entity type (`ObjectListEntityTypeResolve`), and for the `HitOnceStay` value ORs flag bit 1 into the selected 16-level cell-rule slot and writes the parallel cell arrays; the second token list allocates per-cell 0x16-word blocks (`EngineHeapAlloc`, index geometry `(x + z*0xF)*0x31`) stored in `DAT_004b8c08`/`DAT_004b8bc8`, then runs the grid revalidation pass `FUN_0040b1b0`. Sole caller `GameConfigInit`. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-14.bundle](../evidence/legorr-engine-helpers-expansion-14.bundle):
`ev_04a935aeee95ea426374204046071f67efb8e8d0f06ea34144c959fc371fb466`
(batch decompile of all twelve addresses). Caller observations are the
matching `procedure_callers` records, including
`ev_d75637f8531fd4b9d14da5a021341c3452a9d3e69437814cef0962e9fa26f72b`
(0x405950 from `CmdTickGlobal`),
`ev_fc8184a09460df4023ddb12feb22ba017fb16e6f77b5889c2d3615f147034962`
(0x40ab70 from `GameConfigInit`),
`ev_a5332140dbd3895b5b7a9cfa30f986ca89a19956e040d51041429a3b6eceabb9`
(0x4469c0 from `LevelDebugCellRetargetDispatch`),
`ev_e92f0ccde8537a64dabb48f76d526ade5bd7881740063da7d35442025994f2d8`
(0x4438b0), and
`ev_4abb6c4a2dd0bfcefcfa9b07ff600cc35445f8f7de6494f6a89e2b598860ee5a`
(0x444680 from `PathStepAdvanceAndArrivalCheck`).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x4438b0`, `0x442f80`, `0x442b10`, and `0x463230` have
no statically-resolved callers — they enter through indirect or table
references, so their per-frame-vs-per-command context is not provable; the
names above stay mechanical. `0x40d850` has a live call site at `0x40d6ea`
inside an unregistered code gap between `FUN_0040d4f0` and `0x40d850` (a
0x72BD-stride table loop Ghidra does not recognize as a procedure), so that
edge is not in the ledger. Further leads: the `DAT_00557864`–`DAT_005578FC`
slot table is zero-filled in the artifact and needs writer-xref evidence to
say what `PendingTypeValueSlotSelect` selects, and the callers of
`TerrainCellListSurfaceHeightClampMutate` (`FUN_00409790`, `FUN_00409a90`)
remain anonymous pair-list builders. The unresolved set carried from
findings 240/241 (`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`,
`0x44d280`, `0x473460`/`0x473480`/`0x4734a0`) keeps its status.
