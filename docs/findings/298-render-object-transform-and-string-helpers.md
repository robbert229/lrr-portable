# Finding 298 — Render-Object Transform and String Helpers

Three routines have direct render-interface behavior supported by caller context.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x475580 | RenderObjectRotateAxis | Dispatches an axis rotation through a render-object vtable method. A caller passes basis vectors and angles of `±π/2`. |
| 0x475610 | RenderObjectSetIdentityTransform | Builds a 4×4 identity matrix and applies it through the render-object transform interface. |
| 0x476270 | SceneObjectSetFormattedString | Formats a string into a local buffer, allocates an exact-size copy, assigns it through the scene-object interface, and registers the string. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). A direct record for `0x476270` is in [legorr-ddscene.bundle](../evidence/legorr-ddscene.bundle), record `ev_44bce682fa4942b2aeed979dd9bfe9f5f96a786487df3988bd87f1c0c2ec9166`; callers use the formatted label `ActFrame` before scene-object creation. The rotation and identity-transform names rely on body and caller context in the checked-in corpus. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
