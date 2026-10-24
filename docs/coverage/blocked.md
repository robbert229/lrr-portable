# Blocked port candidates

Functions the port loop cannot port safely right now, with the concrete
blocker and the next action. Recorded per `hack/port-loop.md`; nothing here
is skipped for convenience — each entry names the unported dependency that
must land first.

## Sound-subsystem wave

| Function (`address`) | Issue(s) | Blocker | Next action |
| --- | --- | --- | --- |
| `SoundBankEntryLoad` (`0x47a870`) | — | Own cluster: the 600-slot bank table (`DAT_00506698`/stride `0x134`) plus the wav/stream loaders `InMemoryWaveSampleLoad @ 0x47b580` (1670 B) and its flag/handle bookkeeping. Not a blocker-in-itself, but it is the root of every remaining sound chain, so it is scheduled as its own cluster. | Port the bank-entry cluster (loader seam reusing `sound_bank_load`), then the chains below unlock. |
| `SoundSampleMarkerParse` (`0x464d00`) | #318 | Samples the bank record `SoundBankEntryLoad @ 0x47a870` filled (chain at `DAT_00501468`); without it the marker lists have no producer. | After `SoundBankEntryLoad`. |
| `SoundSampleVariantSelect` (`0x464e20`) | #324 | Reads/writes the marker chains #318 builds (plus the ported `RandomNext @ 0x48e170` PRNG draw); dependency chain #318 → bank layer. | After #318. |
| `ConfiguredVariantPropertySet` (`0x465010`) / `Read` (`0x465030`) | #319 / #320 | Thin wrappers over `SoundSampleVariantSelect @ 0x464e20`, `SoundBankEntryVolumeSet @ 0x47b050` (unported), `IndexedRecordStride134WordGet @ 0x47b0d0` (unported). | After #324 and the bank-entry pair. |
| `SoundAliasPlay` (`0x464fa0`) | #325 | The submit route needs `SoundSampleVariantSelect` (#324); the batch path additionally drains through it. | After #324. |
| `SoundObjectDurationGet` (`0x465160`) | #340 | Asks `SoundSampleVariantSelect` (#324) for the entry and `StreamedWaveBufferStepRate @ 0x47b8e0` (unported) for the duration. | After #324 / bank loaders. |
| `SoundHandleStopForward` (`0x465190`) | #331 | Forwarder into the instance layer: `SoundBufferStopHandle @ 0x47b130` and the instance-chain bookkeeping are unported (the stop seam is modelled as a documented no-op in `sound_subsystem_enable_set`). | Port the sound-instance layer (tracks `FUN_0047b1a0` chain). |
| `PositionalSoundSubmit` (`0x465090`) | #328 | Queues through `SoundRequestQueueSubmit` (ported) but submits positions via `SoundAliasPlay`/the route through #324. | After #325. |
| `SoundBatchQueueDrain` (`0x4652d0`) | #330 | Drains the batch records into `SoundAliasPlay` (blocked, above). | After #325. |
| `SoundSubsystemFrameTick` (`0x4651a0`) | #332 | Rotates the queue (ported) but plays through `SoundAliasPlay` (#325) and drains the deferred queue (#329, same route). | After the playback route. |
| `SceneEffectRegistryReset` (`0x465380`) + transient-effect family (`0x4653a0`–`0x465430`, #333–#339) | #333–#339 | No module owns the transient-effect registry yet (`DAT_00552100` region); the reset itself is trivial but lands meaningfully with its consumers. | Scene-effects milestone: pick an owner module first. |
