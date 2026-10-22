# 96 — Whole-file loads close the stream and return a separate heap buffer

*Target: shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Provider: Ghidra 12.1.4 via REA. Evidence: `docs/evidence/legorr-config-and-read-buffer-lifecycle.bundle`.*

`FUN_004800c0` is a whole-file adapter above the generic file layer. It opens a stream with `FUN_0047f6e0`, seeks to end and obtains the length via `FUN_0047fb10`, then invokes the optional `DAT_005343ac` callback with `(path, length, DAT_005343b0)` before allocating the output buffer. On successful allocation it rewinds, requests `length` one-byte items into that allocation through `FUN_0047f980`, writes the original length through the optional size pointer, closes the stream with `FUN_0047fac0`, and returns the buffer pointer. The read count is ignored, so this evidence does not show a short-read check. If allocation fails, it still closes the stream and returns null. The two-argument `FUN_004800a0` wrapper always passes mode `1` to `FUN_004800c0` (`ev_8a576e71604114cae565a89bc4bb0349e7a02fa9b18129e9b65f1c2aab8d6503`, `ev_0a6fa5a9ddf4c5d9293e06b2e29a74fee58bf9ef6b6bfb016f7e76d1b711f8ce`).

This separates stream lifetime from parsed-data lifetime: for example, `FUN_00478e60` receives the returned buffer and tokenizes it in place, while the stream has already been closed by the adapter (the parser's resolved callees are `FUN_00479270` and `FUN_004800a0`, `ev_74621f2e5b01fce10d546938c43a2d54063669b36ce29f0d1027afb7f71ba6ee`). The callback is observed before allocation and reading; its purpose and side effects are not established here.

## Boundary

The result establishes static call order and ownership operations, not runtime callback behavior or allocation-failure behavior beyond the observed null return/close branch. Finding 64 covers the lower-level WAD stream handle; this finding covers the whole-file wrapper used by parsers and asset loaders.
