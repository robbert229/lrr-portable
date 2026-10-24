# Finding 481 — Config, Render, and CRT Global State

`0x005053FC` receives the configuration handle passed to `WeaponTypeConfigLoad`. The loader uses it to enumerate the `WeaponTypes` section, and ray-hit/visibility checks query weapon-related Boolean settings through the same handle.

`0x00556110` receives the `fpclipblocks` configuration value. Camera clipping, fog-range setup, and a view-dependent distance calculation multiply their distances by this float.

`0x00557500` is the pending level-selection overlay flag. `FrontEndStateChange` sets it when returning to the frontend with a saved transition context. The save-menu flow checks it to show the selection overlay and then clears it through `CurrentSelectionContextSet`.

`0x004AFD64` gates CRT behavior that synchronizes descriptor slots 0–2 with Win32 standard handles and emits runtime error messages. Those branches enable the behavior only when this value equals 1; the decompile shows no assignment to the global, so its lifecycle and intended mode name remain unresolved.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
