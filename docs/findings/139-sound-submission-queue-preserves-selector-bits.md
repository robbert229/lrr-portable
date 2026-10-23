# Finding 139 — Deferred Sound Submissions Preserve Selector and Route Bits

When sound submission batching is enabled (`DAT_00502a2c & 8`), `FUN_00465090` stores a request instead of calling `FUN_0047ad70` immediately. The pending record holds the sound-bank alias, an optional sound object and position, and a packed flags word. The low three bits are: bit 0 marks a positional/object submission, bit 1 preserves `param_3` (the selector forwarded to `FUN_0047ad70`), and bit 2 preserves `param_4` (the route choice used by `FUN_00465090`).

`FUN_004652d0(0, 1)` drains pending records. For a non-positional record it calls `FUN_00464fa0(alias, selector)`; for a positional record it calls `FUN_00465090(object, alias, selector, route, position)`. The selector therefore survives the deferred path unchanged. Separately, `FUN_004651a0` resubmits the previous frame's secondary pending list through `FUN_00464fa0(alias, storedSelector)`, then shifts the newly accumulated lists into place.

This clarifies how selector values reach the stream-state argument even when the audio system batches requests. It does not identify every runtime request's alias or establish audible playback.

| Packed bit | Stored from | Drain behavior |
| --- | --- | --- |
| 0 | Set for `FUN_00465090` positional submissions | Choose positional drain path; clear chooses `FUN_00464fa0` |
| 1 | `param_3 & 1` | Forward as selector to `FUN_0047ad70` |
| 2 | `param_4 & 1` | Forward as route argument to `FUN_00465090` |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00465090`, `FUN_004651a0`, `FUN_00464fa0`, and `FUN_004652d0` are in `docs/decompiles/full/LEGORR.full.c` at `0x465090`, `0x4651a0`, `0x464fa0`, and `0x4652d0`. `FUN_00465090` and its positional sound path are also recorded in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle` (`ev_7da0a488bdbf46abfc69ed8002c9fc64f46e9742a0fc060034aedd5bca77cae`). The queue-drain logic is recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_7fa8cb3c53c508e7d07f9336a5a394550b943cae732d33ae0ce9eab61b245647`).
