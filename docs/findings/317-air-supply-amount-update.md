# Finding 317 — Air-Supply Amount Update

`0x424560` updates the global air-supply amount at `DAT_005560d4 + 0x1fc`. It scales the supplied delta by `0.04`, the configured factor at `+0x200`, and an entity-derived value; clamps the result to 0–100; and queues notification 2 when the amount reaches zero unless global bit `0x08000000` suppresses it. Kind-4 entities skip the update unless entity bit `0x20000000` is set. The raw masks remain literal because their meanings are not independently established.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x424560 | AirSupplyAmountUpdate | Updates and clamps the air-supply amount, with a depletion notification gate. |

The body is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The field’s air/warning role is described in [Engine Tick](03-engine-tick.md). No matching retained direct analysis record was found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
