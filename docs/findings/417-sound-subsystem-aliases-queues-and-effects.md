# Finding 417 — Sound aliases, submission queues, and transient effects

This finding maps the Issue #193 assignment and corrects a boundary error in
its area ledger: the functions at `0x00465380–0x00465b00` operate a transient
scene-effect emitter registry and its render resources. Several also coordinate
an optional effect sound, but they are not the sound alias/request subsystem.
Those eight rows are reclassified to `scene-resources-effects`; their canonical
names and generated coverage indexes now reflect the observed behavior.

## Sound request path

`SoundAliasRegistryInit` creates 45 built-in hashed aliases. The `Samples`
config loader resolves names with `SoundAliasLookup` and creates their variant
lists with `SoundSampleMarkerParse`; the existing details about `*`, `@`, and
`#...#` markers and random variant selection are in findings
[107](107-samples-config-sound-alias-variants.md) and
[142](142-sample-hash-marker-sets-directsound-volume.md). Dynamic registration
has an unchecked table bound, already quantified in finding
[178](178-dynamic-sample-alias-table-has-no-capacity-check.md).

Playback helpers route alias variants to the sound bank. Immediate alias and
positional requests are gated by the subsystem-enabled bit. During packed batch
mode they are copied into a ten-entry queue; the separate deferred alias queue
also holds ten requests and rotates at the frame tick. Both can silently drop
requests after capacity is reached (findings
[139](139-sound-submission-queue-preserves-selector-bits.md),
[149](149-deferred-sound-queue-drops-requests-after-ten.md), and
[209](209-batched-spatial-sound-queue-drops-requests-after-ten.md)). The frame
tick temporarily clears batch mode, replays the prior deferred list, rotates
newly queued requests, decrements the global cooldown, invokes the sound-source
spatial refresh thunk, and restores batch mode if it was enabled. It does not
drain or clear packed requests. Queue lifetime across frame updates and disablement is
covered in findings [212](212-frame-sound-tick-does-not-drain-packed-batch-queue.md)
and [213](213-disabling-sound-does-not-clear-packed-requests.md).

## Assigned function map

