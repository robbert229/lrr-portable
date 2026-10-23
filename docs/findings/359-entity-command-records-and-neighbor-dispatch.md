# Finding 359 — Entity Command Records and Neighbor Dispatch

This pass names two entity command-record operations, a cardinal-neighbor dispatch, and a conservative record-array state operation. Numeric record kinds, command codes, and flag meanings are retained literally.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00402800` | `EntityCommand14BatchIssue` | For each requested entry, allocates a type-4 record, stores command code 0x14, conditionally sets flag 0x40 for an initially empty batch, and links the record. |
| `0x00402670` | `EntityCommand15ValidatedRecordQueue` | Validates a nonzero input, allocates a type-0x1a record, stores command code 0xf and the input pointer, then updates record flags. |
| `0x00409cd0` | `CellCardinalNeighborApply` | Builds the four cardinal coordinate offsets and dispatches the neighbor helper for each resulting cell. |
| `0x00402600` | `EntityRecordArrayApplyZeroState` | When its gate passes, sets bits 0x140 on the entity, retrieves a record array, and invokes two helpers with zero for each item; the record and flag semantics remain unresolved. |

Direct pseudocode evidence: `legorr-engine-helper-audit-low-expansion-6.bundle` (`ev_241204fda4fd3d3e2be46c657c9ea8002b63047c4c4d60cac2c9a88457218780` for `0x00402800` and `0x00402670`; `ev_55fe4111c4cce6fc5afcbf91a3151f03e00ccb3870f5317c9484e1d0d4e06334` for `0x00409cd0`; `ev_a4ff1b2c9b323120bb7411b8ae74cfbea332874a35f739b1a313a3ecc581093f` for a caller of `0x00402600`). Caller evidence for `0x00402800` and `0x00402670` is also in that bundle (`ev_c9f54966cb72a4f75bc84ee8862cf102bec3a02d18123f590fd4ce7318ad5881`, `ev_0e07d56b50a5d357e14f64bb6add174ed84a795fec148da6019e06ea39dcaa46`).

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Field and interface meanings remain unspecified where callers do not establish them.
