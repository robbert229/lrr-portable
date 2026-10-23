# Finding 340 — Entity Display and Tool-Name Resources

Two helpers resolve localized entity display names and load the configured tool-name resource handles.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4395e0 | EntityDisplayNameLookup | Selects a localized name table by entity kind and indexes it using the corresponding subtype field. |
| 0x439950 | ToolNameStringResourcesLoad | Loads the 11 configured `ToolNames` entries into a global handle table. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). HUD and selection/description callers use the display-name lookup; object initialization invokes the tool-name loader. No matching direct retained procedure records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
