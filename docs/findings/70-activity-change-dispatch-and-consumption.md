# 70 — Activity-name changes select a typed runner, then commit the transition

Finding 10 identified the per-entity evaluator's activity tuple; this finding
traces the transition helper it calls. In the shipped image, `FUN_00447c40`
does not poll the activity-name table continuously: it returns immediately
when the current name pointer (`entity+0x2e8`) already equals the committed
pointer (`entity+0x2ec`). A different pointer enters a typed runner dispatch,
resets the activity timer, commits the new pointer, and clears one stats-state
bit. The concrete transition is below; activity-name meanings and the typed
runner internals remain partly unknown.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Function body `FUN_00447c40` is a contiguous 578-byte Ghidra
function at `0x447c40–0x447e81`; its decompilation and related helper bodies
are in `docs/evidence/legorr-activity-state-transitions.bundle`.

## Transition order

1. **Pointer identity gate.** If `entity+0x2e8 == entity+0x2ec`, return zero
   without running the transition path. This is pointer equality in the
   decompiled code, not a string-content comparison. Evidence:
   `ev_e12f91fe4461d24ccccd401071a87d3555ebb3e4290840472d6c94cf29730d24`.
2. **Optional command-chain insertion.** For a new activity name other than
   eight explicit table addresses (`DAT_004decc4`, `DAT_004decc8`,
   `DAT_004deccc`, `DAT_004decd4`, `DAT_004decdc`, `DAT_004dece4`,
   `DAT_004ded08`, `DAT_004ded1c`), call `FUN_00403f30`. That helper checks
   the entity's `+0x2f0` chain: if it is empty or its head kind is not `0xe`,
   it allocates kind `0xe`, links the old head through record `+0x60`, and
   installs the new record as the head. The condition and insertion are
   observed; the semantic names of the eight excluded activities are not
   inferred here. Evidence: `ev_e12f91fe4461d24ccccd401071a87d3555ebb3e4290840472d6c94cf29730d24`,
   `ev_cae076c7889d871fdf46dbe0813b8bfdebf151f1cacb7c61f734d6b3447a8f73`.
3. **Typed runner dispatch.** The entity kind at `*entity` chooses a separate
   helper pair. Kind 1 uses `FUN_0046c3f0` and `FUN_0046da50` with the entity's
   `+0x0c` object; kinds 2 and 3 use `FUN_00406e40` / `FUN_00406f80` with
   `+0x10` / `+0x14`; kind 4 uses `FUN_00408570` / `FUN_004085f0` with
   `+0x18`. Each receives the new activity pointer and the current activity
   timer (`+0x2d4`) or secondary value (`+0x2e0`). The returned secondary
   value is stored at `entity+0x2dc`; the primary helper's return value is
   returned by `FUN_00447c40`. These are observed dispatches, not decoded
   human-facing activity semantics. Evidence:
   `ev_e12f91fe4461d24ccccd401071a87d3555ebb3e4290840472d6c94cf29730d24`.
4. **Conditional attached-object follow-up.** The helper reads the entity's
   stats word through `FUN_00469e60` (`entity+0x334`, offset `+0x144`) and
   continues only when bit `0x20000` is set, `entity+0x36c` is non-null, and
   the dword at that object is `2`. On that branch it formats a buffer from
   the activity name and entity values, passes it with the object's `+0x10`
   field to `FUN_00406e40`, reads a value through `FUN_00406f80` using the
   object's `+0x2e0`, writes that result to object `+0x2dc`, and copies object
   `+0x2ec` to object `+0x2e8`. The observed gates and writes do not establish
   the object's domain name or what the formatted buffer represents. Evidence:
   `ev_e12f91fe4461d24ccccd401071a87d3555ebb3e4290840472d6c94cf29730d24`,
   `ev_2e15e65465d8dc412bbf95709983ecf9264e966259510e33e1d46ef4ca59d015`.
5. **Commit and reset.** After the dispatch and optional follow-up, the helper
   writes `0` to `entity+0x2d4`, copies `entity+0x2e8` to `entity+0x2ec`, and
   clears bit `0x20000` in `entity+0x3e4`. The current activity pointer is now
   the committed pointer for the next invocation; the transition-only bit is
   consumed. Evidence: `ev_e12f91fe4461d24ccccd401071a87d3555ebb3e4290840472d6c94cf29730d24`.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| A matching current/committed activity pointer returns early; a changed pointer enters transition processing | observed (`FUN_00447c40`) |
| Eight activity-table pointer values skip the `FUN_00403f30` chain insertion | observed; their meanings are not asserted |
| Kinds 1–4 select the listed runner helper pairs and object slots | observed (`FUN_00447c40`) |
| The follow-up requires stats-word bit `0x20000`, non-null `entity+0x36c`, and object dword `2`; it copies the object's `+0x2ec` to `+0x2e8` | observed (`FUN_00447c40`, `FUN_00469e60`) |
| The transition resets `+0x2d4`, commits `+0x2e8` to `+0x2ec`, and clears entity `+0x3e4` bit `0x20000` | observed (`FUN_00447c40`) |
| Human-facing meanings of activity entries, the type-2 object's identity, and full typed-runner behavior | unknown |
