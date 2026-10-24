# Finding 250 — Config-Band Loaders, Cell-Corner Scan, Action/Command Issues, Record Instantiation, and Render-State Helpers

Nineteen more anonymous procedures now carry names grounded in their own
instruction sequences. Three are the remaining `GameConfigInit` band
candidates (`GameConfigInit` is their sole caller, which grounds the
config-loading family while each name comes from the function's own section
keys). The rest are band candidates grounded by named or ported callers:
`CellInitPass`, `MainSimInteractionAndCellUpdate`,
`TimedStatusAndEventTick`, `EntityEnergyLowTargetRouteCommandTick`,
`UiActionDispatch`, `EntitySpawnRequestQueueProcess`,
`EntityFaceTargetActionStart`, `PathStepAdvanceAndArrivalCheck`,
`EntityCreateByType`, `ManagerTick`, `CellQuadRunStage`,
`SceneEntitySlotTableRebuild`, `SceneChannelSlotsRebuild`,
`TerrainCreate`, `TerrainCellMaterialTransitionBegin`,
`TerrainCellQuadGeometryEmit`, `SceneQuadGridEmit`, `CellQuadRunEmit`,
`ModelRenderRecordStateSetup`, `PolyVariantSectionModelBuild`, and the named
helper `CreateBmpSurface`, plus named callees such as
`EntityVisualLoadACT`, `EntityVisualResourceLoad`, `DDiSceneObjectAttachOrUnlink`,
`EntityCreateWithPosition`, `KeyStateGatedCommandEnqueue`,
`SceneActionNameSet`, `SoundAliasPlay`, `SoundAliasLookup`,
`ObjectTypeNameIndexFind`, `TerrainCellSpecialStatePredicate`,
`MapCellToWorldCenter`, `SurfaceBlitRect`, `Direct3DRenderStateSetCached`,
`DDiSceneMoveOrPose`, `DDiSceneFrameRecordFind`,
`DDiSceneActionRecordField1CGet`, `MeshRecordAllocateAndFill`, and
`EngineHeapAlloc`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x401270 | AdvisorConfigSlotLookup | Loops the key table at `DAT_004b30f8` until the pointer passes `DAT_004b3123`, looking up `Advisor_<key>` config children (`ConfigEntryFind` + `ConfigPathValueGet`, one retry unless `DAT_00505f84` bit `0x400000` is set), tokenizes the value at `DAT_004a13e4` (`BlockTextTokenSplit`), resolves the visual record by name with `EntityVisualLoadACT` through `DAT_004b31ac`/`&PTR_DAT_004a13e0`, and stores the `(record, token, token)` triple into `DAT_004b3124`, with the last two tokens `ParseFloat`-ed and scaled by `25.0`; also applies the position/scale helper `FUN_00475470` and `SceneRenderLockToggle`. Sole caller `GameConfigInit`. |
| 0x40e1e0 | CellCornerOccupancyScan | For cell `(param_2, param_3)`: bails with 0 when the `DAT_004c7df8` grid marks the cell or `TerrainCellSpecialStatePredicate` reports a special state; then scans the four table-neighbor offsets and returns 1 when a neighbor is grid-occupied/special and the paired third table cell is likewise occupied (the special case additionally requires the neighbor itself not special). Sole caller `CellInitPass`. |
| 0x418da0 | HelpWindowButtonHitTest | While the `DAT_004dc650` bits 1/2 are set (then clears bits 8/1): maps the `(param_1, param_2)` cursor against the four-button layout table `DAT_004db904` (stride-7 rectangles written by `HelpWindowButtonLayoutLoad`), recording the hit index in `DAT_004dc648` and setting bit `0x10`; invokes `FUN_00418fc0` per button and `FUN_00419030(index, param_6)` / `FUN_00418f80` feedback, and on the `(345, 350)` corner zone `TimedEffectUpdateRequestSet(0x21)` + `ActionFeedbackSoundDispatch(0)` once with `FUN_00419030(3, 100.0)`. Sole caller `MainSimInteractionAndCellUpdate`. |
| 0x41eac0 | EventTimerCountdownAdvance | Advances the `DAT_004ddcd0` timer by `param_1 * _DAT_004ddcf0` under the `DAT_004de1f8` flags (`0x80` counts down to the `_DAT_004ddce8` floor and raises `0x80`; `0x100` counts up); on reaching `DAT_004ddce0` clears bit `0x80`, clamps, plays `SoundAliasPlay(0x15, 0)`, and branches on `DAT_004ddcfc`: `0x16` runs the `FUN_0045ab00(8)` + `FUN_0045b610` script and raises `_DAT_00500860` bit 2, `0x17` runs `FUN_0045ab00(0xb)` + `FUN_0040e7e0` and raises `DAT_00556ed4` bit 1, otherwise raises flag bit 4; finally latches `DAT_004ddccc/cc4/cc8` from the `DAT_004ddcf4/fc/f8` set. Sole caller `TimedStatusAndEventTick`. |
| 0x427e00 | ToolTipInfoStringsLoad | Copies the eight `ToolTipInfo/<X>Text` config strings (`ConfigEntryFind` + `ConfigStringCopy`) — `HealthText`, `EnergyText`, `CrystalsText`, `OreText`, `StudsText`, `ToolsText`, `CarryObjectText`, `DrivenByText` — into the `DAT_00556e9c`–`DAT_00556eb8` globals. Sole caller `GameConfigInit`. |
| 0x433180 | CellTypeSpeedMultiplierResolve | Filters entity kinds to the twelve drive kinds; unless the entity's `+0xdb` field is set (or `FUN_00469e60` flag bit `0x20000` is set) bails with 0, then for cell `(param_2, param_3)`: cell `+2` char `'\x06'` with `+8` bits `0x200`/`0x200000` clear passes the `param_4` multiplier straight through, and the `+8` bit `0x1000` (rock-fall) branch resolves `RockFallIn` through `ObjectTypeNameIndexFind` + `FUN_0046f1d0` and scales it by `param_4` into `*param_5`. Callers `EntityEnergyLowTargetRouteCommandTick` + `FUN_00443840`. |
| 0x43a650 | ActionKindEntityCreateAndCommandIssue | After the `FUN_0043a7c0` guard, maps action kinds `6/7→10`, `8→9`, `9→8` to index the `DAT_004dee18` position-source table, takes the position record through `FUN_00408810` + `DDiSceneMoveOrPose`, selects the per-kind image global (`DAT_00556374/7c+slot/78/84/88/8c`), creates the entity with `EntityCreateWithPosition`, enqueues `KeyStateGatedCommandEnqueue(0x1c or 0x16, entity, 0, 0)`, and for kinds 9/8 sets the action name `Short`/`Normal` through `SceneActionNameSet`. Callers `UiActionDispatch` plus three anonymous ones. |
| 0x442300 | EntityVelocityVectorDispatch | With no `param_3` vector derives a default direction by rotating `(0,1,0)` with `FUN_00479510`; stores the vector into the entity `+0xab…+0xae` velocity triple (and the `-0.01`-scaled `+0xa8…+0xaa`), dispatches per component kind — `FUN_0046d350` (kind 1), `FUN_00406dc0` (kinds 2/3), `FUN_004086c0` (kind 4), or the `FUN_00475470` pose writer under `+0xfa` bit `0x20000` — and stamps `+0xae` with the `0x3c23d70a` constant. Callers `EntitySpawnRequestQueueProcess` and `EntityFaceTargetActionStart`. |
| 0x4477d0 | PathStepLookaheadEntryWrite | Takes the current step's cell coordinates from the `+0x24/+0x28` stride-`0x14` record array, converts them through the height lookup (`FUN_00431570`) + `MapCellToWorldCenter`, interpolates the context target (`FUN_00442260`), projects a 10%-lookahead point along the difference, re-derives the cell (height lookup + `FUN_0044f870`), writes `(x, y)` into the current step slot's `−0xc/−0x8` fields, ORs the byte at `−0x4` with `8`, updates the context `+0x2f4/+0x2f8` target, and clears the pending `+0x2fc` handle. Sole caller `PathStepAdvanceAndArrivalCheck`. |
| 0x44b680 | EntitySceneObjectTreeAttach | Builds the entity's `+0x32c` attachment record: reads the object component (`EntityTypeComponentGet`) with the `FUN_00472950`/`FUN_00472940` scene-object pair, attaches via `DDiSceneObjectAttachOrUnlink`, scales the entity (`FUN_0046a210`/`a1f0`/`a1b0`), allocates the `FUN_0040f7d0` record list slot, then loops the `FUN_0046a210` count duplicating visual handles (`EntityVisualResourceLoad`), re-attaching them (`DDiSceneObjectAttachOrUnlink`), computing per-node scale/position (`FUN_0046a230`/`a1d0`, `FUN_0040f6c0`) and appending (`FUN_0040fe40`). Sole caller `EntityCreateByType`. |
| 0x4519b0 | VisualModelRecordCreate | Formats the `param_3 param_4` key, loads the visual through `EntityVisualLoadACT` (with the `&PTR_DAT_004a1d5c` path and a `DAT_004a4014` fallback), allocates the 24-byte model record `{id, 0, copy of param_2 name, param_5 lod, kind 2}` with `EngineHeapAlloc`, links it into `param_1 + 0x14` when the model base is given, and returns it. Sole caller `PolyVariantSectionModelBuild`. |
| 0x45a9c0 | PanelTextureSlotsBlit | Walks the panel `param_1` draw-record array `DAT_00500134[param_1*12]` (`0x2c` stride, count `DAT_00500138`) with the `DAT_00500128/12c` global offsets, and per record picks the texture field `+4`/`+5`/`+6` by the state-word bits (`2/4/8/0x10/0x40`, gated on `FUN_0041b9b0`), blitting nonzero ones through `SurfaceBlitRect` (`FUN_0047dcb0`). Sole caller `ManagerTick`. |
| 0x45cdc0 | PriorityImagesConfigLoad | For each entry of the `PriorityImages` section (`ConfigNestedPathChildFind` + index via `FUN_00401cb0`): formats `&DAT_004a1d60` over the entry string, token-splits twice (`DAT_004a1cd4`/`DAT_004a2378`), keeps the label string (`StringDuplicate`) at `DAT_00500f00 + index*4`, resolves a two-token entry through `SoundAliasLookup` into `DAT_005010b0 + index*4`, and loads three texture handles (`FUN_0047d9d0`) into `DAT_00500f6c/fd8/1044 + index*4`. Sole caller `GameConfigInit`. |
| 0x46cff0 | SceneRecordTemplateInstantiate | Copies the `0x7b`-dword template record into the destination (clearing bit 0 of `+0x7a`), duplicates the visual handle through `EntityVisualResourceLoad`, duplicates/attaches the second handle (position/scale/visibility setup + `SceneRenderLockToggle`), duplicates the `+0x78/0x79` sub-resources (`FUN_00451b30`), runs `FUN_0046d260`, then duplicates the `param_1[4]` handle list into the `+0x10`-entry attachment slots (each attached + registered), returning success when the primary handle resolved. Sole caller `EntityCreateByType`. |
| 0x46dd10 | StagedCellCandidateSelect | Walks the `DAT_00549840` staged-entry block list (`DAT_0054beb0` entries, `+0xf4`-stride six-word candidate rows): clears each row count, and for every entry item tries the four `__ftol`-mapped cell coordinates until one is in the `param_2` grid bounds and its cell `+2` char is neither `'\a'` nor `'\1'`, recording `(entry, candidate)` and bumping the count. Sole caller `CellQuadRunStage`. |
| 0x473c60 | XfTextureNameFormat | Picks the frame record by `+0x10` kind (`DDiSceneFrameRecordFind` for 4, record `+4` for 3), reads its name (`DDiSceneActionRecordField1CGet`, or `FUN_00476140` after the `FUN_004897d0` test + `FUN_0049dc10` mapping for the null name), keeps the last backslash path component, and formats the `xf_…_%s` / `xf_%s_…_%i` key (with `param_2` and optional `*param_3`) into the `DAT_00505400` buffer. Callers include `SceneEntitySlotTableRebuild` and `SceneChannelSlotsRebuild`. |
| 0x4748f0 | MeshRecordAllocateOrAppend | When the context's `+0xc` mesh pool object is live, delegates to `MeshRecordAllocateAndFill` directly; otherwise appends the `ESI` handle into the vtable record's grow buffer (`+0x1c` record, index/full checks, `−1` when the buffer is full or the context lookup fails) and dispatches the upload through the context vtable (`+0x48`, `+0x18`). Callers `TerrainCreate`, `TerrainCellMaterialTransitionBegin`, `TerrainCellQuadGeometryEmit`, `SceneQuadGridEmit`, `CellQuadRunEmit`. |
| 0x47d2d0 | SurfacePixelValueEncode | Encodes a color for the surface format: without the `+0x54`-flag `0x40` (palette surface) searches the `+0x50` palette set for the matching byte pair and returns the palette index (`0` after 255 tries); with the flag computes the packed pixel value from the color masks via `FUN_0047d3f0`. Callers `CreateBmpSurface` and `FUN_0047ce20`. |
| 0x482b50 | RenderStateFlagsApply | Maps the `param_1 & 0x1c00` selection (with `param_3`) onto the cached render-state setters: `Direct3DRenderStateSetCached(0x1c/0x1b, …)`, the mode/blend calls `FUN_00482cb0(2,2)/(5,2)/(5,6)` or a `0x1b` reset, `1/3` on `0x16`, `FUN_00483810(4, 2/3/4)` for the `0x20000/0x40000` cases unless the `DAT_00505f84` byte bit `0x20` is set, `0x1c` toggle on `7`, `0xe` from the selection, `FUN_00483540`+`FUN_00483690(1, …)` for the `0x4000` matrix case, and `0x11/0x12` `2/4` versus `1/1` on `0x800000`. Callers `ModelRenderRecordStateSetup` + `FUN_00483b00`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-22.bundle` —
batch decompile record
`ev_e0c58e5bb04ee83c0327854fd9e50249b5fc8c65c5e199787622b70bf41060d8`
(24 addresses: the 19 named picks, the examined-and-deferred
`0x465c50`/`0x473560`/`0x489660`, and the steering-path helpers
`0x4712c0`/`0x4712f0`), plus one `procedure_callers` record per named address
(`0x401270` `ev_9f30318e…`, `0x40e1e0` `ev_7a393403…`, `0x418da0`
`ev_aed19038…`, `0x41eac0` `ev_c5bbebfc…`, `0x427e00` `ev_25071a21…`,
`0x433180` `ev_3e6d86e1…`, `0x43a650` `ev_bb61e3f6…`, `0x442300`
`ev_1f04f27a…`, `0x4477d0` `ev_904fbeaa…`, `0x44b680` `ev_a7c111b0…`,
`0x4519b0` `ev_8a7efeff…`, `0x45a9c0` `ev_b920060f…`, `0x45cdc0`
`ev_70514667…`, `0x46cff0` `ev_3933d369…`, `0x46dd10` `ev_63ec2c3d…`,
`0x473c60` `ev_67a93fe4…`, `0x4748f0` `ev_d95b32fc…`, `0x47d2d0`
`ev_b5711e9a…`, `0x482b50` `ev_172a4a52…`). All investigations used shipped
`reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The command-region trio `0x43a120`, `0x448c90`, and
`0x4490a0` (static call sites still resolve `not_in_procedure`; opcode kinds
`8/9` vs `0xf` vs `6` undistinguished), and the steering-slot pair
`0x471f00`/`0x471d20`, examined a fourth time at instruction level this
batch: the slot-base accessor `0x4712c0(entity)` reads only its first
argument — it returns `entity[3] + 0x138` for kind `1`, `entity[6] + 0xa0`
for kind `4`, and `0` otherwise — so the extra displacement arguments the
velocity-scaled (`*10000.0`) `FUN_004754c0` pose delta is pushed with die at
that call, and `0x471a60` receives the returned base with stale stack
arguments past it, so neither side's dispatch split is mechanically
established. Newly examined and deferred: `0x465c50` (fills a `0x3c`-stride
record slot with a `0.17`-scaled random unit-velocity arc and a `rand%30+40`
delay, called only from `TransientEffectEmitterCreate`/`TransientEffectInstanceUpdate`;
the effect's subject cannot be stated mechanically), `0x473560` (scene-object
display-mode reconfiguration with `FUN_00480870` mode handles and raw
vtable dispatch). These carry forward the
unresolved set from findings 240–249 (`0x4802d0`/`0x480390`/`0x480570`,
`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`,
`0x40b000`, `0x474760`, `0x483690`, `0x471630`, `0x46f770`, `0x435b20`,
the CRT cluster 0x493–0x49Bxxx, and the entity scalar accessors
`FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`). The `900 B`-and-up anonymous band
scan queues 123 further non-CRT candidates for the next batch.
