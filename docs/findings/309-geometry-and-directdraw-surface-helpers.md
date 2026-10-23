# Finding 309 — Geometry and DirectDraw Surface Helpers

Six routines now have direct operation-level names across matrix math, renderer configuration, and surface access.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x479af0 | LineIntersection2DCompute | Computes a 2D line intersection and returns null for a zero denominator. |
| 0x479ce0 | Matrix4Multiply | Computes each output cell as a four-term dot product of 4×4 matrices. |
| 0x478330 | Direct3DRenderPreferencesApply | Maps render-mode and option values to three device calls; exact device-state meanings are unresolved. |
| 0x47de60 | DirectDrawSurfaceDescriptionGet | Queries a surface descriptor and returns selected fields through output pointers. |
| 0x47def0 | DirectDrawSurfacePixelFormatGet | Queries a surface pixel format and extracts selected structure fields. |
| 0x47dfa0 | DirectDrawSurfacePixelRead | Bounds-checks coordinates, locks a surface, reads a pixel using pitch and bit depth, then unlocks. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct pseudocode evidence is retained in [legorr-engine-helpers-expansion-17.bundle](../evidence/legorr-engine-helpers-expansion-17.bundle) for the line-intersection helper, [legorr-engine-helpers-expansion-19.bundle](../evidence/legorr-engine-helpers-expansion-19.bundle) for render-preference setup, [legorr-engine-helpers-expansion-28.bundle](../evidence/legorr-engine-helpers-expansion-28.bundle) for matrix multiplication, and [legorr-engine-helpers-expansion-10.bundle](../evidence/legorr-engine-helpers-expansion-10.bundle) for pixel reading. Surface description and format helpers are supported by their direct bodies and surface-setup caller. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
