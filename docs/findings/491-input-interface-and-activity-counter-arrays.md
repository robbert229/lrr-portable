# Finding 491 — Input, Interface, and Activity Counter Arrays

`0x00575CEC` holds the temporary startup configuration stream opened for the configured startup file, parsed, and closed by `StartupConfigStreamLoad`. `0x00575CE0`, `0x00575CE4`, and `0x00575CE8` are three additional startup stream handles cleared and closed by the same lifecycle helpers; the decompile does not expose their assignments or individual roles.

`0x0076AC4B` and `0x0076AC4D` are current keyboard-state bytes for the Left and Right arrow keys (scan codes `0xCB` and `0xCD`). The UI uses their transitions to move selection backward/forward; simulation input uses the held states for corresponding camera movement/rotation.

`0x004B31AC` is the scene context supplied to advisor scene-controller setup. The advisor config-slot loader uses it as the parent when creating configured scene resources.

`0x004C7214` begins the activation flags for four pose/resource slots per scene owner. Registration scans the four flags, initializes an unused slot, and resource-availability sweeps activate slots when their scene object becomes available.

`0x004C7C74` begins the active flags for groups of 30 transient-render slots. The per-group stride is `0x3D` dwords; configuration creates disabled scene objects and the availability sweep enables their slots when ready.

`0x004DC4D4` begins the category-code array for overlay message candidates. The list builder stores candidate category codes here and parallel values at `0x004DC54C` and `0x004DC5C4`; the text refresh path counts category codes 1 and 4 to select the message text.

`0x004DCD40` is the loaded resource handle for the interface message-tab info text panel. The loader prepares it and the info-message queue tick draws it as the panel scrolls.

`0x004DD9AC` begins 74 per-level simulation-pointer action counters. Level-state reset clears all 74 dwords; pointer-action dispatch increments the counter indexed by action category.

`0x004DDCF0` stores the animated advisor-overlay transition rate, calculated as the configured speed parameter multiplied by `0.04`. The event countdown uses it to advance or return the overlay offset between configured endpoints.

`0x004EAD78` begins six special tooltip-icon surface handles loaded by `ToolTipIconsConfigLoad`; entity feedback uses these for the nonstandard icon categories. The separate 11-entry entity-icon table begins at `0x004EAD90`.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
