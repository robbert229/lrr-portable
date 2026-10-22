# 122 — In-memory WAV loading copies data through the `mmio` read window

## Observed

`FUN_0047b580` prepares an MMIO source through preceding helpers, obtains a payload size in `local_24`, allocates that many bytes with `GlobalAlloc`, and calls `FUN_00489100` with the MMIO handle, size, allocation pointer, a remaining-length structure, and an output count. On success, it stores the reported count in the sound record, closes the MMIO handle, and continues to the DirectSound buffer creation and fill helpers.

`FUN_00489100` calls `mmioGetInfo` to obtain an `_MMIOINFO` snapshot. It limits the copy request to the remaining length at offset `+4` of its fourth argument, subtracts that request from the remaining length, and copies bytes one at a time from `pchNext` to the supplied destination. When `pchNext == pchEndRead`, it calls `mmioAdvance`; if that still yields no readable byte, it reports `0xe103` and zero bytes. After a successful copy it calls `mmioSetInfo` and returns the copied byte count through its fifth argument.

`FUN_004890c0`, called earlier by the loader, seeks to the parent chunk's `dwDataOffset + 4`, sets the requested chunk ID to `data`, and calls `mmioDescend` on that parent. This ties the size/read preparation to the WAV data chunk.

## Inference

The MMIO read-window pointer is consumed internally, while the loader receives a copy in its allocated global-memory destination before DirectSound setup. The loader's upstream open path uses the shared disk/WAD access layer documented in finding 11, so this same handoff applies when that layer supplies a WAD-backed MMIO handle.

## Unknowns and limits

Static analysis only; the target and WAD files were not executed. `mmioGetInfo` exposes a read window and the helper copies from that window; this does not establish Windows' internal file-mapping strategy. The WAD-backed case depends on the existing file/WAD-layer evidence in finding 11. The precise origin and semantics of `local_24` are constrained by the `data`-chunk helper and caller use, but are not assigned a broader format meaning here. Failure cleanup and all alternate stream modes are not fully traced.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-wav-handoff-memory-copy.bundle`. Focused records: `ev_dc7c21b926f91ab25267c5b44752b55a18606e88e98c5e8c85f4e7304252d7e9` (`FUN_0047b580` allocation and call sequence); `ev_744fc3054b294dbc80700f32ab97f978f0085b0daee622302d47624863b7043c` (data-chunk seek and descend); `ev_3266acef2762c44d7601d3974a00fd7bfe52ecbb126b96b23ab8dbc0d542b7e6` (`mmioGetInfo` window copy, advance, and `mmioSetInfo`).
