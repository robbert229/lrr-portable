# 53 — `NextLevel` builds the campaign chain used by the reward handoff

Startup builds separate level-chain tables from the configured `StartLevel`
and `TutorialStartLevel` entries. The builder follows each level group's
`NextLevel` reference until the lookup returns no next group, then stores the
ordered level references and display names in parallel arrays. Finding 52
shows the reward Advance path resolving the current level against one of these
chains before marking progress.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-transition-agent.bundle`.
Focused IDs: `ev_c93c5bc39c81827fb04b25defcbf8a86d93e0f87a9c426d8127c2730e2f05c8d`
(`FUN_004161f0`) and `ev_d599f93f7ced2c80467306c4a6a27ada4855fe20cb07c5c00f957b8351cbcd67`
(`FUN_004154b0`).

## Chain construction

`FUN_004161f0` calls `FUN_004154b0` once with `StartLevel` and again with
`TutorialStartLevel`, storing the returned chain structures in separate
globals. `FUN_004154b0` resolves the requested start entry, follows its
`NextLevel` field repeatedly, and counts entries until the next config lookup
returns zero. It allocates parallel arrays, then fills them with the ordered
level references and their display names.

For each entry, the builder reads `FullName`. If it is absent, the builder
uses `FUN_004105a0` on the level reference to derive a fallback label. If a
`FullName` is present, it passes the value to `FUN_00413f00` with `_` and
space as the replacement characters. The decompiler output establishes this
normalization call, but not the intended UI wording beyond the observed
`FullName` key.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Startup initializes separate chains from `StartLevel` and `TutorialStartLevel` | observed in the complete `FUN_004161f0` body |
| Each chain follows `NextLevel` references until the lookup is empty | observed in the complete `FUN_004154b0` body |
| Display labels use `FullName`, with a helper-derived fallback | observed |
| Present `FullName` values are passed through underscore-to-space replacement | observed |
| Actual configured level order and resulting labels in shipped data | not enumerated in this pass |
| Whether config parsing or other code validates cycles/missing level references | unknown |
