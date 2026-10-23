# Finding 358 — Matrix, Window, Surface, and Heap Helpers

This pass names matrix initialization/translation, nested-list removal, window state initialization, surface format query, global list insertion, indexed table access, and heap wrappers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00479e40` | `MatrixIdentityAndTranslationSet` | Resets the matrix through the identity helper, then stores three supplied values at offsets +0x30, +0x34, and +0x38. |
| `0x0047b330` | `NestedRecordListRemoveMatchingKey` | Recursively searches a nested record list, unlinks and frees a matching nested node, and reports whether the current key matches. |
| `0x0047c170` | `WindowStateInitialize` | Stores the supplied window handle in a global and clears the associated state fields. |
| `0x0047d490` | `SurfacePixelFormatQuery` | Initializes a surface descriptor, calls an interface method, branches on 8- or 24-bit format, and returns success. |
| `0x004792c0` | `GlobalRecordListPushHead` | Links a node to the current global list head and makes it the new head. |
| `0x0047b0d0` | `IndexedRecordStride134WordGet` | Returns the dword at the indexed table slot with stride 0x134. |
| `0x0047d410` | `RendererRecordTableInitialize` | Clears 32 dwords in a global table and sets a separate global initialization flag. |
| `0x0047ff30` | `EngineHeapAllocate` | Forwards the requested size to the engine heap allocator. |
| `0x0047ff40` | `EngineHeapFree` | Forwards the supplied pointer to the engine heap free routine. |

Direct pseudocode evidence: `legorr-sound-instance-spatial-expansion.bundle` (`ev_59d36a91690ba76fab045f2ae62dc4ce8113c8b9998df7788490bb5af99e75be`) for nested-list removal; `legorr-engine-helper-audit-high.bundle` (`ev_9705cb50413dcd983639a8545d897b2bc764e10b549cda7ea5fcc448041cb189`) for the surface query. Caller evidence for the list helper is `ev_d509d0d3966f5705a893f38f92ab83aea49bca958c6a68f826bf22da206a2c16`; surface caller evidence is `ev_54e5e84a8f432f41230b0ec0cbd29f361cd771ac999f19cec5a25e8468c6eb68`. Other listed helpers are supported by complete pseudocode and callers; no matching direct records were found.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Names retain operation-level or offset-based descriptions where semantic fields and interface details are unresolved.
