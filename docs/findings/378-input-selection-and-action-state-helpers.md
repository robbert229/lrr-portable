# Finding 378 — Input Selection and Action-State Helpers

This pass names 23 operation-level routines across input bindings, selection commands, and entity action state. Numeric bit and action labels remain literal where their broader meanings are unresolved.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x0040ce30` | `SelectionMarkerResourcesRelease` | Frees each non-null pointer in the marker resource table, resets its count, and frees the table descriptor. | Marker role inferred from neighboring routines/global pool; pseudocode only. |
| `0x0041c7b0` | `InputActionStateUpdate` | Sets or clears per-action state bit 0, with mode-specific clearing after an input helper. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041c8f0` | `InputActionPressedGet` | Queries an action bit with global override and alternate-state handling. | Used by `0041cab0` and input consumers; pseudocode/caller context. |
| `0x0041c950` | `InputBindingPressedGet` | Queries pressed state for binding types 1/4 with global override and alternate-state handling. | Used by `0041caf0` and other input callers; pseudocode/caller context. |
| `0x0041caf0` | `InputBindingValueGet` | Selects a binding value from current or alternate arrays using active/pressed state. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041cb90` | `InputBindingDefaultValueGet` | Returns a default value from action/binding-index tables using binding ID and mode. | Pseudocode and callers at 00416d70/0041?; semantics stay operation-level. |
| `0x0041f100` | `ObjectInteractionFlagsAllow` | Maps entity flags 0x1000/0x800/0x2000/0x400000 to requested gate bits and calls a predicate helper; returns 1 when unrestricted. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041cea0` | `SelectionObjectCommandUpdate` | When selection mode is 12, scans selected objects and issues command 10 or 11 according to an object flag. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041cf20` | `SelectionTargetCommandUpdate` | Under modes 10/11/18, scans selected objects for a target ID and issues command 12 or updates multi-select. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041cf80` | `SelectionMode7CancelAtPoint` | In selection mode 7, compares the selection point and invokes the clear helper on a match. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0041cfe0` | `SelectionMode5CancelAtPoint` | In selection mode 5, compares the selection point and invokes the clear helper on a match. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x0043a7c0` | `EntityActionKindSupported` | Returns true for supported action kinds when entity bit 8 is set and each kind’s prerequisite passes. | Caller gate for `0043a650`; caller context in `legorr-engine-helpers-expansion-22.bundle`, `ev_e0c58e5bb04ee83c0327854fd9e50249b5fc8c65c5e199787622b70bf41060d8`; action meanings partial. |
| `0x0043a850` | `EntityActionRecordConsumeAndCleanup` | For parent type 3 updates counters/side effects by action kind, clears action field +0xc1, calls an update helper, and conditionally cleans up the parent. | Caller context as above; state/action fields remain opaque. |
| `0x004351a0` | `ActionCodeToSoundAlias` | Maps selected action codes to sound selectors and calls the sound helper. | Pseudocode only; caller context is in checked-in corpus. |
| `0x00436ae9` | `SelectionRecordPointGet` | For record type 1, reads two coordinates through the object at offset +0x28. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x00436b0b` | `SelectionRecordPointSet` | For record type 1, writes two coordinates through the object at offset +0x28. | Checked-in pseudocode in `LEGORR.full.c`; no direct analysis bundle record reported. |
| `0x004388b0` | `EntityActionKindPrerequisiteCheck` | Checks entity kind/capability and sampled resource/random thresholds before allowing an action kind. | Pseudocode/caller state-switch context; no direct evidence bundle record found. |
| `0x00439e40` | `EntityRecordFlag20Query` | When nested record bit 0x20 is set, delegates to an entity query helper. | Pseudocode only; domain-specific probe meaning is uncertain. |
| `0x0043b320` | `EntityIterationCallbackGuard` | Sets global bit 2, runs a callback iteration, then clears bit 2. | Pseudocode only; caller and bit meaning unresolved. |

| `0x00491770` | `CRTHeapReleaseUnusedPages` | Walks CRT arena blocks, releases free 4 KiB pages with VirtualFree, updates metadata, and deletes fully empty blocks. | Caller record `ev_d227513ebbbb1f052dc971b739b2ffd312415a6132718da6144b3e4cb3143c2a` in `legorr-engine-helper-audit-text-io-expansion-9.bundle`; body pseudocode in full.c. |
| `0x00491840` | `CRTHeapLocateAllocationPage` | Finds the arena containing a pointer, validates alignment/page-header offset, and returns page metadata. | Caller record `ev_708075d5c448664d72ab28e97bac07cf967342d827fdadc497d6fdbf8b3cff11` in the same bundle; body pseudocode in full.c. |
| `0x004850e0` | `ObjectField10Get` | Returns the dword at object offset +0x10; callers pair it with +0x0c as dimensions for render rectangles. | Pseudocode and caller context; specific resource type unknown. |
| `0x00485160` | `TextOutputField82CBit2SetClear` | Sets or clears bit 2 at object offset +0x82c. | Pseudocode and caller context; bit meaning unknown. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/engine-region.c). Where no direct record was found, the table says so and the name follows checked-in pseudocode and caller behavior.
