# Finding 371 — Record Pools, Pair Tables, and Buffered Streams

This pass names two free-list pool growers, case-insensitive pair-table operations, and a buffered character-stream reader.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0047e0c0` | `FixedSizeRecordPoolGrow` | Allocates a batch of 0x20-byte records, links them into a free list, and advances the pool count. |
| `0x004807f0` | `RenderResourceInstancePoolGrow` | Allocates a batch of 0x34-byte records and links them into a free list. |
| `0x00488730` | `CaseInsensitivePairTableFind` | Scans 8-byte entries, compares keys case-insensitively, returns the paired value when found, or zero. |
| `0x00488790` | `CaseInsensitivePairTableAppend` | Normalizes/copies a key, writes its paired value, and increments the entry count. |
| `0x0048b4a0` | `BufferedCharStreamReadToString` | Reads bytes from a buffered stream until a zero byte, writes a terminator, and returns 1; stream format and capacity are unresolved. |

Caller evidence for `0x0047e0c0` is `ev_6fb1e4dfab7979bd8bbace07057753e8e2b534684994729e0f81c0c2ea34ed99`. Direct and caller evidence for `0x004807f0` are `ev_ee3bdb54cb55bc9fe08f1c80b6f78ab7caff644d5f61cd31574bea8a89bbff15` and `ev_52f4d8c649a2272633f37a52e5c182fb360387565bbf27a60d0c7aad2624189d`. Pair-table caller records are `ev_453bd987c60d772e8237ab1afec3b7fbed130e71894c901206119cdad49643ef` and `ev_594c7c904d68d3043bfff069599356281c20b1c47401930e8e3ad5dc4a3a3c6b`; buffered-stream caller evidence is `ev_004a2c704d07e83ae6251cc9aa074b626de42c93ce5066cfff448099cd30e682`.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Numeric record kinds remain literal where semantics are unresolved.
