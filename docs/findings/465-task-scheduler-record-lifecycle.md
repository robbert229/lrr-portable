# Finding 465 — Task Scheduler Record Lifecycle

This area contains two linked work paths: global command/task records and per-entity command chains. The pseudocode shows record kinds, fields, list operations, and dispatch flow; where a numeric kind or flag lacks a recovered semantic name, it remains numeric.

| Address | Function | Observed behavior and important flow |
| --- | --- | --- |
| `0x004026c0` | `EntityDeferredWorkSchedule` | For an entity with field `+0x304 == 0` and flag `+0x3ec & 0x100` clear, scans through `EntityPoolCallbackSearch` (`0x00406540`) with `LAB_00402700`; if no match, calls `CmdCreateTask` (`0x00402720`). |
| `0x00402890` | `TaskType23CreateForCategoryMask` | After `FUN_00438ed0` accepts `(arg, mask)`, allocates global kind `0x17`, stores the entity/mask, maps masks `1/2/4,8,0x10,0x20,other` to command kinds `0xc,3,7,4,0xd`, optionally snapshots selection state, then links the record at the global-list head. |
| `0x00402a40` | `CoordinatePairCallbackDispatch` | Wraps its argument with a zero second word and passes the pair to entity-pool callback traversal using `LAB_00402a70`. |
| `0x00402aa0` | `ArgumentCallbackDispatch` | Traverses entities with `LAB_00402ac0` and the supplied context. |
| `0x00402b30` | `CoordinateCallbackDispatch` | Wraps one argument and zero in a two-word context and traverses entities with `LAB_00402c20`. |
| `0x00402b60` | `CoordinateCallbackDispatchByMode` | Uses the same two-word context but selects callback `LAB_00402c70` or `LAB_00402ca0` according to the mode argument. |
| `0x00402ec0` | `EntityConditionalTaskSchedule` | If entity type flags include bit 1, entity word `+0x3e0` lacks `0x400`, and `+0x3ec` lacks `0x10000`, allocates kind `0x1e`, sets entity flag `0x10000`, links it globally as command `0x11`, and applies record flag mask `0x100010`. |
| `0x00402f30` | `EntityThresholdProgressEventQueue` | Allocates kind 10, stores its argument and `0x10` at `+0x3c`, optionally sets flag `0x200000`, then links globally as command 7 when the boolean argument is false or `0x13` when it is true. |
| `0x00402f70` | `EntityTargetCommandQueue` | Allocates kind `0x11`, stores a payload, and appends it to the entity's command chain at `+0x2f0`, using `FUN_00404190` to locate the append position when the chain is nonempty. The recovered body for `0x00404190` only shows a link walk and does not recover the apparent return value used by callers. |
| `0x004030b0` | `CoordinatePairEventQueue` | Allocates kind `0xc`, stores a coordinate pair and payload at `+0x54`, then links globally as command 8. |
| `0x00403190` | `ObjectEventType2Queue` | Allocates kind `0x13`, stores the object and clears a related pointer, snapshots selection, sets the observed `0x10` record flag while clearing `0x40`, then links globally as command 2. |
| `0x00403320` | `ObjectCommandTaskCreate` | For kind-2 entities without `+0x3e0 & 0x400`, resolves a nearby object from world position; for kind 1, resolves its paired object. On success it creates kind `0x15`, stores the resolved object and caller payload, sweeps/relinks the entity chain, then appends using the apparent chain-tail result from `FUN_00404190`. |
| `0x004033f0` | `CoordinatePairTaskRecordQueue` | Allocates kind 0, stores a coordinate pair, applies record flag `0x40` while clearing `0x10`, assigns command kind `0x1a`, and appends globally. |
| `0x00403430` | `TaskRegistryRemoveType0EntryByPair` | Walks the `DAT_004b3358` global list and unlinks/frees the first kind-0 record whose pair at `+8/+0xc` matches the input. |
| `0x004034a0` | `ScalarTaskRecordQueue` | Allocates kind `0x1d`, stores the scalar, assigns command `0x19`, appends globally, then sets record flag `0x40`. |
| `0x004034e0` | `TaskPairRecordEnqueue` | Allocates kind `0x19`, stores a pair at `+8/+0xc`, assigns command `0x18`, and appends globally. |
| `0x00403520` | `TaskRecordKind10Queue` | Allocates kind 10, stores its payload, assigns command `0x16`, appends globally, then sets flag `0x40`. |
| `0x00403560` | `TaskRegistryRemoveType10EntriesForEntity` | Walks `DAT_004b3358`, unlinking and freeing every kind-10 record whose payload at `+0x10` matches the entity. |
| `0x00403650` | `EntityTaskBatchAppend` | For each supplied entity, allocates kind 1, stores the payload and a zero word, sweeps/relinks that entity's command chain, then appends using the apparent chain-tail result from `FUN_00404190`. |
| `0x004038c0` | `TimedTaskRecordInitializeAndLink` | Allocates kind 5, stores payload and three arguments, links globally as command 10, sets record flags `0x4c0` while clearing `0x10`, and returns the record. Used by `TaskRecordCreateAndScanGrid`. |
| `0x004039a0` | `TaskRecordCreateAndScanGrid` | Creates the timed kind-5 record with arguments `(payload,0,-1,0)`, assigns command 9, then calls `CellRecordCountsPruneForEntity` (`0x00403910`) to reconcile a fixed grid of per-cell counts. |
| `0x004039d0` | `TaskRecordKind11TimedQueue` | Allocates kind `0x11`, stores payload and zero, links globally as command 0, then sets flags `0xc0` while clearing `0x10`. |
| `0x00403a10` | `TaskCallbackDispatch` | Traverses entities using callback `LAB_00403a30` and the supplied context. |
| `0x00403af0` | `EntityTaskStateUpdateAfterCleanup` | For the entity's record at `+0x2f0`, clears flag bit 0 when the argument is zero; otherwise, if its head is kind 0, removes that head and for entity kind 3 queues event 7. |
| `0x00403b40` | `EntityPendingTaskFlagClear` | Clears bit 0 in the head task record's flag word at `+0x5c`, when an entity-chain head exists. |
| `0x00403ef0` | `PendingRecordCallbackDispatch` | Dispatches the supplied context across entities using `LAB_00403f10`. |
| `0x00403f60` | `TaskRecordKind14RemoveIfHead` | If the entity-chain head is kind `0xe`, removes it through `EntityCommandChainHeadRemove` (`0x004041e0`). |
| `0x004040a0` | `EntityTaskQueueFlush` | Unless global state `DAT_004b8060 & 6` blocks processing, removes the first kind-0 record through `EntityActivityRecordRemoveByType` (`0x00403c00`); when entity flag `+0x3e0 & 0x400` is set and the record at `+0x308` is kind 8, it also removes kind 9. It then sweeps/relinks the remaining chain and clears references to the entity in the global queue. |
| `0x004041b0` | `EntityTaskChainContainsKind` | Searches an entity's `+0x2f0` chain through `+0x60` links and reports whether any record has the requested kind. |
| `0x004063e0` | `PendingRecordEntityReferenceClear` | Walks the global head list `DAT_004b3354` through `+0x60` links and clears field `+0x48` wherever it references the given entity. |
| `0x00406400` | `PendingRecordCopyToEntityAndFlag` | Allocates a record of the input record's kind, copies 26 dwords while preserving the allocator's pool link, sets flag `0x200`, clears the new record's `+0x60` link, and returns it. Used by `EntityRecordAttachAndRelink`. |
| `0x00407360` | `GlobalTaskRefreshFlagGet` | Returns global `DAT_00557860`, which is toggled by the adjacent `StateFlagToggleWhenZero` helper and gates `ResetObjectAuxiliaryState`; the same flag alters type-2 entity handling in `TickEntityScreenEffects`. |
| `0x00407450` | `IdleQueueRegister` | For kind-2 entities, inserts the entity into a 20-entry pointer/timer table if absent and initializes its paired timer to `0x41480000` (12.5f). If the table is full it returns without insertion. |
| `0x00408360` | `TaskField84GetAndField88Out` | Returns the dword at object `+0x84`; if output is non-null, writes object `+0x88` there. |
| `0x00408970` | `TaskFlagBitQuery` | Computes `1 << (bit & 31)` against object `+0x98`; returns the bit mask when set, except a nonzero third argument forces zero for a set bit. |
| `0x00419e60` | `PendingRecordReleaseIfPresent` | If pending-record count `DAT_004dcd38` is nonzero, releases entry zero through `RecordListEntryRemoveAndRelease` (`0x00419e80`). |
| `0x00419f10` | `PendingRecordReleaseAll` | Repeatedly releases entry zero while `DAT_004dcd38` is nonzero; the removal helper compacts or decrements the fixed five-dword record table. |
| `0x0041eac0` | `EventTimerCountdownAdvance` | Advances a global timer using `param_1 * DAT_004ddcf0`. Depending on state bits, it clamps at `DAT_004ddce8` or `DAT_004ddce0`; the latter expiry path clears state bit `0x100`, submits sound/event calls, and branches on selector `DAT_004ddcfc` (`0x16`, `0x17`, or other) to update flags and global state. This is a specific state machine, not a general callback-list dispatcher. |
| `0x0041f3c0` | `QueueCallbackRequest390` | Uses `EntityPoolFindByCallbackFiltered` (`0x004377b0`) with a predicate to register an entity callback request associated with the observed `0x390` selector. |
| `0x0041f400` | `QueueCallbackRequest3E0` | Uses the same filtered pool walk to register the corresponding callback request for selector `0x3e0`. |
| `0x00425b40` | `PeriodicCallbackDispatch` | Walks eligible entities with `EntityPoolFindByCallbackFiltered` (`0x004377b0`) and a frame callback; its period and predicate remain encoded in the callback body. |
| `0x00426320` | `ObjectiveTaskTextOriginSet` | Stores a coordinate pair in `DAT_00556f24/28` and sets bit `0x8000` in `DAT_00556ed0`; null clears that enable bit. This is a UI consumer of task/objective state rather than a queue operation. |

