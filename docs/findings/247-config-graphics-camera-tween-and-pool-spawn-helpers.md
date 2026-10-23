# Finding 247 — Config/Graphics Quality, Camera Tween/Drag, Cell Search, and Pool/Spawn Helpers

Fourteen more anonymous procedures now carry names grounded in their own
instruction sequences. All fourteen come from the band queued at the end of
finding 246 and have callers that are already named (`GameConfigInit`,
`LevelLoad`, `TutorialScriptCommandDispatch`, `SimCursorInteractionTick`,
`MiscObjectResourcesLoad`, `BindModelTextures`, `EnginePoolAllocate`,
`ObjectiveSidecarAppendGate`-adjacent dispatch, and two sites inside the
`SimulationEntityQueueFlush` tick region), or are anchored by named callees
(`TerrainProjectedCellStateRefresh` calls `SurfaceTriangleAreaCalculate`,
`EnginePoolAllocate` calls the arena creator). The four caller-less command-
region candidates from that queue (`0x43a120`, `0x448c90`, `0x4490a0`, plus
`0x440a10`'s command-table siblings) were audited with `procedure_references`
— they do have single static call sites (see the row for `0x440a10` and the
note below) — and the ones whose command-byte semantics are still not
distinguished are queued for the next batch.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x40b470 | ProjectedQuadScreenAreaCalculate | Projects four quad points through the `param_2` matrix with `FUN_004772b0` plus the perspective divide by the w component; when `param_4 == 0` each corner whose `x`/`y` fall outside the `param_3` bounds or whose depth is negative lowers a `1.0` factor by `0.25` (and requires the two `w`-divided pairs to stay inside `param_3`); returns `(SurfaceTriangleAreaCalculate(a,b,c) + SurfaceTriangleAreaCalculate(c,d,a)) * factor`, i.e. the clipped screen-space area of the quad. Sole caller `TerrainProjectedCellStateRefresh`. |
| 0x40c990 | MiscObjectsEffectSlotBind | For each of ten effect names under the `"MiscObjects"` section (`LazerHit`, `PusherHit`, `FreezerHit`, `PathDust`, `LavaErosionSmoke1`–`4`, `BirdScarer`, `UpgradeEffect`), reads the resource-path value (`ConfigEntryFind` + `ConfigPathValueGet`) and feeds `FUN_0040c920`: that resolves the name through the `PTR_DAT_004a13e0`/`PTR_DAT_004a1d5c` tables, writes the object id into the 14-dword record slot at `DAT_004c79a8 + i*0x38 + 0x30`, and registers it with `FUN_00473b40(id, 1)`. Called once by `MiscObjectResourcesLoad`. |
| 0x41afd0 | InterfaceSurroundImagesConfigLoad | Loops the `12` config slots forming `"InterfaceSurroundImages%d"` keys (index formatted by `FUN_0049db60`), splits the `"x:y"`-style value at the `:` separator (`BlockTextTokenSplit`, `DAT_004a1cd4`) and for each side not equal to `"NULL"` loads the bitmap (`BitmapSurfaceLoad`), applies a zeroed `ImageSurfaceBlitTransformed`, records the surface handle (`FUN_0047d9d0` result) in the `DAT_004dcd5c` table and pushes its parsed width/height (`ParseIntegerWrapper`) as floats into the `DAT_004dcd8c` size table; both tables stride 2 floats/entry to `DAT_004dcde4`. Sole caller `GameConfigInit`. |
| 0x4334f0 | RenderQualityConfigApply | Under the `"Main"` config section (`DAT_004a24e4`) reads `"fpclipblocks"` (`DAT_004b8a14` default) into `_DAT_00556110`; when it is nonzero reads `"quality"` and maps `gouraud`→3, `DAT_004a476c`→2, `wireframe`→0 (unknown string → fail with 0) into `_DAT_005560d0`, then reads the `"dither"`, `"filter"`, `"blend"`, `DAT_004a4740`, `"mipmap"`, and `"linearmipmap"` booleans (`ConfigBooleanGetDefault2`) and hands the mode plus the six `== 1` results to `FUN_00478330`; returns 1 on success. Sole caller `GameConfigInit`. |
| 0x440a10 | AttachedEntityDirectionVectorUpdate | Iterates the entity's attached-object handle list (`param_1 + 0xc2`, count `param_1[0xc9]`), skipping entries with flag bit `0x2000` set at `obj + 1000`; resolves a source entity per `param_1[0]` kind (1: `FUN_0046db20(param_1[3] + cell)` with the cell from `FUN_00469be0`; 2: linked entity `param_1[0xdb]` else `FUN_00406fe0(param_1[4])`; 3: `FUN_00406fe0(param_1[5])`; 4: `FUN_00408770(param_1[6], *(obj + 800))`), reads the source pose via `EntityTypeComponentGet`/`DDiSceneMoveOrPose`/`DDiSceneObjectPositionSet`/`DDiSceneObjectPoseMethodDispatch`, and writes the distance plus normalized source-minus-object vector (object position at `obj + 0x2a0`) into `obj + 0x2ac`–`+0x2b8`. Call site `0x43ee12`, inside the `SimulationEntityQueueFlush` (`0x43eea0`) tick region. |
| 0x44a5d0 | NearestOpenCellCoordSearch | Reads the entity cell (`EntityPositionToTerrainCell`) and world position (`EntityTransformPositionGet`), scans the `21×21` cell box around it, keeps cells whose walkable flag (`TerrainCellWalkableFlagCheck`) is set and — unless `param_6`/`param_5` disable them — no blocked/unit flags (`FUN_00432d90`/`FUN_00432e60`), computes the world-center distance via `MapCellToWorldCenter` (rejecting nearer than `*(+0x10)` when `param_4 != 0`), accumulates `(x, y, dist)` triples for at most `100` cells, sorts them through `ArraySortWithComparator(LAB_0044a7a0)` and returns the nearest cell coordinates through `param_2`/`param_3`. Callers `LevelDebugInputCellMutatorDispatch`, `EntityConditionalTypeFiveSpawn`, and `FUN_00449700`. |
| 0x44dde0 | StringTableEntrySetCurrent | With `param_1 == NULL` clears flag bit 1 of `_DAT_0055608c` and returns; otherwise searches the `50`-entry `{string, a, b}` table at `DAT_00555e08` for an `__strcmpi` match, pointing `_DAT_00556060` at the entry and clearing its second field; when no entry matched, copies the string into the previously reused slot (`EngineHeapAlloc` plus the byte/dword copy loop) and repoints `_DAT_00556060`. Finally raises flag bit 1, sets `_DAT_00556088 = -1.0`, and calls `FUN_0044dec0(0, 0, 0)`. Callers `GameConfigInit` and `LevelLoad`. |
| 0x453da0 | CameraFocusTweenUpdate | While `_DAT_00555d40` is active resolves the tracked entity from the list slot `DAT_00555d44` (`FUN_0042dff0`, max 10) or directly from `_DAT_00555d50`, refreshes its cell/position globals (`DAT_00555d48/4c`), and clamps the viewport target with `ViewportTargetPositionClamp(entity, 0, 1)`. Then advances the camera: the zoom tween (`DAT_00555d54`) steps `_DAT_00555d5c` toward `_DAT_00555d58` by `1/(dt*25)` while feeding the delta to `CameraZoomUpdate(DAT_005560e4, …)` (done inside a ±5 window), and the tilt tween (`DAT_00555d60`) moves `_DAT_00555d68` toward `_DAT_00555d64` scaled by `FUN_004230b0 · (gap/6) · (1/3)`, dispatching `CameraTiltAxisUpdate(cam, f · 0.017453)`; when the `FUN_00431560` scene record's `+0x27c` is set it also kicks `FUN_004538a0` with `{100, 0}`. Sole caller `TutorialScriptCommandDispatch`. |
| 0x45bcd0 | CameraDragFromCursorApply | Hit-tests the cursor against the drag anchor (`FUN_0045b8a0(x, y, DAT_00500824, DAT_00500828, DAT_0050082c)`); a miss without the started bit either returns 0 or clamps the cursor (`FUN_0045b930`) plus `FUN_0047f0d0`; a hit with `mode 4` returns 1. Otherwise marks `_DAT_00500860` bit 1, derives the drag vector through `FUN_0045b900`, and — after the `mode 3` single-axis checks — applies `CameraTiltAxisUpdate(cam, dx · dt · 0.04)` and `CameraPanAxisUpdate(cam, dy · dt · 0.04)` to `DAT_005560e4`, latching vertical bits `0x10/0x20` and horizontal bits `0x40/0x80` from the delta signs. Sole caller `SimCursorInteractionTick`. |
| 0x45d8b0 | ScreenCoordNormalizeInit | Scales the `float` block `DAT_004a8f28`–`DAT_004a9090` by `1/255`; writes the two `1.0` entries of each `DAT_004a90a4`-stride record; stores the seven `0x43xxxxxx`/`0x43xxyyyy` coordinate pairs (`420/180`, `360/180`, `360/400`, `280/400`, `280/180`, `220/180`, `320/40`) duplicated into the mirrored `DAT_004a90xx`/`DAT_004a91xx` slots, then multiplies each pair's `x` by `FUN_00401c30()/640` and `y` by `FUN_00401c40()/480` (`DAT_004a9318 = 7` iterations), fixes the `1.0` constants, and marks `DAT_004a9090/94 = -1`. Sole caller `GameConfigInit`. |
| 0x46faf0 | EntitySpawnRecordSlotInit | Maps the request kind (`1/2/3/4` → cell pair `(19,0)/(16,0)/(17,0)/(5,0)`, `FUN_0046fef0`), validates the entity pointer (`EntityPointer`), claims the first free slot of the `0xac`-stride spawn array (`FUN_0046ff70`, `-1` gives up), then writes the record: `DAT_0050391c` slot flag cleared, `DAT_00503bc4/3bc8` cell x/y, the `param_2` position pairs into `DAT_00503b90…3b98`/`3b84…3b8c`, the `1000.0` speed constant `0x40a00000` at `DAT_00503ba8`, the normalized `param_3` direction into `DAT_00503bac…3bb4`, and the object id at `DAT_00503bbc`; calls `EntityCreateWithPosition(param_1, cell, cell, 0, pos)` storing the instance through `FUN_0046f920`-tracked index. Kind `1` first raises a `1.0` gate via `FUN_00437300`. Caller `FUN_004420d0`. |
| 0x471fc0 | EntityVelocitySlotDispatch | For an entity whose `+0x3ec` flag bit 1 is set, loops the object array's slot count (`FUN_004712c0(entity) + 0xa4`, the type-`1`/`4` array base) calling `FUN_00471d20(entity, param_2, slot)` per slot; otherwise the same loop calls `FUN_00471f00(entity, param_2, slot)`. Call site `0x43ee2d`, inside the `SimulationEntityQueueFlush` (`0x43eea0`) tick region. |
| 0x481ac0 | StringTrailingDigitsSplit | Scans `param_1` from its second-to-last byte while the bytes are decimal digits, parses the digit run backwards into the integer `*param_3`, copies the remaining prefix into `param_2` (dword/byte copy loop, terminating `param_2[iVar7+1]`), and reports the stripped length `(len − pos − 2)` in `*param_4`; returns `NULL` when the tail is not a digit run. Sole caller `BindModelTextures`. |
| 0x4915a0 | EnginePoolArenaCreate | Allocates the pool control block — static `PTR_LOOP_004affb8` block when `DAT_004affc8 == -1`, else a `HeapAlloc` `0x2020` record chained into the loop list — then `VirtualAlloc`s a `4 MB` reserve plus a committed `0x10000` page; the control block records the arena (`lpMem[4]`) and free frontier (`lpMem[5]`), builds `0x400` free-block descriptors (`[next/flag-1, 0xf1]`) after the header, zeroes the committed page, and primes the first `16 KB` of the arena with self-linked `0xf0` blocks; frees the reserve/control block on failure. Callers `EnginePoolAllocate` and `FUN_00491560`. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-19.bundle](../evidence/legorr-engine-helpers-expansion-19.bundle):
the `ev_5b180c7b816a8ad36ecc7840a2955f07fd8b1ae01fa34f8b38c0c8660e5c30da`
record holds the batch decompile of all twenty queued candidates. Caller
observations: `ev_b03773b067b6575efbe0c2b214ba4c32b10d41a9b512bc89c6a818508ecd1e4d`
(0x40b470), `ev_677595ab54e863dd424e3450ade7ed5353fa430d86d3196d88ec6a5fc6b35041`
(0x40c990), `ev_9fea89c442f035e58de6ca802c856a89460f741084f65dd847cf5e6e3b1ba960`
(0x41afd0), `ev_6495580c93e362fd39061861ae1fc0e4a07ba7e7af3cfb7c5d77b8b19e830176`
(0x4334f0), `ev_c8f87a90def05dd9ce27b3d87ff0436966255eea5ecd908a804389192b00be2c`
(0x440a10), `ev_172b55e2f632978cc8723584e891f5af4cd2c5d6ded31366224bdd0c820fe7ae`
(0x44a5d0), `ev_683d5fec51e11ad723d7dc09112071ad399204d19ee4ecdcc92513995ef4a89a`
(0x44dde0), `ev_c98cca08d7930dcd68b696672d47d1ad932063fc49382a0376c44e0634841249`
(0x453da0), `ev_e152495a14dcb06341ed6f9c8408aea6dc1ab85cfe1e821196f7ab3b00b96b7c`
(0x45bcd0), `ev_5287096ab226bc671693cfb01e4abe09f6e9376eb53d773f113f670b54547ff4`
(0x45d8b0), `ev_ae554c76e97196c7d36fce42c24a42b4336ecae92568fa3067c34ea1b8d6c3df`
(0x46faf0), `ev_50a8c5782148a5627cf6e61d66595c252d40142b2f8fe0f857d7a37083e74bc8`
(0x471fc0), `ev_d1c651cd31b3005ba36f6a7f2222000090ce04a34c3650c2f184a85429f6b94d`
(0x481ac0), and `ev_5fafe153eb2007e7865954c16f7c86880479eec1110bedc561840deb2d1c4781`
(0x4915a0). The four originally caller-less commands were re-audited with
`procedure_references`: `ev_84cde83440b6d76c353804ad246faf1d3ae60c023697d117ee85c992e174a1ed`
(0x43a120: call sites `0x404577`/`0x404598` plus the `0x43a2f0` jump-table
data), `ev_0663b6584af0a8920179323ca012e9c0676aa1c5c111b012f1275f89b55c2a57`
(0x448c90: call site `0x40467d`), `ev_812ad93b1b211561945547a6854680484341f223d7bdf2828031423bad3778f6`
(0x4490a0: call site `0x4045c9`), and `ev_35d2cfb6d0f798eb95472ecf1327e00936a39f3724ee1ff37f932b15a86b3f93`
(0x471fc0: call site `0x43ee2d`). Supporting callee decompiles are the
batch records `ev_e31628f471430de4fabc871cdc5a118a6f1cf7bf28a197149f32d4060fa84c05`
(`0x40c920`, `0x46f270`, `0x4712c0`, `0x4712f0`) and
`ev_154cfbe128729f166c7a35c2f2c6cdeaef26eb2ea90ddf7d020f8ba3d0e6ac06`
(`0x401c30`, `0x401c40`, `0x46fef0`, `0x46ff70`). All investigations used
shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x409d50` (the eight-direction cell-line loop its
caller `ObjectiveSidecarAppendGate` runs — needs `FUN_00464110`/`FUN_00409f20`
resolved first), the command-region handlers `0x43a120`, `0x448c90`, and
`0x4490a0` (each writes the entity command record's opcode/flag/coordinate
slots at `param_1[9] ± offset ± param_1[10]*0x14` with opcodes `8/9`, `0xf`,
and `6`, but the three command kinds are not yet distinguished), and the
steering-slot pair `0x471f00`/`0x471d20` (both consume the `FUN_004712c0`
object array; `0x471d20` additionally maintains the laser-beam path through
`FUN_0046f0d0`/`FUN_0046ffb0`/`FUN_0046f270`, so the pair's split needs its
own batch). These carry forward the unresolved set from findings 240–246
(`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`, `0x418a00`, `0x40b000`, `0x474760`, the
CRT cluster 0x493–0x49Bxxx, and the entity scalar accessors
`FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The `900 B`-and-up anonymous band
scan continues to queue candidates for the next batch.
