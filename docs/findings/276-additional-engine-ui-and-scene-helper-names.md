# Finding 276 — Additional Engine, UI, and Scene Helper Names

These fourteen routines now have behavior-based names supported by direct
Ghidra pseudocode and caller evidence where available. Five use the checked-in
full pseudocode corpus as their direct source; the others also have retained
REA evidence records.

| Address | Name | Observed behavior and remaining uncertainty |
| --- | --- | --- |
| 0x4017b0 | AdvisorPositionRecordInitialize | Writes an indexed 0x24-byte advisor-position record from parsed IDs, type, and coordinate pairs, then sets initialization/type flags. Exact field and flag meanings remain open. |
| 0x4065f0 | CubicBezierPointEvaluate | Evaluates the two-coordinate cubic Bernstein curve for four control-point vectors and parameter t. The caller samples it into a generated curve; broader use is unknown. |
| 0x410320 | StringQueueRecordOrFlush | Stores up to fifteen string records, each with associated values; a zero count flushes queued entries through 0x47a180 and clears the queue. The operation performed by 0x47a180 and the stored values' meanings remain unknown. |
| 0x410a10 | MenuItemChoiceStringsAllocate | Allocates a menu-choice descriptor and a zeroed pointer array sized by the choice count. The descriptor's metadata meanings and later ownership remain unresolved. |
| 0x410bf0 | MenuItemNumericRangeRecordAllocate | Returns null unless upper bound exceeds lower bound; otherwise allocates and fills a numeric-range record. The extra fields' meanings remain unknown. |
| 0x4198f0 | TerrainCellLinkedEntriesRemove | For linked-entry categories 6 and 7, removes nodes matching a cell key and deletes a bucket when empty. The category and payload semantics remain unknown. |
| 0x41a1a0 | UIEntryHitTestAndDispatch | Scans registered rectangles for a point hit and optionally invokes the matching entry's handler. Registry entry type and special first-entry behavior remain unresolved; Ghidra's recovered signature differs from a corpus caller. |
| 0x44da50 | ThreeChannelValueTweenStep | Under a mode flag, advances three global values toward a target or zero using elapsed time and latches completion bits. The three channels' meanings are unknown. |
| 0x453920 | AdvisorScrollStepAdvance | Updates a bounded decrementing index, looks up and displays a frame, and runs completion side effects at zero. The index and completion side effects are not fully identified. |
| 0x459900 | HungerImagesPositionConfigLoad | Loads `ObjInfo/HungerImagesPosition`, parses two values into globals, and sets an initialization bit. Coordinate units and consumers remain unknown. |
| 0x459a50 | BubbleImagesPositionConfigLoad | Loads `ObjInfo/BubbleImagesPosition`, parses two values, and sets an initialization bit. Coordinate convention and rendering consumer remain unknown. |
| 0x45b620 | PriorityListButtonActionDispatch | Maps button actions 0–8 to per-entry toggles and 9–16 to list reordering. The precise meaning of each action ID remains unresolved. |
| 0x475c80 | DDiScenePayloadRelease | Dispatches on a record kind to release kind-specific payload members, frees the payload block, and clears its pointer. Kind and interface-slot semantics remain unknown. |
| 0x477370 | DDiSceneRenderSurfacePoolGrow | Allocates a chunk of 0x20-byte entries, links them into a free list, and stores the chunk in a pool table. The exact native object represented by an entry is unknown. |

**Direct evidence and callers.** The five corpus-only direct bodies and callers
are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x4017b0` (called
from `AdvisorPositionConfigLoad`, `0x401530`), `0x4065f0` (sampled by
`0x406820`), `0x410320` (called by `0x412c00`), and `0x410a10`/`0x410bf0`
(allocated from menu parser `0x4140c0`). The binary SHA-256 is
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

Retained REA evidence for `0x4198f0` and `0x41a1a0` is in
[legorr-incident-message-rendering.bundle](../evidence/legorr-incident-message-rendering.bundle):
direct records `ev_320a5d36683556141b57d9b3ef2fab31ca046c968e74c6e355552f09d1ec1f0d`
and `ev_7d28cfef653b3883c15b70a6104c1fcc21d2c380732302863fb3d5fea8fe96df`;
caller evidence for `0x4198f0` is `ev_fea32df42a69532d9e8495caa9df997a7227632ca4bcafa87c66775c865f7e66`.
The caller at `0x426520` for `0x41a1a0` is visible in the full pseudocode corpus.

`0x44da50` is in
[legorr-sim-terrain-third-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-third-pass-2026-10-23.bundle),
direct record `ev_ffe7f690f02f5398140f7979fe85b9887fc6f52b2c4879028bdce00bd2351bcd`;
its per-tick caller is `0x44cfa0`. `0x453920` is in
[legorr-objective-advisor-inline-markup.bundle](../evidence/legorr-objective-advisor-inline-markup.bundle),
direct record `ev_1eee8ff453c8380c00d0888f5c8669953106b577dea44a9c50178ea5311c23bd`.
`0x459900` and `0x459a50` are in
[legorr-checkpoint-reader-expansion.bundle](../evidence/legorr-checkpoint-reader-expansion.bundle),
direct records `ev_269e1c4312778c23543f0031f22b672e79d010f50167081489bd31bf76dff25d`
and `ev_585194ef78769d6e081bb8528ef0391cb4a5ae47d421f3e709667b781754f176`;
both are called by `0x459530`. `0x45b620` is in
[legorr-ui-render-fourth-pass-2026-10-23.bundle](../evidence/legorr-ui-render-fourth-pass-2026-10-23.bundle),
direct record `ev_e4d4f8a095d930d73f07d8bfa2877baefcb0e3d7a103b5dd2ab9fa813b993883`.

`0x475c80` is in [legorr-ddscene.bundle](../evidence/legorr-ddscene.bundle),
direct record `ev_f2bb5036d71b2f1f9695f649aa759ee2a8760d666c6a00fe518fd3980bfe5560`;
scene-object destroy calls it at `0x472bda` in `0x472a50`.
`0x477370` is in
[legorr-ui-render-third-pass-2026-10-23.bundle](../evidence/legorr-ui-render-third-pass-2026-10-23.bundle),
direct record `ev_344b14307ce6ad0fae89718fc5395bfde7323000087b52f7c6174d5e444b5355`;
pool initialization calls it at `0x476e9d` in `0x476e50`, as recorded by
`ev_05c5490ff4e3c4f8c1b5ce16a04628bf048cad3ab2c73397b13be2248723508a`.

All observations are static analysis; no runtime execution is implied.
