# Finding 168 — In-Memory Sound Start Ignores DirectSound HRESULTs

On the in-memory branch, `FUN_0047ad70` calls the selected buffer's `SetCurrentPosition(0)` and then calls `Play`. It does not test either HRESULT. After `Play`, both branches return `bankSlot * 3 + selectedBufferIndex`; the `Play` HRESULT does not affect the returned value. The returned integer therefore identifies the selected bank/buffer slot even when DirectSound reports a start error.

This is distinct from streamed start, where finding 153 traces ignored `SetCurrentPosition` and `Play` HRESULTs before the stream is marked active. In both cases, static call flow does not show that playback became audible.

**Evidence.** Shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. In `FUN_0047ad70` at `0x47ad70`, the common path calls the selected buffer's vtable slot `+0x34` at `0x47afbd` (`SetCurrentPosition`) and does not branch on its result; the `Play` call through slot `+0x30` at `0x47afd4` or `0x47aff7` is followed directly by computing the return identifier. The decompile is retained as REA record `ev_02979193ddb48091c08741d27b02a3ab17faf60623138fd8969a49fc55ae655e` in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle`.
