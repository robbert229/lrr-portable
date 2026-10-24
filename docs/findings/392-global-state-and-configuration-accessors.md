# Finding 392 — Global State and Configuration Accessors

This finding maps the 44 assigned global/engine/level accessors and mutators. The names stay offset- or operation-based where the field's domain meaning is not established. The issue's area is a cross-section of storage helpers: several groups touch the current level state block and engine mode/flags, while others expose display, resource, callback, or generic global values.

| Address | Name | Observed behavior and context |
| --- | --- | --- |
| `0x00401bf0` | `GlobalField505f88Get` | Returns the global dword at `DAT_00505f88`; engine tick notes identify value 10 as loader/benchmark mode (Finding 03). |
| `0x00401c00` | `StartLevelOverrideIfEnabledGet` | If `DAT_00505f84` bit `0x200` is set, returns the address `0x00505f8c`; otherwise returns zero. The override's caller-level policy is documented in Finding 366. |
| `0x00401c20` | `GlobalFlagBit0IsSet` | Returns whether bit 0 of an observed global flag is set (Finding 362). |
| `0x00401c30` | `DisplayWidthGet` | Returns the configured display width used by centering and bounds callers (Finding 347). |
| `0x00401c40` | `DisplayHeightGet` | Returns the configured display height used by centering and bounds callers (Finding 347). |
| `0x00402170` | `LevelNoGatherObjectSetup` | Creates and links records for the NoGather level option during level loading (Finding 225). |
| `0x004065d0` | `GlobalDwordGet` | Returns `DAT_0076ac84` (Finding 365). |
| `0x00407340` | `StateFlagToggleWhenZero` | Toggles `DAT_00557860` between 0 and 1: zero becomes 1; any nonzero value becomes 0. The flag's domain meaning is unresolved. |
| `0x00407540` | `FlaggedObjectTimerRegistryInsert` | Inserts a qualifying object with flag `0x80` and a fixed timer into a 20-slot registry (Finding 312). |
| `0x00408930` | `ConstantZeroFloatGet` | Returns `0.0` (Finding 380). |
| `0x00409d40` | `LevelStateStoredValueSet` | Stores the supplied value in the global level-state slot; the field's domain remains unresolved (Finding 356). |
| `0x0040bdc0` | `SetTimeoutTo10000` | Calls the timeout setter with `10000.0`; caller `FUN_0042f0c0`, purpose unresolved (Finding 381). |
| `0x0040e7e0` | `GlobalFlagBit0Clear` | Clears bit 0 of the observed global flag (Finding 356). |
| `0x0040e910` | `GlobalValueClearIfEqual` | Clears the observed global value only when it equals the supplied value (Finding 356). |
| `0x0040fec0` | Provisional alias `RandomFloatRangeResultDiscard` (existing label `SetGlobalScalarToOne`) | Calls `RandomFloatRange(0, 1.0)` and discards its result; the body shows no persistent global write. |
| `0x0040ffe0` | `GlobalVectorSetAndNotifyListeners` | Stores four supplied global values and broadcasts the update through a callback; caller `FUN_0044bb30` (Finding 381). |
| `0x00410b30` | `GlobalPointerGet_004c76ab24` | Returns the pointer stored in `DAT_0076ab24`. |
| `0x00410b40` | `GlobalPointerGet_004c76ab28` | Returns the pointer stored in `DAT_0076ab28`. |
| `0x00410b50` | `GlobalPointerGet_004c76ab3c` | Returns the pointer stored in `DAT_0076ab3c`. |
| `0x0041b9b0` | `GlobalStateGet_004ddd1c` | Returns the global state pointer/value at `0x004ddd1c`; semantic type unresolved. |
| `0x0041f720` | `CurrentLevelStateFlagsContainMask` | Tests whether the current level-state record's `+0x3f0` word intersects the supplied mask (Finding 338). |
| `0x0041f870` | `GlobalObjectField8CIncrement` | If the global object exists, increments its dword at `+0x8c` (Finding 350). |
| `0x0041f880` | `GlobalObjectField8CSubtract` | If the global object exists, subtracts the supplied value from `+0x8c` (Finding 350). |
| `0x0041f8a0` | `GlobalObjectField98Add` | If the global object exists, adds the supplied value to `+0x98` (Finding 350). |
| `0x0041f8c0` | `GlobalObjectField98Clear` | If the global object exists, clears its dword at `+0x98` (Finding 350). |
| `0x0041f920` | `LevelStateField9CAdd` | Adds the supplied delta to `DAT_005560d4 + 0x9c`; the field's meaning is not established (Finding 338). |
| `0x0041f940` | Provisional alias `GlobalFlagBit20000SetOrClear` (`FUN_0041f940`) | Sets or clears `DAT_00556ecc` bit `0x20000` from the supplied boolean; the flag's domain meaning remains unresolved. |
| `0x0041f970` | `RuntimeFlag40000000Get` | Returns whether runtime flag bit `0x40000000` is set (Finding 376). |
| `0x0041f980` | `GlobalObjectField94Increment` | If the global object exists, increments its dword at `+0x94` (Finding 350). |
| `0x0041f990` | `GlobalObjectFieldA8Increment` | If the global object exists, increments its dword at `+0xa8` (Finding 350). |
| `0x00423080` | `ModeDependentField24Get` | Selects a value from one of two engine records at `+0x24` based on global mode; otherwise returns the mode value (Finding 295). |
| `0x004230b0` | `GlobalFloatField00556F00Get` | Returns global `0x00556f00`; meaning unresolved (Finding 295). |
| `0x00425cd0` | `Global004de548Set` | Stores the supplied value in `DAT_004de548` (Finding 324). |
| `0x00426230` | `GlobalTripleCopyOut` | Copies three global dwords to the caller's output buffer; semantic type unknown (Finding 295). |
| `0x00426360` | `GlobalStateFlagsBits6And9Set` | Sets or clears global flag bits `0x40` and `0x200` from two boolean inputs (Finding 295). |
| `0x004263a0` | `GlobalStateFlagsBit7Set` | Sets or clears global flag bit `0x80` (Finding 295). |
| `0x00429850` | `ActiveEngineFieldB0Get` | Returns active engine-state field `+0xb0` (Finding 295). |
| `0x00429860` | `EngineFloatField210GetByIndex` | Returns a float from the active engine state at `+0x210 + 4 * index` (Finding 295). |
| `0x00429880` | `EngineFloatField260Get` | Returns the active engine-state float at `+0x260` (Finding 295). |
| `0x0042b500` | `GlobalStateField84Get` | Returns `DAT_005560d4 + 0x84`; the field meaning is unresolved (Finding 324). |
| `0x00431560` | `CurrentLevelGet` | Returns the current terrain/map context pointer (Finding 226). |
| `0x00431570` | `CurrentLevelHeightGet` | Reads the dword at offset `+4` from the current terrain/map context (Finding 226). |
| `0x00435580` | `GlobalHandlePositionApplyIfPresent` | If the global handle exists, applies a zero-position update and returns success (Finding 386). |
| `0x00435a20` | `GlobalStateLowByteClearAndRefresh` | Clears the low byte of `DAT_00556ed4`, then calls a refresh helper with `DAT_005563b4` and mode 1 (Finding 329). |

