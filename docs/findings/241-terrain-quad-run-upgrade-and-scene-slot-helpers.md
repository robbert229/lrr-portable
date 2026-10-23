# Finding 241 — Terrain/Quad-Run, Upgrade-Table, and Scene-Slot Helpers

Fourteen anonymous procedures now carry names grounded in their own
instruction sequences and in the named/ported procedures that call them.
Caller context comes from `TerrainCellMutationProgress`,
`ScreenRayTerrainCellPick`, `MapLoadPipeline`, `RewardScreenTick`,
`MenuTick`, `EntityStateReset`, `SimulationPointerActionDispatch`,
`LevelDebugInputCellMutatorDispatch`, `SceneActivityChange`,
`EntityPayloadStateSet`, `SceneActivityPairedChannelUpdate`,
`BuildingTypeConfigRecordParse`, `VehicleTypeDefinitionParse`, and the
ported `GameConfigInit`; none of the names below depends on feature-level
inference.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x46c160 | UpgradeLevelTableParse | Allocates a 1000-entry table through `EngineHeapAlloc`, then loops counters 0–15 formatting each counter's low bits as `Level%i%i%i%i`, looks that key up under the `Upgrades` config node through `ConfigEntryFind`/`ConfigNestedPathChildFind`, and converts every equal-key sibling into a record holding a level bitmask, the zero-based parsed integer, the duplicated token, an optionally split entry name, and the upgrade-name-table lookup through `UpgradeNameEntryFind`. On a zero record count the table is freed and nulled; the found-flag mask is ORed into the destination block's field 3. Called from `BuildingTypeConfigRecordParse` and `VehicleTypeDefinitionParse`, matching the `Upgrade: %i %i %i %i` level key documented in finding 03. |
| 0x46c510 | SceneChannelSlotsRebuild | Zeroes the three parallel channel-slot arrays at scene offsets 0x138/0x15c/0x170 (plus the 0x1dc registration counter), then walks the entity list from 0x134: per node it derives the channel pair through `FUN_0046d9f0`, applies render state (`FUN_00475710`/`FUN_00475610`), either resets via `FUN_00473370` (param 0) or re-registers via `FUN_0046c130` (param ≠ 0), and for payload nodes registers slot handles via `FUN_00473c60`/`FUN_00473bc0` into the parallel arrays with the child pointer recorded at 0x1c4. Called from `SceneActivityPairedChannelUpdate` and `FUN_0046cf90`. |
| 0x4083a0 | SceneEntitySlotTableRebuild | Same slot-registration machinery as `SceneChannelSlotsRebuild` but over the entity-list root at offset 0x9c with arrays at 0xa0/0xb8/0xc4/0xd8: zeroes the registration windows and the 0x144 counter, walks the list, derives position (`FUN_00406f50`) and renderer (`FUN_00406e30`), applies render state, then either resets or re-registers payload nodes with 2–5 slot handles keyed by scene fields 0x1c/0x24/0x20. Both callers pass a clear-register flag: `SceneActivityChange` (re-register) and `EntityPayloadStateSet` (clear). |
| 0x462f20 | TerrainCellQuadGeometryEmit | Indexes the cell record array (`DAT_00552198 + (y*DAT_00552184 + x)*8`); when the flag byte's bit 1 is set, allocates a quad entity through `FUN_004748f0` (mode 4/2/3), positions it (`FUN_00474da0`), updates visibility (`FUN_00474e30`/`FUN_00474b30`), clears bit 1, sets bit 2 and `DAT_00552190=1`, then compares the four corner pointers' z-components to pick the lower corner, reorder the vertex order (flag bit 4), and hand the vertex block to `TerrainCellQuadVertexEmit`. Sole caller is `TerrainCellMutationProgress`. |
| 0x46db90 | CellQuadRunStage | Walks the whole cell grid row by row; for each maximal horizontal run of cells whose record byte reads type 7 it stages a run record through `FUN_0046e9d0` (with `FUN_0046eb30`/`FUN_0046ead0` fallback/merge paths), accumulates vertex staging in `DAT_0054beb8`, sorts the run-record table (`FUN_0048de20` with callback `LAB_0046e9a0`), trims trailing null records, then calls `FUN_0046dd10` and `CellQuadRunEmit`. Sole caller is `MapLoadPipeline`; the run records live in the `DAT_0054beb0`/`DAT_00549840` table. What cell type 7 denotes remains open. |
| 0x46de80 | CellQuadRunEmit | For each staged run record: clears the record's minimum-height field, samples terrain height at its footprint corners via `FUN_004500d0` through the param's terrain lookup (raising the height floor where cells are present), allocates the run's quad entity via `FUN_00473560`/`FUN_00473b40`/`FUN_004748f0`, sets its texture/scale coordinates (`FUN_00475020`), and emits nine vertices per quad with the staged corner offsets through `FUN_00474d30`. Sole caller is `CellQuadRunStage`. |
| 0x4515e0 | TerrainCellRayHitTest | Given a terrain object (stride field at +8, 0x28-byte cell array at +0x28) and cell coordinates, reads both triangles' corner vertices and performs ray/plane intersection tests (`FUN_004798e0`/`FUN_00479b80`) against the incoming ray origin/direction, returning the hit flag and writing the hit point. Callers are `ScreenRayTerrainCellPick` and `FUN_00471630`. |
| 0x43eea0 | SimulationEntityQueueFlush | Iterates the pending-record array of length field 0xc9: clears each record's 0xbf flag, and under the `FUN_00469e60` mode branch either processes the record directly (type 7 special case raising height through the `DAT_00556380` grid and `FUN_00437540`; otherwise `FUN_0043a850`) or appends it to the deferred queue at +0x308/+0x324 setting the 0x400 dirty bit, compacts the array when the queue is full, and finally zeroes the count, the 0xbf flag, and bit 10 of field 0xf8. Sole caller is `EntityStateReset`. |
| 0x41ee80 | CellGridPointHitTest | Gates on state flag 0x800 and the `FUN_0040af90` grid enumeration; converts the float coordinates to cell indices with the ±15/±40 tolerance band, walks the returned cell records comparing against the two 16-byte cell-dimension globals (`DAT_004de1e8`/`DAT_004de1e4`, defaults 0x14/0x28), and returns the matching cell pointer and dimensions. Sole caller is `SimulationPointerActionDispatch`. |
| 0x4355c0 | MenuOptionPanelTextureUpdate | Called only from the ported `MenuTick`: walks the option-panel widget records, and for widgets whose value changed since last tick, redraws the option state into the widget's texture through the surface-lock/blit and text-draw helpers (`FUN_0047a540`, `FUN_0047dcb0`, `FUN_00485230`/`FUN_00485390`/`FUN_004859b0`), updating the cached comparison fields. |
| 0x462ad0 | RewardScrollTickerAdvanceAndDraw | Called only from `RewardScreenTick`: advances the reward scroll accumulator, and each frame re-renders the scrolling label into the reward banner texture via `FUN_0047a180`/`FUN_0047dcb0`/`FUN_004851b0`, blits the result, and updates the ticker position/offset floats. |
| 0x4181b0 | HelpWindowResourcesInit | Called only from the ported `GameConfigInit`: locates the `HelpWindow` and `TextWindowArea` controls, publishes the `CanBuildMessage`, `CanTransportMessage`, and `CanBuildAndTransportMessage` label pointers, and hands the control block to the window-init path (`FUN_00418450`/`FUN_004185f0`/`FUN_00419200`). |
| 0x4403d0 | LevelDebugCellRetargetDispatch | Called only from `LevelDebugInputCellMutatorDispatch`: guards entity type and the +0xbc/+0x60/+0x36c object fields, selects a target cell through the +0x3f0 flag-bit dispatch (`FUN_0044a200`, or the plane-intersect chain `FUN_00439e40`/`FUN_004424f0`/`FUN_004469c0`, or the light branch `FUN_004087e0`), writes the chosen weight float (0.5/0.85/0.88/1.0) and flag/mode bytes into the cell record, computes the four-way direction code from the cell position (`FUN_004754c0`/`FUN_00475500`), invokes the move/path helpers `FUN_00441700`/`FUN_004410f0`, and stores the second object at entity+0x2fc. |
| 0x45b590 | RectContainsOffsetPoint | Takes a rectangle base pointer, a float offset pair, and the candidate point coordinates and returns whether the offset point lies inside the rectangle — the containment test used by the category-cell hit walk. No callers are registered in the symbol table (the reference resolves to an indirect thunk), so the name is grounded in the decompiled arithmetic and its use inside `SimCategoryCellHitTest`'s geometry test. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-13.bundle](../evidence/legorr-engine-helpers-expansion-13.bundle):
`ev_eb37ab68644f9cbb18fccfb804eb12064027b95de72bb965d228b8094ef29386`
(batch decompile of all fourteen addresses). Caller observations are the
matching `procedure_callers` records, including
`ev_540c7ab03009310e9c6a9a5af6ff2688fc46a6e77e098a691579591219954bf3`
(0x46c160), `ev_20e745a60da31c99c48b4c82664fe16b238b25a2d51f8856293d8351f9855fb9`
(0x46db90 from `MapLoadPipeline`),
`ev_b01c255e488180fb0151825a7187240342ba7b50856380130b7398c1a3905f69`
(0x4403d0 from `LevelDebugInputCellMutatorDispatch`),
`ev_2175afeb24a03466dc381e4eed24ec2cc34f8e1a27656e6e8ef20fd4cb1218f4`
(0x4083a0 from `SceneActivityChange`/`EntityPayloadStateSet`),
`ev_2e346e09e87cee40f7ac50e5708bdc68c57146f8dbd289fd4c9801c1d7c81de1`
(0x462f20 from `TerrainCellMutationProgress`), and
`ev_57e973455ef25f0ad1761de808efafa90185408ab1a52f647bf31470c19d47f0`
(0x4515e0 from `ScreenRayTerrainCellPick`).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The three light-property wrappers `0x473460`, `0x473480`,
and `0x4734a0` are byte-identical apart from the native light-interface vtable
slot they dispatch (+0x40, +0x3c, +0x38); without a Direct3D-executable light
interface description the individual properties cannot be told apart, so they
stay anonymous. `0x44d280` (sole caller the ported "editor steps" ticker
`FUN_0044cfa0`) is a timer-driven exponential glide of a 3D position through
the `DAT_004eadec`–`DAT_004eaf0x` global cluster with the velocity components
normalized by `FUN_0044d710`; neither "editor" nor "camera" is directly
evidenced for those globals, so it stays anonymous too. The unresolved trio
`0x4802d0`/`0x480390`/`0x480570` from finding 240 keeps its status. The large
anonymous candidates `0x4438b0`, `0x444680`, `0x4469c0`, `0x442f80`,
`0x442b10`, `0x40d850`, `0x44ead0`, `0x405950`, `0x407a10`, `0x463230`, and
`0x40ab70` remain unnamed pending deeper traces.
