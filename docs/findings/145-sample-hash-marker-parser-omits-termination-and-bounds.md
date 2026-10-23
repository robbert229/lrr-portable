# Finding 145 — The `#...#` Marker Parser Has No Terminator or Length Check

In `FUN_00464d00`, the bytes between a sample item's opening and closing `#` are copied into a 64-byte stack buffer (`local_1d0`). The copy loop has no length check, and after finding the closing `#` the code calls `FUN_0048db70(local_1d0)` without writing a NUL terminator. The called signed-decimal parser (`FUN_0048dad0`) reads digits until it encounters a non-digit, so if the first stack byte after the copied token is also a digit, the parsed value can include bytes beyond the marker.

A token longer than 64 bytes also writes beyond the local buffer before the numeric parser runs. This is a static defect in the configuration parser's handling of `#...#`; the shipped `Samples` section contains no active example, so there is no evidence that the stock config exercises it. Finding 142 describes the intended volume path and DirectSound range; this finding limits claims about the exact value produced by malformed or modded markers.

| Marker condition | Observed parser behavior |
| --- | --- |
| Closing `#` | Terminates the copy loop but is not copied; no NUL is appended before parsing |
| First byte after copied digits is non-digit | Decimal parser stops there |
| First byte after copied digits is a digit | Decimal parser may continue into adjacent stack contents |
| More than 64 marker bytes | Copy loop writes past `local_1d0` |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00464d00` is in `docs/decompiles/full/LEGORR.full.c` and its machine code at `0x464d75–0x464d99` shows the copy, delimiter exit, and immediate parser call with no terminator store or length comparison. The decimal scan is `FUN_0048dad0` at `0x48dad0`. The `Samples` parser pseudocode is recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af`).
