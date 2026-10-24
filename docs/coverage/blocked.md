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
| `SoundObjectDurationGet` (`0x465160`) | #340 | Asks `SoundSampleVariantSelect` (#324) for the entry and `StreamedWaveBufferStepRate @ 0x47b8e0` (unported) for the duration. | After #324 / bank loaders. |
| `SoundHandleStopForward` (`0x465190`) | #331 | Forwarder into the instance layer: `SoundBufferStopHandle @ 0x47b130` and the instance-chain bookkeeping are unported (the stop seam is modelled as a documented no-op in `sound_subsystem_enable_set`). | Port the sound-instance layer (tracks `FUN_0047b1a0` chain). |
| `SceneEffectRegistryReset` (`0x465380`) + transient-effect family (`0x4653a0`–`0x465430`, #333–#339) | #333–#339 | No module owns the transient-effect registry yet (`DAT_00552100` region); the reset itself is trivial but lands meaningfully with its consumers. | Scene-effects milestone: pick an owner module first. |

## Media-playback wave

> Update (2026-10-24): the state machines (`0x4297b0` / `0x4295f0`,
> issues #301 / #302), the playback toggle (`0x429810`, issue #298), and
> the cue-dispatch pair with the cursor-override state (`0x428800` /
> `0x4351a0` / `0x45cc20` / `0x45cc40`, issue #300) are **ported** — the
> first three clusters of this wave.

| Function (`address`) | Issue(s) | Blocker | Next action |
| --- | --- | --- | --- |
| `StreamPlayLevel` (`0x416150`) / `StreamPlayTutorial` (`0x4161a0`) | #299 | Both resolve a `Stream_LevelName_*` alias and submit through `SoundSubmitWithGlobalCooldown @ 0x464ef0` (ported): the duration half asks `SoundObjectDurationGet @ 0x465160` → `StreamedWaveBufferStepRate @ 0x47b8e0` (unported, #340) and the stream state lands at the unported streamed-voice seam (`FUN_0047b970` selector). | After the streamed-voice layer (with #340). |
| `DirectShowVideoPlaybackLoop` (`0x42efc0`) | #303 | Not yet triaged against the movie module; the rect-aware playback seam needs the render-surface restore semantics read against `movie.*` first. | Triage next: shape-compatible rect pump into the existing movie driver. |
