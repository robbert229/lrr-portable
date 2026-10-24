# Finding 237 — Buffered Streams and Win32 File Helpers

The file layer includes a buffered stream reader and a small Win32 compatibility
surface for directory enumeration, drive paths, attributes, and error codes.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x48fe50 | BufferedStreamReadLine | Reads at most the supplied capacity minus one byte, refilling the stream as needed; stops at LF or EOF and NUL-terminates the result. It returns null when EOF arrives before any byte. |
| 0x48fe20 | BufferedStreamGetByte | Consumes the next buffered byte, or refills the stream when the buffered count is exhausted. The game-file byte reader and archive parser call it. |
| 0x48f830 | FileFindNextRecord | Wraps `FindNextFileA`, translates end/error results into the engine's error state, and copies file attributes, timestamps, size, name, and other metadata into the caller's record. |
| 0x48f680 | FileSetReadOnlyAttribute | Reads file attributes, then sets or clears the Win32 read-only bit according to the mode byte; maps failures through the engine error mapper. |
| 0x48fa20 | DriveCurrentDirectoryGet | Gets the process current directory for drive zero, or validates a requested drive and resolves its `X:.` path; copies into a caller buffer or allocates one. |
| 0x48ffa0 | PathResolveFull | Wraps `GetFullPathNameA`, allocating a standard-size buffer when none is supplied and using the current-directory helper for an empty path. |
| 0x496ee0 | Win32ErrorMapToErrno | Stores the raw Win32 error in `DAT_005490d4`, maps known values through a table to the CRT `errno` global `DAT_005490d0`, and applies fallback mappings for other values. |

`DAT_005490d0` is the CRT `errno` storage: CRT math handlers write `0x21`
(`EDOM`) and `0x22` (`ERANGE`), while file-descriptor failures write `9`
(`EBADF`). `DAT_005490d4` holds the corresponding raw Win32 error code; the
error mapper sets it from its argument, and callers also clear it on paths that
reset the error state. These are process-global data addresses in the PE image;
the decompile does not establish thread-local behavior.

**Evidence.** Direct decompiles and caller records are in
[legorr-engine-helper-audit-text-io-expansion-9.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle).
The key direct observations are `ev_a9b1d831b0ddcc59d4591a8274dbaa694c3eaa1ca2057aa4d145502e05f66613`,
`ev_8688f97f74e41e6b24df6574e6d58ff1bbba7de05d148ffb35389f6e71190a8b`, and
`ev_497c4a9f4323ac99889e515be2e663134d51e6ffca523b957820d2e707d8f1d1`.
Caller evidence links stream operations to the game-file reader and archive
parser, and file operations to WAD startup and path scanning. Analyses used
shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
