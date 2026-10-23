# Finding 334 — Camera Bounds and Global Refresh

Four camera helpers store tilt/distance limits or apply a bounded camera value. A fifth helper clears a global byte and refreshes related state.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x436831 | CameraSetTiltBounds | Enables a bound flag and stores limits at `+0x7c` / `+0x80`. |
| 0x436860 | CameraSetTilt | Accepts a value when bounds are absent, override is enabled, or it falls inside the limits; then updates the transform. |
| 0x436953 | CameraSetBoundedRotationValue | Checks bounds at `+0x88` / `+0x8c`, stores the accepted value at `+0x84`, and updates a transform. |
| 0x436a18 | CameraSetDistanceBounds | Enables a bound flag and stores limits at `+0x94` / `+0x98`. |
| 0x435a20 | GlobalStateLowByteClearAndRefresh | Clears the low byte of `DAT_00556ed4` and calls a refresh helper. |

Camera setup loads `mintilt`, `maxtilt`, `mindist`, and `maxdist` before invoking the bound setters. Bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The tilt units and the global refresh state’s semantic role are not independently established. Direct evidence for the low-byte reset is in `legorr-engine-helper-audit-mid-expansion-6.bundle`, record `ev_274dad58b224a5be9b0984bef6a4873ee447b8bddad3001b0253233ebd3a8e98`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
