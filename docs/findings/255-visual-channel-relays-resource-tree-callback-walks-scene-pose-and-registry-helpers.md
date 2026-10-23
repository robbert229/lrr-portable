# Finding 255 — Visual Channel Relays, Resource-Tree Callback Walks, Scene Pose, and Registry Helpers

Fifteen more anonymous procedures now carry names grounded in their own
instruction sequences, each with at least one named/ported caller (directly or
through a documented anonymous relay): `CellSurfaceUpdateDispatch`,
`RockFallStylesConfigLoad`, `KindFourPostCreateResolve`,
`ResourceAvailabilitySweep`, `EntityTeardown`, `TerrainCellStateUpdate`,
`SimCursorInteractionTick`, `SoundMinimumDistanceUpdateMode`, `LwscSceneLoad`,
`ObjectiveSidecarAppendGate`, `PlaceCommandEligibilityCheck`,
`ResourceNameRegisterOrQuery`, `GameConfigInit`, `SaveMenuListEntryAppend`,
`MenuItemImageSet`, `LoadItemStrings`, and `MenuBuild`, plus named callees
`EntityVisualLoadACT`, `EntityVisualResourceLoad`, `DDiSceneAdvanceByTime`,
`DDiSceneObjectDestroy`, `DDiSceneMoveOrPose`, `DDiSceneObjectPositionSet`,
`DDiSceneObjectPoseMethodDispatch`, `AnimatedTextureFrameCount`,
`AnimatedTextureFrameSet`, `CameraTeleportOrOffset`, `SceneRenderLockToggle`,
`XfTextureNameFormat`, `ConfigEntryFind`, `ConfigRgbVectorRead`,
`ConfigPathValueGet`, `ConfigStringCopy`, `ParseFloat`, `BlockTextTokenSplit`,
`RandomFloatRange`, `RandomNext`, `TerrainWalkableNeighborSearch`,
`TerrainGridPick`, `TerrainCoordinateAndOccupancyCheck`,
`TerrainCellOccupancyOrFlagCheck`, `CurrentLevelHeightGet`,
`MapCellToWorldCenter`, `EntityTransformPositionGet`,
`EntityCreateWithPosition`, `CmdCreateTask`, `EngineHeapAlloc`, and
`EngineHeapFree`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x40b880 | CellVisualChannelRelay | On a non-null cell context, runs the fourteen-entry channel table: each value passes through `EntityVisualResourceLoad` into record slots `param_1 + 8 + (cursor + n) * 4` with the cursor `+0x32c` bumped once; then, selecting triplets of loaded channel values times `0xc` off the `param_2` vertex base, issues two `FUN_0040ba00` pose dispatches for the `param_4` angle and its `(param_4 + 2) & 3` opposite. Sole caller `CellSurfaceUpdateDispatch`. |
| 0x40c0d0 | SceneActSlotLoadAndRegister | Through the entry block `DAT_004c71e0 + (param_3 + param_4 * 3) * 0xa4`: loads the visual via `EntityVisualLoadACT(param_1, param_2, PTR_DAT_004a13e0, 1)` with `DAT_004a1ddc` as fallback; on success locks with `SceneRenderLockToggle(*, 1)`, pulls four surface slots each through `EntityVisualResourceLoad` locking every result, formats `XfTextureNameFormat(handle, param_5, 0)` into `ResourceNameRegisterOrQuery(handle, name, 1, &param_3)`, and stores the resolved pair in the block's `+0x48/+0x44` words, answering 1. Sole caller `RockFallStylesConfigLoad` through `FUN_0040bfd0`. |
| 0x409470 | NeighborCellEntitySearchRecurse | Four orthogonal neighbor offsets: succeeds immediately when the neighbor resolves through `FUN_00433060`, `FUN_00438670` yields an entity, and `FUN_00438720(entity, param_2)` answers; otherwise, when `TerrainCoordinateAndOccupancyCheck` or `TerrainCellOccupancyOrFlagCheck` marks the neighbor occupied and `FUN_004322d0` reports nothing pending, runs `FUN_00432270` and recurses one cell deeper from that neighbor — returning 1 anywhere in the recursion or 0 after all four sides. Sole caller `KindFourPostCreateResolve` through `FUN_00409450`. |
| 0x40cb70 | SceneTimeoutEntrySweep | Walks the list's `param_1[10]` live entries; for the `DAT_004c7b68` global list it also accumulates `param_2` into a module timer that, past 12.5, captures the entry position via `DDiSceneMoveOrPose` and dispatches `FUN_004408c0(0, position, DAT_00556470)` before clearing the timer. Each entry whose `DDiSceneAdvanceByTime(handle, param_2)` returns nonzero is destroyed with `DDiSceneObjectDestroy` and the tail entry swapped down into its slot. Sole caller `ResourceAvailabilitySweep` through `FUN_0040cc40`. |
| 0x40ba00 | SceneRotationFlagsApply | Hands the first five arguments to `SceneFrameFromTripletApply`, unlocks via `SceneRenderLockToggle`, loops `AnimatedTextureFrameCount(handle)` writing `param_8` into every frame through `AnimatedTextureFrameSet`, applies `CameraTeleportOrOffset(handle, 1, 0xbca3d70a, 0xbca3d70a, 0)` plus scale-style calls `FUN_004755b0(..., 0x3f81eb85, 0x3f81eb85, param_6)` and `FUN_004750b0(handle, param_7 ×3, 1.0)`, then dispatches the ±π/2 axis rotations through `FUN_00475580` selected by bits 0–4 of `param_9`. Sole caller `CellVisualChannelRelay`. |
| 0x40bb90 | SceneFrameFromTripletApply | From three 3d points: normalizes the `param_3 → param_4` edge and the perpendicular from the `param_4 → param_5` edge, poses via `DDiSceneObjectPoseMethodDispatch(handle, 0, −normal, forward)` and `DDiSceneObjectPositionSet`, stores both unit vectors through `FUN_00475090`, composes a matrix through `FUN_0040bd60`, and installs it with `FUN_004756b0(handle, 1, matrix)`. Sole caller `SceneRotationFlagsApply`. |
| 0x419e80 | RecordListEntryRemoveAndRelease | Under the `DAT_004dcd38` count: resolves entry `param_1` through `FUN_004197b0(param_1, 0)` and releases its `+0xc` payload with `EngineHeapFree`; when the record's `+4` word is zero, shifts every following five-dword record down one slot and decrements the count; clearing `DAT_004dcd48` bit 0 whenever the head entry was dropped. Callers `EntityTeardown` + `TerrainCellStateUpdate` + `FUN_00419e60` + `FUN_00419f10`. |
| 0x410490 | StringRegistryIntern | Empty/null keys answer null; otherwise looks the `DAT_004db8cc` string list through `FUN_00410450`, and on a miss allocates the 16-byte record and an exact-length copy with `EngineHeapAlloc`, stores the string pointer, a null link pair, and the previous `DAT_004db8cc` head, relinks the new record as head, and returns it. Callers `SaveMenuListEntryAppend`, `MenuItemImageSet`, `LoadItemStrings`, and `MenuBuild` through `FUN_00410520`/`FUN_00410560`. |
| 0x44a3d0 | PlaceCellNearestResolve | Reads the entity position with `EntityTransformPositionGet`, sweeps the `DAT_004e84ec` cell-pair list (`DAT_004e8540` entries) through `CurrentLevelHeightGet` + `MapCellToWorldCenter`, tracks the least Euclidean distance from a 10000.0 ceiling, and copies the winning cell pair out through `param_2`, answering 1 (0 when the list resolves nothing). Callers `PlaceCommandEligibilityCheck` through `FUN_00404ed0` + `FUN_00449010`. |
| 0x44ae50 | RandomKindSixScatterSpawn | Loops `param_2` spawns: base position from `EntityTransformPositionGet` — or the `FUN_004087e0` object handle's `DDiSceneMoveOrPose` position when `param_3` is set with record kind 4 — plus two `RandomFloatRange(−5, 5)` jitter terms; validates the jittered point through `CurrentLevelHeightGet` + `TerrainGridPick` + `TerrainWalkableNeighborSearch`, keeping it only when walkable, then spawns kind `6` through `EntityCreateWithPosition(DAT_00556374, 6, 0, 0, x, y, 0)` and registers `CmdCreateTask(rec, 0)`. Sole caller `ObjectiveSidecarAppendGate` through `FUN_0044ade0`. |
| 0x44c7f0 | LightEffectsBlinkConfigApply | Reads `LightEffects/BlinkRGBMax` as an rgb vector (`ConfigEntryFind` + `ConfigRgbVectorRead`), resolves `LightEffects/MaxChangeAllowed` through `ConfigPathValueGet` — defaulting the pointer to `DAT_004b8a14` when unset — and scales the parsed float by `1/255` when nonzero; then copies `LightEffects/RandomRangeForTimeBetweenBlinks` (`ConfigStringCopy`), splits it into two fields through `BlockTextTokenSplit` against `DAT_004a1cd4`, frees the token, and feeds `(rgb triple, scale, field * 25, field2 * 25)` to `FUN_0044c970`, returning 1 only on the two-field parse. Sole caller `GameConfigInit` through `FUN_0044c7c0`. |
| 0x450a80 | CellListMinDistanceScaleApply | Enabling with `param_2` while flag bit `4` of `+0x1cf4` is clear runs `FUN_00474ff0(terrain, row * cols + col, 0.3, 0.3, 0.3)` for every entry of the tracked cell-pair array (`+0x12a2`, ushort pairs, count `+0x1c66`), then raises the bit; with `param_2 == 0` and the bit set, re-runs the same sweep with the three scale arguments zeroed and clears the bit. Sole caller `SoundMinimumDistanceUpdateMode`. |
| 0x476330 | ResourceTreeCallbackWalk | Calls `param_3(param_1, param_4)` and returns 1 when the callback matches; otherwise takes the child-container enumerator through the `+0x4c` interface slot, walks the `+0xc` count of `+0x10` child handles, takes the `DAT_0049f988` interface on each, and recurses `(child, depth + 1)` with the same callback, releasing children as it unwinds until a branch succeeds. Callers `ResourceNameRegisterOrQuery` + `FUN_00473ec0` + itself. |
| 0x489910 | ResourceTreeCallbackCount | The count-returning sibling: tries the callback first, and when the `+0x4c` enumerator slot reports no container, enumerates children through the same `DAT_0049f988` interface recursion and otherwise returns the child count when every branch fails the callback. Callers `LwscSceneLoad` through `FUN_00489520`/`FUN_004895c0` plus `FUN_00489660` and itself. |
| 0x476410 | ResourceNameMatchCallback | The `ResourceNameRegisterOrQuery` dispatch callback (documented indirect user): reads the object's name string through the interface `+0x24`, bails when the name length disagrees with the record's length word, then branches on the mode field — `__strcmpi` for case-insensitive compare, a two-byte-at-a-time exact compare otherwise — raising `EBX[3]` with the object for a match, counting mismatches for mode 1, or accepting only the ordinal at `EBX[6] + 1` for mode 2, and freeing the buffer before answering. Sole documented user: `ResourceNameRegisterOrQuery` through `ResourceTreeCallbackWalk`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-27.bundle` —
batch decompile record
`ev_81e53f10ab1b07600981f692ee5dda8a71d15e2284e8824fe7e2eece0dfdc564`
(19 addresses: the 15 named picks plus the examined-and-deferred
`0x4089d0`/`0x466490`/`0x48e0ad`/`0x48e8e0`), plus one `procedure_callers`
record per named address (`0x40b880` `ev_226adb44…`, `0x40ba00`
`ev_8d495ca1…`, `0x40bb90` `ev_3fe2cf35…`, `0x409470` `ev_5ded34f9…`,
`0x40cb70` `ev_f0da38dc…`, `0x40c0d0` `ev_16c8f07d…`, `0x410490`
`ev_d0794e67…`, `0x419e80` `ev_3407c8cf…`, `0x44a3d0` `ev_42ad4fbc…`,
`0x44ae50` `ev_5c9a2183…`, `0x44c7f0` `ev_098b7f8f…`, `0x450a80`
`ev_f707c2ac…`, `0x476330` `ev_ba0a86c2…`, `0x476410` `ev_528f2495…`,
`0x489910` `ev_d1f7abd2…`). All investigations used shipped
`reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** Newly examined and deferred: `0x4089d0` (2d
point-to-segment distance with an `atan`/`fsin` endpoint branch — its only
static caller is the zero-caller `FUN_00408b00`, so it joins the
indirect-dispatch pool), `0x466490` (checks the cell record's `+0x100` flag,
centers through `MapCellToWorldCenter`, reads the cell's world position, and
dispatches `FUN_00470260(entity, cell, &pos, 0)` raising record bit
`+0x3e0|0x2000000` on success — but zero static callers), `0x48e0ad` (the CRT
`atan` internals: `fpatan` with the `0x3ff00000`/1.0 exponent branches, the
π constant `DAT_004b1fe0`, and `__startOneArgErrorHandling`/`__math_exit`),
and `0x48e8e0` (float10 double-conversion helper with the `0x7ff0` exponent
guard funneling through `FUN_00493150`/`FUN_004931c0`). The command-region
trio `0x43a120`/`0x448c90`/`0x4490a0` (needs a call-site instruction dump)
and the steering-slot pair `0x471f00`/`0x471d20` (the slot-base accessor
`0x4712c0` reads only its first argument) continue to defer, as do the
device-interface vtable trio `0x47e300`/`0x485ea0`/`0x489b30` and the CRT
helpers `0x48e280`/`0x465c50`/`0x473560`/`0x489660`/`0x476e50`/`0x4887c0`.
The zero-caller pool `0x444460`, `0x463d20`, `0x44b500`, `0x41c800`,
`0x43aab0`, `0x439a20`, `0x4440f0`, `0x449ac0`, `0x443510`, `0x448880`,
`0x4420d0`, `0x4492b0`, `0x40f0e0`, `0x44fab0`, `0x424730`, `0x453bb0`, and
`0x44b250` still needs string-key-table or indirect-dispatch evidence, with
the anonymous-chain band entries `0x40f920`/`0x43fc20`/`0x448eb0`/`0x4089d0`/
`0x466490`/`0x410690`/`0x488a70`/`0x470540`/`0x40fde0`-rooted relays carrying
forward. The carry-forward set from findings 242–254
(`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`, `0x40b000`, `0x474760`, `0x483690`,
`0x471630`, `0x46f770`, `0x435b20`, the CRT cluster 0x493–0x49Bxxx, and the
entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`) stands. The
band scan leaves 24 further non-CRT candidates for the next batch.
