# Finding 389 — Linked Records and State Helpers

Issue #220 covers seven small routines that link global record lifetime,
selector-dependent helper behavior, scene-object state dispatch, and
callback-based predicates. Direct bodies are in the checked-in
[full decompile corpus](../decompiles/full/LEGORR.full.c). Operation-level
names are retained where the record's semantic type or callback effect is not
established.

| Address | Function | Observed behavior and callers |
| --- | --- | --- |
| `0x0046a370` | `LinkedRecordChainFree` | Recursively follows the record link at `+0x1c` to the tail, then frees each record through `EngineHeapFree` (`0x0048db80`). No cycle or depth guard is visible. `GlobalLinkedRecordChainClear` (`0x0046a390`) uses it for teardown. |
| `0x0046a390` | `GlobalLinkedRecordChainClear` | Clears `DAT_004aa450`, frees the chain rooted at `DAT_004aa458` through `LinkedRecordChainFree`, then clears the head. `LevelLoad` (`0x00429890`) calls it, establishing a level-transition cleanup path. |
| `0x0046a420` | Existing provisional label `SelectedGlobalPointTransformAndReturnX` | Selects `DAT_005560e4` when `DAT_00556ec8 == 1`, otherwise `DAT_005560ec`; its decompile then shows calls to `0x00431570` and `0x00436b33` before returning `local_c[0]`. Their inferred argument flow is unreliable: `0x00431570` is a zero-argument level-height accessor, while `0x00436b33` has a three-argument camera helper signature but is shown called with two arguments here. `SceneObjectStateDispatch` (`0x0046a510`) uses the returned word when its state flags request it, but the actual value/transform semantics remain unresolved. |
| `0x0046a470` | `LinkedListPushAtGlobalHead` | Writes the current `DAT_004aa458` head into the supplied record's `+0x1c`, then makes the supplied record the new head. `SceneObjectStateDispatch` is its observed direct caller. |
| `0x0046a4c0` | `FlagMaskCallbacksAllPass` | Iterates mask bits 0 through 30. For each set bit, resets `DAT_00503188` to 1, enumerates entities through `EntityPoolFindByCallbackFiltered` (`0x004377b0`) with that bit as callback data, and returns false if callback processing clears the shared flag. It returns true after all selected bits continue. `MainSimFeatureFlagLatch` (`0x00435a50`), `QuitPoll` (`0x004589c0`), and `ObjectiveTick` (`0x00458be0`) call it. The callback-specific meaning varies by caller and is not assigned here. |
| `0x0046a510` | `SceneObjectStateDispatch` | Computes a one-based ordinal from the first set selector bit via `FirstSetFlagOrdinal` (`0x0046a760`). When `param_2` bit 1 is set, allocates a 0x20-byte record, prepends it to the global chain, initializes fields including `+0x10 = 0` and `+0x1c = param_3`, and conditionally copies the `0x0046a420` return value and global fields based on `param_3` bits 1 and 0. It then enumerates entities through `0x004377b0` with callback `LAB_0046a3c0` and the local selector/flag tuple. Callers include `StartLevelPilots` (`0x0041fa80`) and `ObjectiveVoiceAliasPrepare` (`0x00458670`); the callback's per-entity effect remains unresolved. |
| `0x0046a700` | `LinkedRecordConditionalFloatAdvance` (source symbol `FUN_0046a700`) | Walks the `+0x1c` chain. For records whose `+0x10` field is nonzero and `+0x18` equals 2, updates float `+8` by `(+0xc) * delta`. `CameraMoveCommandRoute` (`0x0046a730`) calls it with the chain head and its input delta, then optionally enumerates entities using callback `LAB_0046a5c0` when `DAT_004aa450` is nonzero. The alias describes only this observed field operation; the record's semantic type and the gate's producer remain unresolved. |

## State flow and limits

`SceneObjectStateDispatch` is the observed record producer: its optional 32-byte
allocation is linked at `DAT_004aa458`. `CameraMoveCommandRoute` later walks
that chain and updates a selected float field for a subset of records. The
clear routine resets the associated global flag and frees the list during
`LevelLoad`. The exact meanings of fields `+8`, `+0xc`, `+0x10`, `+0x18`, and
`+0x1c` are not known beyond their use in these routines.

`FlagMaskCallbacksAllPass` is a different callback path: it queries the entity
pool once per set bit and uses a shared continuation flag as its result. Its
name describes the aggregation observed at the wrapper; individual predicate
semantics are owned by callback implementations and call sites.

Follow-up slices that would improve this map:

- Identify writers of `DAT_004aa450` and the lifecycle controlled by its guard.
- Recover the callback bodies at `LAB_0046a3c0`, `LAB_0046a490`, and
  `LAB_0046a5c0` to determine per-entity effects and mask meanings.
- Establish the allocation/record field contract and verify whether the
  recursive chain release can encounter cycles or pathological depth.
- Recover the actual call/argument flow in `0x0046a420` at instruction level;
  the decompiler's calls to `0x00431570`/`0x00436b33` conflict with those
  callees' signatures, so the selected value and transform remain unresolved.

## Evidence

The complete decompiles are in `docs/decompiles/full/LEGORR.full.c`. Retained
records include `ev_406f854beae3509e70f55a6de0059e1dc6f799193827e08c59a49cf99c18b865`
and `ev_bf10563ca590e00ab86202eff38cf9bc56ce2e2b6b1e232472f57c46cfde2f74` in
`legorr-engine-helper-audit-mid-expansion-5.bundle` for point/list and scene
dispatch helpers, and `ev_259bd804d24a294fbdf1af85667dcaefd555930f5d7ab8e1fb81af25c1867985`
in `legorr-engine-helpers-expansion-5.bundle` for the entity callback wrapper.
Existing findings 227, 230, 297, and 348 contain related summaries; this
finding consolidates the issue's seven assigned routines and state links.
