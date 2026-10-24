# Finding 468 — Command Queue and Selection State

This finding maps the fixed-size action queues, their frame dispatch, and the selected-entity array and state transitions. The evidence is the canonical pseudocode in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c), with callers in the same corpus. Function names are descriptive labels; addresses identify the procedures. State meanings below are limited to the observed reads, writes, and call sites.

Related context: [Finding 246](246-entity-move-route-command-and-object-clone-helpers.md), [Finding 256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), and Finding 467 in a concurrent open PR, which covers command creation and routing.

## Action buffering and dispatch

`ActionCommandQueueAppend @ 0x00452010` appends an action entry to the opcode-byte array rooted at `DAT_004ff0bc` and the parallel five-dword argument array rooted at `DAT_004ff0c8`, using `DAT_004ff29c` as the shared count. It stores three scalar values and an optional coordinate pair. This is the interface-action buffer consumed at the beginning of `SelectionRoot`, not the larger selection-operation ring.

`KeyStateGatedCommandEnqueue @ 0x00452060` appends a five-dword operation record to one of two banks at `DAT_004eaf20`, each with capacity `0x800`; `DAT_004fef28` selects the active bank and the associated count is at `DAT_004fef20 + bank*4`. It drops an operation when the active bank is full. It zero-initializes the record before storing opcode/arguments and an optional coordinate pair. The function name's “key state” aspect is not implemented as a visible key test here; `SelectionRoot` checks the per-opcode enable byte at `DAT_0076ab80` when draining the separate action buffer.

`SelectionRoot @ 0x004520d0` coordinates both queues. Unless global bit `0x100000` is set, it drains enabled action-buffer entries into the operation queue. It then invokes `FUN_0045d870` for each queued record, saves the count, clears the active bank, flips the bank selector, and clears the newly active count before dispatching the saved operations. The opcode switch routes to selection filters, clear/remove, `SelectionCommandDispatch`, command constructors, placement helpers, and other subsystems. Thus handlers enqueueing new work during dispatch target the other bank and do not extend the current operation pass. The switch advances by `0x14` bytes per record. `CommandQueueCancelOpcode @ 0x00451f60` searches only the current bank for a matching second field and removes matches by copying the last record over the removed slot; it does not preserve ordering, and its forward index increment means a swapped-in match is not rechecked at that same slot.

`CaseInsensitiveNameTableLookupAtOffset8 @ 0x00451fd0` scans the table beginning at `DAT_004ff190`, compares each record's string at `+8` case-insensitively, and returns the table index on a match. When no match occurs it returns the final comparison result rather than an explicit sentinel; callers should not assume this helper returns `-1` for a miss without further evidence.

## Selection storage and candidate intake

The active selection is a compact array rooted at `DAT_004fef2c`, with count `DAT_004ff298` and an observed maximum of 100 accepted candidates in `SelectionCandidateFilter`.

| Function | Observed behavior |
| --- | --- |
| `SelectionSnapshotCreate @ 0x00452520` | If the selection count is nonzero, allocates `count * 4` bytes, copies the entity pointers, returns the copy and count through outputs, and returns `1`; for an empty selection it returns `0` without writing an empty snapshot. The snapshot's release path is not established here. |
| `ActiveContextRecordGet @ 0x00452580` | Returns the active array pointer and optionally its count when nonempty; otherwise stores count zero and returns null. |
| `SelectedEntitySetHasAny @ 0x004525b0` | Tests whether the count is nonzero. |
| `SelectedEntity @ 0x004525c0` | Returns the first selected pointer when nonempty, else zero. |
| `SelectionRecordArrayGet @ 0x004525e0` | Returns the array base even when empty. |
| `SelectionRecordCountGet @ 0x004525f0` / `SelectionRecordCountReset @ 0x00452600` | Read the count / set it to zero. Resetting the count alone does not clear entity flags. |
| `SelectionContainsEntity @ 0x00452610` | With a nonzero second argument, searches the active array; with zero, tests entity word `+1000` bit `0x200000`. |
| `ArrayFindValueIndex @ 0x00452650` | Searches a pointer-sized integer array, optionally returns the matching index, and reports success as a boolean-like value. |
| `SelectionCandidateAlreadySelectedOrMarked @ 0x00452690` | Returns true if the entity is already selected or its word at `+0x3ec` has bit `2`. |

