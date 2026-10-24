# Finding 240 — Cursor Interaction, Overlay, Menu, and Scene-Light Helpers

Thirteen large anonymous procedures now carry names grounded in their own
instruction sequence and in already-named callers and callees. Caller context
comes from `MainSimInteractionAndCellUpdate`, `MenuInputTick`, `SaveMenuBuild`,
`TimedEffectAndCueUpdate`, `GameConfigInit`, `RectangularGridOverlayGeometryDraw`,
and `SurfaceBlitRect`; none of the names below depend on feature-level inference.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4075a0 | TickEntityScreenEffects | Per-frame tick over four entity slot tables: decrements slot timers, projects live eligible entities to screen coordinates, invokes the screen-ring renderer, and drains the shared screen-effect accumulator into the overlay cue routine. |
| 0x459b00 | RenderEntityScreenRing | When the overlay flag bit 2 is set and the entity gate passes, fills four corner-coordinate arrays with concentric screen rings around the entity's screen point, scales the rectangles by the entity's 0..100 gauge field, fills two gauge rectangles, and emits the ring geometry through the quad-strip helper. |
| 0x411530 | MenuWidgetItemScreenRect | Computes the clickable screen rectangle for dialog widget `param_2`: text kinds measure label width and line height through the format-width and line-advance helpers and clamp against the widget image extents; slider/track kinds take the track rectangle; returns 0 for deferred kinds and out-of-range indices. |
| 0x411790 | MenuPickWidgetUnderPointer | Scans dialog widgets in order, tests the widget rectangle against the stored mouse position through `MenuPointerRectangleHitTest`, defers to the alternate geometry routine when the rectangle routine abstains, and returns the picked widget index. |
| 0x410740 | SaveMenuListEntryAppend | Grows the list-entry array (entries are 0x20-stride records plus per-column index vectors), reallocing the three column buffers; a key ending in `.bmp` is split on commas into three column images that are interned and loaded, otherwise the key string is stored as a plain entry, then per-entry column, banner, and coordinate fields are written. Called only from `SaveMenuBuild`. |
| 0x433770 | SceneLightsBuildFromConfig | Reads `topspotrgb`, `trackspotrgb`, `ambientrgb`, `fplightrgb`, and `fprotlightrgb` through `ConfigEntryFind`/`ConfigRgbVectorRead`, creates the corresponding scene light objects (types 0/1/2/3) plus a default white light and root scene object, positions and enables them, and sets the attenuation/range on the spot and headlight-type lights. |
| 0x451cd0 | MessageStringTableInitialize | Assigns the `Message_*` command-name string pointers into the contiguous global table from `DAT_004ff190` through `DAT_004ff290`, covering every simulation message name the dispatcher accepts. Called during configuration initialization. |
| 0x45adb0 | SimCursorInteractionTick | Runs the cursor/category hit test on the cell under the pointer, and on a hit raises the interaction flag, bumps the hovered record's counter, applies the state-flag-gated special cases (`FUN_00456240` bits 0x10/0x20/0x40/0x80/0x400/0x1000 with the camera-speed accumulator), then dispatches the per-category interaction commands for the picked category/item cell; on a miss it falls back to the track-region hit test. Returns the pick category. |
| 0x45b310 | SimCategoryCellHitTest | Walks the twelve category lists in the `DAT_00500134` family (enabled-flag-gated), hit-tests each 0x2c-stride record through the per-category geometry tests, maintains the hover pair `DAT_005007d0/7d4` and the record hover bits at offset 0x28, maps the hovered record's type through the cue routine, and returns the picked (category, item) pair with sentinels 0xc/0x18. |
| 0x4638a0 | SimTrackCursorHitTest | For enabled track-region records near `DAT_00552158`, hit-tests both cursor slots through the segment test, advances the selected record's cursor index under the `DAT_00552160` mode machine (auto/cursor A/cursor B), updates the entity selection bits at offset 0x18, and on a near miss projects the coordinate onto the segment to derive the cursor index; the exact track semantics (which structure the two slot pointers reference) remain inferred. |
| 0x46b1d0 | TooltipOverlayStateInitialize | Publishes all `ToolTip_*` label pointers into the contiguous table from `DAT_0054bf68`, initializes the grid-overlay geometry block `DAT_0054bf20`–`DAT_0054bf60` (including the rectangle-inset iteration count at `DAT_0054bf28`, cell sizes, spacings, and clamped fractional margins) from the ten initialization parameters, and zeroes the 0x1839-dword overlay record block at `DAT_0054c000`. |
| 0x46bc30 | TimedEffectSlotRender | For one timed-effect slot, snapshots the grid globals, computes the slot's screen origin from its cell fields or its single attached object, clamps the extent into the viewport bounds, draws the cell grid through `RectangularGridOverlayGeometryDraw`, then blits the slot's accumulated effect objects at running offsets through `SurfaceBlitRect`. |
| 0x4734f0 | DDiSceneLightCreate | Allocates a scene object through `DDiSceneObjectAllocate`, tags it type 5 (light), creates the native light through the device interface with the requested type and RGB values, applies the color through the light's interface, and inserts the object at the scene head. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-12.bundle](../evidence/legorr-engine-helpers-expansion-12.bundle):
`ev_878fd28dd62158bc46082e3bdffc198b11151f1eb97ca6d314138473a28c0ed3`
(batch decompile of 0x45adb0, 0x459b00, 0x451cd0, 0x410740, 0x411530, 0x46b1d0,
0x46bc30, 0x433770) and
`ev_e4e7bd19a1c471f40710ed0df141dee5795a743ab287b251341fcc34a8208f4b`
(batch decompile of 0x411790, 0x4075a0, 0x45b310, 0x4638a0, 0x4734f0).
Caller observations for each address are in the same bundle, including
`ev_cdb53ae8294ff8391d87f8aa23232712186df05f5ac2226e5a4d95f57caf44ba`
(0x45adb0 from `MainSimInteractionAndCellUpdate`),
`ev_6b63c70cd1c75a607b576b035baeb59c69f15714ecd23a815e334b04a12f5b21`
(0x411790 from `MenuInputTick`),
`ev_a9b27f6bff5b256185aa4870cc5e6a1eb304f8b1972490b8806c828205f7576a`
(0x410740 from `SaveMenuBuild`), and
`ev_f858c79dd33442b2d5490cb1fb748d7509f6c369150c3a0e33432743005f468d`
(0x46bc30 from `TimedEffectAndCueUpdate`).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Follow-up.** The token-list/directory-scan trio now has mechanical names in
finding 265. The observed caller ignores `0x480570`'s compare result, so its
intended effect remains unknown. The large anonymous candidates `0x418a00` and
`0x45c0d0` remain deferred because the earlier triage found no caller signal.
