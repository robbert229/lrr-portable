# Finding 158 — SFX Volume Slider Sets Primary DirectSound Volume

Although the Options menu labels this control `SFX_Volume`, the callback ultimately calls `SetVolume` on `DAT_005065bc`. Sound initialization creates that object with a `DSBUFFERDESC` whose flags include `DSBCAPS_PRIMARYBUFFER` (`dwFlags = 0x91`), with zero buffer bytes and a null format pointer. The slider therefore changes the primary DirectSound buffer's volume, rather than setting a volume only on individual sound effects.

The streamed WAV path also creates secondary DirectSound buffers. DirectSound mixes secondary buffers into the primary output, and Microsoft's documentation describes primary-buffer `SetVolume` as changing the waveform-audio volume. Thus the SFX-labeled slider controls the shared DirectSound output path, which includes the streamed music aliases as well as ordinary sample playback. This does not establish how the separate `Music_Volume` mixer control combines with the primary-buffer setting on a particular device.

Finding 156 records the slider's attenuation table. This finding clarifies the buffer that receives those values and the resulting scope implied by the API path.

| Item | Observation |
| --- | --- |
| Options label | `SFX_Volume` |
| Buffer receiving `SetVolume` | Primary DirectSound buffer at `DAT_005065bc` |
| Streamed music buffers | Secondary buffers mixed into the primary output |
| Practical control scope | Shared DirectSound waveform output, not a proven SFX-only bus |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Primary-buffer creation is in `FUN_0047a640` (`ev_283012591a3e636bd693bae7155afd79c42c71d389e899ba5069fb0a999260dd` in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle`); the options callback and setter are in finding 156; streamed secondary-buffer creation is in `FUN_0047baa0` (`ev_7cc236e0d2189886aaec15f33fb0c43e84e0f9320de17e56d30b266a46aff417` in `docs/evidence/legorr-streamed-wav-format.bundle`). Microsoft documents that [`DSBUFFERDESC` with `DSBCAPS_PRIMARYBUFFER` creates the primary buffer](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416820%28v%3Dvs.85%29), that [primary-buffer `SetVolume` changes waveform-audio volume](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418742%28v%3Dvs.85%29), and that [secondary buffers are mixed into the primary buffer](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708933%28v%3Dvs.85%29).
