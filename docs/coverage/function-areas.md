# Function areas

This first-pass map assigns every one of the **2564** functions in the full manifest to one of **92** bounded responsibility areas, organized under 8 broader domains.
It classifies where each function belongs; it does not claim every function is fully understood or ported. Target: shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

## How to read the map

- [`function-areas.csv`](function-areas.csv) contains one row per manifest address, with its domain, area, basis, confidence, and any function-specific exception note.
- Area boundaries describe what responsibility is included and where it stops. A function can sit near a boundary; `classification_note` preserves a narrower exception when needed.
- **Basis**: `body` means the pseudocode body supports the assignment; `name+body` uses both; `name` uses the human alias; `import-symbol` uses an import/runtime symbol; `address-cluster` relies on a surrounding cluster; `unresolved` means the role remains opaque and the assignment is provisional.
- **Confidence** describes confidence in area placement, not completeness of behavioral understanding: `high` = the area is clear; `medium` = plausible placement with some ambiguity; `low` = provisional placement that needs follow-up.
- Port/reconstruction status remains in [`ledger.csv`](ledger.csv). Address partitions used during review are not semantic boundaries.

Assignment confidence: 702 high, 1830 medium, 32 low. Basis counts: address-cluster 1, body 50, import-symbol 15, name 1036, name+body 1439, unresolved 23.
23 functions with opaque pseudocode are provisionally grouped by surrounding subsystem. Three CRT no-op stubs have their own `runtime-support-unresolved` area.

## Broader domains

| Domain | Scope | Functions | Areas |
| --- | --- | ---: | ---: |
| `gameplay-simulation` | Entity behavior, commands, objectives, tasks, combat, simulation state, and progression. | 597 | 23 |
| `world-and-terrain` | Terrain cells and grids, spatial queries, placement searches, and route transitions. | 332 | 12 |
| `player-interface` | Front end, input interpretation, menus, HUD, advisor/help, and display-facing controls. | 363 | 14 |
| `scene-and-rendering` | Scene objects and resources, models, viewports, render surfaces, and visual effects. | 488 | 13 |
| `audio-and-video` | Sound, spatial playback, streams, and video playback or decoding. | 155 | 5 |
| `game-data-and-assets` | Configuration, map and level loading, archives, registries, and presentation/resource records. | 153 | 7 |
| `platform-and-runtime` | Win32/DirectX interfaces, startup/window services, CRT, and operating-system/runtime support. | 316 | 11 |
| `shared-primitives` | Cross-cutting state accessors, lists/strings, pools, math, and small reusable helpers. | 160 | 7 |

## Bounded areas

