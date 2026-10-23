# Finding 257 — Configured Tiny-Monster Spawning and Mutual Activity Pairing

Two more anonymous procedures now carry names grounded in their own instruction
sequences plus static call sites inside named procedures. Both root at
`EntityFlagWordBehaviorEvaluate` through documented anonymous relays, and cite
named/ported callees `ObjectListEntityTypeResolve`, `EntityCreateWithPosition`,
`RouteStepCommandIssue`, `EntityTeardown`, `EntitySharedEligibilityCheck`,
`EntityStateReset`, `EntityActivityRequestSet`, `SceneActivityUpdateDispatch`,
`EntityActivityListRegister`, `EntityTransformPositionGet`, and
`CurrentLevelHeightGet`.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x44afb0 | ConfiguredTinyMonsterSpawnAndTeardown | Maps the parameter entity onto the `RockMonster`/`Lavamonster`/`Icemonster` type lists through `ObjectListEntityTypeResolve` (matched by the entity's coordinate pair), selecting the `TinyRM`/`TinyIM` replacement name for `RockMonster`/`Lavamonster` kinds and `TinyIM` for `Icemonster`; then resolves the replacement type entry, reads the entity position via `EntityTransformPositionGet`, and up to four times calls `EntityCreateWithPosition(kind, type, cell, 0, x, y, random % 7)`, zeroes the new entity's `+0x340` word to `-1.0`, and retries — tearing the spawn down with `EntityTeardown` when `RouteStepCommandIssue` fails and otherwise raising flag word `+0xf9` bit `0x20` and returning. Sole static call chain: call site `0x44bffb` inside relay `FUN_0044bfd0`, itself dispatched by call site `0x43cfd6` inside `EntityFlagWordBehaviorEvaluate`. |
| 0x44b250 | PairingActivityRaiseAndMutualLink | Requires `EntitySharedEligibilityCheck(param_2, 1)` with the `+0x3e4` flag words clear of pairing bits (`param_2` bits `0x100`/`2`, `param_1` bit `1`) and `FUN_0043baf0(param_1)` empty; when `FUN_00469d70(param_1)` is under the `+0x340` threshold it runs `FUN_0041d010(param_2)`, resets both entities via `EntityStateReset(..., 0, 1)`, dispatches `EntityActivityRequestSet(37, 1)` for `param_1` and `EntityActivityRequestSet(38, 1)` for `param_2` each followed by `SceneActivityUpdateDispatch`, registers both through `EntityActivityListRegister`, sets `param_1`'s pairing bit `1` and `param_2`'s bit `2` at `+0x3e4`, and links the pair mutually at `+0x3c0`. Sole static call chain: call site `0x445a57` inside relay `FUN_004456e0`, dispatched by call site `0x43ee37` inside `EntityFlagWordBehaviorEvaluate`. |

**Evidence.** `docs/evidence/legorr-engine-helpers-expansion-29.bundle` —
batch decompile record
`ev_d4647dc0f66e7d02dd910bbd1bd88a44d04613beefcebe719c6320e071c4b9b2`
(2 addresses) plus `procedure_callers` records
`ev_2dc3e993…` (`0x44afb0`) and `ev_8f23f6ab…` (`0x44b250`). The
grounding call sites come from `procedure_references`
(direction: incoming) cross-references, which resolve the relay chain quoted
above. All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** `0x44fab0` is the last live candidate of the 900–6900-byte
band: its only static entry is relay `FUN_0044b7a0`, whose sole incoming
cross-reference is a call from an instruction address inside itself
(`0x44b879`), so no named root reaches it — it defers to the indirect-dispatch
pool with the data-reference-evidence group. The band scan is otherwise
exhausted: every remaining anonymous 900–6900-byte procedure is already named,
or sits in the deferred sets — `0x43fc20` (zero-caller `FUN_0041f6c0` roots),
the CRT-region helpers (`0x48e280`/`0x48e680`/`0x48e0ad`/`0x48e8e0`/
`0x48e4f0`/`0x48ed30`/`0x465c50`/`0x473560`/`0x489660`/`0x476e50`/`0x4887c0`,
plus the 0x493–0x49Bxxx cluster), the device-interface vtable trio
`0x47e300`/`0x485ea0`/`0x489b30`, the explicitly-unresolved `0x4802d0`/
`0x480390`/`0x480570`, and the carry-forward set from findings 242–256
(`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`,
`0x40b000`, `0x474760`, `0x483690`, `0x471630`, `0x46f770`, `0x435b20`, the
entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`).
