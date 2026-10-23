# 98 — A helper-returned pointer follows a separate route-marker path

FUN_0043fff0(actor) reads the actor's current cell, then calls FUN_004389e0(actor, 1) for a pointer. It proceeds only when that pointer and its FUN_0043a5f0 model are non-null. After converting the model position to a cell, it checks the destination with FUN_0044f6d0. A destination in the actor's current cell goes directly to FUN_00441700; otherwise the function asks FUN_004410f0 for a route and returns zero if the route query fails.

If FUN_00441700 accepts the path setup, the routine writes the helper-returned pointer at actor+0x2fc, writes 0x13 into the final path record, and returns one. It does not perform the explicit capability-bit and actor+0x304 checks visible in finding 97. The shipped image contains a direct call to this routine at 0x40470e.

**Evidence:** REA/Ghidra 12.1.4 on shipped reference/LegoRR.exe (SHA-256 c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f), bundle legorr-entity-target-route-markers.bundle. Function dossier: ev_4fcfa14fd68919f130a84b29f1f66814d3a0705f3f836a096dcaabcf2bcf1762; direct callsite instruction: ev_370578675728e795a02a3a61f9925e7a26ff51828c8be782f15ee730a73b3607.

## Boundary

The helper's selection rule and the pointer's identity are unknown. The evidence establishes static path setup and the pointer/marker writes only; it does not identify an attack, creature, or other gameplay purpose. No runtime behavior was observed.
