# 97 — Explicit entity routes bind a supplied pointer after route setup

FUN_0043fe70(actor, supplied) accepts an explicit second pointer only when the actor's FUN_00469ea0 result has bit 0x2, the supplied pointer's result has bit 0x1, and actor+0x304 is zero. It reads the actor's current cell, resolves a model for supplied, converts that model's position to a cell, and checks the destination with FUN_0044f6d0. If the cells differ, it asks FUN_004410f0 to route between them and returns zero if that route fails. A same-cell destination skips that route query. Either accepted path proceeds through FUN_00441700.

Only when that path setup succeeds does the function store supplied at actor+0x2fc, write byte 0x15 into the final path record, call FUN_004373d0(supplied, 1), and return one. The shipped image contains a direct call to this routine at 0x4046fe.

**Evidence:** REA/Ghidra 12.1.4 on shipped source/LegoRR.exe (SHA-256 c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f), bundle legorr-entity-target-route-markers.bundle. Function dossier: ev_ce960ddc23b9a065a93ad2f26a6b2cf9c3c406c157326151cedbf62a14923307; direct callsite instruction: ev_5622113ec0f6b3d496993f82720cb3fd55451b4d1ec583d874de1c01c20e0221.

## Boundary

This is static control-flow and field-write evidence. The names and runtime meanings of the capability bits, actor+0x304, actor+0x2fc, the 0x15 path marker, and the final helper call are not established here. The target's identity or combat role is also unknown. No runtime behavior was observed.
