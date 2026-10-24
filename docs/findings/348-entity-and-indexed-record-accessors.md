# Finding 348 — Entity and Indexed-Record Accessors

This pass names entity state transitions, an unresolved selector-dependent helper, a list insertion helper, and indexed or nested record accessors. Offset-based names avoid assigning semantic meanings to fields. Direct pseudocode evidence includes `legorr-energy-low-target-routing.bundle` (`ev_63bc1dc1e07eef2799ed123860bfd5aa5e4e55258112a29f6e10265d22117c43` for `0x0043c280`; `ev_a2a614141f5621fecf6eaa2c3282ccd06c1116028b6cc19dbef96d88a6d4210b` for indexed accessors) and `legorr-deferred-simulation-pass-2-2026-10-23.bundle` (`ev_6ce3a0b8d4a0d4a195132f950746ba82110d7c4958527287ae2862e808d2f17c` for the state-1A helpers). The camera/list helpers have direct checked-in pseudocode and caller context in `LEGORR.full.c`; record accessors are also used by entity simulation and task routines.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0043c280` | `EntityNestedFloatAddUnlessBlocked` | For type 4, returns early when the eligibility helper is false; otherwise adds a nested float value to the caller-provided float and returns zero. |
| `0x0043c4c0` | `EntityFinishState1A` | Conditionally derives and checks a value, may invoke a follow-up routine, clears mask 0x04000000 at offset 0x3e4, and submits action 0x1a. |
| `0x0043c530` | `EntityMarkAndFinishState1A` | For a non-null type-2 object whose referenced record has type 0x1a, sets mask 0x04000000 and calls the state-1A completion routine. |
| `0x0046a420` | Existing provisional label `SelectedGlobalPointTransformAndReturnX` | Selects a camera/context global based on `DAT_00556ec8`, then the decompile shows calls to `0x00431570` and `0x00436b33` before returning a local stack word. Those apparent arguments conflict with the callees' signatures (`CurrentLevelHeightGet` takes no arguments; `CameraGetPositionWithTerrainHeight` takes three), so the actual value and transform flow remain unresolved. |
| `0x0046a470` | `LinkedListPushAtGlobalHead` | Links the supplied record to the current global list head and makes it the new head. |
| `0x00469a90` | `IndexedValueAppend` | Appends a value to the per-object sequence and increments that sequence count. |
| `0x00469ac0` | `IndexedRecordWordAtOffsetB0Get` | Returns a 32-bit field at offset 0xb0 in the indexed 0x150-byte record. |
| `0x00469af0` | `IndexedRecordWordAtOffsetB4Get` | Returns a 32-bit field at offset 0xb4 in the indexed 0x150-byte record. |
| `0x00469b20` | `IndexedRecordWordAtOffsetB8Get` | Returns a 32-bit field at offset 0xb8 in the indexed 0x150-byte record. |
| `0x00469b50` | `EntityIndexedRecordWordAtOffsetBCGet` | Uses indices stored in the entity and returns the 32-bit indexed-record field at offset 0xbc. |
| `0x00469b80` | `EntityIndexedRecordWordAtOffsetE0Get` | Uses indices stored in the entity and returns the 32-bit indexed-record field at offset 0xe0. |
| `0x00469be0` | `EntityIndexedRecordWordAtOffsetF0Get` | Uses indices stored in the entity and returns the 32-bit indexed-record field at offset 0xf0. |
| `0x00469d00` | `NestedFloatAtOffset9CGet` | Returns the nested float at offset 0x9c. |
| `0x00469d40` | `NestedFloatAtOffset6CGet` | Returns the nested float at offset 0x6c. |
| `0x00469d50` | `NestedFloatAtOffset74Get` | Returns the nested float at offset 0x74. |
| `0x00469d60` | `NestedFloatAtOffset78Get` | Returns the nested float at offset 0x78. |
| `0x00469d70` | `NestedFloatAtOffsetE8Get` | Returns the nested float at offset 0xe8. |
| `0x00469d90` | `NestedFloatAtOffset70Get` | Returns the nested float at offset 0x70. |
| `0x00469da0` | `NestedFieldAddressAtOffsetA0Get` | Returns the address of the nested field at offset 0xa0. |
| `0x00469db0` | `NestedFloatAtOffset7CGet` | Returns the nested float at offset 0x7c. |
| `0x00469dc0` | `NestedFloatAtOffset80Get` | Returns the nested float at offset 0x80. |
| `0x00469e00` | `IndexedRecordFloatAtOffsetDCGet` | Returns the float at offset 0xdc in the indexed record. |
| `0x00469e20` | `NestedFloatAtOffsetDCGet` | Returns the nested float at offset 0xdc. |
| `0x00469e40` | `NestedWordAtOffsetACGet` | Returns the nested 32-bit value at offset 0xac. |
| `0x00469ec0` | `IndexedRecordWordAtOffset144Get` | Returns the 32-bit field at offset 0x144 in the indexed record. |
| `0x00469ee0` | `IndexedRecordWordAtOffset148Get` | Returns the 32-bit field at offset 0x148 in the indexed record. |
| `0x00469f00` | `IndexedRecordWordAtOffset14CGet` | Returns the 32-bit field at offset 0x14c in the indexed record. |
| `0x00469f60` | `IndexedRecordWordAtOffsetD0Get` | Returns the 32-bit field at offset 0xd0 in the indexed record. |
| `0x00469f90` | `NestedWordAtOffsetC0OrC4Get` | Returns nested offset 0xc0 when the selector is zero, otherwise nested offset 0xc4. |
| `0x00469fe0` | `NestedFloatAtOffset8CGet` | Returns the nested float at offset 0x8c. |

The checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c) and the named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Names describe observed operations and offsets; unresolved record fields, enum meanings, and vtable slots remain intentionally unspecified.