| Address | Corrected name | Observed responsibility and route |
| --- | --- | --- |
| `0x00464740` | `SoundAliasRegistryInit` | Allocates a `0x7bc`-byte hash table, registers the 45 built-in `SFX_*` names, and registers a callback for enabling dynamic alias registration. |
| `0x00464c50` | `SoundDynamicAliasRegistrationSet` | Sets or clears bit 1 in `DAT_00502a2c`; `SoundAliasLookup` uses this bit to decide whether an unknown name may be appended. |
| `0x00464c70` | `SoundAliasLookup` | Hashes and scans the built-in plus dynamic alias table; when registration is enabled, appends unknown names without checking the table capacity (finding 178). |
| `0x00464d00` | `SoundSampleMarkerParse` | Splits a sample list, interprets leading markers, loads each sample through `SoundLoad`, and links successful slots into the alias variant chain (finding 107). |
| `0x00464e20` | `SoundSampleVariantSelect` | Counts an alias's variant chain, chooses a random member, swaps it to the chain head, and returns its sound-bank slot (findings 107, 147). |
| `0x00464e80` | `DeferredSoundPlayExpire` | If a pending sound index exists and the timer is positive, submits that alias and clears the pending index and timer. |
| `0x00464ec0` | `DeferredSoundTimerSetIfIdle` | Sets a deferred timer only when no positive timer is active; clears the pending index and returns success. |
| `0x00464ef0` | `SoundSubmitWithGlobalCooldown` | When the global cooldown is clear, plays an alias, records its ID, sets cooldown to selected sample duration times 25, and optionally returns the sound handle. |
| `0x00464f60` | `SoundRequestQueueSubmit` | Appends alias and selector to the separate deferred queue while its count is below ten; overflow is silently ignored (finding 149). |
| `0x00464fa0` | `SoundAliasPlay` | Selects an alias variant and submits it to the sound bank when sound is enabled; under packed batching, saves alias and selector for later replay. |
| `0x00465010` | `ConfiguredVariantPropertySet` | Selects an alias variant and forwards the selected slot to the sound-bank property setter. |
| `0x00465030` | `ConfiguredVariantPropertyRead` | Selects an alias variant and forwards the slot to the sound-bank property getter. |
| `0x00465050` | `SoundSubmissionRouteWrapper` | Extracts a wrapped scene-object pointer when non-null, then forwards to `PositionalSoundSubmit`. |
| `0x00465090` | `PositionalSoundSubmit` | Selects a variant and submits either object-attached or explicit-position sound; during packed batching stores a `0x18`-byte request if fewer than ten are queued, else returns zero (finding 209). |
| `0x00465160` | `SoundObjectDurationGet` | Selects a variant and returns its sound-bank duration; selection also rotates the chosen variant to the chain head. |
| `0x00465190` | `SoundHandleStopForward` | Forwards a sound handle to the sound-bank stop helper. This address had a misleading `SoundSubsystemFrameTick` label. |
| `0x004651a0` | `SoundSubsystemFrameTick` | Advances cooldown, temporarily disables batch mode while submitting the previous deferred list, rotates the current deferred queue, resets its count, invokes spatial refresh, and restores batch mode when it was previously enabled; it does not drain packed requests. |
| `0x00465250` | `thunk_FUN_0047b390` | Calls `SoundSourceSpatialRefreshAll` through a thunk; the refresh logic is described in findings 162–164 and 196–199. |
| `0x00465260` | `SoundSubsystemEnableSet` | Sets or clears the sound-enabled bit; on disable, optionally stops all sound and clears the global cooldown alias. |
| `0x004652a0` | `SoundSubsystemEnableSetAndStop` | Calls the enable setter with the stop-all option enabled. |
| `0x004652b0` | `SoundBatchModeGet` | Returns the packed-batching flag. This address was mislabeled as a queue update. |
| `0x004652c0` | `SoundDeferredQueueDrain` | Forwards its argument to `SoundBatchQueueDrain` as the replay/clear option. |
| `0x004652d0` | `SoundBatchQueueDrain` | Enables batch mode when requested; otherwise clears it and, when requested, replays and clears all packed requests. |
| `0x00465370` | `SoundSubsystemEnabledGet` | Returns the sound-enabled bit; this address was mislabeled as dynamic-alias state. |
| `0x00465380` | `SceneEffectRegistryReset` | Clears the transient-effect registry head, tail, and shared texture count. Reclassified to scene resources/effects. |
| `0x004653a0` | `TransientEffectTexturesPreload` | Loads numbered BMP textures into the shared transient-effect texture table. Reclassified to scene resources/effects. |
| `0x00465430` | `TransientEffectEmitterCreate` | Allocates an emitter record and scene context, creates textured particle slots, seeds an initial burst for applicable modes, and links the record into the effect registry. Reclassified to scene resources/effects. |
| `0x00465970` | `TransientEffectRegistryReleaseAll` | Walks the emitter registry, releases each record, then clears its head. Reclassified to scene resources/effects. |
| `0x004659b0` | `TransientEffectInstanceRelease` | Releases the emitter render resource and scene object, unlinks it, stops an associated sound handle when present, and frees the record. Reclassified to scene resources/effects. |
| `0x00465a90` | `TransientEffectRegistryEnableSet` | Walks all registered emitters and applies an enabled state. Reclassified to scene resources/effects. |
| `0x00465ac0` | `TransientEffectEnableSet` | Updates the wrapped scene resource and mirrors enabled state in the emitter record. Reclassified to scene resources/effects. |
| `0x00465b00` | `TransientEffectInstanceUpdate` | Updates emitter records and particle slots, optionally submits an associated positional sound once, and may submit effect sound when emission ends. The zero-handle latch is detailed in finding [221](221-lava-effect-latches-zero-sound-handle.md). Reclassified to scene resources/effects. |

## Evidence limits and follow-ups

The queue flags, capacities, alias hash behavior, and effect-record links are
directly visible in the checked-in decompilation. The dynamic alias table's
configured overflow threshold and shipped-config count are independently
measured in finding 178. Scene effect resource and particle semantics are
inferred from the constructor's scene-context, texture, mesh, and particle-slot
calls; the external object type and some field meanings remain unresolved.
