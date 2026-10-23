# Finding 191 — Level Video Stops on Mouse Button Down

The level-video wrapper calls `FUN_00415700` with its input-cancel parameter set to `1`. In that mode, the playback loop returns as soon as the DirectInput keyboard buffer contains any nonzero byte or either mouse-state byte (`DAT_0076ab40` or `DAT_0076ab3c`) is nonzero. The window-message handler sets those bytes on mouse-button down, so a press is enough to stop a level video; it does not wait for button release. The credits AVIFIL32 loop uses a different predicate: it exits on keyboard state or a mouse press-to-release transition (finding 181).

This describes the static skip condition for the level-video call path. It does not establish frame timing or whether an input event is delivered during every movie presentation interval.

**Evidence.** Shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00429890` at `0x429890` calls `FUN_00415990(param_1, 1)`; `FUN_00415990` passes that parameter to `FUN_00415700` at `0x415700`. The loop checks `FUN_0047efb0`, `DAT_0076ab40`, and `DAT_0076ab3c` before updating the stream. `FUN_004784c0` at `0x4784c0` sets the mouse-state bytes on button-down messages. The functions are in `docs/decompiles/full/LEGORR.full.c`.
