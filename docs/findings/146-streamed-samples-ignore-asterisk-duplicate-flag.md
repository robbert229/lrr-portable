# Finding 146 — Streamed Samples Ignore the Asterisk Duplicate-Buffer Flag

`FUN_00464d00` processes markers in this order: an optional leading `*`, then an optional `#...#` volume token, then an optional `@`. It passes the resulting stream flag and duplicate flag separately to `FUN_0047a870`.

`FUN_0047a870` branches on the stream flag. Without `@`, it calls `FUN_0047b580` and forwards the duplicate flag; that path reaches `FUN_0047b6c0`, which can create two DirectSound duplicate buffers. With `@`, it takes a separate stream-loading branch that never reads the duplicate-flag argument and never calls `FUN_0047b580` or `FUN_0047b6c0`. Therefore, a valid `*@path` entry is streamed, but its `*` does not request the three-buffer WAV rotation. A `*` appearing after `@` is not recognized as the duplicate marker because the parser checks for `*` only at the beginning of the item.

The shipped config has no active item combining `@` and `*`, so this conclusion comes from the distinct parser/loader branches rather than a stock example.

| Sample path prefix | Stream branch | Duplicate buffers |
| --- | --- | --- |
| `*path` | In-memory WAV | Requested; may create two duplicates |
| `@path` | Streamed | Not requested |
| `*@path` | Streamed | `*` flag is ignored |
| `@*path` | Stream loader receives a path beginning with literal `*` | `*` is not parsed as a marker |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Marker parsing and the two `FUN_0047a870` branches are in `docs/decompiles/full/LEGORR.full.c` at `FUN_00464d00` and `FUN_0047a870`; the parser is recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af`). WAV duplicate creation is in `docs/evidence/legorr-sound-duplicate-selection.bundle` (`ev_42a48d242a5c0f775c2ebfcbf3fe6538712d8722f03b1612835635c5ed8d561f`, `ev_25c4d48e45c95cd515dd377a1a625bd499d49863be3da19227bc1f86aaff5218`). The `@` branch is recorded in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle` (`ev_98a95a387f00ede6b484669f9341f5e0b38636f97b03e08fe8bccb6df4bc1b1f`).
