# Finding 485 — Menu Selection State and Building-Tolerance Color

`0x0055701C` is the main-menu quit-action result exposed by `QuitMenuProbe`. The menu state builder receives its address, and `OptionsLoadOrDefault` clears it.

`0x00557020` and `0x00557024` hold the selected item indices for the save-menu and in-game frontend contexts. Both use `-1` to mean no selection; `SaveSlotOpen` and `IsInGame` test their respective sentinels, and `FrontEndSelectedAction` uses the selected index to resolve the action associated with the active context.

`0x00557080` begins six save-game context records at `0xB8`-byte strides. `SaveMenuBuild` initializes the records from configured Save Game labels; slot refresh and context lookup use the same table.

`0x00556F2C`, `0x00556F30`, and `0x00556F34` store the three normalized components of the configured BuildingTolerance RGB color. `GameConfigInit` reads three integer components and scales each to 0–1; selection-rectangle rendering passes the triplet to its colorized draw helper.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