### gameplay-simulation

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `combat-effects-projectiles` — Combat Effects and Projectiles | 18 | Entity motion scaling, impulse/raycast hit scans, effect selection, spawn/flight records, and surface/effect query support. |
| `command-issue` — Command creation and issue routing | 20 | Gameplay command construction and routing from object, cell, selection, coordinate, or target inputs; excludes execution of queued tasks. |
| `command-queue-selection` — Command Queue and Selection | 25 | Queued action commands, selection root and selected-entity state, candidate filtering, selection changes, and selection command dispatch. |
| `command-registry` — Command registry and command-chain lifecycle | 21 | Command-kind registration, global command ticking, command allocation/release, chain linking, traversal, purge, and cancellation. |
| `entity-activity` — Entity activity and runtime records | 43 | Per-entity activity/task state, activity-list membership, activity callbacks, payload setup/teardown, and entity record linkage. |
| `entity-activity-actions` — Entity Activities and Action Completion | 42 | Activity state, action prerequisites and completion, carrying/deposit behavior, timers, entity interaction, and activity classification. |
| `entity-lifecycle-pools` — Entity Lifecycle and Pools | 38 | Entity creation, teardown, activity toggles, pooled records, deferred entity lists, callbacks, and entity kind dispatch. |
| `entity-motion-actions` — Entity Motion and Action Execution | 36 | Route setup, action state, transforms, movement accumulation, pose/orientation, vector stepping, and movement callbacks. |
| `entity-placement-spawning` — Entity Placement and Spawning | 29 | Cell/coordinate placement helpers, predug coordinates, random/scatter/monster spawning, and entity pairing/activity setup. |
| `entity-stats-and-accessors` — Entity Stats and Record Accessors | 63 | Entity stats configuration plus indexed/nested entity record fields, capability/state flags, movement values, and upgrade admission queries. |
| `entity-task-command-setup` — Entity Task and Command Setup | 21 | Build-placement command construction, task matching, display/resource names, target eligibility, and task-directed command issue. |
| `objective-economy` — Objectives, upgrades, and resource counters | 16 | Objective/level counters, upgrade/resource checks and debits, status text origins, and related level-state accounting. |
| `objective-runtime` — Objective Runtime | 9 | Objective text/configuration, objective arm/setup, objective voice preparation, and objective ticking/evaluation. |
| `particles-and-grid-expansion` — Particles and Grid Expansion | 12 | Timed particle/visual segments and grid-cell buffers, expansion entity placement, line-of-sight flags, and invalidation. |
| `savegame-persistence` — Save slots and persistent state serialization | 22 | Save-slot read/write/commit, save tree traversal, record serialization, visited-level tracking, and save contexts. |
| `selection-group` — Selection groups and selection state | 19 | Selection-group record/resource lifecycle, selected-entity predicates, and selection context state. |
| `simulation-dispatch-damage` — Simulation Dispatch and Damage | 21 | Frame-level entity updates, simulation/spawn queues, event dispatch, damage application, and target-route commands. |
| `simulation-lifecycle` — Simulation tick and engine state lifecycle | 15 | Engine pre-tick, main simulation step, periodic callbacks, global simulation flags/reset, time scale, and level exit/reset. |
| `task-scheduler` — Task records and scheduling | 42 | Task record creation, type queues, timers, deferred work, task callback dispatch, and task registry cleanup. |
| `tasks-rewards-checkpoints` — Tasks, Rewards, and Checkpoints | 46 | Task block and reward setup, checkpoint serialization/progress, level start selection, reward screens, scoring, and save-menu transitions. |
| `timed-effects-and-cues` — Timed Effects and Cues | 12 | Timed effect/cue request state, timestamps, update logic, overlay geometry, and effect-slot rendering. |
| `velocity-and-collision` — Velocity and Collision Queries | 20 | Velocity slot access/blending, segment and ray intersection, nearest scene hit, wall visibility, and entity-radius checks. |
| `weapon-definitions` — Weapon Definitions | 7 | Weapon type configuration parsing and read accessors for recharge, discharge, range, and wall-destruction parameters. |

### world-and-terrain

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `object-search-placement` — Object search and placement eligibility | 29 | Candidate object searches, placement validation, nearby-cell scans, occupancy checks, and placement-related path selection. |
| `routing-activity-transitions` — Routing and Activity Transitions | 41 | Path-step advancement, route selection/commit, cell-step commands, proximity movement, and terrain/activity transitions. |
| `spatial-queries` — Spatial Queries and Candidate Search | 13 | Mask-based spatial queries, world/grid lookups, candidate filters, and spatial eligibility checks. |
| `terrain-cell-state` — Terrain cell state, neighbors, occupancy, and traversal | 101 | Cell flags/state, neighbor refresh, occupancy/walkability checks, cell mutation/update queues, and terrain-cell interaction. |
| `terrain-command-feedback` — Terrain command feedback and cell interaction | 8 | Terrain-cell command feedback, command predicates, pointer/cell command routing, and command-related cell actions. |
| `terrain-data-grid` — Terrain Data and Grid Operations | 54 | Terrain construction and cell mutation, surface/material queries, coordinate conversion, surface geometry data, and dirty/change queues. |
| `terrain-erosion` — Erosion and terrain propagation | 13 | Lava/erosion timing, pending-cell queues, stage registration, and neighbor propagation/mutation. |
| `terrain-grid-resources` — Terrain grid and cell resource tables | 17 | Terrain descriptors, tile/dependency grids, cell resources, tile variants, visual channels, and grid table initialization/disposal. |
| `terrain-overlay-rendering` — Terrain and Overlay Rendering | 30 | Surface resources, terrain-cell quad emission/normals, indexed surface visibility, and scrolling/info-panel resources. |
| `terrain-path-search` — Terrain Grid Path Search | 3 | Adapters and searches that evaluate traversable terrain neighbors and return grid routes. |
| `terrain-scene-geometry` — Terrain Scene Geometry | 12 | Terrain cell quad staging/emission, surface heights, grid surfaces, edge flags, and map-row span buffers. |
| `world-entity-registry` — World entity creation and pointer registries | 11 | World object creation, object pointer registries, entity lookup/reference helpers, and object-list entity reference resolution. |

