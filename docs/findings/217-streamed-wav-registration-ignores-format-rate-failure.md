# Finding 217 — Streamed WAV Registration Ignores a Failed Format-Rate Probe

In the streamed branch of `FUN_0047a870`, the game first opens the path through
its file layer and records the file size. It then calls `FUN_00488e70` to
probe WAV metadata and stores the returned value in the sound record. That
helper returns zero if `mmioOpenA`, RIFF/WAVE descent, `fmt ` descent, or the
16-byte format read fails. The caller does not test that value: it sets the
streamed-entry flag and marks the sound-load branch successful anyway.

Thus a streamed alias can be registered even when this MMIO format-rate probe
failed. This does not prove that a later playback request will fail or succeed;
the playback path performs its own WAV parsing (findings 150 and 153). The
probe's zero value is also used as the denominator by `FUN_0047b8e0` for
stream-duration calculation, and that helper has no zero guard before the
division.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
In the streamed branch of `FUN_0047a870` at `0x47a870`, the result of
`FUN_00488e70` at `0x488e70` is stored at record offset `+0xa0`; the code then
sets flag bit `0x8` and sets its local success flag without checking the
stored value. `FUN_00488e70` returns zero on each failed open/chunk/read path.
`FUN_0047b8e0` at `0x47b8e0` divides the stored file size by that field for
streamed entries. Finding 218 traces how this probe opens the path separately
from the game's file-layer handle. These functions are in
`docs/decompiles/full/LEGORR.full.c`; finding 150 documents the separate
PCM-only playback gate.
