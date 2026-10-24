# Finding 484 — Next-Button Gate and Allocation Retry Flag

`0x004A673C` records whether the frontend has a configured NextButton image. `LoadMenuBuild` sets it from whether the loaded image handle at `0x00556EEC` is nonzero. Advisor timer advancement consults the flag when deciding whether to advance or wait for the next interaction; the associated helper can play the global sound when the gate is active.

`0x00549154` controls allocation recovery. Engine allocation and CRT aligned calloc pass this zero-initialized flag to their retry helpers. When it is nonzero, a failed allocation invokes the low-memory handler and retries; when it is zero, allocation failure returns directly. No writer for the flag is present in the decompile.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
