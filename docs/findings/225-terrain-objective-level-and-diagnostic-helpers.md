# Finding 225 — Newly Identified Terrain, Objective, Level, and Diagnostic Helpers

The added names below come from Ghidra pseudocode and their callers in the shipped
reference/LegoRR.exe. Full generated pseudocode is in
../decompiles/engine-region.c. Names describe observed behavior; they do not
establish broader engine semantics beyond the listed call sites.

## Terrain and map helpers

| Address | Added name | Observed behavior |
| --- | --- | --- |
| 0x440c70 | TerrainGridPathSearch | Validates start and target cells, then searches four neighboring cells using the terrain-cell predicate. It tracks visited cells, reconstructs a route, and can instead stop when a supplied callback accepts a cell. |
| 0x44f640 | MapCellToWorldCenter | Bounds-checks a cell against map dimensions and converts its indices to the center X/Z coordinates using map scale and origin. |
| 0x44f940 | TerrainSurfaceHeightInterpolate | Locates the terrain tile under an X/Z position, chooses a tile triangle, and interpolates its corner heights. It returns zero if tile lookup fails. |
| 0x40ce80 | MapCellStateBufferRebuild | Frees and recreates a per-cell state buffer sized from the map dimensions, binds it to the map, and clears related state slots. |

## Level and simulation helpers

| Address | Added name | Observed behavior |
| --- | --- | --- |
| 0x402170 | LevelNoGatherObjectSetup | Creates and links records for the NoGather level option; called from level loading. |
| 0x40bf70 | RockFallStyleNameLookup | Performs a case-insensitive lookup of a configured RockFallStyle name and returns its table index. |
| 0x40e930 | ErosionTimingStateInit | Converts parsed erosion timing values to engine ticks and resets the erosion state arrays. |
| 0x4191a0 | MainSimStateAdmissionPredicate | Returns true only when its active-state predicate and two global state bits are set while SystemStateFlags(0) is clear. The exact higher-level state represented is unresolved. |
| 0x41c440 | AdvisorCallbackScheduleGate | Under two global flag checks and an advisor-state byte, schedules LAB_0041c470 through the callback queue. The callback’s role is not established here. |
| 0x425ce0 | ScrollScaleSetAndClamp | Updates the scroll scale through the slider quantizer, then clamps the stored value to the range 0–3. |
| 0x428800 | ActionFeedbackSoundDispatch | Maps event codes 0–5 to sound cue IDs, applies the paired cue adjustment, and restores the previous sound state. |
| 0x46b7c0 | TimedEffectAndCueUpdate | Advances timed effects and their cue/event state from the engine tick path. |

## Objective text and type configuration

| Address | Added name | Observed behavior |
| --- | --- | --- |
| 0x4262e0 | ObjectiveScoreTextOriginSet | Sets or clears the score-text origin and its draw flag. Objective tick callers use (470, 315). |
| 0x426320 | ObjectiveTaskTextOriginSet | Sets or clears the task-text origin and its draw flag. Objective tick callers use (130, 315). |
| 0x4574e0 | ObjectiveTextOverlayLoad | Parses objective/completion/failure text and mission-briefing text from the objective configuration, preparing the corresponding text surfaces. |
| 0x46c6f0 | VehicleTypeDefinitionParse | Parses a vehicle-type definition, including its wheel-mesh fields, from configuration. |
| 0x46eb80 | WeaponTypeConfigLoad | Enumerates and loads configured weapon types during startup. |

## DirectX error reporting

DirectXResultDiagnosticDispatch (0x489dd0) switches on the DirectX interface
category and HRESULT. It sends known DirectDraw and Direct3D Retained Mode errors
to a shared reporter with the symbolic error name and message, then returns the
original result. Unknown categories and codes use generic diagnostic messages.

**Evidence:** [legorr-engine-helpers-expansion.bundle](../evidence/legorr-engine-helpers-expansion.bundle)
contains the Ghidra batch-decompile observations for these functions.

