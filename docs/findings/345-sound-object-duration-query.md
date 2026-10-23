# Finding 345 — Sound-Object Duration Query

`0x465160` resolves a sound object, queries its interface for a duration-like value, and returns zero when resolution fails. Callers use the result as sound playback timing/countdown.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x465160 | SoundObjectDurationGet | Resolves an object, invokes the duration query, and returns zero when no object is available. |

The body and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained procedure record was found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
