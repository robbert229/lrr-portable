# Finding 248 — Config-Band Loaders, Display-Mode Setup, Directional Activity, and Flight-Record Helpers

Fourteen more anonymous procedures now carry names grounded in their own
instruction sequences. Eight of them are the remaining `GameConfigInit`
band candidates: `GameConfigInit` is their sole caller, which grounds the
family (config loading / string-table initialization) while each name comes
from the function's own strings or stores. The rest come from the finding-247
queue (`0x409d50`, whose two blocking callees `FUN_00464110` and
`FUN_00409f20` were resolved first this batch) and from band candidates whose
named callers or callees (`InstanceQuadVerticesBuild`,
`EntityNeighborBlockedCellStepProbe`, `LavaEffectSelectAndPlace`,
`EntityCreateWithPosition`, `CellInitPass`, `DDiSceneObject*`) ground the
behavior.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x401530 | AdvisorPositionConfigLoad | Formats `"AdvisorPositions_%d_%d"` keys from `param_3`/`param_4` (`FUN_0048d9e0`), walks the config section (`ConfigSectionFind` + `ConfigEntryNext`), parses each entry key through `FUN_004013e0`, splits the value name (separator `DAT_004a13e4` via `FUN_00477440`/`FUN_004014c0`), validates the name against the name/entity tables (rejects the `DAT_004a13f4` placeholder unless `FUN_0045a370` resolves it; requires `FUN_0046aad0`/`FUN_00464c70` hits), converts the two numeric tokens to floats (`FUN_0048d980`), and hands `(parsed entity, cell, name, x, y)` pairs to `FUN_004017b0`. Sole caller `GameConfigInit`. |
| 0x4070b0 | BubbleActionStringsInit | Stores the fixed table of `Bubble_*` caption strings (`CantDo`, `Idle`, `CollectCrystal`/`Ore`/`Stud`/`Dynamite`/`ElecFence`/`Barrier`/`Drill`/`Spade`/`Hammer`/`Spanner`/`Laser`/`Pusher`/`Freezer`/`BirdScarer`, `Carry*` for each carried item, …) into the contiguous `DAT_00557900`–`DAT_00557998` pointer block in slot order. Sole caller `GameConfigInit`. |
| 0x409d50 | EntityCellLineLavaEffectPlace | Reads the entity velocity (`FUN_004422a0`), cell (`EntityPositionToTerrainCell`) and world position (`EntityTransformPositionGet`), picks one of four cardinal directions by the dominant signed axis, builds the rotated cell-coordinate list from the entity's line buffer (`DAT_005561e0 + entityId * 0x14c`) through `FUN_00464110` (quadrant-rotated/translated point copy), and walks the cells skipping consecutive duplicates: clears per-cell work state (`FUN_00432360`/`FUN_004323c0`), and unless the entity `+0x3ec` bit 3 once-flag is set, samples height (`CurrentLevelHeightGet` + `TerrainSurfaceHeightInterpolate`), jitters with `D3DRMVectorRandom`, places effect kind 3 with `LavaEffectSelectAndPlace`, and calls `TerrainCellWorkFlagClear(cell, 0x3b)`; every visited cell goes through `FUN_0040cea0` (`CellInitPass` plus the four-neighbor propagation), and `FUN_00409f20` finally raises the once-flag through the build-placement command loops. Sole caller `ObjectiveSidecarAppendGate`. |
| 0x41aa00 | EntityTableMapsAllocate | Allocates two parallel sets of three zeroed engine-heap maps via `EngineHeapAlloc` (`FUN_0048dbd0`), sized by the two entity-count globals `DAT_0055627c`/`DAT_00556288`: a `dword×count` array (`DAT_004ddad4`/`DAT_004ddad8`), a byte map (`DAT_004dd99c`/`DAT_004dd9a0`, zeroed four-dwords-then-tail), and another `dword×count` array (`DAT_004dd9a4`/`DAT_004dd9a8`). Sole caller `GameConfigInit`. |
| 0x4345f0 | PanelsConfigEntryApply | Formats `"Panels_%d_%d"` keys, walks the entries, and for entries whose object name resolves through `FUN_0045a370`: byte-copies the value string, tokenizes it at the `DAT_004a13e4` separator, registers the name (`FUN_0045a2e0(name, key, 4)`), and converts the four numeric tokens through the chained `FUN_0048db70` conversions into a packed value stored via `FUN_0045a650`. Sole caller `GameConfigInit`. |
| 0x434af0 | TextMessagesConfigLoad | Walks the `"TextMessages"` section mapping each name → value through `FUN_0046aad0`/`FUN_0046ab10`, then walks `"TextMessagesWithImages"`, copying the value and splitting it into three tokens at the `DAT_004a1cd4` separator for `FUN_0046abb0(id, token, token, token)`. Sole caller `GameConfigInit`. |
| 0x437150 | ToolTipIconsConfigLoad | For each `"ToolTipIcons"` entry, matches the entry name against the 11-name table at `DAT_005561fc` (→ `FUN_0047d9d0(path)` handle stored into `DAT_004ead90 + i`) with fallback to the 6-name table at `DAT_004ead60` (→ `DAT_004ead78 + i`); `"Blank"` fills `DAT_004eadbc`, the `PTR_DAT_004a1e98` name fills `DAT_004eadc0`, and an exhausted section releases the handle via `FUN_0047dbf0(DAT_004eadc0,…)`. Sole caller `GameConfigInit`. |
| 0x444260 | EntityRandomCellStepRouteIssue | Gives up while `FUN_0043baf0` marks the entity busy; reads the current cell (`FUN_004424f0`) and tries up to 16 times: draws a random index (`thunk_FUN_0048e170() % 16`) into the fixed 16-entry `(dx,dy)` step table, route/occupancy-checks the neighbor (`TerrainCellRouteAndOccupancyCheck`), takes the cell surface (`FUN_00431570` → `TerrainCellSurfaceVerticesGet`), jitters the target through `FUN_004798b0`, and issues the route with `TerrainNeighborPathCheck` + `FUN_00441700` route setup, freeing the scratch buffers and returning 1 on success. Sole caller `EntityNeighborBlockedCellStepProbe`. |
| 0x447950 | EntityDirectionalActivityStart | Requires the `+0xf8` bit `0x1000` unset and `EntitySharedEligibilityCheck(e, 1)` (re-indirecting through the attached entity at `+0xf0` when the `+0xf9` bit 1 is set); without a vector it picks activity opcode `0x24`/`0x25` by `param_3`, otherwise normalizes the entity `+0xab/+0xac` offset and selects the 45°-cone opcode `0x17`–`0x1a` from the vector's dot/cross sign; issues `EntityStateReset(e, 0, 1)` + `EntityActivityRequestSet(e, op, 1)`, retries with opcode 0 when `SceneActivityUpdateDispatch` fails, then ticks `EntityActivityTick(e, 1.0)` and raises the `0x1000` flag (clearing bit 31 of `+0xfa` for kind 3). Callers include `LaserSpatialSoundAndImpact` and the laser-update helpers `FUN_0046f270`/`FUN_0046f380`. |
| 0x46a7f0 | UiMessageStringsInit | Stores the 27 action/help message strings (`s_Text_Walk`, `s_Text_Drill`/`NoDrill`/`CantDrill`, `Reinforce`/`NoReinforce`/`CantReinforce`, `PickupCrystal`, `Encyclopedia`, `RadarHelp`, `BuildHelp`, `SelectionHelp`, `Drive`/`CantDrive`, `MakeTeleporter`, `CannotPlaceBuilding`, `CrystalOreDisabled`, `CrystalFound`, `CavernDiscovered`, `OreFound`, `AirSupplyLow`/`AirSupplyRunningOut`, `SpaceToContinue`, `GameCompleted`, `ManTrained`, `UnitUpgraded`) into the `DAT_00503190`–`DAT_005031f4` message pointer block, remembers the four caller pointers (`DAT_0050361c/620/65c/660`), and defaults `DAT_00503668` to `100.0` when `param_5` is zero. Sole caller `GameConfigInit`. |
| 0x46fc70 | ObjectFlightCurveRecordInit | The trajectory sibling of `EntitySpawnRecordSlotInit` in the same `0x2b0`-stride spawn-record array: maps the flight kind through `FUN_0046fef0` to cell coordinates (`FUN_0042ed70`) and claims the first free slot (`FUN_0046ff70`, `-1` gives up), then writes the start position into `DAT_00503b84…3b98`, the end-minus-start vector normalized at `DAT_00503bac…3bb4`, the `4.0` speed constant (`0x40800000`) at `DAT_00503ba8`, builds the 20 %/33 % arc control offsets (`FUN_00406760`) and the 50-sample flight curve (`FUN_00406820`/`FUN_004067b0`) into the slot, creates the entity with `EntityCreateWithPosition`, and registers the instance with `FUN_0046f920`. Caller `FUN_004420d0`. |
| 0x47e510 | DirectDrawDisplayModeSetup | Fills the display-mode descriptor: flags `|1` plus `|4`/`|2` when the width/height pointer arguments are set (with the dimensions stored at `+0x24`/`+0x28`, else `+0x84 = 512`), per-pixel format `+0x70` from `param_3` (`0xf` defaults to 16, `+0x84 = 0x40 | (param_4 != 0)*0x3800 | 0x840`), RGB masks `0xff0000/0xff00/0xff` for 24-bit, `0xf800/0x7e0/0x1f` for 16-bit, `0x7c00/0x3e0/0x1f` for the 15-bit format, and full masks plus flag `0x60` for 8-bit; enumerates through the vtable `DAT_0053399c` method `+0x18`, runs `FUN_0047e760`/`FUN_0047e7b0` on success, zero-fills a 256-entry palette and feeds it through the vtable method `+0x14` and the surface's `+0x7c` method in the 8-bit path, and verifies the 1/5/5/1-channel bit sums equal `0xf` in the 16-bit path. Sole caller `FUN_0047e4c0`. |
| 0x482500 | InstanceQuadVertexDataFill | For the quad instance at `*(instance + 8) + param_2 * 0x70`, writes the three vertex positions from `param_7` in the `0x20`-stride vertex block, then the `param_6`-minus-position delta plus its normalized direction as the per-vertex data, and the `param_7` pairs as the trailing fields. Sole caller `InstanceQuadVerticesBuild`. |
| 0x4882e0 | SceneNodeTransformMatrixApply | Resolves the scene object from `*(*param_1 + 0x14)` indexed by the `ushort` handle at `param_2 + 6`; without the `param_2 + 0x28` bit `4`, composes the rotation from the three `param_4` Euler angles (`fsin`/`fcos` products), scales by `param_5`, adds the entity's `+0xc/+0x10/+0x14` offsets rotated into the `param_3` translation, and applies the 4×4 matrix (translation row plus `1.0` scale diagonal) through the object vtable method `+0x38`; with the bit set it instead reads the object's current transform through vtable methods `+0x5c`/`+0x74`, scales its diagonal by `param_5`, swaps in the `param_3` translation, and sets that matrix. Callers `FUN_00488170`/`FUN_004889a0`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-20.bundle` —
batch decompile records
`ev_fb654e9ce69c90085cda97e49f50a18cc194e4faf49dda19c52176f3299c7377`
(22 addresses: the 13 band picks plus the queue blockers `0x464110`, `0x409f20`,
`0x40cea0`, `0x41f8e0`, `0x471310`, and the context entries `0x435b20`, `0x471630`,
`0x483690`, `0x46f770`) and
`ev_daa06eaa6dd243fb4dfa9672bcea0ae2176f1885024ba7df471d2f04f7a782f5`
(`0x409d50`), plus one `procedure_callers` record per named address. All
investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The three command-region handlers `0x43a120`, `0x448c90`,
and `0x4490a0` (each writes the entity command record's opcode/coordinate slots
at `param_1[9] ± offset ± param_1[10]*0x14` with opcodes `8/9`, `0xf`, and `6`,
but the three command kinds are still not distinguished; their single static
call sites were audited in finding 247), the steering-slot pair `0x471f00`/
`0x471d20` (both consume the `FUN_004712c0` object array — now known to return
the `+0x98 + slot*4`-stride slot base, and `0x471d20` also maintains the laser
path through `FUN_0046f0d0`/`FUN_0046ffb0`/`FUN_0046f270` — but neither side's
dispatch split is established), and this batch's deferred picks `0x483690`
(renderer matrix set-if-changed behind anonymous callers), `0x471630`
(ray-vs-cell-box minimum-distance scan feeding the laser region), `0x46f770`
(mode-switched hit response with `LavaEffectSelectAndPlace` kinds 0/1/2), and
`0x435b20` (188-byte display-object group record whose only caller is
`GameConfigInit`). These carry forward the unresolved set from findings
240–247 (`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`, `0x40b000`, `0x474760`, the CRT cluster
0x493–0x49Bxxx, and the entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/
`0x46a0a0`). The `900 B`-and-up anonymous band scan continues to queue
candidates for the next batch.
