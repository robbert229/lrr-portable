# 80 — Kind-4 entities enter `Activity_Deposit` through a gated state transition

The zero-target path in `FUN_0043a300` requires entity flag `+0x3e0 & 0x400`
and stats-state bit `0x4` or `0x2`. When both gates pass, it sets
`+0x3e0` bit `0x2000`, selects `Activity_Deposit`, dispatches the activity
change, and sets a one-second duration. This finding describes the helper's
code-level handoff, not a user-visible resource transfer.

**Observations from** REA/Ghidra 12.1.4 over the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-object-state-and-health-ui-expansion.bundle`](../evidence/legorr-object-state-and-health-ui-expansion.bundle).
Focused records: `ev_ea92a0ece07e08ac1dfa019605a3c1f36f15a24b7ddabf868b7d719e651c6fe5`
(`FUN_0043a300`), `ev_b7d5f6209b7732b8e1263b99cf01b8d70d4a01c94f1b54cbd1d5e2cd844d77d7`
(`FUN_004409e0`, activity setter), `ev_0d2cc128e6351032b75eec9bfcfb090ef6e0a9673d6430702aac90c19384bdcf`
(`FUN_00447c40`, activity-change consumer), and
`ev_c6ca6ad9cb2fffd3dee93b71c10071efe14236ecba93343bc4ffb6e547c42bdb`
(`FUN_00436c20`, activity-name table initialization).

## Transition

`FUN_0043a300` checks `entity+0x3e0 & 0x400` and stats-state bit `0x4` or
`0x2`. If both gates pass, it sets `entity+0x3e0` bit
`0x2000`, then calls `FUN_004409e0(entity, 0x1d, 1)`. The setter indexes the
activity-name table at `DAT_004decc4`, writes the selected activity pointer to
`entity+0x2e8`, writes parameter `1` to `+0x2d8`, and resets timer `+0x2d4` to
zero. The initializer places the literal `Activity_Deposit` at table slot
`0x1d` (29). `FUN_00447c40` consumes a changed activity name through the
entity-kind-specific runner, updates its mirror at `+0x2ec`, resets `+0x2d4`,
and returns its runner result. The helper then passes `1.0f` to
`FUN_00447b30` and returns success.

## Boundary

The observed transition associates the named `Activity_Deposit` with explicit
gate bits whose meanings are not recovered, so successful deposit would be a
guess. No resource count transfer, carried-object pointer change,
pickup animation, or drop-off completion is shown on this branch. The same
helper has a separate nonzero-target path involving route selection; that path
is outside this finding. No runtime session was used, and static analysis does
not establish when the transition executes in a level.
