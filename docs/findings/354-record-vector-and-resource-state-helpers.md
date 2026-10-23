# Finding 354 — Record, Vector, and Resource State Helpers

This pass names record-field accessors, numeric helpers, linked-record updates, indexed pairs, callback state, and resource-state operations. Offset and code names remain literal when their domain is unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00435ef4` | `RecordFields4And14Set` | Writes two supplied values to record offsets +4 and +0x14; record type and field meanings are unresolved. |
| `0x00435f0b` | `RecordField14Get` | Returns the dword at record offset +0x14; record type and field meaning are unresolved. |
| `0x004422a0` | `SurfaceVectorNormalize` | Reads a two-component value through helper calls, normalizes the pair, and writes it back; the vector domain is unresolved. |
| `0x0044d250` | `FloatNegateAbsoluteValue` | Replaces the supplied float with its negative absolute value. |
| `0x0044e6b0` | `IndexedFlagValueSetOne` | Stores 1 at the indexed table slot computed from the low byte of the index. |
| `0x00451c30` | `LinkedRecordField4Clear` | Walks a linked list through offset +0x14 and clears each node’s dword at offset +4. |
| `0x004532e0` | `IndirectHandleResolveInPlace` | For a value carrying mask 0x20000, resolves a missing cached table value through a function pointer and writes the result back; tag and table semantics are unresolved. |
| `0x00453900` | `DeferredCallbackSet` | Stores the supplied value in the global later checked by the companion invoke helper. |
| `0x00453910` | `DeferredCallbackInvokeIfSet` | When the stored global is nonzero, invokes the fixed callback helper. |
| `0x0045a6e0` | `IndexedPairSet` | Stores two supplied values in indexed global slots with stride 0x30; the pair meaning is unresolved. |
| `0x0045a700` | `IndexedPairGet` | Reads two values from the same indexed global slots; the pair meaning is unresolved. |
| `0x0045b900` | `IntegerPairDivideToFloatPair` | Writes two integer inputs divided by a shared divisor as float outputs. |
| `0x00462ab0` | `RecordField18Set` | Writes a supplied value at record offset +0x18; record type and field meaning are unresolved. |
| `0x00462ac0` | `RecordFlags24Or` | ORs supplied bits into the dword at record offset +0x24; flag meanings are unresolved. |
| `0x00463730` | `IndexedFloatPairSet` | Converts two integer inputs to floats and stores them in an indexed pair of global slots. |
| `0x00465ac0` | `ResourceEnableSet` | Calls a helper on the resource at offset +0x290 and sets or clears bit 0 at +0x294 from the supplied boolean. |
| `0x0046c0f0` | `RecordResourceRelease` | Releases the resource pointer stored at record offset +4; record type and ownership semantics are unresolved. |
| `0x0046ced0` | `PairedResourceStateSet` | Calls the same state helper for one resource/object pair and, when present, a second pair. |
| `0x0046cfc0` | `SixResourceSlotsSetState` | Iterates six resource slots beginning at offset +0x10 and forwards the supplied state to each nonzero slot. |

These entries are directly visible in the checked-in pseudocode and caller context. No matching direct retained evidence records were found for this candidate set.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Names preserve unresolved command codes, fields, flags, and interface semantics.
