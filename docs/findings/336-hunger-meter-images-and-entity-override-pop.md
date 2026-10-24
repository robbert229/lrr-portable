# Finding 336 — Hunger-Meter Images and Entity Override Pop

Two helpers load object hunger-meter image slots and pop a saved record for a type-2 entity. The override record’s full field mapping remains uncertain.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4599c0 | LoadObjectHungerMeterImages | Loads `HungerImage_%i` resources under `ObjInfo/HungerImages`, stores their handles, and applies a zero-to-zero color key to each loaded resource. |
| 0x459240 | PopEntityStateOverride | Decrements a type-2 entity’s saved-record count, restores field `[0xfc]`, applies a saved word, and restores a short string. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The zero arguments go to the color-key helper described in [Finding 116](116-reward-button-color-key-setup.md); this establishes a black color key, not that the resource is hidden. The hunger-image caller also loads the associated position data. No matching direct retained procedure records were found for these helpers. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
