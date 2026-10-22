# 52 — Advance gates campaign completion marking on the level result

The reward-screen Advance path does more than write a checkpoint: it resolves
the current level's index in the active level chain, and calls the chain-row
marker only when the level state at `+0x27c` equals `1`. The marker ORs low bits
into a `0x3190`-stride row, with the exact bit combination depending on the
chain mode and index. This ties campaign progress marking to a specific level
result value while keeping the bit meanings unresolved.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-transition-agent.bundle`.
Focused IDs: `ev_0959789f3c06aa946dd69cff2d45107564790585934448399dcdef76fdee05cc`
(`FUN_00462270`), `ev_8e78b0254f1403524e8f7525c811c41490385f843b828438e87f4e842ba4bae3`
(`FUN_00416e00`), `ev_550630b232cbfbdffebc24405f4c44dc9dcc0972d9938c1b6cbd10c35c72d217`
(`FUN_00416e20`), `ev_370b353fd11c81ff4477ac890656b129ac6b81e1ab6632b01018d433bd245d46`
(`FUN_00416d60`), and `ev_5c83863ce01271f542160d327bce38134f98b4ed65f34273f32a306a6aa05ee6`
(`FUN_00417eb0`).

## Observed handoff

`FUN_00462270` obtains the current level name through `FUN_00431560`, selects
one of two configured chain structures via `FUN_00416e00`, and resolves that
name to an index with `FUN_00416e20` (case-insensitive string comparison). It
then checks the current level status at `levelState+0x27c`; only status `1`
causes a call to `FUN_00417eb0(index)`. The checkpoint blob is copied to the
corresponding chain row through `FUN_00417f40` on the surrounding path.

`FUN_00417eb0` requires an available chain and writes into a row at
`chain+0x34 + index*0x3190`. Depending on the boolean returned by
`FUN_00416d60` and the index, it ORs bit `1`, or bits `1` and `2`, into the
row's first word. For one branch and indices above `7`, it can mark bit `1`
alone when the chain count check passes; for the other branch, indices above
`7` return without a mark.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Advance calls the chain marker only when current level status equals `1` | observed in the complete `FUN_00462270` body |
| Current level name is resolved case-insensitively in the selected chain | observed across `FUN_00416e00` and `FUN_00416e20` |
| The marker writes one or both low bits into a `0x3190`-stride row | observed in `FUN_00417eb0` |
| Human-facing meaning of status value `1` in this gate | not established here |
| Meaning of chain-row bits `1` and `2`, and the boolean selecting chain behavior | unknown |
| Whether other progression writes occur outside this Advance path | not exhaustively traced |