### player-interface

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `advisor-message-ui` — Advisor and Message UI | 16 | UI message strings/layout, advisor text records and animation/display, text lookup/load, and tooltip overlay state. |
| `advisor-scene` — Advisor scene and action state | 16 | Advisor scene setup, position configuration, action requests, and scene progression; excludes the later HUD advisor overlay. |
| `camera-selection-state` — Camera and Selection State | 4 | Camera controls, pointer panning, terrain-height positioning, and the small selection-record point accessors. |
| `display-mode-selection` — Display mode selection UI | 3 | Initializes DirectDraw for the setup path and populates/filters display-mode choices and refresh-rate controls. Device rendering/presentation is separately classified. |
| `font-and-ui-text` — Font and UI text rendering | 25 | Loads/releases font resources, measures or draws formatted text, wraps UI text, or builds/draws text buffers. Excludes low-level string parsing and unrelated gameplay state. |
| `frontend-menu-flow` — Front-end menu flow and options | 31 | Menu run/tick/build/show flow, front-end state transitions, option values, save/load menu composition, and menu actions. |
| `hud-panels-and-overlays` — HUD Panels and Overlay Resources | 24 | Health/hunger/bubble visuals, panel and overlay resource loading/layout, slot hit tests, text drawing, and panel state helpers. |
| `input-device-and-cursor` — Input devices and cursor state | 8 | Initializes/polls device input or updates keyboard/cursor state. Higher-level command/gameplay interpretation is outside this range. |
| `input-selection-actions` — Input, pointer actions, and selection commands | 49 | Key/binding state, input action dispatch, cursor/tile selection, selection-mode cancel/update, pointer hit tests, and action-request queues. |
| `simulation-ui-controls` — Simulation UI and Player Controls | 63 | Manager/cursor interaction, category and priority controls, camera drag/zoom, cursor configuration, and command/region remapping. |
| `tutorial-advisor-control` — Tutorial and Advisor Control | 30 | Tutorial script load/dispatch, advisor tokens and scrolling, camera focus/tweens, deferred callbacks, and advisor voice cues. |
| `ui-help-tutorial` — Help, tutorial, and interface resources | 24 | Help-window configuration/layout, tutorial paths/icons, interface image/button resources, tooltip strings, and tutorial-facing menu actions. |
| `ui-menu-widgets` — Menu widgets and menu rendering | 42 | Menu item/widget construction, pointer hit testing, layout/text positioning, slider handling, menu overlay drawing, and widget state. |
| `ui-message-queue` — Incident and information message queue | 28 | Message catalog/template setup, message nodes and queue operations, message dispatch/timers, and active incident panels. |

