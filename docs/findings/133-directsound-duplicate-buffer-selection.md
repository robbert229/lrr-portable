# Finding 133 — Optional DirectSound Duplicates Are Selected by a Three-Buffer Rotation

The sound-bank setup passes its third argument through `FUN_0047b580` to `FUN_0047b6c0`. When that argument is nonzero, the loader creates the optional duplicate buffers at sound-record offsets `+0x124` and `+0x128`, alongside the primary buffer at `+0x120` (finding 120). After successful loading, `FUN_0047a870` sets bank-entry flag `0x4` when the same argument is nonzero.

At submission, `FUN_0047ad70` checks that flag. If clear, it selects the single pointer at bank-entry `+0x120`. If set, it increments the bank entry's selection index, wraps it to zero after index 2, and selects one of the three pointers at `+0x120`, `+0x124`, or `+0x128`. The selected interface receives the playback call through vtable offset `+0x30`; the helper returns the bank slot multiplied by three plus the selected index.

This establishes the selection policy for the optional duplicate buffers: the bank-entry flag selects between one buffer and round-robin use of three buffers. The static evidence does not establish runtime audibility or the audible effect of the rotation. `FUN_0047a870` ties the third argument to this flag; finding 107 documents the higher-level sound alias marker that supplies the loader parameter.

**Evidence.** Fresh Ghidra records for `FUN_0047ad70` (`ev_b1b47042…daa5d2`), `FUN_0047a870` (`ev_25c4d48e…aff5218`), and `FUN_0047b580` (`ev_dc7c21b9…252d7e9`), together with the canonical creation record from finding 120, are in [the evidence bundle](../evidence/legorr-sound-duplicate-selection.bundle). Static analysis of shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
