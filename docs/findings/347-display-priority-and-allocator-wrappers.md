# Finding 347 — Display, Priority, and Allocator Wrappers

This pass names direct display-dimension accessors, priority-entry table operations, and engine allocator wrappers. The priority state field is left at operation level because its meaning is not established by the direct accesses. Direct pseudocode evidence includes `legorr-engine-helper-audit-terrain-expansion-9.bundle` (`ev_17e34a9ed0ceeae71d599e68fd6091b5d98cd7694c1ec5a1cfe78e1a833c6fe5`) and `legorr-engine-helpers-expansion-19.bundle` (`ev_154cfbe128729f166c7a35c2f2c6cdeaef26eb2ea90ddf7d020f8ba3d0e6ac06`) for display dimensions; `legorr-engine-helper-audit-low-expansion-6.bundle` (`ev_241204fda4fd3d3e2be46c657c9ea8002b63047c4c4d60cac2c9a88457218780`) supports priority ordering and allocator wrappers. Other direct behavior and caller context are in the complete pseudocode.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00401c30` | `DisplayWidthGet` | Returns the configured display-width global. The display setup routine writes the width, and callers use it for centering and bounds. |
| `0x00401c40` | `DisplayHeightGet` | Returns the configured display-height global. The display setup routine writes the height, and callers use it for centering and bounds. |
| `0x00402090` | `PriorityEntryOrderValueSet` | Stores the supplied value in the indexed priority-menu order table. |
| `0x004020a0` | `PriorityEntryStateSet` | Stores the supplied value in the indexed priority-entry state table; the precise field meaning is unresolved. |
| `0x004020b0` | `PriorityEntryStateGet` | Returns the indexed priority-entry state value; the precise field meaning is unresolved. |
| `0x004021e0` | `EngineMemoryAllocate` | Forwards the requested allocation size to the engine allocator. |
| `0x00402210` | `EngineMemoryFree` | Forwards the pointer to the engine free routine, which checks the custom allocator before falling back to HeapFree. |

The checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c) and the named excerpts are in [engine-region.c](../decompiles/engine-region.c). Names describe observed operations and offsets; unresolved record fields, enum meanings, and vtable slots remain intentionally unspecified.
