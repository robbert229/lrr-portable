# Finding 175 — Null AVI Frame Fetch Leaves and Reblits Previous Pixels

`FUN_0047eb20` requests the current frame index with `AVIStreamGetFrame`. When the API returns null, it skips both `FUN_0047e9e0` (the copy into the movie surface) and the time-based next-index update, then returns `1` through its common tail. The movie command loop `FUN_0040a0c0` ignores that return value and calls `FUN_0047ed00` unconditionally afterward.

Consequently, on a null frame result, the movie surface retains the pixels from its prior successful copy and the caller submits that surface to the shared render target again. Because the frame index and clock base are not updated on this path, subsequent iterations retry the same index until some other state changes. This describes any null result; it does not establish that null corresponds to end-of-stream for a shipped clip.

**Evidence.** Shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Fetch, null test, successful copy, time/index update, and common return are in `FUN_0047eb20` at `0x47eb20`; unconditional update/draw calls are in `FUN_0040a0c0` at `0x40a0c0`; surface blit is in `FUN_0047ed00`. The decompilation is in `docs/decompiles/full/LEGORR.full.c`. Finding 68 records the null-fetch state behavior; finding 172 traces the frame blit target.