`SelectionCandidateEligibilityCheck @ 0x00452870` requires entity flag word `+0xfa` bit `0x100`, a successful `FUN_004385b0(entity,1)`, and clear bits in words `+0xf9` and `+0xf8`; type `1` has an additional `+0xf8` bit `0x40000` rejection. `SelectionCandidateFilter @ 0x004526e0` first follows `+0xc1` when nonzero, then applies that eligibility test. Depending on its second argument, it may reject already-selected/marked candidates after calling `FUN_0044c4a0`; otherwise it inserts a not-yet-selected candidate while the count is below 100, sets selected bit `0x200000` in word `+0xfa`, and refreshes entity activity state. For a qualifying type-2 entity it can also reset its action state and issue a turn action. A final flag-gated branch updates a visual resource and returns success. `SelectionCandidateFilterDefault @ 0x004526c0` calls this filter with mode `1`.

`EntityRandomValidCellCommandCreate @ 0x00452430` obtains grid dimensions from two helper lookups, tries up to 1000 random coordinate pairs, and, if `FUN_00432b50` accepts one, queues operation `0x24` with the supplied entity and chosen coordinates. This is a producer for the operation dispatcher; it does not add an entity to the selection array itself.

## Selection state transitions and consumers

`SelectionStateTick @ 0x004528c0` classifies the current selection by entity kind and flag combinations. The resulting classes drive one HUD context update through `CommandContextPostToHud @ 0x0041b2d0` (contexts `9`–`0x0e` are visible in the calls). A second pass removes entries that do not satisfy the chosen class conditions and clears their selected bit. The code establishes a selection-wide compatibility filter and context choice, but the semantic names of the individual classes and all flag combinations remain unresolved.

`SelectionClear @ 0x00452be0` walks the selected array. For entries carrying selected bit `0x200000`, it marks word `+0x3ec` bit `2`; it then clears selected bit `0x200000` and word `+0x3ec` bit `1` for each entry and sets the active count to zero. The mark bit is intentionally distinct from the selected bit because candidate filtering consults both.

`SelectionEntityRemove @ 0x00452c50` searches the selection array, updates HUD context to `0` when removing index zero, removes the entry by moving the last pointer into its slot, and clears that entity's selected bit. It returns `1` only when it found and removed the entity.

`SelectionObjectDetach @ 0x004524b0` combines command cancellation and selection cleanup: it calls `CommandQueueCancelOpcode` and `SelectionEntityRemove` for the object. It also clears `DAT_00556160` and issues a reset operation when the object matches that global pointer; if it matches the entity stored at `DAT_005560e8 + 4`, it clears that field, records the entity's world position, and sets global selection-dirty bit `0x4000`.

`SelectedEntityStateMonitor @ 0x00452cc0` performs entity cleanup for the selected state: type-2 entities with a nested entity have bits cleared on the nested entity; type `4` calls `FUN_00409ff0`; other entities call `FUN_00437540`, with an extra `FUN_0040d0a0` for type `0x0b`. Its loop variable counts down from the selection count, while the checked-in pseudocode repeatedly references the base symbol `DAT_004fef2c` rather than visibly advancing an array pointer. This unusual shape is recorded as an unresolved decompile/correctness question, not generalized into a claim that every selected entity is processed.

`SelectionCommandDispatch @ 0x00452d60` checks a primary object at `DAT_00556160` first, then selected entries. A candidate must have flag byte `+0xfa` bit `0x10` and satisfy a type/nested-entity condition. For a type-2 entity with a nested entity it dispatches using that nested pointer; otherwise it passes the entity itself to `FUN_004295f0` with the supplied operation and returns `1`. It returns `0` when no eligible candidate exists. The helper's downstream meaning is outside this function.

## Main flow and limits

The direct flow is: interface handlers append actions or enqueue operations; `SelectionRoot` applies enabled actions, advances the double-buffered operation queue, and dispatches each opcode; selection handlers update the pointer array and per-entity flags; `SelectionStateTick` derives a compatible selection class and HUD context; selection removal/detach paths clear array membership and selected state. `SelectedEntity` and `ActiveContextRecordGet` provide read APIs used by UI and simulation callers.

Several helper names and integer opcode values are anchored by the checked-in name map and call sites, but the complete action table, allocation ownership for snapshots, semantic labels for selection classes, and the return contract of the case-insensitive lookup miss remain open. `CommandQueueCancelOpcode`'s swap-removal is unordered by construction; whether skipped swapped-in matches are expected depends on the queue's duplicate policy, which is not established by this function.

## Focused follow-up slices

- Trace all producers of the action buffer at `DAT_004ff0bc` and document opcode names, queue capacity, and overflow behavior.
- Identify the owner/free path for arrays returned by `SelectionSnapshotCreate`.
- Reconcile `SelectedEntityStateMonitor`'s repeated base-symbol access with the executable call/data evidence.
- Determine the intended miss return for `CaseInsensitiveNameTableLookupAtOffset8` and how callers validate it.
- Map the selection-class flag matrix in `SelectionStateTick` to named entity/activity concepts.
