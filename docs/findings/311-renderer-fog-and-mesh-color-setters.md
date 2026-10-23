# Finding 311 — Renderer Fog and Mesh Color Setters

Four renderer helpers now have direct behavior-based names. Fog state and mesh color fields are described at the interface level without assigning undocumented meanings.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x473dc0 | RendererFogEnableAndColorApply | Toggles renderer fog state and applies the packed fog color when enabled, otherwise applies zero. |
| 0x473e70 | RendererFogModeSet | Forwards a fog mode and secondary renderer state through interface calls. |
| 0x473ea0 | RendererFogParametersSet | Forwards three fog parameters to the renderer. |
| 0x474ff0 | SceneMeshColorSet | Sends indexed mesh RGB values through the lower-level color setter. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Fog setters are called from per-frame fog handling and level fog configuration; mesh color setup is called with RGB values during render-object setup and state changes. No matching direct-analysis bundle records were found for these exact routines. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
