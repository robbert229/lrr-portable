# Finding 457 — Entity Stats Configuration and Record Accessors

This issue's 63-function area has three distinct responsibilities: load the per-level `Stats` configuration into indexed records, expose those records and an entity's currently selected record, and provide small field readers used throughout simulation and UI code. Evidence below is the checked-in decompile in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c) at each listed address; direct pseudocode supports the operations, but the original field names and most field meanings remain unknown.

## Configuration and indexed records

`EntityStatsConfigLoad @ 0x004667e0` iterates the entries under a `Stats` configuration section. For each entry whose level name resolves through `FUN_0042e8b0`, it reads the configured `Levels` value, allocates a level/type table if absent, and allocates a zeroed array of 0x150-byte records sized from that value. It then populates record fields from named keys including `RouteSpeed`, drill-time keys, `BuildingBase`, `CostCrystal`, damage/radius/time fields, and related stats. Missing scalar values are parsed from the empty-string fallback; selected fields apply explicit defaults (including `WakeRadius` 20 and random-move time 200). This is evidence of a configuration loader and record layout, not a complete schema: key-to-offset mapping is visible in the assignments, but several values are derived through helpers and the meaning of many offsets is not established.

`IndexedValueAppend @ 0x00469a90` appends a value to one of four grouped tables at `DAT_005030d8`, using the corresponding count in `DAT_005030e4`, then increments that count. The loader calls it after processing all entries with `(group,value)` pairs `(0,8)`, `(1,12)`, `(2,11)`, `(3,10)`. `GroupedCodeTableFindValueIndex @ 0x0046a120` searches each group's populated prefix, returns success and the group index through its output pointer on the first match, and returns failure otherwise. The four grouped tables therefore provide an exact value-to-group lookup; semantic labels for the groups are not established here.

The indexed-record readers at `0x00469ac0`, `0x00469af0`, `0x00469b20`, `0x00469e00`, `0x00469ec0`, `0x00469ee0`, `0x00469f00`, `0x00469f60`, `0x0046a0c0`, and `0x0046a0f0` address a record by type and level indices in `DAT_00502bd8`, then read a fixed field offset. The variants with a third argument add `index * 0x150`; the others read the first record. They expose offsets `b0`, `b4`, `b8`, `dc` (float), `144`, `148`, `14c`, `d0`, `fc`, and `100` respectively.

`EntityBuildingIndexSet @ 0x00469c10` validates a requested per-entity record index against the configured count in `DAT_00502c28`. On success it increments one counter for the old selection and another for the new selection when they differ, stores the selected index at entity offset `0x330`, caches that record's address at `0x334`, and returns 1. An out-of-range index returns 0 without changing these fields. `EntityIndexedRecordWordAtOffsetBCGet @ 0x00469b50`, `EntityIndexedRecordWordAtOffsetE0Get @ 0x00469b80`, `BuildingDepositCountGet @ 0x00469bb0`, and `EntityIndexedRecordWordAtOffsetF0Get @ 0x00469be0` read offsets `bc`, `e0`, `ec`, and `f0` from the selected record, using the entity's type/level indices plus selected-record index at offset `0x330`.

## Entity field accessors and flag update

The remaining entity-record readers are direct offset accessors through `entity + 0x334` (the offset is relative to that record pointer). The complete address-to-offset map is:

