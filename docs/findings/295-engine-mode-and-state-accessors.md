# Finding 295 — Engine Mode and State Accessors

Ten low-address routines expose mode-dependent fields, global values, flags, and indexed engine fields. The names stay literal where the game meaning is unknown.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x423080 | ModeDependentField24Get | Selects a value from one of two engine records at offset `+0x24` according to the global mode; otherwise returns the mode value. |
| 0x4230b0 | GlobalFloatField00556F00Get | Returns the global value at `0x00556f00`; its meaning is unresolved. |
| 0x425d80 | CurrentEngineModeIsZero | Returns whether the current global engine mode is zero. |
| 0x426230 | GlobalTripleCopyOut | Copies three global dwords to the caller's output buffer. Their semantic type is unknown. |
| 0x426360 | GlobalStateFlagsBits6And9Set | Sets or clears global flag bits `0x40` and `0x200` from two boolean inputs. |
| 0x4263a0 | GlobalStateFlagsBit7Set | Sets or clears global flag bit `0x80`. |
| 0x4263c0 | GlobalSubsystemStateReset | Clears two globals, invokes reset helpers, and clears state bits through the preceding setters. |
| 0x429850 | ActiveEngineFieldB0Get | Returns the active engine-state field at offset `+0xb0`. |
| 0x429860 | EngineFloatField210GetByIndex | Returns a float from active engine state at `+0x210 + 4*index`. |
| 0x429880 | EngineFloatField260Get | Returns the float at active engine-state offset `+0x260`. |

Bodies and caller references are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). These routines appear in the low-range REA audit bundle [legorr-engine-helper-audit-low.bundle](../evidence/legorr-engine-helper-audit-low.bundle); field meanings not established by their callers remain unresolved. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
