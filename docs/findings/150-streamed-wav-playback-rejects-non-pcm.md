# Finding 150 — Streamed WAV Playback Rejects Non-PCM Formats

The streamed-WAV parser `FUN_00488f10` can return a format descriptor containing non-PCM `WAVEFORMATEX` extension data. The playback helper `FUN_0047baa0` then checks the descriptor's format tag before creating a DirectSound buffer. If the tag is not `1` (PCM), it releases the parsed format and returns failure. The buffer-creation call at vtable slot `+0x0c` is reached only for PCM streams.

This corrects finding 108's earlier inference that preserved non-PCM extension data reaches DirectSound. The parser accepts and retains that metadata, but this playback helper rejects the corresponding format before the API call. `FUN_0047b970` propagates the failure and does not mark the selected stream state active.

| Stage | Non-PCM format behavior |
| --- | --- |
| `FUN_00488f10` parser | Parses and retains extension metadata |
| `FUN_0047baa0` playback helper | Rejects tag values other than PCM (`1`) and releases the descriptor |
| DirectSound buffer creation | Not called for a rejected non-PCM stream |
| `FUN_0047b970` stream state | Returns failure; active marker is cleared |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The format gate and buffer-creation call are in `FUN_0047baa0` at `0x47baa0`; its caller's failure handling is in `FUN_0047b970` at `0x47b970`, both in `docs/decompiles/full/LEGORR.full.c`. The parser and helper are recorded in `docs/evidence/legorr-streamed-wav-format.bundle`: `ev_924b7b3de6c79a1a97b8e4deec48b1cf86361a7daecd5e411f6eece84233793d` (`FUN_00488f10`), `ev_7cc236e0d2189886aaec15f33fb0c43e84e0f9320de17e56d30b266a46aff417` (`FUN_0047baa0`), and `ev_6486d5a9df603739f3e911a3ddbecf8de816bbeef37a6d5935882e522703faf8` (`FUN_0047b970`).
