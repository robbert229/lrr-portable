# Finding 495 — Tutorial Command and Screen-Effect Pools

`0x004A5950` is the command-type dword at offset eight in a 12-byte tutorial command record. Indirect handle resolution invokes the record callback for type zero, while the tutorial dispatcher branches on this field for other command behaviors.

`0x004B8A58` begins ten numbered digit bitmap handles loaded from `0.bmp` through `9.bmp`; the count renderer selects one by the current decimal digit. `0x004B8A84` begins a pool of ten screen-effect quad render records, each eight dwords. Allocation claims a free record, and the effect tick updates active records. `0x004B8BC4` is the exclusive end of that pool.

`0x004C7178` stores whether the global mode setter received zero. Cell-entry loading branches on this flag, but its wider meaning remains unresolved.

`0x004DB8A4` is the exclusive end of the five save-menu thumbnail surface handles beginning at `0x004DB890`; the load and release loops use it as their boundary.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
