# 44 — Reward Save and Advance buttons take separate persistence routes

The reward screen's `SaveButton` and `AdvanceButton` do not call one shared
checkpoint routine. In `FUN_00461dd0`, the Save zone enters the save-slot UI
through `FUN_00462030`, while the Advance zone calls `FUN_00462270` directly.
This corrects finding 09's summary that either reward-zone handler ends in the
same checkpoint step.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-expansion.bundle`.
Focused IDs: `ev_25ff9fad3264901ea7b394dbe3b6e6f4ebb725dc443fba45a8edd203aad76e88`
(`FUN_00461dd0`), `ev_aeeb9678a4ef9ab1a01ffd9ad9909647030e1a81034276de21816af6a19ce8eb`
(`FUN_00462030`), `ev_0959789f3c06aa946dd69cff2d45107564790585934448399dcdef76fdee05cc`
(`FUN_00462270`), and `ev_5fa402f6b757672cbb1a354c73a71c0f43edcf4125196d4a6c4c0643f89dfc89`
(`FUN_00462780`).

## Observed routes

`FUN_00461dd0` first requires a current level state and a checkpoint blob.
When the level status at `levelState+0x27c` is `1`, it hit-tests the widget
record at `DAT_00555b68` (the Save button). A hit calls `FUN_00462030`, sets
the second output argument to `1`, then submits sound index `0x29` through
both the direct and queued sound paths. `FUN_00462030` enters the save-menu
flow and waits for its selection loop; its slot-write path is conditional on
the menu result.

The same handler hit-tests the widget at `DAT_00555b80` (Advance). That hit
sets the first output argument to `3`, submits the button sound, and calls
`FUN_00462270`. This routine writes the current level's checkpoint record,
commits the save context, initializes slot `5` when no active slot exists,
and refreshes the level-tree context. `FUN_00462780` is another caller of
`FUN_00462270`, through a separate input path.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Save and Advance widget hits dispatch to different functions | observed in the complete `FUN_00461dd0` body |
| Save enters a modal save-menu route | observed in `FUN_00462030`; selected-slot outcome is conditional |
| Advance directly commits the active checkpoint context | observed in `FUN_00462270` |
| Exact user-visible meaning of the two output arguments (`1` and `3`) | unknown |
| Runtime whether the Save route results in a slot write for a given user choice | unknown; no interaction replay was captured |
