# Finding 483 — Pointer Registry, Input Snapshot, and Allocator Globals

`0x005563B8` begins the ten-slot object-pointer registry. `OListEntityRecordParse` clears it and appends successfully parsed entity pointers while the count at `0x005563E0` is below ten; indexed lookup bounds access by that count, and removal clears matching slots.

`0x00557048` is the level-start action flag passed into the pause-menu state. The level-start accessor returns whether it is nonzero, and the menu overlay gate clears it while establishing a new pause-menu state.

`0x0076AA00` begins the previous-poll 256-byte DirectInput keyboard snapshot. `InputStatePoll` copies the prior current-key buffer here before clearing and refilling the current buffer at `0x0076AB80`; input handlers compare the two buffers to detect transitions.

`0x0076CD88` holds the shared texture-directory string handle. `SharedTextureDirectorySet` interns the supplied path here, and renderer shutdown releases the stored string.

`0x004B1FDC` contains the allocator's `0x1E0`-byte (480-byte) small-block threshold. The engine allocator and aligned calloc route requests at or below this limit through the small-block allocator and use the process heap for larger requests.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
