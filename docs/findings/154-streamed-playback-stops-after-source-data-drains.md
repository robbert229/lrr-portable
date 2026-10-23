# Finding 154 — Streamed Playback Stops After Source Data Drains

The DirectSound buffer starts with the looping flag described in finding 152, but the source WAV is not rewound when it reaches its end. During updates, `FUN_0047bdb0` reads the next stream region into the ring. When a read is shorter than the requested region, the helper marks the source as exhausted and fills the rest of that region with PCM silence. Later updates fill the remaining ring regions with silence; once the cursor has passed the queued data, the helper calls `FUN_0047ba20` to stop the DirectSound buffer and release the stream state.

The loop flag therefore keeps the circular DirectSound buffer running while the engine refills it. It does not make a finite streamed WAV repeat indefinitely. The static code does not establish the exact stop delay under a particular frame cadence or audio device.

| Source state | Ring-buffer action |
| --- | --- |
| Stream data remains | Read the next region from the WAV source |
| A read reaches the source end | Mark the source exhausted and pad the short region with silence |
| Exhausted source while queued regions drain | Fill later regions with silence |
| Cursor passes the final queued data | Call `FUN_0047ba20` to stop and release the stream |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The source-read, silence-padding, and drain checks are in `FUN_0047bdb0` (`ev_2d235b60c636e045a39b4d1c8de8e65d59d3158ba293bc5965ae730f754a9421`) in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle`; initial buffer filling is recorded as `FUN_0047bc30` (`ev_3084209a51322f47dc031f88cb0e6f27f3577bce949d24468593543cfe9deee0`); the stop and cleanup helper is `FUN_0047ba20` (`ev_3191e8679e599a56da9ce0b4c1969d533b63db64ef61b1d1092c0adb426784e2`). All three functions are also in `docs/decompiles/full/LEGORR.full.c`.
