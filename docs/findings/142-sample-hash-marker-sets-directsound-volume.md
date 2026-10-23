# Finding 142 — The `#...#` Sample Marker Sets DirectSound Volume

Inside each comma-delimited sample item, `FUN_00464d00` recognizes a leading `#...#` after an optional `*`. It parses the text between the hashes as a signed decimal integer with `FUN_0048dad0`, removes that prefix from the path, and passes the integer as the fourth argument to `FUN_0047a870`. The parsed number is therefore a per-sample volume value, not part of the filename.

`FUN_0047a850` accepts values from `-10,000` through `0`, inclusive; `FUN_0047a870` stores an out-of-range value as `0`. During playback, `FUN_0047ad70` reads that field and calls the selected sound buffer's vtable slot `+0x3c` with it. The slot is `IDirectSoundBuffer::SetVolume`; DirectSound specifies this value in hundredths of decibels, with `0` as unattenuated volume and `-10,000` as silence. Global in-memory WAV playback subtracts `0x320` (800 hundredths of a decibel) before the SetVolume call; finding 148 details this mode-specific adjustment and its lower-range edge.

The `#...#` branch is processed per sample item and can precede the optional `@` marker. The shipped `Samples` section has no active `#...#` example, so this documents the implementation and accepted range rather than a demonstrated stock-config use. The parser's temporary token buffer is not terminated or length-checked; finding 145 records that limitation and its effect on exact values for malformed or modded markers.

| Stage | Function | Behavior |
| --- | --- | --- |
| Parse marker | `FUN_00464d00` → `FUN_0048dad0` | Read signed decimal volume from between `#` delimiters |
| Validate/store | `FUN_0047a850` / `FUN_0047a870` | Accept `[-10,000, 0]`; use `0` for out-of-range values |
| Playback | `FUN_0047ad70` | Pass stored value to sound-buffer vtable `+0x3c` (`SetVolume`); mode 2 subtracts 800 |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Parser and sound-record setup are in `docs/decompiles/full/LEGORR.full.c` at `FUN_00464d00`, `FUN_0048dad0`, `FUN_0047a850`, and `FUN_0047a870`; playback's volume call is at `0x47ae77–0x47ae8b`. The parser is recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af`, `ev_98a95a387f00ede6b484669f9341f5e0b38636f97b03e08fe8bccb6df4bc1b1f`), and the volume getter/setter is in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle` (`ev_4bbc340886c6fd88b848f754c38702dc6efda550bf1b4e3547c3f45be8a0a414`). Microsoft's [`IDirectSoundBuffer::SetVolume` documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708939%28v%3Dvs.85%29) defines the attenuation units and range.
