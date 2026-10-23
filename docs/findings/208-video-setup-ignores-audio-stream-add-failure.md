# Finding 208 — Configured Video Continues if Adding Primary Audio Fails

The configured-video setup requires the primary-video stream to be added successfully, but it treats primary-audio stream addition as best effort. If the audio `AddMediaStream` call fails, the helper clears that HRESULT and continues to `OpenFile`. The caller can therefore proceed with video even when the requested default-rendered audio stream was not added.

**Evidence.** Static disassembly of shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. In `FUN_004722b0` (`0x4722b0`), the primary-video `AddMediaStream` call at `0x47230c` is checked and a failing HRESULT branches to cleanup. The primary-audio call at `0x472327` is followed by `test esi, esi; jge 0x472332; xor esi, esi` (`0x47232a–0x472332`), so failure is converted to success before the helper calls `OpenFile` at `0x47235e`. Finding 189 identifies the two calls as primary video and primary audio and records the audio flags as `AMMSF_ADDDEFAULTRENDERER`.

This is a conditional setup path; no audio-stream failure was induced at runtime.
