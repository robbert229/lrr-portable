# Finding 494 — Configured Lookup and Help-Window Tables

`0x004B327C` begins the 27-entry name-pointer table searched by `CmdKindByName`, which maps a command-kind name to its zero-based index.

`0x004C7990` begins the configured rock-fall style name handles. The config loader appends each style name, and the lookup compares names case-insensitively to return a style index.

`0x004DB904` stores the X coordinate of the help window's Continue button after applying the menu overlay offset. `0x004DB914` and `0x004DB918` hold the two adjacent loaded ContinueButton resources; the exact visual role of the second handle is not distinguished in the available consumer. `0x004DB934` holds the loaded CancelFutureButton resource.

`0x005030D8` begins a grouped numeric-code table. The lookup scans eleven 16-byte records, uses a count in each record to bound that group's values, and returns the matching group index. The code domain remains unknown.

`0x00503190` begins the UI action/status message string-pointer table, populated with entries such as Walk, Drill, Reinforce, and CrystalFound.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