## State connections and limits

The most coherent cluster is the engine/level state block rooted at `DAT_005560d4`: the engine tick describes it as the game/level block and identifies `+0x9c` as level time and `+0x8c/+0x98` as timers. Several assigned accessors read or update these offsets, and tick/level callers provide the state-lifecycle context (Finding 03). A separate mode/flag cluster controls engine execution and refresh behavior; the exact meaning of individual bits stays literal unless the caller establishes it. Current-level getters expose a context pointer and its `+4` value, which neighboring terrain/path helpers use as a bound or height-related value (Finding 226). The remaining globals and record offsets are intentionally not assigned semantic labels from their names alone.

All listed routines have bodies in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Related prior analyses include [engine tick and shutdown](03-engine-tick.md), [display and allocator wrappers](347-display-priority-and-allocator-wrappers.md), [current-level state and resource helpers](338-current-level-state-and-resource-helpers.md), [engine mode and state accessors](295-engine-mode-and-state-accessors.md), [object field helpers](350-low-address-string-and-global-field-helpers.md), and [viewport/global refresh helpers](329-viewport-and-global-refresh-helpers.md). This finding consolidates the assigned slice and distinguishes observed storage operations from interpretations that still depend on caller context. Potential follow-up slices include identifying the meanings of `DAT_005560d4` fields beyond established tick usage and tracing the three pointer globals to their owners.
