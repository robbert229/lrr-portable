# Finding 216 — AVI Decoder Is Asked for the Best Display Format

The credits AVI path opens the video stream, then calls
`AVIStreamGetFrameOpen(stream, 1)`. Microsoft documents the second parameter as
`lpbiWanted`: it may be `NULL` for a default format or the special
`AVIGETFRAMEF_BESTDISPLAYFMT` value to ask for a format suited to the display.
The Windows multimedia header defines that special value as `1`, so the
binary's literal requests the best display format rather than supplying a
specific `BITMAPINFOHEADER`.

This delegates the decompressed frame format choice to AVIFIL32. The game's
subsequent copy helper has explicit 15-bit and 16-bit branches; it does not
write frame rows for other selected depths (finding 123). The call does not
identify which format a particular machine's decoder chooses, so the shipped
runtime display depth remains unresolved by this static analysis.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_0047e800` at `0x47e800` calls `AVIStreamGetFrameOpen` with literal `1`
after opening the `vids` stream; `FUN_0047e9e0` at `0x47e9e0` contains the
15-bit and 16-bit copy branches. Microsoft documents the [`AVIStreamGetFrameOpen` second parameter](https://learn.microsoft.com/en-us/windows/win32/api/vfw/nf-vfw-avistreamgetframeopen) and the best-display-format option. The functions are in
`docs/decompiles/full/LEGORR.full.c`; finding 123 records the row-copy path.
