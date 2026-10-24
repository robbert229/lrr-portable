# Finding 479 — Token Pool Blocks and Emergence Counter

The DAT table at `0x00506498` stores block pointers for the block-text token free list. `BlockTextTokenPoolInit` clears 32 slots. `BlockTextTokenPoolGrow` allocates `2^n` records of `0x20` bytes, stores the new block pointer in slot `n`, links each record through its field at `+0x1c`, and chains the previous free-list head after the block. `BlockTextTokenPoolDestroy` releases populated blocks.

`0x00555324` counts successful creature emergence spawns. `CreatureSpawnFromEmergence` increments it only after constructing a creature, assigning its emergence behavior, and checking for the supported monster types. A later stats-record builder copies this count into its record at `+0x2988`.

`0x00555330` is a float accumulator. `EntityDamageApply` adds the damage argument when the health-related values returned by its helper calls pass a comparison. The stats-record builder copies the accumulator to `+0x2994`, capped at 100 points per counted emergence spawn. The accumulator's full gameplay meaning is not established by these uses alone.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
