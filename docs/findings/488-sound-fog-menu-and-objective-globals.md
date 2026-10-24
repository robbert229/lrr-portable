# Finding 488 — Sound, Fog, Menu, and Objective Globals

`0x00506698` begins the fixed-stride sound-bank entry table. `SoundBankEntryLoad` chooses a free slot using flags at `0x005067C8 + n * 0x134`, stores the sound filename and entry metadata into the record at `0x00506698 + n * 0x134`, and marks the slot initialized. The table spans 600 records.

`0x005493D4` is the CRT code-page fallback mode. `CrtCodePageGet` sets it for requests that select the operating-system OEM or ANSI code page, and clears it for an explicit code page. When `GetCPInfo` fails, `CrtLocaleCodePageTableUpdate` allows the fallback path only when this flag is set.

`0x0055640C`, `0x00556410`, and `0x00556414` hold the RGB components of the configured `FogColourRGB`. Level loading passes the color to the renderer; fog animation blends it with the configured high-fog RGB triplet beginning at `0x00556418`.

`0x00556EC4` is the configured minimum distance for 3D sounds in top view, defaulting to 150 when the config value is zero. Level setup and camera-mode transitions pass it to the DirectSound listener distance helper.

`0x005574FC` is a menu-scene light handle created with RGB components of 0.8. The load-menu tick temporarily disables it while drawing a transition scene, then restores it.

`0x004FFE30` is the objective-success flag. Objective status dispatch sets it to 1 for success and clears it for ordinary or crystal failure; the getter returns its current value.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
