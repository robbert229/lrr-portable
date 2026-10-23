# Finding 322 — Resource-Binding and Map-Row-Span Helpers

Three helpers initialize resource bindings and construct buffered row spans. The row-span routines preserve the observed byte-value gate without assigning a gameplay meaning to value 7.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x46c030 | RenderResourceBindingInitialize | Initializes a four-field record and tries resource lookups using the supplied values and fallback keys. |
| 0x46ead0 | AppendMapRowSpanToBuffer | Stores `(column, row)` float pairs for a half-open column range and advances the buffer count. |
| 0x46eb30 | StartMapRowSpanBufferRecord | Starts a buffer record, appends a row span, and increments the record count. |

Bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). A direct decompile record for `0x46c030` is retained in [legorr-rev.bundle](../evidence/legorr-rev.bundle), record `ev_4dbeaedaa2926743fb3abc8caae41c02f635288cc131e391c8d302b621abe95d`. No direct retained records were found for the row-span routines; caller context shows contiguous runs of cells with byte value 7 being written into the buffer. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