### scene-and-rendering

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `camera-viewport` — Camera, viewport, and view context | 15 | Camera bounds/pan/tilt/zoom, viewport projection/target state, camera ray routing, and view-context updates. |
| `graphics-rendering-surfaces` — Graphics, rendering, and pixel surfaces | 127 | Configures renderer/device state, prepares model/mesh/terrain geometry, draws or blits surfaces, and converts pixel formats. Resource ownership and scene-level animation are separate areas. |
| `model-and-animation-assets` — Model, LightWave, and animation assets | 12 | Parses LightWave model structures or creates/resolves model objects and applies model animation tracks. Rendering submission and generic scene-resource management are separate. |
| `render-animation` — Scene objects, animation, and renderer records | 55 | Scene/render object records, transforms, animation timing/visibility, render callbacks, and object-list transform updates. |
| `renderer-scene-lifecycle` — Renderer and Scene Lifecycle | 8 | Visual type tables, renderer object-pool shutdown, shared texture directory, scene hooks, and root/object allocation. |
| `scene-object-animation` — Scene objects, actions, and animation | 81 | Owns scene-object lifecycle and links, action/property records, frame advancement and callbacks, transforms, and entity visual setup. It stops at resource loading and lower-level renderer/device services. |
| `scene-object-effects` — Scene Objects and Visual Effects | 26 | Entity-to-scene attachment, scene transforms, lighting/blink/fade/move configuration, and effect-motion cancellation. |
| `scene-resource-animation` — Scene Resources and Animation | 31 | Resource binding/release, animation advancement, scene activity channels, vehicle definition parsing, attached resources, and scene object resource slots. |
| `scene-resource-lifecycle` — Scene and render resource lifecycle | 70 | Resolves, loads, tracks, caches, retains, releases, or clones scene/model/texture resources and animation data. Excludes rendering the resource and file/archive primitives beneath the loaders. |
| `scene-resources-effects` — Scene resources, lights, markers, and transient effects | 33 | Scene resource handles, lighting/markers, effect slots, scene act/pose registration, and cleanup. |
| `ui-render-tree` — UI render-tree traversal | 4 | Lays out, enumerates, processes, or releases hierarchical UI render entries; excludes font/text drawing and general model rendering. |
| `utility-render-effects` — Screen effects and UI text primitives | 16 | Screen-effect state/opacity/render lane dispatch, scrolling text, text conversion, and low-level screen blits. |
| `viewport-and-rendering` — Viewport and Scene Rendering | 10 | UI render records, terrain view tiles, viewport transforms/picking, and terrain tile style resolution. |

### audio-and-video

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `audio-playback-mixing` — Audio playback, mixing, and spatial sound | 57 | Initializes audio devices and banks, configures listener/source state, plays/stops tracked or streamed samples, or manages CD/wave playback. File access below the audio layer is separate. |
| `media-playback` — Movie, stream, and audio playback | 13 | Movie/stream playback, menu audio timing, CD audio watchdog, audio toggles, and playback wait loops. |
| `sound-subsystem` — Sound Subsystem | 33 | Sound aliases and samples, submission queues, deferred playback, positional sound, stream selection, and sound-instance ticking. |
| `video-playback` — Video Playback | 11 | DirectShow graph/sample setup, duration/frame-bound handling, render surface creation, frame update/blit, and teardown. |
| `video-playback-decoders` — AVI and FLIC playback | 41 | Opens, advances, decodes, seeks, blits, or releases AVI/FLIC playback state and frame data. Excludes general renderer surface operations. |

### game-data-and-assets

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `config-text-parsing` — Configuration and text parsing | 26 | Tokenizes or parses game configuration/text records, performs config lookup and value conversion, or supplies string helpers used by those readers. Excludes general CRT routines and UI text rendering. |
| `game-config-types` — Game configuration and entity type tables | 7 | Configuration callbacks, type/name tables, building/vehicle/upgrade/monster type initialization, and config-file resource parsing. |
| `game-file-archive-and-pools` — Game files, archives, and record pools | 78 | Routes game file operations, mounts and reads WAD/archive payloads, resolves asset paths, scans directory trees, or allocates fixed-size game records. Codec internals are grouped with archive decoding; CRT file primitives remain separate. |
| `level-map-loading` — Level and map loading pipeline | 15 | Level/map load orchestration, terrain layers, map dimensions/cell data, object list parsing/resolution, and pending level name. |
| `map-and-visual-resources` — Map and Visual Resource Records | 9 | Map-file access and visual/model/list resource record creation, lookup, and release. |
| `registry-and-startup-streams` — Registry and startup streams | 7 | Creates or reads registry paths/values and startup configuration streams. Excludes game archive/file routing and generic CRT stream implementation. |
| `resource-config-loading` — Interface and presentation config loading | 11 | Loads presentation-facing panels, sounds, text messages, surface descriptions, and interface config resources. |

