# Finding 351 — Text Output, Task Table, and Indexed Record Helpers

This batch names text-output operations, task-table cleanup, configured-list property access, attached-entity lookup, indexed text-record accessors, and a global buffer release.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0045a5b0` | `UITextOutputPositionAndDraw` | Adds two record offsets to the supplied coordinates, rounds them, then updates and draws the output object; text-display callers use it while placing text. |
| `0x0045a620` | `UITextOutputObjectRelease` | Passes the stored output-object handle to a release helper. |
| `0x00462960` | `TaskTableContentsRelease` | Releases each entry’s nested handle and entry, then the backing array and table object; returns 1. |
| `0x00465010` | `ConfiguredVariantPropertySet` | Selects an item from a configured list and applies it through a property setter; the selected value’s domain is unresolved. |
| `0x00465030` | `ConfiguredVariantPropertyRead` | Selects an item from a configured list and passes it to the property reader; the decompilation does not explicitly return that reader’s result, and the selected value’s domain is unresolved. |
| `0x0046db20` | `AttachedEntitySlotObjectGet` | Returns a cached object for the requested slot, resolving and caching it from a table when absent. |
| `0x00463ea0` | `IndexedTextMetricsPairGet` | Reads the two dwords at offsets 0x1c and 0x20 from an indexed record. |
| `0x00463ed0` | `IndexedTextMetricsPairSet` | Writes the two dwords at offsets 0x1c and 0x20 in an indexed record and sets offset 0x24 to 1. |
| `0x00466210` | `EntityFieldGridBufferRelease` | Frees the global buffer and clears its pointer; allocation and indexed use are adjacent helpers, but the field meaning is unresolved. |

Direct pseudocode evidence: `legorr-incident-message-rendering.bundle` (`ev_7dd7b8b071572b759c5ebe375bac5c7ad01f840c0a954825c88a5bb613896002`, `ev_b9a60d4ce479b00dc81d3fc9654084e14fd00a5c9b2bfc2c6f90bdb19e94072b`); `legorr-tasktable.bundle` (`ev_79b9ad4ab167080014aa93367c3bd28c6b5e74a192706ef1539dfead9bef949a`); `legorr-samples-config.bundle` (`ev_bc2b8a8e49a94ac680730d213bd47dcc4764aaf562d870403b987176dea65a7e`, `ev_956a43d52d9e7247c51225244e64f9ee0bfea3f3df7c247f37be85e3c0d72a00`). Remaining indexed and buffer helpers are supported by the checked-in pseudocode and callers.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field and interface meanings remain unspecified wherever callers do not establish them.
