# Finding 350 — Low-Address String and Global-Field Helpers

This batch names a case-insensitive string-table search and guarded updates to global-object fields. Field offsets are retained in names because their meanings are unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0041a9c0` | `CaseInsensitiveStringTableIndexFind` | Scans a fixed string table with case-insensitive comparison, writes the zero-based matching index to the output pointer, and returns whether a match was found. |
| `0x0041f870` | `GlobalObjectField8CIncrement` | If the global object exists, increments its dword at offset 0x8c. |
| `0x0041f880` | `GlobalObjectField8CSubtract` | If the global object exists, subtracts the argument from its dword at offset 0x8c. |
| `0x0041f8a0` | `GlobalObjectField98Add` | If the global object exists, adds the argument to its dword at offset 0x98. |
| `0x0041f8c0` | `GlobalObjectField98Clear` | If the global object exists, clears its dword at offset 0x98. |
| `0x0041f980` | `GlobalObjectField94Increment` | If the global object exists, increments its dword at offset 0x94. |
| `0x0041f990` | `GlobalObjectFieldA8Increment` | If the global object exists, increments its dword at offset 0xa8. |

No matching direct retained evidence records were found for these entries. The lookup and guarded field operations are explicit in the complete pseudocode; caller context includes string-table initialization/use and the global-field update sites.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Field and interface meanings remain unspecified wherever callers do not establish them.
