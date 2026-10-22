# Finding 130 — `+0x3f4` has no other direct field reference in the shipped image

The activity-list appender `FUN_0044b370` writes zero to entity `+0x3f4` (see the existing activity-capacity evidence). A complete initialized-memory byte search for the x86 little-endian displacement `f4 03 00 00` found four matches in `.text`:

- `0x44b389`, inside `FUN_0044b370`: the appender's known store to `+0x3f4`.
- `0x464233` and `0x464249`, inside `FUN_004641c0`: both decode as `MOV` reads from `[ESP + 0x3f4]`, with stack references (`stack:0x10` and `stack:0xc`), not entity-relative reads.
- `0x487c78`, inside `FUN_00487ba0`: this byte begins one byte into the `CALL` at `0x487c77` (`e8 f4 03 00 00`, calling `0x488070`); it is the relative call displacement, not an instruction or field access.

Thus the binary scan found no other direct instruction displacement of `+0x3f4` that reads an entity field. This is a bounded negative census of the literal displacement in initialized image memory; it does not exclude code that derives an equivalent address through pointer arithmetic or a helper. The field's meaning remains unknown.

Evidence: [canonical REA bundle](../evidence/legorr-activity-3f4-reader-census.bundle). The bundle includes the previous activity-capacity evidence, the complete displacement search (`ev_fab1583e…54516c4`), and instruction-level decodes for each non-writer match.