## Observed scheduler structure

- `CmdAlloc` (`0x00406440`) pops a 26-dword record from `DAT_004b31f8`, clears its 26 dwords, restores its self-link at `+0x64`, stamps `timeGetTime()` at `+0x50`, and stores the numeric kind at offset 0. `CmdPoolGrow` (`0x004064c0`) replenishes the free list. `CmdFree` (`0x00406480`) optionally runs cancellation handling, releases owned data unless flag `0x200` says otherwise, then pushes the record to the free list.
- Global records use `+0x60` links and distinct heads: `DAT_004b3354` for `CmdLinkHead` (`0x00404030`) and `DAT_004b3358` for `CmdLinkTail` (`0x00404070`). Both initialize the timestamp-like `+0x14`; head linking stores a command-kind table value at `+0x1c`, while tail linking clears flag bits 0 and 4.
- Per-entity chains start at entity `+0x2f0`, use record `+0x60` links, and are swept by `CommandQueueSweepRelink` (`0x00404100`) into global lists or the free pool according to flags. `EntityDrainCommandChain` (`0x00403a50`) shows a second drain path guarded by `DAT_004b8060` bit 1.
- Queue producers use `CmdLinkHead`, `CmdLinkTail`, or append to entity chains; `EntityTaskQueueFlush` and the broader dispatch in `EntityCommandUnlinkByKind` (`0x00403d10`) consume/remove records and clear entity references. This connects the scheduler's global queue, per-entity work, and callback scans without establishing the intended meanings of every numeric kind.

