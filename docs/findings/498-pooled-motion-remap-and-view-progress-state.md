# Finding 498 — Pooled Motion, Remap, and View-Progress State

Pooled records with a `0x2B0`-byte stride use `0x00503B84` for an initial three-component position vector and `0x00503B90` for a current position vector. The spawn and flight-record initializers seed both from the supplied position. `0x00503BA8` is initialized to 5.0 by the spawn initializer and 4.0 by the flight-curve initializer; its unit remains unknown. The same initializers store their first argument at `0x00503BBC` and a source/configuration value at `0x00503BC4`, whose precise types remain unresolved.

`0x00555BE0` begins the level-identifier remap pairs loaded from configuration. Command-reference remapping replaces an old identifier with its paired new identifier.

`0x00555DF0` is set during advisor inline voice rendering and checked by the timer/update path while advisor activity is running. Its exact reset condition remains unresolved.

`0x00556088` tracks the last displayed file-loading progress fraction. The display helper resets it to -1.0 and stores each new fraction after redrawing, using the prior value to avoid redundant refreshes.

`0x005560A0` is the upper limit for camera pointer-pan offset. `0x005560A8` is a movement scale used to advance that offset. `0x00556158` temporarily stores the viewport target-position clamp mode; its encoding remains unresolved.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
