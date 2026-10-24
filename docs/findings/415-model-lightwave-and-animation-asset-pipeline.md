# Finding 415 — Model, LightWave, and Animation Asset Pipeline

This finding maps the 18 functions listed by Issue #190 to their callers,
callees, and data flow. Thirteen entries belong to the LightWave/model/animation
path described here. Five entries were grouped into that area incorrectly; the
call graph places them in sound state, generic scene-resource lifecycle, WAD
archive indexing, or shared asset-path parsing. The coverage indexes below
reflect those corrections.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
The function bodies are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c).
The detailed LightWave chunk and model evidence is in [finding 11](11-model-and-scene-formats.md),
the `.uv` grammar is in [finding 19](19-uv-grammar.md), and the texture binding
path is in [finding 13](13-texture-system.md).

## Route overview

`ModelResourceResolveOrLoad @ 0x4885c0` and `EntityVisualLoadACT @ 0x472cd0`
reach `LwoModelEntryLoad @ 0x4809e0`. It creates the temporary model object
through `ModelObjectCreate @ 0x48d7c0`, assembles its render mesh, runs the
material pass, and frees the temporary parse object. `ModelObjectCreate` calls
`LoadLWO @ 0x48d2c0`, which fills vertex, polygon, and surface records and may
retain a `.uv` stream for the material pass. The pass calls `ParseUV @
0x481c50` when that stream exists. The full file grammar, mesh construction,
and shipped-corpus validation are already recorded in findings 11 and 19.

The scene-object animation path is separate from parsing: `SharedObjectAnimationAndTimedEventUpdate
@ 0x487ba0` calls `SharedObjectFloatTrackInterpolateApply @ 0x488070` to sample
a stored float track and forwards the interpolated value to its downstream
handler at `0x4880d0`. Scene action cloning starts at
`SceneActFrameRecordCloneOrCreate @ 0x489660`, which calls
`ObjectHierarchyRecursiveClone @ 0x4899f0` for recursive object and child
duplication. Those clone/resource functions are generic scene-resource
lifecycle work, rather than LightWave parsing.

## Assigned functions

