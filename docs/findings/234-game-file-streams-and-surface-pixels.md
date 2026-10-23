# Finding 234 — Game-File Streams and Surface Pixel Operations

Two helper groups clarify how the portable game's file wrapper reads text and
how its DirectDraw surface code converts colors and writes pixels.

## Game-file stream helpers

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x47f590 | WadMountPathCacheStore | Caches a nonempty path of at most 0x400 bytes and sets a validity flag; otherwise clears the cache. Its caller is the WAD mount startup routine. |
| 0x47fc20 | GameFileReadByte | Reads the next byte from either the WAD-backed cursor or the file stream, returning -1 at the WAD end or for an unsupported stream type. |
| 0x47fce0 | GameFileReadLine | Reads through the byte helper until NUL, newline, or 0xff; returns the buffer when at least one byte was read. The terminator is written one position after the last consumed byte. |
| 0x47fd40 | GameFileReadLineDispatch | Routes line reads through the WAD reader or the standard file-stream reader based on the handle's system type; unknown types emit a diagnostic and fail. |
| 0x47fdb0 | GameFilePrintFormat | Rejects formatted output to WAD streams, diagnoses unsupported handle types, and forwards standard file streams to the formatted writer. |
| 0x47fe20 | GameFileSystemTypeGet | Returns the system-kind field at the start of the game-file handle; callers use it to select WAD versus standard file operations. |
| 0x47ff50 | GameFileHandleRelease | Closes the underlying standard or WAD stream when present, then frees the wrapper allocation. |
| 0x47ffc0 | MountedPathPrefixRewrite | When the cached mount path is valid and matches the input prefix case-insensitively, formats the remaining suffix into a shared output buffer. |

## Surface pixel helpers

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x486090 | SurfaceFillPixelSpans | Locks the active DirectDraw surface, packs one color, and writes each requested horizontal span through the selected pixel callback. |
| 0x486210 | SurfaceFillPixelSpanRecords | Reads fixed-stride records containing color and span data, then fills each span through the same callback. |
| 0x486390 | SurfaceDrawCirclePixels | Samples sine/cosine around a full turn, clips the resulting points against stored surface bounds, and writes each visible point. |
| 0x4864d0 | SurfaceColorPackToPixel | Converts three floating color components to the surface's packed pixel value using the cached channel widths and shifts; the 8-bit path returns zero. |
| 0x486550 | SurfaceLockConfigurePixelWriter | Locks the surface, reads its pixel-format masks, calculates channel shifts, and selects a writer for the reported pixel depth. |
| 0x486650 | SurfaceUnlockResetPixelWriter | Unlocks the surface and clears the cached lock, format, bounds, and writer state. |
| 0x486690 | SurfacePixelWriterSelectByFormat | Selects the 8-, 16-, 24-, or 32-bit pixel-writing routine, with additional 16-bit choices based on the lock descriptor. |

**Evidence.** Direct decompiles are preserved in
[legorr-engine-helpers-expansion-9.bundle](../evidence/legorr-engine-helpers-expansion-9.bundle)
(records `ev_b658def7afa4372c8eccae16aee5cdf6bbc57c4135d9d5adeb95d5c85c57156c`
and `ev_ef8b3d3611bb75acfe4e212f4ab70f072531fb6c0e977b75d380861aeaccc8b5`).
The same bundle contains caller evidence for the file dispatch and surface
writer groups. Analyses used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