| Address | Function | Read |
| --- | --- | --- |
| `0x00469cb0` | `EntityComponentFloatAt20` | float `+0x20` |
| `0x00469cc0` | `TerrainTypeMovementSpeedGet` | float `+0x24 + index*4` |
| `0x00469ce0` | `EntityComponentFloatAtD4` | float `+0xd4` |
| `0x00469d00` | `NestedFloatAtOffset9CGet` | float `+0x9c` |
| `0x00469d20` | `EntityComponentFloatAtD8` | float `+0xd8` |
| `0x00469d40` | `NestedFloatAtOffset6CGet` | float `+0x6c` |
| `0x00469d50` | `NestedFloatAtOffset74Get` | float `+0x74` |
| `0x00469d60` | `NestedFloatAtOffset78Get` | float `+0x78` |
| `0x00469d70` | `NestedFloatAtOffsetE8Get` | float `+0xe8` |
| `0x00469d90` | `NestedFloatAtOffset70Get` | float `+0x70` |
| `0x00469da0` | `NestedFieldAddressAtOffsetA0Get` | address `+0xa0` |
| `0x00469db0` | `NestedFloatAtOffset7CGet` | float `+0x7c` |
| `0x00469dc0` | `NestedFloatAtOffset80Get` | float `+0x80` |
| `0x00469de0` | `EntityEnergyDecayRateGet` | float `+0x84` |
| `0x00469e20` | `NestedFloatAtOffsetDCGet` | float `+0xdc` |
| `0x00469e40` | `NestedWordAtOffsetACGet` | dword `+0xac` |
| `0x00469e60` | `EntityStatsWordGet` | dword `+0x144` |
| `0x00469e80` | `EntityStateFlagsGet` | dword `+0x148` |
| `0x00469ea0` | `EntityCapabilityFlagsGet` | dword `+0x14c` |
| `0x00469f20` | `ScreenShakeFlagRaise` | float `+0x88` |
| `0x00469f90` | `NestedWordAtOffsetC0OrC4Get` | dword `+0xc0` if selector is zero, else `+0xc4` |
| `0x00469fc0` | `FUN_00469fc0` | dword `+0xc8` |
| `0x00469fe0` | `NestedFloatAtOffset8CGet` | float `+0x8c` |
| `0x0046a000` | `Entity334FloatField90Get` | float `+0x90` |
| `0x0046a020` | `Entity334FloatField98Get` | float `+0x98` |
| `0x0046a040` | `Entity334FloatField94Get` | float `+0x94` |
| `0x0046a060` | `PermittedActivityCapacityCheck` | dword `+0xcc` |
| `0x0046a080` | `Entity334FloatFieldF4Get` | float `+0xf4` |
| `0x0046a0a0` | `EntitySpeedComponentGet` | float `+0xf8` |
| `0x0046a170` | `Entity334FloatField130Get` | float `+0x130` |
| `0x0046a190` | `Entity334FloatField134Get` | float `+0x134` |
| `0x0046a1b0` | `Entity334FloatField120Get` | float `+0x120` |
| `0x0046a1d0` | `Entity334FloatField128Get` | float `+0x128` |
| `0x0046a1f0` | `Entity334FloatField124Get` | float `+0x124` |
| `0x0046a210` | `Entity334WordField13CGet` | dword `+0x13c` |
| `0x0046a230` | `Entity334FloatField12CGet` | float `+0x12c` |
| `0x0046a250` | `Entity334FloatField138Get` | float `+0x138` |
| `0x0046a270` | `EntityCollisionRadiusGet` | float `+0xe4` |
| `0x0046a290` | `Entity334FloatField114Get` | float `+0x114` |
| `0x0046a2b0` | `Entity334FloatField118Get` | float `+0x118` |
| `0x0046a2d0` | `Entity334FloatField11CGet` | float `+0x11c` |
| `0x0046a2f0` | `Entity334FloatField110Get` | float `+0x110` |
| `0x0046a310` | `Entity334FloatField10CGet` | float `+0x10c` |

Offset-preserving names are deliberate: the values' domain meanings are not proved by the accessor bodies. `EntityCollisionRadiusGet` has stronger caller evidence: `EntityCentersWithinObjectRadius @ 0x00472130` uses it in a distance comparison (see Finding 357). `TerrainTypeMovementSpeedGet` indexes a per-entity float array using its second parameter.

The indexed-record readers `EntityStatsWordGet @ 0x00469e60`, `EntityStateFlagsGet @ 0x00469e80`, and `EntityCapabilityFlagsGet @ 0x00469ea0` read offsets `0x144`, `0x148`, and `0x14c` via the current entity record pointer. The related indexed readers at those offsets take explicit type and level indices instead. `NestedWordAtOffsetC0OrC4Get @ 0x00469f90` selects offset `0xc0` when selector is zero and `0xc4` otherwise; `FUN_00469fc0` returns the word at offset `0xc8`.

`EntityRecordFlag400ToggleAndRefresh @ 0x0046a330` toggles only bit `0x400` in the selected record's word at `+0x148`, then calls `FUN_0043c570` on the entity in either branch. The target name `EntityStateFlagsGet` describes the accessor, but the meanings of other bits in this word are unresolved. See Finding 367 for this toggle and other entity field use.

## Admission and other small helpers

Despite their assigned names, `BuildingUpgradeAdmissionCheck @ 0x00469f40` and `PermittedActivityCapacityCheck @ 0x0046a060` are raw readers: the former returns a dword from `DAT_00502c28` at `type * 0x3c + level * 4`; the latter returns the dword at entity-record offset `+0xcc`. The actual admission decision is performed by callers. `BuildingUpgradeAdmissionCheck` is called in the upgrade path documented by Finding 99; its name should not be read as proof that it performs a check itself.

`EntityStatsConfigLoad` also calls `StringTableIndexLookup @ 0x00408c80` for configured `BuildingBase` names (Finding 393). Other configuration/decompile relationships and previously documented local behavior are cross-referenced in Findings 348, 357, and 367. `EntityEnergyDecayRateGet @ 0x00469de0` reads offset `0x84`; `ScreenShakeFlagRaise @ 0x00469f20` actually reads float offset `0x88` and does not raise a flag in its body. These existing labels are therefore only provisional names, not reliable semantic descriptions.

## Limits and follow-up

The decompile establishes field offsets, allocation strides, control flow, and the exact operations above. It does not establish the complete `Stats` schema, the semantic names of all record fields, config reload/free ownership, or what each selection counter means. The implementation slices most likely to benefit from follow-up are (1) documenting `Stats` key-to-offset mapping with source config examples and (2) tracing ownership/reload lifetime of `DAT_00502bd8`; neither should be implemented from this static evidence alone. Existing findings already cover the specific upgrade decision and several offset accessors, so this finding consolidates the assigned area without duplicating those claims.
