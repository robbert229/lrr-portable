# Finding 368 — Record Pool and Recursive List Helpers

This pass names fixed-size record acquisition, recursive nested-list removal, and two-interface cleanup. The pool node and interface types remain unspecified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0047a580` | `FixedSizeRecordPoolAcquire` | Pops a record from the free list, grows the pool if empty, clears the record, self-links its +0xbf0 field, stores the argument at offset 0, and returns the record. |
| `0x0047b2e0` | `NestedRecordListRemoveByKeyRecursive` | Recursively searches child links, unlinks and frees a matching child, and reports whether the current record key at +8 matches. |
| `0x0047e480` | `TwoInterfaceFieldsReleaseAndReset` | Sets the object vtable, releases non-null interface fields at +0x18 and +0x10, and clears both fields. |

Caller evidence for `0x0047a580` is in `legorr-engine-helpers-expansion-9.bundle` (`ev_c4bd0c5ddfd43126e0c837724a68a4f92e78bb3d26d6ad6387bd8beed30e6e76`). Direct pseudocode evidence for `0x0047e480` is in `legorr-ui-render-fifth-pass-2026-10-23.bundle` (`ev_7c2411b4a9c6ae3069ebad3222d390a97cabb2cfcebe479529bf13d54ea0a553`); caller evidence is `legorr-engine-helper-audit-text-io-expansion-9.bundle` (`ev_01457480c8d70a5b557d05d849685bb3f792b45879a3af4519a3b6a512323755`). Recursive list behavior is visible in complete pseudocode and callers.

The complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Numeric task kinds and unresolved field semantics remain literal.
