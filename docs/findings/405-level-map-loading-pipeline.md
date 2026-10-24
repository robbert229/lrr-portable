# Finding 405 — Level and Map Loading Pipeline

This finding maps the 15 functions assigned to `level-map-loading` and connects them to the deeper format and terrain analysis in [Finding 07](07-level-loading.md). The runtime flow has three stages: select/tear down a level, read its config-backed inputs and materialize a grid, then expose the loaded context to gameplay and rendering. The 15 functions are all already present in the coverage ledger; this is a documentation and cross-reference update.

## Entry, orchestration, and level lifetime

| Address | Function | Observed role |
| --- | --- | --- |
| `0x0042f0c0` | `PendingLevelName` | If a level context exists, releases its owned level resources and clears level-scoped flags/state, then sets the global context pointer to null and returns the saved pending-name value. `KeepRunning` uses this result to decide whether to start a load (Finding 06). This routine is a teardown/reset gate as well as a pending-name accessor. |
| `0x00429890` | `LevelLoad` | Allocates and zeroes the 0x284-byte level state, installs it globally, then reads level-group config values and map paths. It prepares runtime parameters (including timer/upgrade values), resolves auxiliary object types, loads the object/entity lists and script-related resources, and calls `MapLoadPipeline`; further consumers are detailed in Findings 07, 08, 115, and 117. |
| `0x0042b510` | `MapLoadPipeline` | Creates terrain/render state, opens the configured SurfaceMap to establish width/height, allocates and initializes `width * height` records of stride `0x48`, then applies layer loaders. It requires the predug and erosion/cryore loaders to succeed; later layer loaders are called in order, terrain-map success sets a state flag, and a full `CellInitPass` runs before terrain finalization and grid staging. It returns success/failure to `LevelLoad`. |

`LevelLoad` reads map paths from the level's config group; the dimensions come from the first map image, not separate config values. Each grid-layer loader checks its image dimensions against that established width/height. The `.map` image is read as a whole, with its header and row-major 16-bit cells accessed in place (`MapOpen2`, `MapFileDimensionsGet`, and `MapFileCellGet`); see Finding 07 for the format and the complete voxel layout.

## Layer ingestion and level-specific records

| Address | Function | Observed role |
| --- | --- | --- |
| `0x0042bd20` | `LoadPredugLayer` | Reads the predug grid and updates per-cell flag words; qualifying edge-wall cells also invoke the cell update path that marks neighboring cells. The write of walkability-related `0x10` through that update is traced in Finding 07; deferred coordinate-pool behavior is in Finding 224. |
| `0x0042bf40` | `LoadEmergeLayer` | For each supplied cell value, increments zero or odd values before storing `value >> 1` at record byte `+0x06`. Even nonzero values instead take a random branch that also calls `FUN_0040ea10` with the cell coordinates and a selector in the 0–3 range. |
| `0x0042c060` | `LoadLayerM` | Reads a dimension-matched grid into byte `+0x34` of each `0x48` cell record. |
| `0x0042c120` | `LoadLayerL` | Reads a grid whose nonzero values index a dynamically grown array of `0x4c`-byte records. Each cell is marked as the first entry or one of five additional entries; Finding 109 traces the `EmergeMap` caller and record-slot encoding, while downstream meaning remains unresolved. |
| `0x0042c330` | `EmergeMapTriggerSpawn` | Searches that `LoadLayerL` record array for a matching coordinate pair with an expired/nonpositive timer, tries its populated entries through the emerge spawn path, and on success resets the timer and optionally returns the selected entry's coordinates. |
| `0x0042c6a0` | `LoadPathAiLayers` | Reads a grid and subtracts the supplied bias byte before storing each result at cell byte `+0x05`. |
| `0x0042c760` | `LoadFallinLayer` | Reads fall-in values and updates cell flag state according to the configured bias and neighboring-cell eligibility. Successful fall-in handling also initializes terrain/cell state through helper calls. |
| `0x0042c9d0` | `LoadBlockPointers` | Reads per-cell counts into `+0x3c`, derives a per-cell threshold from the initialized seed at `+0x20` and `FallinMultiplier`, and for counts above four stores the excess count and sets the associated flag at `+0x38`. Sets the global “block pointer map loaded” flag only after the complete dimension check/scan succeeds. |
| `0x0042cc90` | `LoadTerrainMap` | Reads the terrain/material grid, subtracts the supplied bias, and stores the result at cell byte `+0x04`; it returns success only for a readable map with matching dimensions. |

The row-major cell index is `y * width + x`. `MapLoadPipeline` initializes the record grid before the layer loaders run, so the layer functions write into shared level state rather than building independent grids. The load pipeline's ordering and field offsets are summarized in Finding 07; interpretation of several flag bits remains partial and is called out there.

## Map and type lookup helpers

| Address | Function | Observed role |
| --- | --- | --- |
| `0x0042e8b0` | `ObjectListEntityTypeResolve` | Case-insensitively matches a name against the configured object/type-name table and returns its type discriminator, index, and optionally record pointer. It also has explicit special-name cases (including TV camera, power crystal, and processed ore); callers include `LevelLoad` and the object-list parser (Finding 226). |
| `0x004319e0` | `MapFileCellGet` | Obtains the mapped image and dimensions, then returns the 16-bit cell at `image + 0x10 + 2 * (y * width + x)`. The recovered accessor does not perform explicit coordinate bounds checks. |
| `0x004334c0` | `MapFileDimensionsGet` | Reads the width and height dwords at mapped-image offsets `+8` and `+0xC`. Layer loaders compare these values with the active level dimensions before iterating. |

## End-to-end flow and limits

```text
KeepRunning
  └─ PendingLevelName: tear down/reset prior level and return pending name
      └─ LevelLoad: allocate level state, read config group and resources
          └─ MapLoadPipeline: create terrain, establish map dimensions,
              allocate cell records, apply layers, initialize each cell,
              finalize terrain and stage the cell grid
```

- The split between `LevelLoad`'s config/object/script setup and `MapLoadPipeline`'s spatial layers is visible in the call structure. Finding 07 supplies the full config-key list, `.map` header/cell format, field table, and `.blx` unused-code conclusion.
- `MapLoadPipeline`'s required success gates are terrain creation, texture-set setup, cell allocation, and successful predug and erosion/cryore loads. The other layer-loader results are not used as gates in the recovered caller: `LoadTerrainMap` only conditionally sets `state+0x110`, after which cell initialization and finalization continue. Their absence or failure should not be described as preventing the whole pipeline from returning success.
- The recovered `MapFileCellGet` body indexes without checking coordinates. Callers generally iterate dimensions obtained from the same map image, but the helper alone does not establish safe behavior for arbitrary coordinates.
- `EmergeMapTriggerSpawn`'s record layout is clear from writes and reads, but the semantic relationship between its six slots and gameplay trigger types is not fully established (Finding 109).
- Follow-up slices, if needed: determine the semantic contract and consumer of the six-entry `LoadLayerL` records; document failure/cleanup behavior for partial `LevelLoad` and layer-load failures; audit arbitrary-coordinate callers of the map accessor.

## Evidence and related findings

Primary decompiles: `docs/decompiles/full/LEGORR.full.c` at the function addresses in the tables. Existing detailed analyses: Findings 06 (load decision), 07 (level config, map format, cell layout, layer data), 08 (save/checkpoint state), 109 (emerge records), 115 (PTL resource), 117 (fall-in multiplier), 224 (predug coordinate pool), and 226 (object-list type lookup).
