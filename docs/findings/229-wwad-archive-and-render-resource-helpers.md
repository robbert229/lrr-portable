# Finding 229 — WWAD Archive and Render-Resource Helpers

Direct decompilation and caller tracing identified five omitted procedures in
the resource and rendering layers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x48b4e0 | WwadArchiveIndexLoad | Opens an archive in binary mode, verifies the four-byte `WWAD` signature, reads the entry count, and builds parallel filename tables plus per-entry records. The archive mount path calls it. |
| 0x483b70 | Direct3DDeviceStateApply | Caches requested device surfaces and applies six render-state arguments through Direct3D device vtable calls. The device-state caller is 0x483a90. |
| 0x480900 | CachedRenderResourceInstanceCreate | Copies a 13-dword template, allocates 0x70-byte per-entry records, runs two setup helpers per entry, creates and attaches a COM object, and updates reference/state fields. The exact resource class remains uncertain. |
| 0x477590 | FormattedTextEscapeDecodeAndEmit | Formats text, converts `\\n` escape pairs to newlines and underscores to spaces, then forwards the result to the text emitter. |
| 0x47d800 | Bitmap24BitSurfaceConvert | Reads three source bytes per pixel and packs the channels into 16-bit output pixels for the 24-bit surface conversion path. |

**Evidence.** Direct decompiles are in REA batch
`ev_09e309b4d7979e8a9b6298dbe1f72cd1c019e38604b283fa34cffb37c79add87` and the
[`legorr-engine-helpers-expansion-4.bundle`](../evidence/legorr-engine-helpers-expansion-4.bundle)
and [`high-range audit bundle`](../evidence/legorr-engine-helper-audit-high.bundle).
The archive signature bytes and mount caller, Direct3D device-state callers,
render-resource callers, and bitmap conversion chain are recorded in that
bundle. The source is shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
