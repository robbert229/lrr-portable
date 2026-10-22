# Finding 123 — AVI frame helper copies bottom-up DIB rows into a pixel buffer

*Target: shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.*
*Provider: Ghidra 12.1.4 via REA. Evidence:
`docs/evidence/legorr-avi-frame-row-copy.bundle`.*

The AVI player's frame routine passes a non-null `AVIStreamGetFrame` result to
`FUN_0047e9e0`. That helper copies rows from the returned frame block into a
buffer referenced by the AVI object's surface state. Its address arithmetic
uses the DIB header-size word, the surface pitch, and the configured rectangle
dimensions. This narrows the “frame decode to display” path in finding 14: the
application copies already-decoded frame bytes into its pixel buffer; the
codec/decompressor is behind AVIFIL32 and is not shown here.

## Observed behavior

`FUN_0047eb20` calls `AVIStreamGetFrame` with the stream and stored frame
index, checks for a null result, and on a non-null result calls
`FUN_0047e9e0(result)` (`ev_40fec63af89b63e454a3a18680f961ff2eda2c702d4f94e0e20c05957b32aba3`).

In `FUN_0047e9e0`, the destination pointer is loaded from the surface-state
record at `+0x40`. The helper derives the destination row pitch from that
record's `+0x2c` field, and the byte count per row from the AVI object's
`+0xa4` minus `+0x9c` fields. It initializes the source row at
`frame_pointer + *(uint32_t *)frame_pointer + (height - 1) * pitch`, where
the first term is the DIB header-size field. For each row it copies the row
byte count, advances the destination by one pitch, and moves the source back
by one pitch. The two accepted branches resolve to 15- or 16-bit mode and use
the same byte-copy loop (`ev_57b420ba3a40680ff524c8048f07836b5733300cdff5473b18a0be5a2c47dc4d`).

Before the copy, a `FUN_0047e760` call is made only when the surface-state
flag at `+8` is clear and that helper returns nonzero. The paired
`FUN_0047e7b0` call occurs after the copy only when that condition was met.
This conditional bracketing is observed; its exact API/lock semantics are not
established by these records.

## Interpretation and limits

The source traversal from the last row toward the first, while the destination
advances forward, is consistent with converting bottom-up DIB row order into
top-down destination row order. That is an inference from the pointer
arithmetic. Static evidence does not establish actual codec output, palette
handling, whether every frame uses this format, the role of the surrounding
surface callbacks, or the later presentation/scan-out path. The helper's
identical byte-copy loops do not perform a visible 15-to-16-bit color
conversion.
