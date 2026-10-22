# 89 — A kind-4 monitor gates deposit handoff on count and state

`FUN_0043bb20` runs only for kind-4 entities. It derives a comparison limit
from `+0x324` and `+0x328`; when the stats-state word at `entity+0x334+0x144`
has bit `0x2`, the limit can be reduced by a value from `FUN_00469bb0`. If the
count at `+0x324` is below that limit, or entity bits `+0x3e0 & 0x2000` or
`+0x3e8 & 0x1000000` are set, the monitor clears `+0x3e8` bit `0x1000000`.
Otherwise, when stats-state bit `0x4` or `0x2` is set, it calls
`FUN_0043a300(entity, 0)`. Finding 80 documents that helper's separate
`Activity_Deposit` transition gates.

**Observations from** REA/Ghidra 12.1.4 over the shipped `source/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-object-state-and-health-ui-expansion.bundle`](../evidence/legorr-object-state-and-health-ui-expansion.bundle).
Focused record: `ev_3e985d8bc5115b18d6eb332f64f3f284fae55bf550617e09fb39efe26f7b6b83`
(`FUN_0043bb20`).

The compared fields and flag meanings remain unknown, so this does not name
the count as an inventory threshold or claim that deposit completes. It is
static evidence; runtime conditions were not observed.
