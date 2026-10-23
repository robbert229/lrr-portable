# Finding 155 — Shipped Music Streams Exceed the Ring-Buffer Capacity

The streamed music aliases `SFX_AmbientMusicLoop` and `SFX_MusicLoop` map to `Atmosdel.wav` and `stats.wav` in the shipped configuration (finding 144). Both files are PCM WAVs. Their durations are about 90.97 seconds and 12.31 seconds, respectively, while the streamed playback path requests a ring buffer of about three seconds (finding 151). This confirms that the ring is refilled during playback rather than holding an entire shipped music track at once.

| Alias | Streamed file | Format | Duration | File size |
| --- | --- | --- | ---: | ---: |
| `SFX_AmbientMusicLoop` | `Atmosdel.wav` | 2-channel, 16-bit, 44.1 kHz PCM | 90.970 s | 16,047,068 bytes |
| `SFX_MusicLoop` | `stats.wav` | 2-channel, 16-bit, 44.1 kHz PCM | 12.311 s | 2,171,782 bytes |

These measurements establish asset duration and format from the reference installation's on-disk `Data` directory. The alias-to-path mappings are in the WAD's `Lego.cfg`; the two WAV files themselves are not entries in either WAD archive. They do not establish exact buffering latency or when a particular audio device presents the final samples.

**Evidence.** The alias-to-path mapping is documented in finding 144 from `Lego.cfg` in `reference/LegoRR1.wad` (WAD SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`). The streamed buffer sizing is documented in finding 151 from `FUN_0047baa0`. WAV metadata was read from the reference installation files `reference/Data/Sounds/Streamed/Atmosdel.wav` (SHA-256 `1877cb4df758ae31641025a07a8c31e8fe57c142c076cf75971068ccd3f07908`) and `reference/Data/Sounds/Streamed/stats.wav` (SHA-256 `650623199ab8210ac742a90d5560c38a9690b1988ce260b5ba02c5f0d3422bfe`). Neither name appears in the WAD entry tables.
