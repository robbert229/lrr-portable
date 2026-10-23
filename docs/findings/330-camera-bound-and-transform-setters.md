# Finding 330 — Camera Bound and Transform Setters

Four camera helpers store tilt/distance bounds or apply bounded values. Caller configuration establishes the tilt and distance roles; the transform axis for the value at `+0x84` is left unspecified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x436831 | CameraSetTiltBounds | Enables a bound flag and stores two tilt limits at `+0x7c` / `+0x80`. |
| 0x436860 | CameraSetTilt | Accepts a tilt value if bounds are absent, override is enabled, or the value lies in range; stores it and updates the transform. |
| 0x436953 | CameraSetBoundedRotationValue | Checks limits at `+0x88` / `+0x8c`, stores the accepted value at `+0x84`, and applies a transform. |
| 0x436a18 | CameraSetDistanceBounds | Enables a bound flag and stores minimum/maximum distance values at `+0x94` / `+0x98`. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Camera setup reads `mintilt`, `maxtilt`, `mindist`, and `maxdist` before calling the bound setters. No matching direct retained analysis records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
