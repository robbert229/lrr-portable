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
