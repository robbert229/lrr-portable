# Blocked port candidates

Functions the port loop cannot port safely right now, with the concrete
blocker and the next action. Recorded per `hack/port-loop.md`; nothing here
is skipped for convenience — each entry names the unported dependency that
must land first.

## Sound-subsystem wave

> Update (2026-10-24): `SoundBankEntryLoad @ 0x47a870` and the slot helpers
> (`0x47aad0`, `0x47ab00`, `0x47a850`) are **ported** — the bank-record root
> landed as one cluster, and the marker-parse/variant-select pair
> (`0x464d00` / `0x464e20`, issues #318 / #324) landed with it as the
> configured-variants chain model — together with the property pair and
> its bank accessors (`0x465010` / `0x465030` / `0x47b050` / `0x47b0d0`,
> issues #319 / #320). The submission route (`0x464fa0` / `0x465090` /
> `0x4652c0` / `0x4652d0` / `0x4651a0`, issues #325 / #328 / #329 / #330 /
> #332) landed as a cluster — its play/pump calls are documented
> instance-layer seams (TODO(corpus)), and every state change around them
> is modelled. The rows below stay blocked on what remains of the chain.

| Function (`address`) | Issue(s) | Blocker | Next action |
| --- | --- | --- | --- |
| `SoundHandleStopForward` (`0x465190`) | #331 | Forwarder into the instance layer: `SoundBufferStopHandle @ 0x47b130` and the instance-chain bookkeeping are unported (the stop seam is modelled as a documented no-op in `sound_subsystem_enable_set`). | Port the sound-instance layer (tracks `FUN_0047b1a0` chain). |
| `SceneEffectRegistryReset` (`0x465380`) + transient-effect family (`0x4653a0`–`0x465430`, #333–#339) | #333–#339 | No module owns the transient-effect registry yet (`DAT_00552100` region); the reset itself is trivial but lands meaningfully with its consumers. | Scene-effects milestone: pick an owner module first. |

## Media-playback wave

> Update (2026-10-24): the state machines (`0x4297b0` / `0x4295f0`,
> issues #301 / #302), the playback toggle (`0x429810`, issue #298), the
> cue-dispatch pair with the cursor-override state (`0x428800` /
> `0x4351a0` / `0x45cc20` / `0x45cc40`, issue #300), the rect-aware
> video pump (`0x42efc0`, issue #303), and the stream-name triggers
> (`0x416150` / `0x4161a0` with `0x465160` / `0x47b8e0`, issues
> #299 / #340) are **ported** — every ledger row in this wave is
> ported. What remains is not a ledger row: #297's streamed-playback
> platform seam (the corpus's `StreamedWavePlaybackStart @ 0x47b970`
> selector streams PCM progressively through DirectSound; the port's
> bank loader feeds whole-file payloads to the audio seam) and the
> instance-layer handles behind the recorded cooldown handle word
> (#331, above).

## Terrain wave (2026-10-24)

The flag-primitive leaf clusters are **ported** (`src/game/level.*`: the
record-flag reads/writes, secondary-word bits, gated bit tests, the
coordinate-pair check). The rows below stay blocked on unported seams
they hand work to:

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `TerrainCellFlag100SetAndTrack` (`0x432100`) | Raises secondary bit 0x100, appends the coordinate to the per-cell track queue (`DAT_0055684c` pair written at the `DAT_00556e8c` cursor, unguarded), then calls the per-cell `CellInitPass @ 0x42f6f0`. No module owns that queue, and the port models CellInitPass only as a scoped bulk load pass. | Give the cell track/refresh queues an owner. |
| `TerrainCellVisitedFlagSetAndTrack` (`0x432270`) | Raises the visited bit, then appends to the visited-coordinate queue (`DAT_00556528` pair at the `DAT_00556848` cursor). The queue and its flush (`TerrainCellUpdateQueueFlush @ 0x4321a0`) have no owner. | After the cell update-queue seam. |
| `MapOpen2` (`0x480170`) | The slot-table opener: resolves the file through the resource-open pair (`FUN_0047f6e0`/`FUN_0047fac0`), reads the payload with `WadFileRead @ 0x47f980`, and installs it into an allocated slot. The resource-open/wad read seam (the port opens `.map` files through `level_map_open`) and the `DAT_005343ac` reader hook are unported. | After the wad resource-read seam. |
| `TerrainCellFlagsSetAndRefresh` (`0x4323f0`), `TerrainCellPrimaryFlagsMaskClearAndRefresh` (`0x432360`), `TerrainCellFlagsSetMask100400` (`0x432450`), `TerrainCellFlagsSetMask20108000` (`0x432490`), `TerrainCellFlag40000000ClearOrSet` (`0x432560`) | Each raises its flag set and refreshes through the per-cell `CellInitPass @ 0x42f6f0` entry (the flag-only twins of this family — `SecondaryFlagBit0*`, `SecondaryFlag8*`, `Flag80000ClearOrSet`, `Flag10000000SetOrClear`, `WorkFlagSet` — are already ported as leaves). A faithful per-cell refresh needs the corpus routine's full body; the load-time scoped port derives the same flag side only in bulk. | Faithful per-cell CellInitPass port. |
| `TerrainCellSurfaceAppearanceApply` (`0x44f5c0`) and the surface byte's writer side | The apply hands the cell to the renderer through the appearance setter (`FUN_00475020`, a scene/renderer COM seam) and only then clears the block-table `+0x24` byte. The read side (`TerrainCellSurfaceTypeGet @ 0x44f540`) is ported and answers the zero state until this lands. | After the renderer appearance seam (with the other COM-seam blockers). |

## Entity stats records (2026-10-24)

The entity record readers (the `+0x334` field getters) are **ported** as
offset-preserving leaves (`src/game/entity.*`, finding 457) with the
record modeled on `EntitySlot.stats_record`; what remains here is the
Stats configuration layer they hand off to:

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `EntityStatsConfigLoad` (`0x4667e0`) | Reads the `Stats` config section and materializes the indexed record table (`DAT_00502bd8`), the per-type counts (`DAT_00502c28`), and the grouped code tables — the port models records as a zero template because no config loader fills them (TODO(corpus)). | Port the config loader against `BlockFile` with a per-level name resolve. |
| `StatsLevelRecordCountForStatType` (`0x42ef40`) | Its per-type counts are the section-count globals (`DAT_0055627c/80/84/88/8c`) written by the type-table loaders: Vehicle and RockMonster are ported, but `MiniFigureTypeTableLoad` (`0x42cf50`), `BuildingTypesLoad` (`0x42d2b0`) and `UpgradeTypesLoad` (`0x42d460`) are not, and their stride-record type tables (`DAT_005561d4` family) are unmodeled in the port. | After those loaders model their count globals (and their stride tables if the stats fill needs them). |
| `StringTableIndexLookup` (`0x408c80`) inside the loader | The loader consults it for the `BuildingBase` default; the wide-name table (`DAT_004b8a3c` / count `DAT_004b8a50`) is initialized by the blocked reference-binary name tables (string-table initializations, above). | After the name tables land. |
| Indexed-record readers (`0x469ac0/af0/b20/e00/ec0/ee0/f00/f60`, `0x46a0c0/0f0`) and `BuildingUpgradeAdmissionCheck` (`0x469f40`) | They address the config tables above by type/level (and `type*0x3c + level*4`); with no loader ported there is nothing to index. | After the config loader. |
| `IndexedValueAppend` (`0x469a90`), `GroupedCodeTableFindValueIndex` (`0x46a120`) | Append/lookup over the same unowned grouped tables. | Fold into the config-loader port. |
| `EntityRecordFlag400ToggleAndRefresh` (`0x46a330`) | Toggles record `+148` bit 0x400, then calls `KindFourPostCreateResolve @ 0x43c570`, which is unported. | After that resolver. |

## Engine heap internals (2026-10-24)

The heap **seam** (`src/gods98/heap.*`: alloc/retry/aligned/free entries)
is **ported** onto the host allocator; the corpus's host-heap manager
machinery it routes around remains unported, and nothing in the port
depends on it:

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `EnginePoolAllocate` (`0x491900`) | Small-size pool inside the corpus heap's arena (`DAT_004b1fdc` gate, slot lists) — host-heap manager state with no port consumer; the aligned entry skips it. | Only if an allocator-exactness corpus is added. |
| `CRTHeapLocateAllocationPage` (`0x491840`), `SmallHeapPageFreeSlotUpdate` (`0x4918a0`) | Page-directory walk + free-slot bookkeeping over the corpus heap's arena pages (`PTR_LOOP_004affb8`); the port frees straight through the host allocator. | Fold in only with a dedicated allocator port. |
| `CrtNewHandlerInvokeForAllocationRetry` (`0x491d90`) | The registry global (`DAT_00549158`); the port passes the retry handler per call instead, and nothing registers one (the corpus's unregistered case). | Blocked-by-design (documented deviation). |

## CRT x87 stub thunks (2026-10-24)

`CrtNoOpStub` (`0x491ff5`) and `CrtNoOpStubVariant` (`0x497870`) decompile
to empty bodies, but their call sites consume FP state — `CrtAcosCore`
(`0x48e0ad`) reads EAX/ST0 back after `0x491ff5`, and
`CrtFpuControlWordUpdate` (`0x497770`) passes the control word to
`0x497870` (an `fldcw`-shaped wrapper). The "no-op" reading is a Ghidra
body artifact; porting them as empty functions would port unproven
semantics. **Blocked** pending an x87 control/status stub corpus (they
are host-CRT internals; nothing in the port drives them).


## Surface pixel-format COM vtable pair (2026-10-24)

`DirectDrawSurfacePixelFormatGet` (`0x47def0`) and `PixelChannelMaskGet`
(`0x47df50`) are pure-parameter pseudocode, but both drive the surface's
**COM vtable**: they zero a stack `DDSURFACEDESC2`, call the surface's
`GetPixelFormat` slot (`*(*param_1) + 0x54`), and read the descriptor's
channel masks/RGBBitCount back. The port's `DisplaySurface` is an
SDL/host framebuffer behind the platform seam with no COM vtable and no
`DDSURFACEDESC2` layout, so there is no object the pseudocode's
`*param_1` load could address. **Blocked** until the DirectDraw seam (or
a descriptor-shaped mock) lands; the miniwin build's fixed RGBA32 layout
means the masks this pair derives are already compile-time constants
here.


## CRT OS-bound and register-word residuals (2026-10-24)

The pure-leaf scan's last ten named rows are pseudocode-clean only
because Ghidra hoists OS and x87 state out of the body. Each is
blocked on state the port does not model; porting them as constants or
host lookalikes would claim unproven semantics (the same bar the x87
stub-thunk note above applies).

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `MmioHandleAndGlobalBufferRelease` (`0x4891d0`) | Frees a Win32 `HGLOBAL` and closes an `HMMIO` handle; the port's media loader feeds whole payloads through the audio seam and models no MMIO handles or HGLOBAL blocks. | Media-handle lifecycle model (the MMIO platform seam). |
| `CrtFindHandleClose` (`0x48f950`) | `FindClose` on a corpus directory-search handle, with the failure path writing `DAT_005490d0 = 22` and returning `-1`; the port enumerates directories natively and has no corpus search handle to close. | File-find platform seam, if/when its corpus callers land. |
| `PointerRangeIsReadable` (`0x499690`), `PointerRangeIsWritable` (`0x4996b0`), `FunctionPointerIsExecutable` (`0x4996d0`) | `IsBadReadPtr`/`IsBadWritePtr`/`IsBadCodePtr` wrappers — the answer is host address-space validity, and deprecated Win32 probes have no faithful portable equivalent. | If ported callers of these land, gate on port-owned ranges instead. |
| `CrtFpuPrecisionModeCodeGet` (`0x49200c`), `FpuStatusWordGetVariantA` (`0x493be0`), `FpuStatusWordGetVariantB` (`0x493bf0`), `FpuControlWordUpdateMasked` (`0x493c10`) | x87 register readers: the precision bit comes from the corpus's CRT-bootstrapped control word and the status word reflects accumulated FP state. The port has no x87 registers to read, and constant answers would claim provenance. | Same blocker as the x87 stub thunks above — needs the control/status register model first. |
| `CrtUnwindAddressResolve` (`0x495a60`) | Pure pointer arithmetic, but over the PE exception/unwind tables (`base + table-entry words`); the port models no PE exception data, so there is nothing to validate the resolved addresses against. | CRT loader internals, if an SEH-shaped need appears. |


## Cell-record counter block and spatial-query family (2026-10-24)

The terrain cell-command issuers end their links with
`TerrainCellRecordCountUpdate` (`0x403810`), which the port records as a
documented **seam**: its head runs `SpatialObjectQueryCenterMask01`
(`0x438a60`) / `SpatialObjectQueryCenterMask02` (`0x438ae0`) and
`ActionKindEntityCreateAndCommandIssue` (`0x43a650`) — the spatial object
query and the action-kind creation chain, all unported — and its tail
increments the `DAT_004b34f4` cell-record counter block indexed by
`kind * 0xf0 + mode * 0x10 + arg`, which the port does not model (the
same counter block that the `CmdFlushAll` zero path clears). The
issuers' own observable halves — the record shape, the guards, the
terrain marks and the links — land in `cmd.*`, so only the counter
update and the query-driven side effects are missing there.

`BuildingDepositCountGet` (`0x469bb0`) belongs to the blocked stats-config
layer (the `DAT_00502bd8` indexed record family), which also gates the
query helpers above. **Next action:** the spatial object-query model,
then the counter block, which together let `0x403810` land as a leaf.


## Wrapper/thunk seams and model-shaped gaps (2026-10-24)

Several small named rows are wrappers whose callee or callback body the
port does not model. They stay out until their dependency lands:

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `SceneObjectListApplyFloatCallback` (`0x40ff50`), `SceneObjectListApplyPositionCallback` (`0x410070`), `SceneObjectListApplySecondPositionCallback` (`0x4100b0`) | Tail calls into the ported `PointerStackForEach` (`0x40fe90`) with bare code-thunk callbacks (`LAB_*` labels with no pseudocode) — the callback bodies cannot be recovered from the check-in. | Recover or re-derive the thunk bodies from the scene-object layer, then wrap. |
| `EntityAnimationStateSet` (`0x439cd0`) | Tail call into `EntityBuildingIndexSet` (`0x469c10`) — gated by the blocked stats-config layer (`DAT_00502c28` admission word). | Stats-config layer (shared blocker). |
| `MainMenuTextItem` (`0x4103d0`) | Calls `ScrollingTextDisplay` (`0x40a0c0`), which the port models only as the credits scroller with its own config/text sources; the menu's text/position globals (`DAT_00556f50/6f54/613c`) have no model. | Generalize the scroller into a shared draw API, or land the menu text layer. |
| `WadArchiveRecordAtIndex` (`0x48bce0`) | Indexes the 16-slot, 36-byte archive-context table at `DAT_00575500` (`&base + i * 0x24`, finding 01); the port models no per-archive context block. | Model the WAD mount's context table, then the stride accessor lands as a leaf. |
| `SceneObjectCoordinateTransformInPlace` (`0x475070`) | COM-style vtable dispatch (`(**(code **)(*param_1 + 0x108))`) on the renderer object — the scene/COM seam stays unported. | Renderer COM seam (shared blocker). |
| `DisplayDeviceCapsQueryDiscard` (`0x477f80`) | Win32 `GetDC`/`GetDeviceCaps` on the corpus window word with the result discarded — the port has no device-context model at the display seam and the corpus read was dead. | Land with a display-seam model, if one appears. |
| `WindowRectAdjustForWindowedMode` (`0x478310`) | `AdjustWindowRect` behind the corpus window-mode flag word, mutating the caller's RECT — pure OS window metrics the port does not model. | Window/OS seam (shared blocker with `WindowTitleSet`). |
| `GlobalStateZeroTransitionDetect` (`0x435550`) | Compares its stored word against the renderer progress word (`DAT_0076ab3c`, blocked via `FrontendProgressValueChanged`); the corpus side that raises the word is unported. | Renderer progress-word seam (shared blocker). |
| `CallbackOperationRunAndReturnStatus` (`0x410280`) | Runs the ported `PointerStackForEach` with a bare code-thunk callback (`LAB_00410230`, no pseudocode) whose return value drives the status word. | Recover the thunk body with the scene-object callback family. |
| `CrtNlgFrameStateRecord` (`0x48efe6`) | Stores the unaligned `EAX`/`EBP` register pair and `EBP+8` into three globals — frame-pointer capture from an asm stub; the port has no corpus frame layout to validate against. | Same blocker as the x87/CRT register stubs. |
| `SoundPositionSetFromVector` (`0x47ad10`) | Tail call through the sound-handle vtable slot `+0x4c` with a vector triple — the sound object's COM seam is unported (shared with the scene/renderer vtable family). | Sound-object vtable model, if one lands. |
| `EntityTransformPositionGet` (`0x442260`) | Reads the entity's transform object through the entity-type component resolver (`0x44a200`, quote-only: "no object layer yet") and the pose getter — the port's resolver answers 0, so the wrapper has no observable behavior to pin. | Object-layer model behind the component resolver. |
| `SceneObjectSetAnimationWithFallback` (`0x46c130`) | Both animation-name calls ride `SceneActionNameSet` (`0x473370`, 1980 bytes) — quote-only scene-assembly seam with no callable port body. | Land the scene-assembly name setter. |
| `GlobalResourceHandleLoadAndEnable` (`0x40c720`) | Feeds the ACT loader (`0x472cd0`, ACT branch ported) an unmodeled data-pointer pair (`PTR_DAT_004a13e0`) and parks the handle word (`DAT_004c7de0`) unowned; the visible half is already the load-act mirror's call. | Model the load callback pair (the port mirrors kind-4 only today). |
| `DockDefinitionMatchesCell` (`0x438f60`) | Reads the docks table (`s_Docks_004a5060`, unmodeled reference data) through the blocked name-table resolver (`0x42e8b0`). | Dock-table reference data plus the name-table blocker. |
| `SceneResourceDrawAtOrigin` (`0x46d350`) | Tail call through `DDiSceneObjectPoseMethodDispatch` (`0x475470`, quote-only vtable seam) with the renderer-object argument — the scene/COM seam blocker family. | Renderer COM seam (shared blocker). |
| `GlobalVectorSetAndNotifyListeners` (`0x40ffe0`) | Stores the four vector words and tail-calls the ported `PointerStackForEach` (`0x40fe90`) with the `LAB_0040ff90` code thunk — the listener callback body has no pseudocode (the scene-object thunk blocker family). | Recover the listener thunk body, then wrap. |
| `PopEntityStateOverride` (`0x459240`) | The stack-pop arm tail-calls `EntityBuildingIndexSet` (`0x469c10`) — same stats-config-layer blocker as `EntityAnimationStateSet` (`0x439cd0`) above; the completed-objective stack itself rides the ported `GameContext` OSF table. | Stats-config layer (shared blocker); then wrap the pop arm over the OSF pop + building-index + rename. |
| `QueueActionRequestType2` (`0x41f230`), `QueueActionRequestValue` (`0x41f270`), `QueueActionRequestFlag100000` (`0x41f2b0`), `QueueActionRequestIndex` (`0x41f340`), `QueueCallbackRequest390` (`0x41f3c0`), `QueueCallbackRequest3E0` (`0x41f400`) | Build the on-stack request frame and hand it to `QueueEnqueueRequest` (`0x4377b0`) with a bare `LAB_*` code-thunk body (`LAB_0041f190/1f390/1f3e0`, no pseudocode) — the queue dispatcher seam blocker family. | Recover the thunk bodies with the queue-dispatcher model, then wrap. |
| `CrtFloatingPointFeatureProbe` (`0x490280`) | Win32 `GetModuleHandleA("KERNEL32")`/`GetProcAddress("IsProcessorFeaturePresent")` and the live import call — pure OS feature-probe seam with no portable behavior the port models. | OS/runtime seam (shared with the CRT register-stub family); model as a feature flag if a caller ever branches on it. |

## Scanner triage (2026-10-24)

Rows surfaced by the named-wrapper scan and triaged this iteration; each
stays blocked until the named dependency lands. (Five earlier triage picks
— the uint96 pair, the token pool, and the two pressed-getters — were
re-triaged as portable and landed in iteration 116, so they are not
recorded here.)

| Function (`address`) | Blocker | Next action |
| --- | --- | --- |
| `ScrollRectPayloadRecordCreate` (`0x463810`) | Reads and writes through the scroll-rect payload's surface-handle object — every field access rides the surface vtable/COM seam (`DDiSurface*`-style dispatch) that the port does not model. | Surface-object handle model (shared renderer-COM family). |
| `WaypointVectorRandomScaleApply` (`0x44d710`) | Indexes the waypoint record band under `DAT_004eae**` and scales it with an x87 `float10` multiply whose load/store rounding the port cannot pin without the renderer waypoint records and an x87 model. | Waypoint record band plus x87 float10 model. |
| `MenuShow` (`0x413e20`) | Walks the menu item pointer array (`DAT_005572xx` band) and its per-item draw blocks — the menu-item machinery behind the frontend's menu model is unported, so there is no observable effect to pin. | Menu-item pointer-array model with the frontend menu layer. |
| `InfoPanelStringEntriesInitialize` (`0x4636b0`) | Walks a varargs triplet list into the `DAT_005521xx` info-panel string band; both the varargs walker contract and the string band have no port owner. | Info-panel string band, then the varargs init leaf. |
| `LightEffectsMoveParametersStore` (`0x44cef0`) | Copies the light-effects move parameter set through `DAT_004eadd8`/`DAT_004eaddc` — the light-effects parameter band belongs to the renderer effects layer, which the port does not model. | Light-effects parameter band (renderer effects seam). |
| `TerrainCellPaletteResourceAtGridPosition` (`0x429160`) | Maps grid coordinates through the `DAT_005560d4 + 0x7c`/`+ 0xb8` band into palette-resource records — the grid→palette resource association lives in the unported level-resource bookkeeping. | Grid-to-palette resource band in the level/resource seam. |
| `AnimationObjectTransformSet` (`0x406dc0`) | Delegates the pose write to the camera facing/pose layer (`cam_set_facing` side of the camera module) — the animation object's transform owner is that unported pose seam. | Camera/animation pose seam. |
| `SurfaceVectorNormalize` (`0x4422a0`) | Tail call into `0x44a200`, the entity-type component resolver, which the port answers zero (object layer TODO) — the normalize's object lookup has no observable behavior. | Object-layer model behind the component resolver (shared with `EntityTransformPositionGet`). |
| `AdvisorAnimationEndCleanup` (`0x4011c0`) | Strides the advisor records at `DAT_004b2e04` (`stride 0x24`) and touches their animation-complete words — no port owner for the advisor record band. | Advisor record band with the advisor/mission layer. |
| `UiOverlayAnimationApply` (`0x45c5f0`) | Drives the `DAT_005008xx` overlay animation band (timers, scales) — the overlay runtime band is unported reference state. | Overlay band in the frontend/UI layer, then the apply leaf. |
| `AdvisorTextBoxRecordCommit` (`0x46a9e0`) | Commits into the `DAT_005036xx` advisor-text band with `__ftol` float→int rounding of the layout fields — band unported and the x87 rounding mode affects the committed integers. | Advisor text band plus x87 `__ftol` rounding model. |
| `PriorityLabelLayoutApply` (`0x45cf00`) | Scans the two parallel priority-word bands to find the layout slot — both band owners are unported reference state with no accessor pair to write through. | Priority-band model (HUD/layout layer). |
| `CellResourceEntryMatches` (`0x40af40`) | Matches through the record identity pair with the `+0x330` gate word and derives the task kind via `0x439200` — the record identity band and the task-kind selector's owner have no port model. | Record identity pair plus the `0x439200` task-kind model. |
| `ToolNameStringResourcesLoad` (`0x439950`) | Walks the `s_ToolNames_004a5068` reference-binary name table through the config lookup chain into the `DAT_00556228..254` band — reference name-table blocker family. | Reference-binary name tables (shared blocker). |
| `TypeFourEntityCallbackDispatch` (`0x447900`) | Type-4 tail call into `QueueEnqueueRequest` (`0x4377b0`, ported) with the bare `LAB_00447920` code thunk — queue-dispatcher thunk family. | Recover the thunk bodies (shared with the Queue\* LAB band). |
| `EntityAtGridPositionLookup` (`0x438670`) | Same family: enqueue at `LAB_004386b0` with the thunk's store-through the on-stack triple as the observable return — queue-dispatcher thunk family. | Same queue-thunk family. |
| `IsAverageValueAbove99` (`0x418120`) | Sums a float table (count at +0x18, entries from +0x1c) read through the `SavBuildCtx`-modeled accessor — Ghidra's register shuffling leaves the buffer's true owner ambiguous; no port owner for the band. | Pin the buffer's owner through the call-site band, then port. |
| `ConfigRgbVectorRead` (`0x479170`) | Reads an `r::g::b` config string through `ConfigStringCopy` (seam-cited; the config-record lookup `0x4792e0` is unported) and splits on the `DAT_004a1cd4` separator — config-lookup chain without a port owner. | Config-record lookup (shared config-seam blocker). |
| `ReadAngleSettingRadians` (`0x479110`) | Same config-lookup chain via `ConfigPathValueGet` (seam-cited), defaulting to the corpus's `DAT_004b8a14` empty-string byte; degrees→turns→radians scaling rides the x87 float10 multiply. | Config-record lookup (shared config-seam blocker). |

## Frontier closure triage (2026-10-25)

The portable-now frontier (all callees ported) that has no port-side model.
Grouped by blocker family; every row names its address and next action:

- **D3DRM library calls** — `D3DRMVectorRandom` (`0x479500`),
  `D3DRMVectorRotate` (`0x479510`): tail calls into the Direct3D-RM runtime
  vector helpers. Renderer seam (shared). Next: the renderer seam, or
  D3DRM-shaped vector helpers if the call sites need them.
- **CDAudio platform seam** — `CDAudioErrorTextGet` (`0x489260`),
  `CDAudioTrackPlayFrom` (`0x489210`), `CDAudioDeviceOpen` (`0x489330`),
  plus the `WaveDataRead` (`0x489100`) HMMIO copy loop
  (`mmioGetInfo`/`mmioAdvance`/`mmioSetInfo`): OS-bound multimedia I/O with
  corpus error-state DATs. Next: the media platform seam (shares #297).
- **Win32 window/message/timer band** — `WINMM.DLL::timeGetTime`
  (`0x478000`), `WindowMessageDispatchFilter` (`0x478010`),
  `Win32MessagePumpDispatch` (`0x478040`): the OS window pump; the port's
  host window layer replaces the corpus's `MSG` dispatch. Next: platform
  seam, if/when the corpus pump's callers land.
- **CRT/loader/platform internals** — `CrtLocalUnwindFromFrame`
  (`0x496895`, PE unwind tables, shares `CrtUnwindAddressResolve`'s blocker),
  `CrtCodePageLocaleIdLookup` (`0x496530`, Win32 locale API),
  `CrtCtypeTablesReset` (`0x496590`, corpus ctype tables),
  `CrtHeapBlockFree` (`0x491710`) and `CRTHeapReleaseUnusedPages`
  (`0x491770`, arena pages, shares the `0x491840` blocker), `__fload`
  (`0x492381`, x87 float-load stub family), `StdFileHandleAcquire`
  (`0x495000`) and `CrtFileDescriptorCloseSlot` (`0x499150`, CRT stdio/fd
  tables), `CrtEnvironmentBlockGet` (`0x496190`,
  `GetEnvironmentStrings` copy), `CrtRuntimeErrorMessageBoxDisplay`
  (`0x499730`, `MessageBoxA`). Next: fold into an allocator/CRT-runtime
  port only when a corpus caller needs the observable.
- **Queue-LAB thunk closure** — `FindNearbyPlacementCandidate` (`0x409450`)
  and `NeighborCellEntitySearchRecurse` (`0x409470`): gated on the
  `EntityAtGridPositionLookup` (`0x438670`) callback whose body is an
  unrecoverable `LAB_004386b0` thunk. Next: recover the grid-lookup thunk
  bodies from the spatial-query layer.
- **Renderer/surface seams** — `FrontendProgressValueChanged` (`0x411840`,
  DAT_0076ab3c renderer word), `SurfacePixelWriterSelectByFormat`
  (`0x486690`, surface COM vtable selection), `DDiSceneRenderSurfacePoolGrow`
  (`0x477370`, render surface pool), `BresenhamLineWalkClipped` (`0x486720`,
  painter callback DAT_005407e8 with the DAT_005407ec..f8 clip-window
  floats — none modeled). Next: renderer seam family.
- **Entity registry / spatial seams** — `EntityPoolCallbackSearch`
  (`0x406540`, DAT_004b31c8 handle band + code-thunk callbacks; the refresh
  dispatch quotes it), `EntityDirectedLineSideCheck` (`0x470630`, behind
  `EntityTransformPositionGet` (`0x442260`)'s renderer-transform seam),
  `TypeFourEntityCallbackDispatch` (`0x447900`) stays recorded in the
  callback-registry family above. Next: pool-registry model.
- **Name/string-table families** — `BubbleActionNameToIndex` (`0x407300`),
  `LocalizedObjectNameGet` (`0x439860`), `LocalizedObjectTheNameGet`
  (`0x439790`, both stride the DAT_004b8a3c wide-name-table band),
  `ResourceStringTablesFree` (`0x487760`, frees the same string-table
  block). Next: reference-binary name-table initialization.
- **Config-record chain families** — `LevelNodeGet` (`0x417090`, archive
  record chain through DAT_005560c4/c0 + the LevelLinks table),
  `CampaignChainSnapshotCreate` (`0x418040`, same config-archive chain),
  `BubbleConfigTextureLoad` (`0x407240`, config + texture + name-table
  mix), `ToolTipIconsConfigLoad` (`0x437150`), `HealthBarConfigLoadAndShadeDerive`
  (`0x459560`, 22 config DATs), `OverlayMenuGate` (`0x415370`, DAT menu
  band), `IncidentMessageCatalogInit` (`0x419230`, DAT string catalog).
  All route the blocked config-record lookup (`0x4792e0`) and/or the
  string/name tables. Next: config-record lookup + loader DAT band.
- **DAT token-table band** — `DirectoryTokenListCompare` (`0x480570`):
  compares against the DAT_0076a9e0-counted, stride-0x400
  DAT_005769e0 directory-token table; both globals are unmodeled
  (same band family as the renderer-word blockers). Next: model the
  token table when its loader lands.

Rescan note: this closes the recorded set for the current portable-now
frontier; anything the scanner flags later lands in the same families.
