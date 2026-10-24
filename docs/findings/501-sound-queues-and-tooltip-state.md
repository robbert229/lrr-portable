# Finding 501 — Sound Queues and Tooltip State

`0x00502B30` and `0x00502B58` begin paired ten-entry arrays for pending sound requests. Submission stores the request ID and its paired parameter at the same queue index; the sound frame tick transfers both arrays to the active queue and resets the pending count.

`0x004FFE48` is an objective voice outcome selector: its setter stores the value, and objective voice preparation chooses mode 2 when it is zero or mode 4 when it is nonzero. Other domain meanings remain unresolved.

`0x0054BEB8` is the surface-record pointer field in staged cell quad-run pairs, with the paired offset at `0x0054BEB4`. The stage path copies pending run data from these pairs, but their complete ownership semantics remain unresolved.

`0x0054BF60` is the lower endpoint of the tooltip overlay's third color component, derived at 60 percent of the supplied component and clamped to [0,1]. `0x0054BF68` begins tooltip-name strings such as RadarBlock, UnitSelect, Construction, and InterfaceMenu; lookup maps a name to its tooltip index.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