| Issue address and name | Current name and role | Caller/callee evidence |
| --- | --- | --- |
| `0x004809e0` `LwoModelEntryLoad` | Loads a model entry, creates the parser object, builds the render object, performs its material pass, then frees the parser object. | Called by `EntityVisualLoadACT @ 0x472cd0` and `ModelResourceResolveOrLoad @ 0x4885c0`; calls `ModelObjectCreate @ 0x48d7c0`, `CachedRenderResourceInstanceCreate @ 0x480870`, `MaterialPass @ 0x480ac0`, and `ModelObjectFree @ 0x48d870`. |
| `0x00481c50` `ParseUV` | Reads the optional line-based `.uv` stream; writes per-corner UVs and can duplicate vertices at seams. | Sole caller is `MaterialPass @ 0x480ac0`; detailed grammar and 60-file corpus validation are in finding 19. |
| `0x00482620` `MeshSegmentVertexFill` | Writes position, fixed attribute triple, and per-vertex pair into a selected 0x70-byte mesh segment record, at 0x20-byte vertex stride. | Sole caller is `MeshBuildEightSidedSegment @ 0x470760`; this is segment geometry population. |
| `0x004827d0` `RecordFlag2CBit0Set` | Sets or clears bit 0 at `record + 0x2c`. | Its sole caller is `ResourceEnableSet @ 0x465ac0`, which passes the sound instance subrecord at `instance + 0x290`. Reclassified to the sound-subsystem area; the bit's product meaning remains unknown. |
| `0x00488070` `SharedObjectFloatTrackInterpolateApply` | Masks the supplied key index to 16 bits, reads floats from the track at object `+0x1c`, and uses the last key directly or linearly blends adjacent keys by the supplied fraction. | Called from `SharedObjectAnimationAndTimedEventUpdate @ 0x487ba0`; forwards the sampled value to the downstream handler at `0x4880d0`. |
| `0x004899f0` `ObjectHierarchyRecursiveClone` | Copies object state through interface methods and, when no live clone is supplied, duplicates, registers, and recursively clones the object's children. | Called by `SceneActFrameRecordCloneOrCreate @ 0x489660` and recursively; calls `ObjectInterfaceArrayResourceCreate @ 0x489b30`. Reclassified to generic scene-resource lifecycle. |
| `0x00489b30` `ObjectInterfaceArrayResourceCreate` | Enumerates an object's interface-array descriptor, queries the selected interface on each entry, and creates cached render-resource instances from the results. | Sole caller is `ObjectHierarchyRecursiveClone @ 0x4899f0`; reclassified with that scene-resource lifecycle path. |
| `0x0048bce0` `WadArchiveRecordAtIndex` (issue calls it `LwoParserRecordAtIndex`) | Returns a pointer into the 0x24-byte mounted-WAD registry table at `0x575500 + index * 0x24`. | Called by `WwadArchiveIndexLoad`, WAD entry lookup/open functions, and registry accessors adjacent to `0x48bce0`; no LWO chunk handler calls it. Renamed and reclassified to game-file archive/pool handling. |
| `0x0048c0c0` `LwoStringDuplicate` | Allocates an even-padded copy of a NUL-terminated string. | Used by `ModelObjectCreate`, `LoadLWO`, and `LwoSurfaceSettings` for model and surface names; see finding 11. |
| `0x0048c120` `LwoTextureProjectionModeParse` | Maps case-insensitive `PLANAR`, `CYLINDRICAL`, and `SPHERICAL` names to modes 0, 1, and 2; unknown names return 0. | Called by `LwoSurfaceSettings @ 0x48c820`; also recorded in finding 227. |
| `0x0048c1d0` `LwoReadBigEndianFloat` | Reads four bytes, reverses their order, advances the input pointer, and returns success. | Called by `LwoPoints @ 0x48c210` and `LwoSurfaceSettings @ 0x48c820` for big-endian float fields. |
| `0x0048c210` `LwoPoints` | Reads `size / 12` three-component points from `PNTS` and writes the vertex count and array. | Dispatched by `LoadLWO`; uses `LwoReadBigEndianFloat` for each coordinate. |
| `0x0048c360` `LwoCurves` | Copies and walks the `CRVS` chunk records, then frees the temporary data; the loader does not retain curve geometry. | Dispatched by `LoadLWO`; finding 11 documents the discarded chunk behavior. |
| `0x0048c3e0` `LwoPolygons` | Reads polygon vertex counts, big-endian indices, and surface references into per-polygon metadata. | Dispatched by `LoadLWO`; finding 11 documents the format and corpus-specific zero-based index cases. |
| `0x0048c690` `LwoSurfaceNames` | Reads the even-padded NUL-terminated `SRFS` name list and stores names when the caller provides an output table. | Dispatched by `LoadLWO`; names are subsequently matched against `SURF` records. |
| `0x0048c820` `LwoSurfaceSettings` | Initializes a surface record and interprets its `SURF` subchunks, including color, texture image, projection, and material flags. | Dispatched by `LoadLWO`; uses `LwoStringDuplicate`, `LwoTextureProjectionModeParse`, and `LwoReadBigEndianFloat`. Detailed layout is in finding 11 §5b. |
| `0x0048d2c0` `LoadLWO` | Opens `<base>.lwo`, checks `FORM`/`LWOB`, dispatches `PNTS`, `SRFS`, `SURF`, `CRVS`, and `POLS`, skips unknown chunks, and optionally opens `<base>.uv`. | Sole caller is `ModelObjectCreate @ 0x48d7c0`; its output fields become the parser model consumed by the mesh/material pass. |
| `0x00490090` `AssetPathComponentsParse` (issue calls it `MaterialResolve`) | Splits an input path into an optional two-character prefix, directory, basename, and extension, with each output bounded to 255 bytes. It does not look up or resolve a material. | Called by model creation, model texture binding, and FLIC open; renamed and moved to shared asset path handling. |

## Index corrections and limits

The issue's list has all 18 addresses, but five names or area assignments do not
match the call evidence. `RecordFlag2CBit0Set` is the sound instance's enabled
bit setter; the two recursive clone helpers manage shared scene resources;
`0x48bce0` accesses mounted-WAD records; and `0x490090` only splits path
components. The coverage README, ledger, area CSV/report, canonical decompile
names, and Ghidra name map were updated for those corrections. Finding 230's
description of `0x48bce0` as an LWO chunk-table accessor is corrected alongside
this finding.

The code evidence establishes parser behavior and direct call edges, not all
runtime ownership or visual results. `LoadLWO` and `.uv` corpus behavior have
existing static/corpus validation (findings 11 and 19); the object-interface
GUID's concrete type and the semantic meaning of the `+0x2c` sound flag remain
unresolved. Focused follow-up issues should address those runtime semantics if
they affect a concrete behavior slice.
