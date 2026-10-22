# Finding 128 — Permitted-Activity Appender Capacity

The activity-list installer at `FUN_0044b370` is gated by a capacity returned from `FUN_0046a060`. The getter reads the dword at `entity + 0x334 + 0xcc`; it does not calculate the capacity from the list span. This bounds the setter's capacity source to the entity's attached stats block. How that stats-block field is populated remains unresolved.

In `FUN_0044b370`, the setter first writes zero to `entity + 0x3f4`, then returns without changing the list if the capacity is zero. For a nonzero capacity, it clears `entity + 0x3ec` bit `0x4`, recomputes the can-do result and may set that bit, then handles the requested activity:

- A special request (`FUN_0044b350(param_2)` true) scans the current entries when count is nonzero. If an existing entry is also special, that slot is replaced in place and the count is unchanged.
- Otherwise, when count is below capacity, the request is written at index `count` and the count increments.
- When count equals capacity, entries shift left and the new request is stored in the last slot. The count remains at capacity.

The setter's `+0x3f4` zero-store is directly visible in the decompile and instruction record. These observations do not establish what the field represents or whether another code path later reads it. The semantics and population path for stats `+0xcc` are also still open; the port's configurable capacity remains a model choice rather than a recovered default.

Evidence comes from fresh Ghidra analyses of the shipped `LegoRR.exe` (`FUN_0044b370`, evidence `ev_67faa0af…116ff1`; `FUN_0046a060`, evidence `ev_485e4675…2a70a`). See [the canonical bundle](../evidence/legorr-permitted-activity-capacity.bundle).
