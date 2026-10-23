# Finding 306 — String-Cache and Interface-Message Loaders

Two routines now have names grounded in their direct lookup and resource-loading behavior.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x410520 | StringCacheLookupOrLoad | Rejects null or empty keys, finds or creates a case-insensitive cache entry, loads its value when uncached, and returns it. |
| 0x419430 | InterfaceMessageTabInfoTextLoad | Loads the interface message-tab information string, stores it in a global, and retains the resource handle. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x410520` is used by resource and config paths; `0x419430` is called from interface initialization `0x41fb50` with the `Interface_MessageTabs_infoOverFl` key. No matching retained direct-analysis records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
