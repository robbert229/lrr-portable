# Finding 126 — Effect table dispatches timed opacity updates

The simulation tick's `FUN_0040aa10` is a table dispatcher. It starts at
`DAT_004b8a84`, advances entries by 32 bytes (`puVar1 + 8` dwords), and stops
at `DAT_004b8bc4`, so the walked span is ten entries. It calls
`FUN_0040aa40(entry, dt)` only when the entry's byte at offset `+0x1c` has bit
0 set. This establishes the active-entry dispatch and table bounds directly
from the shipped executable.

`FUN_0040aa40` treats entry `+0x18` as elapsed time and runs while it is below
15.0. It subtracts `dt` from entry `+0x14`, then, when entry `+0x08` is
nonzero, loops from zero to that count and calls
`FUN_00483420(*(entry+4), index, 1 - elapsed/15, 4)`. It then adds `dt` to
elapsed time. Once elapsed time reaches 15.0, it clears bit 0 at entry `+0x1c`
and calls `FUN_00473b40(*entry, 1)`.

`FUN_00483420` resolves an indexed item through a `+0x70`-stride array and
clamps the supplied scalar to `[0,1]`. In mode 4, it changes item flags at
`+0x6c` according to that scalar and writes the scalar to a related record at
`+0x10`. This supports describing the helper as an opacity/fade update: the
value is a 15-second linear ramp from 1 toward 0. The function does not itself
render pixels; it updates per-item state that a later render path may consume.

## What this does and does not establish

This identifies an independent timed effect-table lane and its per-item state
update. The available evidence does **not** establish that its entries are
screen-space effects: there are no observed screen coordinates, viewport
selection, or direct draw call in this lane. The later consumer of the updated
item flags/scalar is also unresolved. Therefore this closes the dispatcher and
fade-update portion of the TODO, but leaves the lane's screen-space identity
and final drawing path open. Port behavior should not assume screen-space
coordinates from this evidence alone.

**Evidence.** REA/Ghidra 12.1.4, exact shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Records are in [`legorr-screen-space-effect-lane.bundle`](../evidence/legorr-screen-space-effect-lane.bundle):
`ev_3d925b810d3e388b71186a9895a2e83ee9c084a70a2949713821586a1af4ac99`
(dispatcher),
`ev_841ead11269906cb773134dfca3c15bd6d5e898f2366df0ef8c15b95880b54da`
(timed entry helper), and
`ev_0f2a7d58b1f97fe75770cbcce87155c2beaac4bbe78f34b1c2a5d8ff31343bf4`
(indexed item state update).
