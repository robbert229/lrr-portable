# Finding 253 — Terrain Scatter, Erosion Propagation, Cell Queries, Menu Records, and Buffer Helpers

Fifteen more anonymous procedures now carry names grounded in their own
instruction sequences, each with at least one named or ported caller:
`TerrainCellVariantUpdateQueue`, `EntityCellPathEffectsApply`,
`OListEntityRecordParse`, `DeferredOListEntityCreateAndLink`,
`AnimatedTextureReplace`, `MeshBuildEightSidedSegment`, `LwoSurfaceSettings`,
`PathStepAdvanceAndArrivalCheck`, `0x46f920` (ported),
`LavaErosionNeighborMutate`, `TerrainStampImpactHandler`, `CellInitPass`,
`SaveMenuBuild`, `SavMenuList`, `DirectDrawRendererInit`,
`SimCursorInteractionTick`, `MenuTick`, `SceneRecordTemplateInstantiate`, and
`TerrainPointerCommandUpdate`, plus named callees `EntityTeardown`,
`CampaignLevelFallbackLabel`, `TerrainGridPick`, `TerrainCellSurfaceVerticesGet`,
`TextBufferDrawWrapped`, `TextBufferReset`, `TextBufferAppendFormat`,
`LavaErosionNeighborEligible`, `LwoLayerPolygonVertexDataRead`,
`EngineHeapAlloc`, `EntityCreateWithPosition`, `MapCellToWorldCenter`,
`SurfaceBlitRect`, `SceneRenderLockToggle`, `DDiSceneFrameUpdateAndCallbacks`,
`DDiSceneObjectPositionSet`, and `DDiSceneObjectPoseMethodDispatch`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4068c0 | PathArcLengthInterpolate | Walks the cumulative step-length array at `param_1 + 0x66` until the running sum passes `param_2`, then interpolates the `(x, y)` pair between the surrounding vertex pairs (float-pair array at `param_1`, stride `uVar5*2`) with the fraction `(param_2 − entry[i]) / entry[i]`, returning the segment index; at the last segment it just copies the final pair. Callers `PathStepAdvanceAndArrivalCheck` + ported `0x46f920`. |
| 0x40c230 | SceneSlotPoseScaleRegister | Searches the four slot words of the entry block (`DAT_004c7214 + (param_1 + DAT_004c79a4 * 3) * 0xa4`) for a free one; on a hit runs `SceneRenderLockToggle(handle, 0)`, `DDiSceneFrameUpdateAndCallbacks(handle, 0)`, `DDiSceneObjectPositionSet(handle, 0, param_4, param_5, param_6)`, and `DDiSceneObjectPoseMethodDispatch(handle, 0, param_7, param_8, …, −2.0)`, stores `param_2`/`param_3` into the slot's `+0x24`/`+0x34` fields, clears the slot word, and returns 1; returns 0 when all four are taken. Callers `LavaErosionNeighborMutate` + `TerrainStampImpactHandler`. |
| 0x40e350 | CellEntityKindCreateOrRemove | Through the cell grid table (`DAT_004c7df8`, row stride `DAT_004c7dfc + 0x14`): with `param_4 == 0` tears down the cell's tracked entity (`EntityTeardown`) and zeroes both slot words; with `param_4` set and the slot empty centers through `MapCellToWorldCenter` and creates `EntityCreateWithPosition(DAT_0055639c, kind 0xe, 0, 0, center, 0)` into the slot. Sole caller `CellInitPass`. |
| 0x40f5f0 | ErosionEightNeighborPropagate | Runs the eight neighbor-offset table: for each neighbor that passes the `FUN_004502c0(DAT_005560d4 + 4, x, y)` bounds check, when the cell is free per `FUN_0040f190(cell, 0)` and `LavaErosionNeighborEligible` answers, applies `FUN_0040f330(cell, param_2, 0)` (mutual recursion with `LavaErosionNeighborMutate`). Sole caller `LavaErosionNeighborMutate`. |
| 0x4105f0 | MenuParamRecordCreate | Allocates the 92-byte record (`EngineHeapAlloc(0x5c)`), zeroes the 23 dwords, and distributes the fourteen parameters into the `+10…+22` slots, resolving the `+8/+9` pair through `CampaignLevelFallbackLabel(param_2/param_3)`, `+10 = param_1`, the `+12/+13`, `+14/+15`, `+16/+17`, `+20/+21` pairs from `param_5…param_13`, the untouched `+19`, and `param_14` last. Callers `SaveMenuBuild` + `SavMenuList`. |
| 0x4129d0 | MenuOverlaySurfaceAndTextSetup | When `*param_1` equals either game-state id in `*DAT_00556fc4 + 4/8`: blits the record's surface (`SurfaceBlitRect(record[1], 0, rect +6/+7/+8/+9)`), then for the text buffer runs `TextBufferDrawWrapped(obj, 0, 1.0, 0)`, `TextBufferReset(obj)`, `TextBufferAppendFormat(obj, &DAT_004a238c)`, conditionally appends the record's `+0x14a` (first state) or `+0x18a` (second state) string with a trailing marker reset, and raises `DAT_004db8c4`. Sole caller `MenuTick`. |
| 0x419de0 | RecordListEntryMoveToFront | When `param_1` is below the `DAT_004dcd38` count and nonzero: rotates the `DAT_004dca18` five-dword-stride record array so the entry at `param_1` becomes the head (the tail-shift copy loop moves each entry down, the saved entry lands at index 0), then raises `DAT_004dcd48` bits 0 and 1. Callers `SimCursorInteractionTick` + `FUN_0041a1a0`. |
| 0x435230 | RandomVariantCellScatter | Clears the 10×10 tried-cell grid and loops up to `(2·param_2)²` attempts (`>> 2` when `param_3 == 0`): two `rand()` draws map into cells within `param_1[0/1] ± param_2`; untried cells are queried with the cell test `FUN_0040f190(cell, 1)` — returning early when the cell is occupied and `param_3` is set — and marked tried. Sole caller `TerrainCellVariantUpdateQueue`. |
| 0x4507d0 | TerrainCellEdgeVectorQuery | After the grid-header bounds check (`param_1[2]/[3]` extents), classifies the edge code from the cell position: at both minima → 2 (with the `-1` steps), first row → 1, first column → 3 (via `-1`), and `+1` when the cell byte `+0x19` bit 1 is set; queries `LwoLayerPolygonVertexDataRead(param_1[9], row * cols + col, code, 1, &vec)` and copies the resulting triple out. Callers `CellInitPass` + `TerrainStampImpactHandler`. |
| 0x451b30 | LinkedRecordListFlatten | Walks the six-dword linked list (link at `+0x14`) counting entries, allocates a `count * 0x18` block via `EngineHeapAlloc`, and copies each entry's six dwords into the stride-`0x18` record slots with the slot flag bit `1` raised, the slot-forward address stored, the `+1` word cleared, the header word raised with bit `2`, and the last entry's link word nulled. Callers `SceneRecordTemplateInstantiate` + `FUN_00406cb0`. |
| 0x45e6d0 | TerrainPointPickResolve | After the grid-rectangle bounds (`param_1[1]/[2]` origin, `param_1[3]/[4]` extents), transforms the `(param_2, param_3)` point through the centered inverse-scale mapping (`1/param_1[5]`, `param_1[6…8]`) and runs `TerrainGridPick(*param_1, x, y, param_5, param_6)`, writing the picked coordinates out on success. Callers `TerrainPointerCommandUpdate` + `FUN_0045e820`. |
| 0x464110 | RotatedPairsScratchFill | Copies `param_3` `(x, y)` float pairs from `param_2` into the global scratch `DAT_005013c8`, rotating each through the `((param_4 + 32768) mod 4)` quadrant cases (swap-and-negate variants) and translating by `(param_1[0], param_1[1])`, and returns the scratch pointer. Callers `EntityCellPathEffectsApply`, `OListEntityRecordParse`, `DeferredOListEntityCreateAndLink`, `AnimatedTextureReplace`. |
| 0x4780b0 | RendererWindowModeApply | Stores the extent pair at `DAT_00505924/28`, raises `DAT_00505f84` bit 2 for nonzero `param_1`, runs `FUN_0048b280(param_1)`, then: for `param_1 == 0` installs window style `0x80c80000` (`SetWindowLongA(-16)`), `SetWindowPos` to the `(param_2, param_3)` origin with the stored extent, and `ShowCursor(FALSE)`; otherwise `SetWindowPos` to the desktop window rect and `SetCursor(NULL)`; both branches end with `ShowWindow(SW_SHOW)` and `SetActiveWindow`. Sole caller `DirectDrawRendererInit`. |
| 0x482620 | MeshSegmentVertexFill | Fills `param_4` vertex entries of the render record's slot `param_2` (record base at `param_1 + 8`, `0x70` stride, vertex buffer at slot `+0xc`) at `0x20` stride: the three position dwords from `param_5 + 8` (advanced 3 per entry), the fixed `param_6` triple at `+12/+16/+20`, and the `param_7` pair at `+24/+28` (advanced 2 per entry) — the segment twin of `MeshVertexRecordFill`. Sole caller `MeshBuildEightSidedSegment`. |
| 0x48c040 | StringCaseSubstringFind | Takes both string lengths the open-coded way, and when `param_1` has length greater than 1 and `param_2` is longer than one character, walks `param_1` advancing one character per step while `__strnicmp` over the leading `len(param_2) − 1` characters fails, returning the first match position (with the match index into `param_3` and length into `param_4`) or null. Sole caller `LwoSurfaceSettings`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-25.bundle` —
batch decompile record
`ev_69cba2c3782f8969d797025859837b79afe7d184db79f76640f057f46567147d`
(15 addresses), plus one `procedure_callers` record per named address
(`0x4068c0` `ev_ff894802…`, `0x40c230` `ev_cd58e630…`, `0x40e350`
`ev_02a7c1c3…`, `0x40f5f0` `ev_46fcf204…`, `0x4105f0` `ev_f9175244…`,
`0x4129d0` `ev_acdc9693…`, `0x419de0` `ev_2914aac5…`, `0x435230`
`ev_e18d598b…`, `0x4507d0` `ev_b821f4fb…`, `0x451b30` `ev_aaffad55…`,
`0x45e6d0` `ev_1576672f…`, `0x464110` `ev_89a9fea5…`, `0x4780b0`
`ev_db467a89…`, `0x482620` `ev_c3418854…`, `0x48c040` `ev_718427f5…`).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The command-region trio `0x43a120`, `0x448c90`, and
`0x4490a0` (static call sites still resolve `not_in_procedure`; the next look
needs a call-site instruction dump), and the steering-slot pair
`0x471f00`/`0x471d20` (the slot-base accessor `0x4712c0` reads only its first
argument — kind 1 → `entity[3] + 0x138`, kind 4 → `entity[6] + 0xa0`, else 0 —
so the pushed displacement arguments die there). Previously examined and
deferred: `0x476e50` (device-vtable pooled record allocation), `0x4887c0`
(recursive scene-tree registration through the `DAT_00505914` vtable
`+0x10/+0x20/+0x38`), and `0x48e280` (CRT-band FPU-precision float
conversion). Zero-caller band addresses still need string-key table or
indirect-dispatch evidence before naming: `0x476410`, `0x444460`, `0x463d20`,
`0x44b500`, `0x41c800`, `0x43aab0`, `0x439a20`, `0x4440f0`, and the
anonymous-chain band entries `0x40f920`/`0x43fc20`/`0x448eb0`/`0x40b880`/
`0x44c7f0`/`0x410490`/`0x40ba00`/`0x40bb90`. These carry forward the
unresolved set from findings 242–252 (`0x4802d0`/`0x480390`/`0x480570`,
`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`,
`0x40b000`, `0x474760`, `0x483690`, `0x471630`, `0x46f770`, `0x435b20`,
`0x465c50`/`0x473560`/`0x489660`, the CRT cluster 0x493–0x49Bxxx, and the
entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The band scan
leaves 62 further non-CRT candidates for the next batch.
