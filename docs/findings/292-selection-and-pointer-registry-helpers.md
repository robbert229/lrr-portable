# Finding 292 — Selection and Pointer-Registry Helpers

Four anonymous helpers now have behavior-based names supported by their bodies and caller context where available.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x0042dfc0 | ObjectPointerRegistryClearMatch | Scans a global pointer table and zeroes every slot that equals the supplied pointer. Entity teardown calls it. |
| 0x0042dff0 | ObjectPointerRegistryGetSlot | Returns a nonzero entry from the same table when the requested index is in range. |
| 0x004293b0 | SelectionRectangleRender | When selection dragging is active, converts cursor coordinates and draws a four-vertex rectangle. No static caller was resolved. |
| 0x00424830 | SelectedObjectOutlinesDraw | Iterates selected objects and invokes the outline-render helper for each one. |

Direct bodies are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The two pointer-table helpers use `DAT_005563b8`; `0x0042dfc0` is called during entity teardown, and `0x0042dff0` is used by animation/effect routines. No matching direct REA bundle records were found for these functions. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
