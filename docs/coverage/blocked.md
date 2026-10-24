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
> configured-variants chain model. The rows below stay blocked on what
> remains of the chain.

| Function (`address`) | Issue(s) | Blocker | Next action |
| --- | --- | --- | --- |
| `ConfiguredVariantPropertySet` (`0x465010`) / `Read` (`0x465030`) | #319 / #320 | Thin wrappers over the now-ported `SoundSampleVariantSelect @ 0x464e20` plus `SoundBankEntryVolumeSet @ 0x47b050` / `IndexedRecordStride134WordGet @ 0x47b0d0` — both small bank-record accessors, port as this cluster's next iteration. | Port the two accessors with the wrapper pair. |
| `SoundAliasPlay` (`0x464fa0`) | #325 | The select half is ported, but the play route itself runs through the instance layer (`DAT_00506684` handle chain, `FUN_0047b1a0` role) which is unported. | Port the sound-instance layer (tracks the `FUN_0047b1a0` chain). |
| `SoundObjectDurationGet` (`0x465160`) | #340 | Asks `SoundSampleVariantSelect` (#324) for the entry and `StreamedWaveBufferStepRate @ 0x47b8e0` (unported) for the duration. | After #324 / bank loaders. |
| `SoundHandleStopForward` (`0x465190`) | #331 | Forwarder into the instance layer: `SoundBufferStopHandle @ 0x47b130` and the instance-chain bookkeeping are unported (the stop seam is modelled as a documented no-op in `sound_subsystem_enable_set`). | Port the sound-instance layer (tracks `FUN_0047b1a0` chain). |
| `PositionalSoundSubmit` (`0x465090`) | #328 | Queues through `SoundRequestQueueSubmit` (ported) but submits positions via `SoundAliasPlay`/the route through #324. | After #325. |
| `SoundBatchQueueDrain` (`0x4652d0`) | #330 | Drains the batch records into `SoundAliasPlay` (blocked, above). | After #325. |
| `SoundSubsystemFrameTick` (`0x4651a0`) | #332 | Rotates the queue (ported) but plays through `SoundAliasPlay` (#325) and drains the deferred queue (#329, same route). | After the playback route. |
| `SceneEffectRegistryReset` (`0x465380`) + transient-effect family (`0x4653a0`–`0x465430`, #333–#339) | #333–#339 | No module owns the transient-effect registry yet (`DAT_00552100` region); the reset itself is trivial but lands meaningfully with its consumers. | Scene-effects milestone: pick an owner module first. |
