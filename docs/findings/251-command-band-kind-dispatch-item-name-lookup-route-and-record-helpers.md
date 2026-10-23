# Finding 251 — Command-Band Kind Dispatch, Item Name Lookup, Route Commands, and Record/Geometry Helpers

Fourteen more anonymous procedures now carry names grounded in their own
instruction sequences, each with at least one named or ported caller:
`UiActionDispatch`, `MainSimInteractionAndCellUpdate`,
`EntityEnergyLowTargetRouteCommandTick`, `EntityPoseOrientationDispatch`,
`AdvisorOverlayUpdate`, `SimulationPointerActionDispatch`,
`AdvisorAnimationPositionAndRequestUpdate`, `OListEntityRecordParse`,
`DeferredOListEntityCreateAndLink`, `ObjectiveSidecarAppendGate`,
`SceneActivityUpdateDispatch`, `EntityNeighborBlockedCellStepProbe`,
`EntityMotionToSceneNodeSync`, the ported step tickers `0x44cfa0`/`0x450f30`,
`PolyVariantSectionModelBuild`, `CellQuadRunStage`, `LevelDebugInputCellMutatorDispatch`,
and `MaterialPass`, plus named callees `XfTextureNameFormat`,
`MeshRecordAllocateAndFill`, `DDiSceneObjectAllocate`,
`DDiSceneObjectAttachOrUnlink`, `SceneRenderLockToggle`, `EntityTypeComponentGet`,
`EntityCreateWithPosition`, `MapCellToWorldCenter`, `SceneActionNameSet`,
`RandomFloatRange`, `EngineHeapAlloc`, `EntityStateFlagsGet`, and
`SceneActivityUpdateDispatch`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x402960 | EntityKindCommandIssue | When the entity's `+0x3e4` bit `0x10` is clear, allocates the kind-`0x12` command record (`FUN_00406440`) holding the entity at `+0x10`, raises the bit, and classifies the movement kind from the flag accessors (`FUN_00469ea0` bit `0x100` sets `+0x3c` to 1; otherwise the `FUN_00469e60` bit pairs `0x2000000`/`0x1000000` select 2, 4, or 1); raises the record's `+0x5c` bit `0x100000` when the flag bit `0x40` is set with `+0x3e0` bit `0x400` clear, invokes `FUN_00402600(record, inverted +0x5c bit 20)`, clears `+0x5c` bit `0x40`, and finalizes with `FUN_00404030(record, 0xc)`. Callers `UiActionDispatch` + `MainSimInteractionAndCellUpdate`. |
| 0x40a4b0 | QuadMeshRenderRecordCreate | For a valid entity (`FUN_0040aae0`) with a nonzero count under 999: allocates the render record (`FUN_0040a5c0`), zeroes `+3/+4/+6`, stores the negated mass (`FUN_00469d50`) at `+5`; for a new record takes the object component (`FUN_0044a200`), allocates the scene object (`DDiSceneObjectAllocate`), registers the `0x3000` mode callback `LAB_0040a740` with `FUN_00480870`, and builds the default quad twice over with four `MeshRecordAllocateAndFill(pool-handle, 4, 2, 3, {0,2,1,0,3,2})` calls; for a reused record re-attaches it (`DDiSceneObjectAttachOrUnlink` + `SceneRenderLockToggle` off); finishes by filling the count through `FUN_0040a5e0`. Sole caller `EntityEnergyLowTargetRouteCommandTick`. |
| 0x40c5b0 | ResourceSlotRegisterName | Scans the three `DAT_004c71e0` group tables (`DAT_004c79a4` base, `0x29`-dword stride) and their four `+5`-entry slot windows for the slot whose `+8` handle matches `(param_1, param_2)`; then walks up from `param_2` for the first index under the group count (`+0x12`) whose registration bit in `+0x25` is unset, sets that bit, formats the name for the slot handle with `XfTextureNameFormat(handle, +0x11 name base, &index)`, and applies it through `FUN_00473bc0(handle, name, 0, 0)`. Sole caller `EntityPoseOrientationDispatch`. |
| 0x41b3c0 | EntityQueryFlagCount | Query-kind counter: `0x13` returns `DAT_00556288`; `0x14` counts the `DAT_0055627c` entity list members where (`FUN_00469ee0(1,i)` flag bit `0x1000` is set or bit `0x2000` is clear) and `FUN_00469f00(1,i)` bit 8 is clear; `0x15` counts members where flag bit `0x800` is clear or the second flag's bit 8 is set; any other query returns the `DAT_004ddc08` pair entry (`+ param*8`). Callers `AdvisorOverlayUpdate`, `SimulationPointerActionDispatch`, `AdvisorAnimationPositionAndRequestUpdate`, `FUN_0041c1c0`. |
| 0x438460 | EntityKindRecordUpdateDispatch | Kind dispatch over the record: kind 4 runs `SceneActivityUpdateDispatch`, raises the `+0xfa` bit `0x20`, and cascades the `FUN_00469e60` flag bits `8/2/4` into `FUN_004038c0(record, 6/7, 0, 0)` / `FUN_004039a0()` / clearing the bit, then `FUN_00439990` and raises `+0xfb` bit `0x200000`; kind 1 resets through `FUN_0046cfc0(record[3], 0)`; kinds 2/1 run `FUN_0043f560`; every kind notifies `FUN_00418830(kind, record[1], 0, +0xfb bit 0x200)`; kind 4 increments the counter `_DAT_004eadc4`, kinds 2/1 run `FUN_004034a0`, kinds 1/4 raise `+0xfb` bit `0x40000`. Callers `OListEntityRecordParse`, `DeferredOListEntityCreateAndLink`, `ObjectiveSidecarAppendGate`. |
| 0x4396c0 | ItemNameByKindLookup | Returns the name string for an item kind: `8` → `Dynamite`, `6` → `PowerCrystal`, `7` → the `PTR_DAT_004a1e98` entry for index 0 else `ProcessedOre`, `9` → `Barrier`, `0xb` → `ElectricFence`, `0xc` → `SpiderWeb`, `0xd` → `OohScary`, `-1` → `tvcamera`, and kinds 1–4 index the `DAT_005561e8`/`1ec`/`1f0`/`1f4` name-pointer tables by `param_2`; otherwise null. Callers `SceneActivityUpdateDispatch` + `FUN_00460310`. |
| 0x449700 | RouteStepCommandIssue | Without the `EntityStateFlagsGet` bit `0x100000`, resolves a candidate cell through `FUN_0044a5d0` and accepts it when `FUN_00449880(x, y, 0x40400000)` passes; otherwise takes the cell target through `FUN_0044a4f0` and either succeeds when the entity's cell (`FUN_004424f0`) is already the target or issues the step route through `FUN_00441700`, freeing the temporaries (`FUN_0048db80`); on success writes step byte `10` into the stride-`0x14` step array at the current entry, stores the float target at `+0x2f4/+0x2f8`, and wakes the command queue (`FUN_00402aa0`). Callers `EntityNeighborBlockedCellStepProbe` + `FUN_0044afb0`. |
| 0x44bb30 | EntityNodeScaleUpdate | Rewrites the entity's `+0x32c` attachment record pair: composes the first value by chaining the `FUN_0046a1d0`/`1f0`/`1b0` accessors, doubling at each stage when `param_2` is nonzero, and writes it with `FUN_0040ff70`; composes the second through `FUN_0046a230` → `1d0` → `1f0` → `1b0` (with the doubled `3.0`/`4.0` factors under `param_2` too) and writes it with `FUN_0040ffe0`. Sole caller `EntityMotionToSceneNodeSync`. |
| 0x44d0d0 | DriftOffsetStepApply | Gated by `DAT_004eaec8` bit 1: decrements the `_DAT_004eae04` timer by `param_1` and returns until it expires, then redraws the timer from `RandomFloatRange(DAT_004eae08, DAT_004eae0c)`, maps `rand()%1000 * 0.001` onto the sweep `-range..+range` of the `DAT_004eadf8/eadfc/eadf4+0x10` axis pair set, and — when the accumulated `fVar + DAT_004eae10` stays under the `DAT_004eae14` bound, or after clamping each axis through `FUN_0044d250` — accumulates the offsets into `DAT_004eadec/eadf0/eadf4` and `_DAT_004eae10`. Sole caller the ported ticker `0x44cfa0` (sibling of `JitterStepApply`). |
| 0x4512d0 | TerrainCellNormalSample | For the terrain header (column/row bounds `*param_1`/`param_1[1]`, cell base `param_1[10]`, row stride `param_1[2]`, `0x28`-byte cells): when the cell `(param_2, param_3)` is in bounds, picks the corner triple at cell `+0` (rotation 0) or `+0xc` (rotation 2), rotates the `param_4` selection when the cell byte `+0x19` bit 1 is set, and for rotations 1/3 sums both corner triples and normalizes the result (`1/|v|`) into `param_5`, returning 1; out of bounds zeroes the vector and returns 0. Sole caller the ported `0x450f30`. |
| 0x451ab0 | PrimitiveModelRecordCreate | Allocates the 24-byte model record `{id 0, 0, heap copy of the `param_2` name, param_3, kind 2, 0}` through `EngineHeapAlloc`, links it into `param_1 + 0x14` when the model base is given, and returns it — the no-load sibling of `VisualModelRecordCreate`. Sole caller `PolyVariantSectionModelBuild`. |
| 0x466250 | ExpandCellEntitySpawn | Refuses when the cell flag word `+8` bit `0x100` is already set; otherwise runs the `FUN_004663f0` cell-spawn request, centers through `MapCellToWorldCenter`, creates the entity with `EntityCreateWithPosition(DAT_00556390, kind 0xc, 0, 0, center, handle)`, records it with `FUN_00466380`, raises the cell entity's `+0x3e0` bit `0x100`, and sets its action name to `Expand` (`SceneActionNameSet`) with `FUN_00475140(… , 0)`. Sole caller `LevelDebugInputCellMutatorDispatch`. |
| 0x46e9d0 | StagedCellCandidateCollect | For every cell index in `[param_2, param_3)`: walks the `DAT_004beb0` staged-entry blocks (`DAT_00549520`, `0x10a`-float stride, entry count at `+200`) and their `(x, y)` float pairs, appending each distinct `(previous, block)` pair whose pair-x equals the loop cell and pair-y equals `param_1 - 1` into the parallel `DAT_0054beb4`/`DAT_0054beb8` arrays, incrementing the `DAT_0054bf04` counter. Sole caller `CellQuadRunStage` (paired with the `StagedCellCandidateSelect` batch-11 name). |
| 0x4826c0 | MeshVertexRecordFill | Fills `param_4` vertex entries of the render record's slot `param_2` (record base at `param_1 + 8`, `0x70` stride, vertex buffer at slot `+0xc`) at `0x20` stride: the three dwords from `param_5 + 8`, the three from `param_6` (`+0/+4/+8`), and the `*param_7`/`param_7[1]` pair — three records' worth of position/normal/attribute fields advanced 3/1/2 units per entry. Sole caller `MaterialPass`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-23.bundle` —
batch decompile record
`ev_f566111ade0b7a764f73c8d4fee8476cf2dd41c90333b5e9913307a009d3df7d`
(15 addresses: the 14 named picks plus the examined-and-deferred `0x476e50`),
plus one `procedure_callers` record per named address (`0x402960`
`ev_9dc98271…`, `0x40a4b0` `ev_88df9087…`, `0x40c5b0` `ev_cf2b0e57…`,
`0x41b3c0` `ev_30cf25a1…`, `0x438460` `ev_fd9cc947…`, `0x4396c0`
`ev_1d37bf2b…`, `0x449700` `ev_97641793…`, `0x44bb30` `ev_4881198c…`,
`0x44d0d0` `ev_5a4bc8ec…`, `0x4512d0` `ev_8fbe646c…`, `0x451ab0`
`ev_b49611a8…`, `0x466250` `ev_8569ab7c…`, `0x46e9d0` `ev_e693a529…`,
`0x4826c0` `ev_91263e15…`). All investigations used shipped
`reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The command-region trio `0x43a120`, `0x448c90`, and
`0x4490a0` (static call sites still resolve `not_in_procedure`; opcode kinds
`8/9` vs `0xf` vs `6` undistinguished), and the steering-slot pair
`0x471f00`/`0x471d20` (instruction-level facts on record: the slot-base
accessor `0x4712c0` reads only its first argument — kind 1 → `entity[3] +
0x138`, kind 4 → `entity[6] + 0xa0`, else 0 — so the pushed displacement
arguments die there and `0x471a60` receives stale stack arguments). Newly
examined and deferred this batch: `0x476e50` (bounds-checks the screen through
the device-vtable size calls `+0x6c/+0x68`, takes a pooled record from the
`DAT_0076ad60` free list — created by `FUN_00477370` when empty — and builds
the object through the `DAT_00505914` vtable `+0x50` call plus an `+0x18`
callback, called from `GameConfigInit` and `FUN_00476dc0`; the vtable's
`+0x50` semantics cannot be stated mechanically). Zero-caller band addresses
still need string-key table or indirect-dispatch evidence before naming:
`0x476410`, `0x444460`, `0x463d20`, `0x44b500`, `0x41c800`, `0x43aab0`,
`0x439a20`, `0x4440f0`, and anonymous-chain band entries
`0x40f920`/`0x43fc20`/`0x4887c0`/`0x448eb0`/`0x40b880`/`0x44c7f0`/`0x410490`/`0x40ba00`/`0x40bb90`/`0x403750`.
These carry forward the unresolved set from findings 241–250
(`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`, `0x40b000`, `0x474760`, `0x483690`,
`0x471630`, `0x46f770`, `0x435b20`, `0x465c50`/`0x473560`/`0x489660`, the
CRT cluster 0x493–0x49Bxxx, and the entity scalar accessors
`FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The band scan leaves roughly 105
further non-CRT candidates for the next batch.
