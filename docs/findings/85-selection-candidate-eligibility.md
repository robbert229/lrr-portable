# 85 — Selection candidates pass a compound eligibility gate before mode handling

Before `FUN_004526e0` handles its selection mode, it sends the candidate through
`FUN_00452870`. That predicate requires an entity state bit, a shared state
predicate, and several clear flag masks; kind-1 entities face one additional
flag exclusion. The shared helper also checks a nonnegative float at `+0x340`.
This is an eligibility gate separate from the add, duplicate, and deselection
branches described in findings [06](06-command-queue-and-selection.md) and
[77](77-selection-clear-marker.md).

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-selection-eligibility-expansion.bundle`](../evidence/legorr-selection-eligibility-expansion.bundle).

## Predicate

`FUN_004526e0` calls `FUN_00452870` before branching on its mode. The filter
returns true only when all of these checks pass:

- dword `entity+0x3e8` has bit `0x100` set;
- `FUN_004385b0(entity, 1)` returns true;
- byte `entity+0x3e4` has neither bit in mask `0x0a` set;
- dword `entity+0x3e0` has no bits in mask `0x24000000` set;
- unless the entity kind at offset zero is 1, dword `entity+0x3e0` has no
  `0x40000` bit set.

The called helper requires that dword `entity+0x3e0` has no bits in
`0x700100`, has no `0x20000` bit, and that dword `entity+0x3e4` has no
`0x100000` bit. It also requires the float at `entity+0x340` to compare at least
zero. Its final conditional is bypassed by the literal second argument `1` at
this call site. Evidence:
`ev_d3df7b8f1ba452140eacb1f1e1ffc245da5242d7cb91f8e30e58f25aff60e5dd`,
`ev_1e9a6608f896327f159c2e17ea8172fb498b28ea99e90c042ae8536da5c027de`.

`FUN_00452870` has a complete resolved direct-caller inventory naming
`FUN_004526e0` as its only direct caller, and the selection routine's own
prefilter is visible in its body. Evidence:
`ev_c352fc010a1663bb2b88ce23de29e5b762e65c7f265f3210bd9aaeb41476f37a`,
`ev_55bbc27367562205fe44d9664c9ee2ec32b32eba750a2f1d7cb692fd01f57d89`.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| The listed flag and float conditions gate candidate processing in `FUN_004526e0` | observed (`FUN_00452870`, `FUN_004385b0`) |
| The helper's final condition is bypassed by argument 1 from the selection filter | observed at the call site |
| Human-facing meanings of the flags, the float field, and why kind 1 gets the extra exclusion | unknown |
| Candidate acceptance during a live game interaction | not runtime-verified |

This is static analysis of the shipped executable. It identifies the code's
predicate, but does not assign domain meanings to undocumented entity fields or
confirm how a particular input event reaches it.
