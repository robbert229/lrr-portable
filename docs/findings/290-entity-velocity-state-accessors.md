# Finding 290 — Entity Velocity State Accessors

Three routines expose a type-dependent entity velocity-state record and indexed float slots within it.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4712c0 | EntityVelocityStateRecordGet | Returns an embedded state address for entity kinds 1 and 4, using different base offsets; other kinds return null. |
| 0x4712f0 | EntityVelocitySlotValueGet | Reads the indexed float at offset `+0x98 + slot*4` from the state record. |
| 0x471310 | EntityVelocitySlotValueSet | Writes the indexed float at offset `+0x98 + slot*4` in the state record. |

Bodies and slot-dispatch callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct records for `0x4712c0` and `0x4712f0` are in [legorr-engine-helpers-expansion-19.bundle](../evidence/legorr-engine-helpers-expansion-19.bundle), record `ev_e31628f471430de4fabc871cdc5a118a6f1cf7bf28a197149f32d4060fa84c05`. The write helper is in [legorr-engine-helpers-expansion-20.bundle](../evidence/legorr-engine-helpers-expansion-20.bundle), record `ev_fb654e9ce69c90085cda97e49f50a18cc194e4faf49dda19c52176f3299c7377`; dispatch/caller context appears in the expansion-19 bundle, record `ev_5b180c7b816a8ad36ecc7840a2955f07fd8b1ae01fa34f8b38c0c8660e5c30da`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
