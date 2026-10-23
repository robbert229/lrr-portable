# Finding 385 — Stream, Surface, AVI, and Route State Helpers

This pass names 6 routines across the mid and high ranges. Names follow the observed stream, surface, playback, and state operations.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x0047ded0` | `DirectDrawSurfaceUnlock` | Calls the DirectDraw surface vtable unlock method after pixel access. | Caller unlocks after reading and comparing pixel values; context `ev_4c8f547e011bded8d18a8468e03056e2106973f22c99bcf86f5fb9f8edbf0acd` in `legorr-engine-helper-audit-text-io-expansion-9`. |
| `0x0048c010` | `WadStreamLengthGet` | Returns the length of the indexed virtual stream. | Callers use the result to clamp seek/read positions and detect end-of-stream; pseudocode evidence `ev_2c5dc954776ccd3a5b24e04044d740e7eb5e5eae17e84c129dfbd4cf210b9ab4` in `legorr-wad-stream-contract`, caller evidence `ev_50b14dac0cd42f8acc3a7ee94bf1a348c698c9bf9a6760315958ebcd2dc6f7ec`. |
| `0x0047ebe0` | `AviStreamFrameCountGet` | Returns the guarded stream frame-count field at object offset `+0x2c`. | Constructor `FUN_0047e800` initializes it from AVI stream info; caller compares it to the current frame index. Context `ev_dadf84acaca3aed422ec63ee08e843ab8d871fa7bcf898b345097a24ca26e6a6` in `legorr-engine-helper-audit-text-io-expansion-9`. |
| `0x00495a60` | `CrtUnwindAddressResolve` | Computes an address from unwind metadata base and offset fields. | Pseudocode evidence `ev_36334223f7669850e141b6b9f51c0aac28f193cb85b661447986065e96ccd126` in `legorr-crt-third-pass-2026-10-23`. |
| `0x0044cf70` | `ScenePositionResetAndEffectMotionCancel` | Reads the current owner pose into position state and clears state bit `0x80`. | Caller `FUN_004288e0` updates a scene transform before calling it; evidence `ev_7479e850be1cf44732679a120831ff8ccf2231130460878ab4f4503721bf00bb` in `legorr-engine-helper-audit-terrain-expansion-8.bundle`. |
| `0x0044da20` | `RoutePositionBlockedStateSet` | Sets or clears state bit `0x200` based on whether a cursor/ray target lookup failed. | Caller `FUN_00426520` passes the failed-lookup condition; evidence `ev_3406ffd8a909bb28794a384d1e88cb9f3ef00f262d70d81867bc827746ed3f3b` in `legorr-engine-helpers-expansion-2.bundle`. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/engine-region.c).