### platform-and-runtime

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `application-startup-window-loop` — Application startup, window loop, and preferences | 16 | Parses launch/runtime options, initializes or pumps the main window/frame lifecycle, applies window/display preferences, and handles activation/close messages. Excludes low-level device implementation and gameplay input semantics. |
| `engine-callback-records` — Engine callback/task records | 1 | Allocates and initializes a task callback record; scheduling and callback behavior are outside this allocation helper. |
| `interface-record-and-diagnostics` — Interface records and DirectX diagnostics | 20 | Manipulates generic interface/record fields, queries COM-style interfaces, or formats/dispatches DirectX results. More specific scene, renderer, and resource lifecycle operations are classified separately. |
| `platform-import-thunks` — Platform import thunks | 15 | Small entry stubs transfer control to imported Direct3D, DirectDraw, AVI, DirectInput, DirectSound, or kernel routines. |
| `runtime-exceptions-and-process` — CRT process startup, termination, and exceptions | 41 | Implements process entry/exit, callback execution, SEH/C++ exception search and unwind, or termination handlers; not game-level control flow. |
| `runtime-file-streams` — CRT and OS file/stream I/O | 39 | Implements standard or Win32 descriptor/stream open, read, write, seek, buffering, scan, flush, close, and error mapping. Game archive semantics are outside this boundary. |
| `runtime-memory-management` — Runtime and engine memory management | 24 | Initializes, allocates, resizes, or releases CRT/engine heaps and pool arenas, including allocation retry paths. |
| `runtime-numeric-and-floating-point` — CRT numeric and floating-point runtime | 83 | Implements compiler runtime integer arithmetic, x87/FPU behavior, floating-point conversion, formatting, or math exception paths; these are runtime support rather than game behavior. |
| `runtime-strings-locale-and-format` — CRT strings, locale, and formatting | 61 | Implements CRT character classification/conversion, locale/environment handling, string operations, numeric parsing/formatting, and timezone/date helpers; not application configuration semantics. |
| `runtime-support-unresolved` — Compiler/runtime no-op stubs | 3 | The function body immediately returns; the intended CRT/platform contract is not established. |
| `system-input-save-state` — System Input and OSF State | 13 | Input release/options/quit polling and OSF-like state add/read/write/snapshot handling. |

### shared-primitives

| Area ID | Functions | Responsibility boundary |
| --- | ---: | --- |
| `geometry-math-primitives` — Geometry and matrix primitives | 15 | Computes reusable geometric relations, random values, or matrix transforms used by graphics and spatial queries; does not own a gameplay subsystem. |
| `global-state-accessors` — Global state and configuration accessors | 44 | Small accessors/mutators for global flags, fields, scalar/vector values, and level state not owned by a narrower subsystem. |
| `linked-records-and-state-helpers` — Linked Records and State Helpers | 7 | Linked-record cleanup, global list operations, selector-dependent helper, flag-mask callback checks, and scene-object state dispatch. |
| `math-geometry` — Vector and geometric math | 29 | Scalar/vector operations, curve sampling, geometric distance/projection, and matrix helpers; excludes world-grid path and occupancy decisions. |
| `motion-config-utilities` — Motion Helpers and Resource Utilities | 13 | Small motion/interpolation helpers plus loader/profile, string-table, progress, and resource-handle utilities. |
| `record-pool-allocation` — Record and resource pools | 6 | Allocates or grows fixed-size record pools and render-resource instance pools; pool consumers and higher-level resource ownership are outside this area. |
| `utility-strings-lists` — String, list, and small record utilities | 46 | String interning/replacement, generic lists, resource-handle lists, and small record allocation/copy helpers. |
