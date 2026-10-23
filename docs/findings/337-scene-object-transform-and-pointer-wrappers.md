# Finding 337 — Scene-Object Transform and Pointer Wrappers

Four wrappers invoke scene-object operations for coordinate transforms, matrix assignment, and pointer access. The exact coordinate point/vector convention and matrix mode values are not established.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x475070 | SceneObjectCoordinateTransformInPlace | Dispatches through scene-object vtable offset `+0x108` with the supplied coordinate buffers. |
| 0x475090 | SceneObjectCoordinateTransformToOutput | Dispatches through vtable offset `+0x84` with input and output buffers. |
| 0x4756b0 | SceneObjectTransformMatrixSet | Dispatches a matrix and mode value through vtable offset `+0x38`. |
| 0x473680 | SceneObjectWrappedPointerGet | Returns the pointer stored at the start of the supplied wrapper. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). A direct procedure record for `0x473680` is retained in `legorr-ddscene-hooks.bundle`, record `ev_422712ac0f8653440414d65904f99e5d4eb4bbd8d20db282868a00560eff2188`. The other wrappers are supported by pseudocode and caller context. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
