# 100 — Upgrade command commits the next building index

The UI command switch in `FUN_0041dca0`, case `0x41`, calls `FUN_004387f0` and then closes the upgrade interaction. The commit helper proceeds only for type-4 entities and only when `FUN_00438770` accepts the request. It debits the selected resource path, clears the entity's `+0x3d8` progress field, advances both the building payload and entity index from `+0x330` to `+0x330 + 1` (`FUN_004089a0(payload, next)` and `FUN_00469c10(entity, next)`), creates effect variant 9 at the entity position, and emits `FUN_00418830(type, object-id, previous-index, 0)`.

The worker tick `FUN_004475c0` also reaches this helper when the linked target's flag `0x400000` is set, then clears that flag. In its adjacent progress path, it accumulates work at target `+0x340` and calls `FUN_00402f30` below 100. This connects the user-facing upgrade command and the worker-driven progression to the same upgrade commit helper.

The transition is an indexed upgrade-state change. The evidence does not establish that this path selects the separately named `Activity_Upgrade` animation entry.

## Evidence

- `ev_9436f7d09be2f41d8c7c6380ea7c049c194b3fe4a7aee9ba644499a1512e38b5` — `FUN_0041dca0` routes command `0x41` to the upgrade helper.
- `ev_11af674e7b1a8aa3cb67a887884e4bbd066fe1649e249241e24d660c01f2ff1b` — `FUN_004387f0` gate, resource debit, index advance, effect, and event emission.
- `ev_5eba9bab2413790e5cd28062dd7c922413ff37a0caf6e3d11a62d6622b2c7f78` — `FUN_004475c0` worker/progress branch that invokes the helper.
- `ev_dfe96beaefc96b91ba82e059cbdf98a86f481912be0675741fd8ca28a592db20` — `FUN_004089a0` payload-side index setter.

Observations use REA/Ghidra 12.1.4 over shipped `source/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), 2026-10-22. Bundle: `upgrade-transition-focused.bundle`.
