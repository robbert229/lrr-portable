# Finding 321 — Transform-Collection and Surface-Config Helpers

Two helpers now describe the collection-level transform pass and a configured surface-type lookup. The per-object callback `0x4247d0` is named in finding 313; this wrapper enumerates the objects and applies that callback.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x424810 | ApplyDefaultTransformToFlaggedRenderObjects | Enumerates render objects and applies the flagged-object transform callback. |
| 0x435190 | SurfaceTypeConfiguredValueAtIndex | Returns the indexed configured value from the table populated while reading `SurfaceTypeDescriptions`. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The transform wrapper has a direct batch-decompile record in `legorr-engine-helper-audit-low-expansion-5.bundle`, record `ev_96a977baa11dd29928be4a2bf16f89bdc83e04195aefa4cfda355ef37e33fd0e`; callback behavior is also directly analyzed in that bundle family, record `ev_abae089b9e3e3f77d9c4d6333a5a777c7d46951b342f10227057fb3c9a406c32`. No retained direct record was found for the surface-type lookup. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
