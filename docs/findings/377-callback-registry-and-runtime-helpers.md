# Finding 377 — Callback, Registry, and Runtime Helpers

This pass names 36 operation-level routines across callback state, registry/resource handling, entity transitions, rendering, and C runtime support. Numeric flags and opaque resource meanings remain explicit.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x00408b00` | `PointToPolylineMinimumSegmentDistance` | Computes distance to the first 2D segment, scans the remaining segments at 8-byte stride, and returns the minimum. | Pseudocode in `LEGORR.full.c`; no matching direct function dossier found. |
| `0x00408d70` | `TerrainDescriptorCoordinateCountLimitCheck` | Counts matching coordinates and compares the count against a limit derived from type and descriptor flags. | Pseudocode only; type/flag meanings unresolved. |
| `0x0040a3d0` | `ScreenEffectFlagsClearAndRelease` | For each registry entry with bit 0 set, clears it and calls a release/disable helper. | Shutdown caller `FUN_0042f0c0`; caller record `ev_b21891371f1dae9eb62d40041b699d2418fab0409135e779d014e4124e97c94d` in `legorr-ui-render-fourth-pass-2026-10-23.bundle`. |
| `0x0040ab30` | `GridRecordFlagMaskClear` | Walks a rectangular record block and clears mask 0x10c from each word. | Shutdown caller `FUN_0042f0c0`; caller record `ev_b21891371f1dae9eb62d40041b699d2418fab0409135e779d014e4124e97c94d` in `legorr-ui-render-fourth-pass-2026-10-23.bundle`; field meaning unknown. |
| `0x0040f7d0` | `CallbackStateRecordCreate` | Allocates a 0x28-byte record, stores a helper result, and initializes status fields from two booleans. | Caller `FUN_0044b680`; pseudocode only. |
| `0x0040f850` | `CallbackStateListDispose` | Frees each callback-state entry, then frees the list head/array. | Caller `FUN_0044bae0`; pseudocode only. |
| `0x0040fdb0` | `ThreeValueRecordCopyAndMarkReady` | Copies three dwords into a pointed-to record and sets owner field 4 to 1. | Caller `FUN_0044bc30`; `ev_22b7dd721d06f3dbb8d15b4490de5941d810cd4b29f29eeb2de0b9cbd42113ca` in `legorr-deferred-simulation-pass-2-2026-10-23.bundle` (caller context). |
| `0x00410280` | `CallbackOperationRunAndReturnStatus` | Initializes local status to 1, invokes a callback dispatcher, and returns the possibly updated status. | Caller `FUN_0044bf00`; `ev_7a603d1ec3ab8db2d2dbe53320f73539e7fcbfb8bf4d10dc1ea21b295966d448` in `legorr-deferred-simulation-pass-2-2026-10-23.bundle` (caller context). |
| `0x00410b60` | `ThreeWordRecordAllocateAndInitialize` | Allocates 12 bytes and stores three supplied words. | Caller `FUN_004140c0`; `ev_374f086328b72e3e39b6e5396da2ef15e6652bb06cacf1cfd80aacdffb21ba9b` in `legorr-frontend.bundle` (caller context). |
| `0x00410b90` | `BoundedFloatRecordCreate` | Returns null unless param_3 exceeds param_2, then allocates a seven-word record containing both floats and five other arguments. | Caller `FUN_004140c0`; `ev_374f086328b72e3e39b6e5396da2ef15e6652bb06cacf1cfd80aacdffb21ba9b` in `legorr-frontend.bundle`; range semantics unresolved. |
| `0x00417280` | `RegistryValueLookupWithLock` | Looks up a key, performs paired unlock/cleanup, and returns the found value or zero. | Pseudocode only; key/value domain unknown. |
| `0x004172d0` | `RegistryValueIndexLookupOrNotFound` | Uses the preceding lookup to return the found entry’s first dword or 0xffffffff. | Caller `FUN_00416d70`; `ev_98b42e56cccb8211f6e9c2a8006ba3ef7447fc481616aa75a6799223a11a91b1` in `legorr-frontend.bundle` (caller context). |
| `0x004189d0` | `RenderTransitionCleanupIfActive` | When global bits 1 and 0 are both set, invokes two cleanup helpers and clears bit 0. | Pseudocode only; flag meanings unresolved. |
| `0x00438720` | `EntityAvailableResourceSpend` | Checks whether spending is allowed, verifies available amount, sends message 0xe on shortage, otherwise deducts the amount. | Pseudocode only; resource meaning unknown. |
| `0x0043bbc0` | `EntityDetachCarriedObjectAndResetState` | For a type-3 object with a type-6 attachment, clears link fields/flags, resets floats, releases a secondary resource, and sets an owner flag. | Pseudocode only; exact types and flag meanings uncertain. |
| `0x004437f0` | `EntityDispatchEvent31ForPendingFlag` | For a type-3 entity matching observed flag conditions, sets bit 0x20000000, sends event 0x31, and calls a helper. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00444620` | `EntityEnterEvent21StateOnce` | When state guards allow, starts/stops a helper, sets a flag, sends event 0x21, then runs helper/finalizer. | State semantics unresolved. |
| `0x00447730` | `EntityProgressAdvanceBy25` | Adds 25 to field +0x344, clears +0x2fc, clamps to +0x340, and returns 1 when exceeded. | Field meanings unknown. |
| `0x00447b00` | `EntityDispatchEvent5AndSetFlag` | Sends event 5, calls a helper, and sets entity flag bit 0x200000. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x004487c0` | `EntityClearNestedState8Fields` | When owner bit 0x400 is set and nested record type is 8, clears fields +0x2f8/+0x2f4 and calls an owner helper. | Pseudocode only; flag/type meanings unknown. |
| `0x0044d9a0` | `VectorWithinAxisTolerance` | Returns true when each coordinate differs from a global vector by no more than its corresponding global tolerance. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00451bc0` | `ResourceListUpdateMatchingKey` | Traverses a linked list, matches key +0xc, lazily creates/caches a resource, then updates it using fields +0 and +0x10. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00459530` | `RenderThreePassUpdate` | Calls three render/configuration subpasses with the same two arguments. | Exact subpass meanings unresolved. |
| `0x0045abb0` | `InputDeviceSetFlag20` | Sets or clears bit 0x20 in a selected device/slot flag field. | Device table semantics inferred; numeric bit meaning preserved. |
| `0x00463e40` | `FloatRoundToNearestInt` | Rounds a float to nearest integer using floor/fraction comparison and a round-up helper. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00466750` | `GridCellInvalidateAndClearFlags` | Marks a grid cell’s entity dirty, clears entity bit 0x02000000, and clears cell record bit 0x100. | Pseudocode only; flag meanings unknown. |
| `0x00496530` | `CrtCodePageLocaleIdLookup` | Maps code pages 0x3a4/0x3a8/0x3b5/0x3b6 to fixed locale values and returns zero otherwise. | Called while switching code-page tables; locale interpretation inferred from values. |
| `0x00496590` | `CrtCtypeTablesReset` | Zeros 0x101 bytes of character classification data and clears code-page/locale state fields. | Called by code-page switching on unavailable/invalid paths; pseudocode only. |
| `0x00497040` | `CrtStreamReadByte` | Handles EOF/error flags, refills a FILE-like buffer when empty, returns the next byte, and updates pointer/count. | Pseudocode and stream-reader caller context only. |
| `0x00497570` | `CrtStreamBufferFlushAndRelease` | Flushes an internally owned buffer when flagged and optionally clears its pointer, capacity, and ownership state. | Pseudocode and caller `FUN_0048fed0` context only. |
| `0x00499710` | `WideStringLength` | Returns the length of a NUL-terminated 16-bit string. | Pseudocode; used by a wide-string no-conversion path. |
| `0x00499730` | `___crtMessageBoxA` | Exact VC5 `LIBC.LIB` `crtmbox.obj` entry; the MSVC build links this public runtime helper, while portable builds retain the local implementation. It lazily resolves `MessageBoxA`, selects an owner window when available, and displays the supplied runtime message. |

| `0x00498f50` | `CrtStreamUngetByte` | MSVC x86 calls VC5 `LIBC.LIB`'s `_ungetc` directly (100% Reccmp); portable builds retain the local stream-state implementation. | The runtime owns the target stream pushback entry. |

| `0x004965c0` | `CrtAnsiCodePageTablesInit` | Calls the shared code-page initializer with sentinel -3, which resolves to the active ANSI code page. | Pseudocode only; no direct evidence bundle record was found. |
| `0x004975d0` | `CrtCharacterToLowerLocale` | Lowercases ASCII when locale handling is disabled; otherwise uses ctype and the configured locale/code page. | Pseudocode only; no direct evidence bundle record was found. |
| `0x00498ae0` | `CrtStreamBufferAllocate` | Allocates a 4 KB stream buffer and falls back to a two-byte inline buffer on allocation failure. | Pseudocode only; no direct evidence bundle record was found. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/legacy/engine-region.c). Where no direct record was found, the table says so and the name follows checked-in pseudocode and caller behavior.
