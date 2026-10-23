# Finding 152 — Streamed Playback Reuses Its Buffer as a Looping Ring

After loading a streamed WAV, `FUN_0047b970` resets the selected DirectSound buffer's position to zero and calls its vtable slot `+0x30` (`IDirectSoundBuffer::Play`) with `(0, 0, 1)`. The third argument is the play-flags parameter; DirectSound defines flag `1` as `DSBPLAY_LOOPING`, which makes playback wrap to the start of the buffer until stopped.

The stream update path calls `FUN_0047bdb0` for both stream states from `FUN_0047b390`. That helper reads buffer cursors, locks buffer regions, and fills them from the stream data as playback advances. Combined with finding 151, this means the approximately three-second DirectSound allocation is a reusable ring-buffer capacity, not a three-second limit on stream playback.

| Operation | Code path | Observed behavior |
| --- | --- | --- |
| Start stream | `FUN_0047b970` | Set position to zero, then `Play(0, 0, 1)` |
| API flag meaning | `IDirectSoundBuffer::Play` | Flag `1` loops the buffer until explicitly stopped |
| Refill | `FUN_0047b390` → `FUN_0047bdb0` | Inspect cursors and fill locked regions from stream data |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The position reset and `Play` arguments are in `FUN_0047b970` at `0x47b970`; the cursor and refill path is in `FUN_0047bdb0` at `0x47bdb0`, called by `FUN_0047b390` at `0x47b390`, in `docs/decompiles/full/LEGORR.full.c`. Finding 136 records the stream start and update call flow; finding 151 records the ring-buffer capacity. Microsoft documents the [Play flags and looping behavior](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708933%28v%3Dvs.85%29).
