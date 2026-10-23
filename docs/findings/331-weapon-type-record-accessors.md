# Finding 331 — Weapon-Type Record Accessors

Four getters read weapon-type fields whose meanings are confirmed by the parser writing the corresponding named configuration keys into the same offsets.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x46f110 | WeaponTypeGetRechargeTime | Reads record offset `+0x0c`, populated from `RechargeTime`. |
| 0x46f140 | WeaponTypeGetDischargeRate | Reads record offset `+0x14`, populated from `DischargeRate`. |
| 0x46f170 | WeaponTypeGetRange | Reads record offset `+0x1c`, populated from `WeaponRange`. |
| 0x46f1a0 | WeaponTypeGetWallDestroyTime | Reads the indexed value populated from `WallDestroyTime_%s`. |

Getter and parser bodies are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained bundle record was found for these four getters; the parser’s named fields corroborate each accessor. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
