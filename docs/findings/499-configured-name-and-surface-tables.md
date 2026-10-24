# Finding 499 — Configured Name and Surface Tables

`0x00556228` begins eleven localized tool-name string handles loaded from the ToolNames config category. `0x00556EBC` holds the configured OreRequiredText tooltip string used by entity feedback formatting.

`0x005562D8` begins the terrain surface-type name table; lookup returns the indexed name or a diagnostic fallback for a null entry. `0x00556320` begins the corresponding table of configured surface-type values, populated by the description loader and exposed through an indexed getter.

`0x005764F4` is set by the window-mode setter and cleared during startup stream reset. Its exact mode encoding is unresolved. `0x005765E0` begins a 256-entry keyboard key-name table initialized with names such as KEY_ESCAPE, KEY_ONE, KEY_RETURN, and KEY_A; named-option lookup searches the table.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
