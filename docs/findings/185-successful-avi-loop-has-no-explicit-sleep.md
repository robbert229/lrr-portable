# Finding 185 — Successful AVI Playback Has No Explicit Loop Sleep

The end of `FUN_0040a0c0` calls `FUN_0047ed80` to read the movie object's readiness byte. It calls `Sleep(10)` only when that byte is zero; otherwise it runs `FUN_00477f30(0)` and immediately starts the next loop iteration. The AVI constructor `FUN_0047e800` initializes the byte to zero and sets it to one after `AVIStreamOpenFromFileA`, frame-retriever open, and stream-info retrieval all succeed. The movie methods inspected do not clear it during playback. Thus a successfully opened movie follows a loop with no explicit sleep call.

This does not establish that the loop consumes a full CPU core or has a particular frame rate: frame retrieval, the DirectDraw blit using `DDBLT_WAIT`, and the window/update pump may block. It records the absence of a deliberate sleep in the successful-stream branch.

**Evidence.** Shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Constructor initialization and successful-open assignment of the readiness byte are in `FUN_0047e800` at `0x47e800`; `FUN_0047ed80` at `0x47ed80` returns that byte. The conditional `Sleep(10)` and normal `FUN_00477f30(0)` path are in `FUN_0040a0c0` at `0x40a0c0`. `FUN_0047ed00` performs the DirectDraw blit with `0x01000000`; all functions are in `docs/decompiles/full/LEGORR.full.c`.