## Unresolved slices

- Recover semantic names for the 26 record kinds, command-kind table at `DAT_004b32e8`, and flag bits in `+0x5c`; preserve the observed values meanwhile.
- Resolve the apparent return contract of `FUN_00404190`: callers use its result to append after a task-chain tail, but the recovered function body/signature does not expose a return value.
- Identify the complete global queue tick/dispatch order, including how `DAT_004b8060` gates drain, flush, and removal operations.
- Recover the callback predicates and context layouts for the `LAB_00402a70`, `LAB_00402ac0`, `LAB_00402c20`, `LAB_00402c70`, `LAB_00402ca0`, `LAB_00403a30`, and `LAB_00403f10` walks.
- Determine the meanings of timer globals and selector values in `EventTimerCountdownAdvance`; determine whether the `IdleQueueRegister` table's 12.5f timer is reset elsewhere.
- Establish the ownership and lifetime relationship among the pending-record list, entity chain head, and copied records created by `PendingRecordCopyToEntityAndFlag`.
- Trace objective text-origin state to its renderer and identify the condition that consumes/clears `DAT_00556ed0 & 0x8000`.

## Related findings

- Finding 366 records earlier task-record queue helpers and field accessors.
- Finding 369 covers queue flush and entity task state cleanup helpers.
- Finding 286 maps task registry and category-mask helpers.
- Finding 312 maps object task and registry helpers.
- Finding 401 documents the distinct engine callback-record allocator; do not conflate it with `CmdAlloc`'s 26-dword pool.

Direct evidence is in [the full decompile](../decompiles/full/LEGORR.full.c). Function names and offsets above follow recovered pseudocode; semantic labels beyond explicit call relationships remain open questions.
