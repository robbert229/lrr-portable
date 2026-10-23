# Finding 366 — Task Records, Callback Dispatch, and Pointer Stacks

This pass names task-record queue/cleanup helpers, generic callback dispatch, a pointer stack, and small state/vector accessors. Numeric record and command kinds remain literal.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00401bf0` | `GlobalField505f88Get` | Returns the global dword at DAT_00505f88. |
| `0x00401210` | `AdvisorSceneVisualStateInitialize` | Passes the observed global value to two display helpers and writes the observed float constant to an adjacent global. |
| `0x004033f0` | `CoordinatePairTaskRecordQueue` | Allocates a task record, copies the coordinate pair to offsets +8/+0xc, sets the observed flag and command kind 0x1a, and appends the record. |
| `0x004034a0` | `ScalarTaskRecordQueue` | Allocates a task record, stores the scalar at +0x10, sets command kind 0x19, queues it, and sets flag 0x40. |
| `0x00403520` | `TaskRecordKind10Queue` | Allocates kind 10, stores the argument, sets command kind 0x16, queues the record, and sets flag 0x40. |
| `0x004038c0` | `TimedTaskRecordInitializeAndLink` | Allocates kind 5, stores four arguments, links it into the observed list, and applies the observed flag masks. |
| `0x004039a0` | `TaskRecordCreateAndScanGrid` | Creates a task record with fixed arguments, assigns command kind 9, and calls the grid-scan helper. |
| `0x004039d0` | `TaskRecordKind11TimedQueue` | Allocates kind 0x11, stores the argument, initializes the record through a helper, and sets mask 0xc0. |
| `0x00403ef0` | `PendingRecordCallbackDispatch` | Dispatches the supplied argument through the fixed pending-record callback. |
| `0x00403f60` | `TaskRecordKind14RemoveIfHead` | Removes the entity’s linked-list head through the removal helper only when the record kind equals 14. |
| `0x0040fe40` | `PointerStackPush` | When both pointers are nonzero, links the item to the current head through item offset +0x9c, updates the head, and increments the count. |
| `0x0040fe60` | `PointerStackPop` | When the count is nonzero, returns the head, advances it through item offset +0x9c, and decrements the count. |
| `0x0040fe90` | `PointerStackForEach` | Traverses the item chain through offset +0x9c and invokes the supplied callback with container, item, and context. |
| `0x0040fcc0` | `VectorScaleComponentsInPlace` | When the float pointer is nonzero, multiplies its three components by the three supplied scale values. |
| `0x00408360` | `TaskField84GetAndField88Out` | Returns the dword at object offset +0x84 and optionally writes the dword at +0x88 to the output pointer. |
| `0x00408970` | `TaskFlagBitQuery` | Tests the selected bit in the dword at +0x98; when the third argument is nonzero, a set bit is reported as zero. |
| `0x0040f5e0` | `GlobalConfigValueSet` | Stores the supplied value in DAT_004a1ee4. |

Direct pseudocode and caller context are in the full corpus. Related evidence includes `legorr-ui-render-fourth-pass-2026-10-23.bundle` (`ev_de1f37015f1140f0ab5457c0da167edc5e5e2f6bd6f280471746e831dc32faa3`) for the advisor initialization caller; `legorr-engine-helpers-expansion-23.bundle` (`ev_f566111ade0b7a764f73c8d4fee8476cf2dd41c90333b5e9913307a009d3df7d`) for task-record callers; and `legorr-wake-riding.bundle` (`ev_4125e49b75c80c4243e6cdc29adb8eb72c27657124533b0ad6041a1a7c4bdcb7`) for direct pseudocode of `0x00403f60`.

The complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Numeric task kinds and unresolved field semantics remain literal.
