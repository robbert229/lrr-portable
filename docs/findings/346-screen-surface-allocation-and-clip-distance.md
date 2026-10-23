# Finding 346 — Screen-Surface Allocation and Clip Distance

Two renderer wrappers allocate a surface within display bounds and forward a configured clip-distance value. The allocation node fields and vtable slot semantics remain unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x476e50 | ScreenBoundedSurfaceResourceAllocate | Checks requested extents against display dimensions, obtains a `0x20`-byte free-list node, invokes a renderer interface with resource/position/extent values, and links the node on success. |
| 0x476fb0 | RendererObjectSetClipDistance | Dispatches a scalar through the object at `+0x10` using vtable offset `+0x3c`; callers pass the configured `tvclipdist` value. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct pseudocode evidence for `0x476e50` is in `legorr-engine-helpers-expansion-23.bundle`, record `ev_f566111ade0b7a764f73c8d4fee8476cf2dd41c90333b5e9913307a009d3df7d`; for the clip-distance wrapper it is in `legorr-engine-helper-audit-render-expansion-9.bundle`, record `ev_77d33b453abb5e0818a3c20d7632b32618254e6b6e550b92fed8257ce6b6b323`. Caller configuration supports the clip-distance name, though the vtable operation is not independently resolved. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
