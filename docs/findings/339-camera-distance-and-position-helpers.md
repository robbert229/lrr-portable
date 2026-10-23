# Finding 339 — Camera Distance and Position Helpers

Four helpers set a bounded camera distance, set limits for a scalar camera value, read the stored rotation-like value, or return model position with terrain height in its z component.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x436a4a | CameraSetDistance | Accepts a distance when bounds are absent, override is enabled, or the value lies within `+0x94` / `+0x98`, then stores it at `+0x90`. |
| 0x436921 | CameraSetScalarBounds | Enables a bound flag and stores limits at `+0x88` / `+0x8c`; the axis semantics are unresolved. |
| 0x436b33 | CameraGetPositionWithTerrainHeight | For object type 1, reads model position and replaces output z with the terrain height at its x/y. |
| 0x436b79 | CameraRotationGet | Returns the float at camera offset `+0x84`, which the paired setter bounds and applies through a transform. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Camera setup and distance controls provide caller context. No matching direct retained procedure records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
