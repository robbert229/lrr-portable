# Finding 148 — Global WAV Playback Applies an Additional 8 dB Attenuation

`FUN_00464fa0(alias, selector)` sends playback through `FUN_0047ad70` with first argument `2`. On its in-memory WAV branch, `FUN_0047ad70` reads the sample's stored volume field and, when that mode equals `2`, subtracts `0x320` before calling the selected buffer's vtable slot `+0x3c` (`IDirectSoundBuffer::SetVolume`). Since DirectSound volume units are hundredths of a decibel, this is an additional 8 dB attenuation for the global alias-playback route.

The positional wrapper `FUN_00465090` calls `FUN_0047ad70` with first argument `0` or `1`, so that path does not apply the extra subtraction. The adjustment is in the in-memory WAV branch; streamed aliases take the separate stream path. Finding 142 documents the stored volume and its accepted input range.

Because the configured range can reach `-10,000`, subtracting 800 can produce a `SetVolume` value below DirectSound's accepted minimum for requests below `-9,200`. The call site does not inspect the `SetVolume` result. No active shipped `#...#` sample invokes this edge; its audible or buffer-state result is not established.

| Playback route | First argument to `FUN_0047ad70` | SetVolume adjustment |
| --- | ---: | ---: |
| Global alias playback via `FUN_00464fa0` | `2` | Subtract `800` hundredths of a decibel on in-memory WAV path |
| Positional playback via `FUN_00465090` | `0` or `1` | No 800-unit subtraction |
| Streamed alias | Either route enters stream branch | This WAV-buffer adjustment is bypassed |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00464fa0` and `FUN_00465090` are in `docs/decompiles/full/LEGORR.full.c` and the `FUN_0047ad70` call setup is at `0x47ae77–0x47ae8b`. The wrappers are recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_be8e1e65cbfba0701c22725feb605a1782556c1323859d2ca1f54a393cf230fa`) and `docs/evidence/legorr-sound-instance-spatial-expansion.bundle` (`ev_7da0a488bdbf46abfc69ed8002c9fc64f46e9742a0fc060034aedd5bca77cae`). Microsoft documents the [SetVolume units and valid range](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708939%28v%3Dvs.85%29).
