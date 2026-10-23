# Finding 218 — Streamed WAV Metadata Probe Bypasses the Game File Layer

The streamed branch of `FUN_0047a870` uses `FUN_0048eb90` to open a sample
through the game's file layer and obtain its size. It closes that handle, then
passes the resolved path string to `FUN_00488e70`. That helper opens the path
again with `mmioOpenA(path, NULL, 0)`: it receives neither the game's open
handle nor a custom MMIO I/O procedure. Microsoft documents a non-null file
name with default MMIO I/O as a standard file open.

The size check and WAV-format probe therefore use different access paths. If a
sample is available only through the game's WAD layer and has no corresponding
standard file path, the MMIO probe cannot use the already-open WAD handle. Its
zero result is still accepted by the streamed registration path (finding
217). This narrows the WAD-backed uncertainty in finding 14, but does not
establish whether the shipped files also exist loose or what a particular
installation exposes to WinMM at runtime.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_0047a870` at `0x47a870` obtains file size through `FUN_0048eb90`, closes
that handle, and passes the path to `FUN_00488e70`. The latter calls
`mmioOpenA(path, NULL, 0)` at `0x488e70`. Finding 64 documents the game's
WAD-aware file-handle path; finding 217 documents the unchecked metadata
result. Microsoft describes the default file-name behavior in the [`mmioOpen` documentation](https://learn.microsoft.com/en-us/previous-versions/ms712181%28v%3Dvs.85%29).
