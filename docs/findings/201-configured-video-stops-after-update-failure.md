# Finding 201 — Configured Video Stops When Its Update Callback Fails

The configured-video playback wrappers treat a false result from the sample-update callback as the end of that playback invocation. The boot/level helper returns when an update fails or its stop-input gate fires, and its caller then destroys the video wrapper. The level-video path repeats the callback only while it returns true, then destroys the wrapper after the loop. Neither wrapper retries the same opened stream after a failed sample update or blit.

**Evidence.** In `reference/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), `FUN_00415700` (`0x415700`) calls `FUN_004726d0` (`0x4726d0`) once before its render loop and again at the loop tail; a false result returns from the helper. `FUN_00415910` and `FUN_00415990` call `FUN_004726f0` (`0x4726f0`) immediately after `FUN_00415700` returns. The level route `FUN_0042efc0` (`0x42efc0`) loops while `FUN_004726d0` returns true, then calls `FUN_004726f0`. That cleanup calls `FUN_00472430` to release the wrapper's retained interfaces, then frees the wrapper. Finding 200 shows that the update callback returns false on a nonzero sample-update or blit HRESULT.

This establishes control flow for one configured-video playback invocation. It does not rule out a later caller opening the same file again as a separate invocation.
