# Finding 288 — Render Object and Mesh Helpers

Four render helpers now have operation-level names based on their direct bodies and caller usage. The object flag and texture selector values are kept explicit or neutral because their broader meanings are unknown.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x470630 | EntityDirectedLineSideCheck | Gets an entity's XY position and returns whether its signed dot product against a directed line is negative. It is used by segment-intersection and proximity helpers. |
| 0x473b20 | EntityFlag80Set | Sets or clears bit `0x80` at object offset `+0x14`; callers resolve an object and forward a boolean. |
| 0x474a60 | SceneSurfaceTextureBindByMode | Maps a selector to binding flags, resolves a surface/texture pair, and invokes a render-interface method. Mesh callers pass selector `3` after record creation. |
| 0x475020 | SceneMeshColorAndAlphaSet | Sends RGB values to a color helper and alpha to an opacity helper in mode `4`. Callers pass mesh-record color and opacity values. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x470630` is used by `0x4702b0` and `0x470540`; `0x473b20` is called by `0x449cd0`; `0x474a60` is called from mesh construction in `0x44e0c0` and `0x463f00`; and `0x475020` is used by mesh color/opacity setup in `0x44f090` and `0x44f5c0`. No retained direct procedure-analysis records were found for these exact targets. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
