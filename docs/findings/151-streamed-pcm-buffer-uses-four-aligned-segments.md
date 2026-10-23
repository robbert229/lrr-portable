# Finding 151 — Streamed PCM Buffer Uses Four Aligned Segments

After the PCM-only format check, `FUN_0047baa0` computes a segment size from `nSamplesPerSec * nBlockAlign * 3 / 4`, using integer arithmetic. It rounds that size up to a multiple of `nBlockAlign`, then sets the DirectSound buffer's byte size to four times the aligned segment size. The requested buffer is therefore approximately three seconds of PCM data, with the total byte count aligned to complete sample frames.

The code establishes the requested capacity and alignment. It does not by itself establish actual playback latency or how much audio is queued ahead at a particular moment; those depend on the write and play cursors handled by the stream refill helpers.

| Quantity | Calculation |
| --- | --- |
| Initial segment bytes | `floor(nSamplesPerSec × nBlockAlign × 3 / 4)` |
| Aligned segment bytes | Round initial segment up to a multiple of `nBlockAlign` |
| DirectSound buffer bytes | `4 × aligned segment bytes` |

This follows the PCM acceptance gate documented in finding 150. The dimensions come from the parsed WAVE format, so the resulting capacity varies with sample rate and block alignment.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The segment calculation, block-alignment round-up, and `DSBUFFERDESC.dwBufferBytes` setup are in `FUN_0047baa0` at `0x47baa0` in `docs/decompiles/full/LEGORR.full.c`. The function is recorded in `docs/evidence/legorr-streamed-wav-format.bundle` (`ev_7cc236e0d2189886aaec15f33fb0c43e84e0f9320de17e56d30b266a46aff417`).
