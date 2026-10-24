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

