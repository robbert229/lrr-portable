# Finding 502 — Fog, Menu-Wipe, Bubble, and WAD Records

`0x00556470` stores the configured BirdScarer radius used by scene-effect placement. `0x00556EF8` stores the high-fog colour pulse period: it defaults to 1500 when FogRate is zero, otherwise FogRate multiplied by 25, and the fog update uses it to advance a sine phase.

`0x005574F8` stores the menu-wipe start timestamp. The wipe starter records the current timer tick, and the menu loop advances the transition from elapsed time.

`0x00557864` begins the bubble-action texture handle table, populated by mapping configured action names to indices. `0x00557900` begins the matching action-name table, with entries such as CantDo, CollectCrystal, CarryOre, and Repair.

`0x00575500` begins 15 WAD archive records with a `0x24`-byte stride. Archive lookup indexes this table, and allocation scans for and clears an unused record.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
