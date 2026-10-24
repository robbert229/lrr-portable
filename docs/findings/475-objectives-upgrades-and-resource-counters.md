# Finding 475 — Objectives, Upgrades, and Resource Counters

This finding maps the 16 procedures assigned to objective-economy. Unless marked otherwise, descriptions are direct observations from the canonical pseudocode. Names such as “resource” and “objective” follow the function names and surrounding call paths; opaque fields and stat identifiers remain unresolved.

## Level resources and upgrade tables

`UpgradeResourceCheck @ 0x0041f900` returns level field +0xa0 for argument 0 or +0xac for any nonzero argument, or zero when no level object is active. `LevelResourceBalanceAddAndSound @ 0x0041f9a0` increments the corresponding field and plays sound 1 or 5. `UpgradeResourceDebit @ 0x0041f9e0` subtracts the requested amount and plays a related sound. These are direct reads/writes of the level fields; the precise resource names are not established in this slice. Resource checks and debits are consumed by upgrade and command paths; `SimulationEntityQueueFlush` calls the balance-add helper in one state transition.

`UpgradeTypesLoad @ 0x0042d460` counts `UpgradeTypes` configuration entries, allocates a 0x10-byte record per entry plus a pointer table, initializes each record from its configuration block, copies the entry name, and stores the corresponding name pointer. It returns failure if required allocations fail and success after loading the entries. `UpgradeNameEntryFind @ 0x0042cd50` performs case-insensitive lookup over that pointer table and returns the matching record at base + index*0x10, or zero. `LevelUpgradeNameStringsLoad @ 0x00427f80` reads the `UpgradeNames` configuration, accepts `Level...` keys whose parsed suffix is below 16, resolves their string values, and stores them in a 16-slot global table.

`BuildingUpgradeStateChangedEventDispatch @ 0x00418830` first applies helper checks and updates the building upgrade state when permitted. With its fourth argument zero, it stores the three selector values, looks up a selector-specific message for kinds 1, 2, and 4, resets a text buffer, sets global update bits, and calls `CategoryCellCandidateListBuild @ 0x00418920`. That helper builds parallel arrays of category/cell candidates accepted by another predicate. The selector table has no names in the decompile, so its meaning is not assigned here.

## Objective and level counters

`LevelStateCountersClear @ 0x0041a920` clears the level-state word and several counter arrays, with loop lengths based on vehicle/upgrade/stat counts, clears a 300-word block, and calls `ObjectiveSelectionStateReset`. The pseudocode shows which storage is cleared but does not name each counter. `ObjectiveSelectionStateReset @ 0x0041a990` clears the selected objective index, restores another field from its default, and sets bits 0x84 in the objective-state word.

`EntityQueryFlagCount @ 0x0041b3c0` returns the global count for selector 0x13; selector 0x14 scans vehicle records and counts entries passing either of two flag masks unless another per-record bit is set; selector 0x15 counts entries passing mask 0x800 or an alternate per-record bit. Other selectors index a pair of global counters. `CategoryValueIsNeitherZeroNorFifteen @ 0x0041b470` returns true exactly when its argument is neither 0 nor 15. `StatsLevelRecordCountForStatType @ 0x0042ef40` maps stat types 1–4 to the four category totals, type 7 to constant 2, type 10 to the upgrade count, and several listed types to constant 1; other values return type minus 1.

`IsAverageValueAbove99 @ 0x00418120` obtains a table, sums entries from index 8 to its count, and compares the integer average against 99. It returns false when no table is available. The divisor is `count - 8`; at count 8 the pseudocode divides by zero, and below 8 the unsigned subtraction wraps. Whether valid configuration can reach those sizes is unresolved. The code does not identify what the values measure.

## Runtime updates and UI state

`AirSupplyAmountUpdate @ 0x00424560` updates the global amount at +0x1fc unless the entity is kind 4 without bit 0x20000000. It adds frame time × 0.04 × the level value at +0x200 × an entity-derived factor to the existing +0x1fc amount. It clamps the result to 100; at zero, when global bit 0x8000000 is clear, it dispatches event 2. The field and event use air-related names elsewhere, but the arithmetic above is the direct evidence.

`ObjectiveScoreTextOriginSet @ 0x004262e0` stores a supplied x/y pair in globals and sets bit 0x4000 of `DAT_00556ed0`, or clears that bit on null. `LevelRecordTimerCountdown @ 0x0042c440` walks fixed-size 0x4c-byte records and subtracts frame time from each positive timer at record offset +0xc; it does not clamp after reaching zero.

## Caller and data flow

`LevelStateCountersClear` directly calls `ObjectiveSelectionStateReset`, tying the objective selection reset to level counter cleanup. `BuildingUpgradeResourceAdmissionCheck` uses both `UpgradeResourceCheck` results against configured thresholds; `BuildingUpgradeCommit` debits a resource and dispatches `BuildingUpgradeStateChangedEventDispatch`; `EntityActionRecordConsumeAndCleanup` adds a resource through `LevelResourceBalanceAddAndSound`. `GameConfigInit` calls `UpgradeTypesLoad` and `LevelUpgradeNameStringsLoad`, which populate separate lookup/display tables. `ObjectiveTick` calls `ObjectiveScoreTextOriginSet` to publish or clear the score-text position. `EntityQueryFlagCount` and `StatsLevelRecordCountForStatType` read the same global totals that `LevelStateCountersClear` uses as array loop bounds; it does not reset those totals. These call/data-flow links are direct; gameplay labels for individual counters remain inference or unresolved.

## Unresolved questions and follow-up slices

- Identify the resource represented by level fields +0xa0 and +0xac, and determine whether subtraction can underflow in valid callers.
- Name the counter arrays cleared by `LevelStateCountersClear` and the record flags counted by selectors 0x14/0x15.
- Trace upgrade purchase callers to establish how `UpgradeResourceCheck` and `UpgradeResourceDebit` are sequenced and what prevents insufficient balances.
- Resolve the selector/message mapping in `BuildingUpgradeStateChangedEventDispatch` and its subsequent notification callbacks.
- Identify the table entries averaged by `IsAverageValueAbove99` and the level-record types whose timers are decremented.

**Evidence.** Canonical pseudocode in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), procedures 0x00418120–0x0042ef40. The assigned procedures are listed in [function-areas.csv](../coverage/function-areas.csv) under `objective-economy`.
