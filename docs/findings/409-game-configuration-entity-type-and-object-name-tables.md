# Finding 409 — Game Configuration Entity Type and Object-Name Tables

The entity-type configuration path is a staged table build inside
`GameConfigInit` (0x0041fb50). Several per-type loaders were already described
individually in findings 226, 227, 238, and 243; this pass connects their
initialization order, shared storage pattern, and downstream object-name lookup.

| Address | Name | Observed behavior and relationship |
| --- | --- | --- |
| `0x0042cda0` | `VehicleTypeInit` | Counts `VehicleTypes` entries, allocates a `0x1ec`-stride definition array plus the parallel name-pointer array at `DAT_005561e8`, then walks the entries again. Each row is parsed by `VehicleTypeDefinitionParse` (`0x0046c6f0`), then `FUN_0046d2c0` sets the row's `+0x1e8` state bit and applies mode 1 through `SceneRenderLockToggle` to the scene handle at `+8` and, when present, the second handle at `+0xc`. The config entry name is copied alongside the row. |
| `0x0042cf50` | `MiniFigureTypeTableLoad` | Counts `MiniFigureTypes`, allocates `0x74`-stride rows plus a parallel name-pointer array, and parses each entry through `TypeTableResourceRowLoad` (`0x00406990`). Successful rows pass through `FUN_00406c90`, which applies mode 1 through `SceneRenderLockToggle` to the row's scene handle at `+4`; each entry name is copied into the parallel array. |
| `0x0042d100` | `RockMonsterTypesLoad` | Uses the same `0x74` row layout, shared row parser, `FUN_00406c90` scene-handle mode call, and parallel-name pattern as the mini-figure loader, against the `RockMonsterTypes` section and its own count/table globals. |
| `0x0042d2b0` | `BuildingTypesLoad` | Counts `BuildingTypes`, allocates `0x14c`-stride records at `DAT_005561e0` plus a parallel name-pointer array at `DAT_005561f4`, and parses each entry through `BuildingTypeConfigRecordParse` (`0x00407d60`). Successful rows pass through `FUN_00408670`, which applies mode 1 through `SceneRenderLockToggle` to the handle at `+4` and, when present, the second handle at `+0x7c`; entry names are copied alongside the records. |
| `0x00406990` | `TypeTableResourceRowLoad` | Per-entry parser shared by mini-figure and rock-monster rows. It resolves the entry's `.ae` data and visual, fills the `0x74`-stride row with configured null names/frame values and related model data, then builds the row's poly-variant model lists. See Finding 243 for the field-level trace. |
| `0x00407d60` | `BuildingTypeConfigRecordParse` | Zero-initializes the `0x14c`-byte building record, loads its configured visual data, and parses its attachment-null, pivot, shape, and power-level fields. It returns failure when the required visual load fails. See Finding 238 for the field-level trace. |
| `0x0042d600` | `ObjectNamesLoad` | Builds parallel string-handle arrays for vehicle, mini-figure, rock-monster, building, and upgrade entries. For each table it uses the copied type name to find the matching key in `ObjectNames`, then stores the value processed by `FUN_00477590`. It also loads named object labels such as `PowerCrystal`, `ProcessedOre`, `Dynamite`, `Barrier`, and `ElectricFence` into dedicated globals. `ObjectTheNamesLoad` (`0x0042da20`) is a separate subsequent loader. |

## Initialization and failure gates

`GameConfigInit` runs `UpgradeTypesLoad`, `VehicleTypeInit`,
`MiniFigureTypeTableLoad`, `RockMonsterTypesLoad`, and `BuildingTypesLoad` in a
short-circuit success chain. A zero return skips the remaining configuration
work inside that branch. Each table loader first counts section entries and
allocates storage from that count; a zero-entry section does not reach its
second traversal and returns zero. During the second traversal, a parser
failure frees the parallel name array and the record array instead of reporting
success. This makes the presence and successful parsing of the configured type
sections a startup dependency, rather than optional empty tables.

The object-name step occurs later in the same initializer, after the type
loaders have populated the per-type name arrays. `ObjectNamesLoad` is `void`, so
its outcome is not part of the earlier loader success chain. The decompile also
shows a distinct upgrade-name path: when an upgrade label lookup succeeds, the
slot is assigned the processed value, followed by an unconditional zero store
to that same slot. The reason for that write is unresolved; callers or data
would need to establish whether upgrade labels are intentionally left empty.

## Evidence and related findings

Complete Ghidra pseudocode for the seven assigned functions and the
`GameConfigInit` call sequence is in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Supporting direct-decompile
and caller records include
[`legorr-engine-helpers-expansion-2.bundle`](../evidence/legorr-engine-helpers-expansion-2.bundle)
(rock-monster, building, and object-name loaders),
[`legorr-engine-helpers-expansion-3.bundle`](../evidence/legorr-engine-helpers-expansion-3.bundle)
(mini-figure loader),
[`legorr-engine-helpers-expansion-15.bundle`](../evidence/legorr-engine-helpers-expansion-15.bundle)
(shared type-row parser and its callers), and
[`legorr-engine-helper-audit-simulation-expansion-10.bundle`](../evidence/legorr-engine-helper-audit-simulation-expansion-10.bundle)
(building parser, loader, and caller relationship). The earlier field-level
traces are in [Finding 226](226-omitted-simulation-terrain-resource-text-procedures.md),
[Finding 227](227-newly-identified-engine-and-terrain-helpers.md),
[Finding 238](238-reward-terrain-and-simulation-helpers.md), and
[Finding 243](243-config-loader-file-descriptor-and-crt-helpers.md).
