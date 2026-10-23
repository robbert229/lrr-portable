# Finding 165 — Object Spatial-Update Registry Has Twenty Slots

`FUN_0047b1a0` creates an object-linked sound record and then scans the global pointer range `DAT_00533884` through `DAT_005338d0` for the first empty entry. The range contains 20 pointers. It stores the provider pointer in that slot and returns; if every slot is occupied, the helper returns without reporting a failure. Its caller in `FUN_0047ad70` does not receive a registration result.

The helper does not check whether the same provider is already present before using an empty slot. Thus registry capacity counts registration calls, not distinct providers: multiple attached sounds for one object can occupy multiple entries. When the registry is full, the sound submission path does not receive a registration failure, so a newly encountered provider may not be visited by the periodic spatial-state sweep. An already registered provider's linked sound records remain reachable through its existing entry. This is a static capacity/path observation; no shipped level was shown to exceed the capacity.

Finding 162 traces the registry's update behavior. The call site and fixed pointer range are in `docs/decompiles/full/LEGORR.full.c`: `FUN_0047b1a0` at `0x47b1a0`, its caller `FUN_0047ad70` at `0x47ad70`, and the update walk in `FUN_0047b390` at `0x47b390`.
