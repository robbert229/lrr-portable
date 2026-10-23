# Finding 364 — Bit Count, Cursor, and Directory Helpers

This pass names a second set-bit counter, a client-relative cursor setter, and a helper that creates a supplied subdirectory under the configured directory prefix.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0047e7e0` | `BitCount32ByPosition` | Counts set bits by checking all 32 bit positions. |
| `0x0047f0d0` | `CursorSetClientPosition` | Converts a client-relative coordinate using the window origin, stores the input pair in globals, and calls SetCursorPos. |
| `0x0047f6a0` | `SaveDirectoryEnsureExists` | Formats a path from the configured directory prefix and supplied directory name, then calls the directory-creation helper. |

Direct pseudocode evidence: `legorr-engine-helper-audit-text-io-expansion-9.bundle` (`ev_d75b809b7beed400983aea831c39d7440674032f2c622d9ed5be41c89bb26690`) for the bit count and cursor setter; `legorr-engine-helpers-expansion-9.bundle` (`ev_ef8b3d3611bb75acfe4e212f4ab70f072531fb6c0e977b75d380861aeaccc8b5`) and the text-I/O bundle above for the directory helper.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field, flag, and callback semantics remain at the operation level where callers do not establish more.
