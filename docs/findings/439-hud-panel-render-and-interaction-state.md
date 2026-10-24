# Finding 439 — HUD Panel Rendering and Interaction State

This finding maps how panel names and configuration feed HUD resources, bounds, drawing, and hit tests. It ties together the assigned 24 functions rather than assigning user-facing meanings to every table field. Direct observations are calls, branches, global-field accesses, and constants in the checked-in pseudocode; descriptions such as “panel category” follow the caller's config keys and table lookups. Related traces include [Finding 243](243-config-loader-file-descriptor-and-crt-helpers.md), [Finding 271](271-panel-layout-and-advisor-render-helpers.md), [Finding 336](336-hunger-meter-images-and-entity-override-pop.md), [Finding 363](363-geometry-string-lookup-and-entity-state-helpers.md), and [Finding 386](386-spatial-ui-com-and-crt-runtime.md).

## Configuration and render initialization

`RenderThreePassUpdate @ 0x00459530` is a direct dispatcher: it calls `HealthBarConfigLoadAndShadeDerive @ 0x00459560`, `HungerImagesPositionConfigLoad @ 0x00459900`, then `BubbleImagesPositionConfigLoad @ 0x00459a50` with the same two inputs. The caller in the game-configuration setup invokes it before other configuration/render initializers. These passes populate shared display state; they do not draw the panels themselves.

The health-bar pass reads `ObjInfo::HealthBarPosition`, `HealthBarWidthHeight`, `HealthBarBorderSize`, `HealthBarBorderRGB`, `HealthBarBackgroundRGB`, `HealthBarRGB`, and `HealthBarVertical`. It stores coordinates and dimensions in globals, derives lighter/darker border shades by adding/subtracting 40 percent per RGB channel and clamps those derived channels to `[0,1]`. A successful load sets health-bar flag bit `0x2`; the vertical config flag additionally sets bit `0x8`. The hunger pass first loads the `HungerImage_%i` resources from `ObjInfo::HungerImages`; for each loaded resource it calls the surface color-key helper with six zero arguments, establishing a zero-to-zero (black) color key, then reads `HungerImagesPosition` and sets flag bit `0x1`. Finding 116 traces the helper's color-key behavior. The bubble pass reads `BubbleImagesPosition` and sets flag bit `0x10`. Each loader returns without setting its feature bit when the required position tuple fails to parse; these functions do not clear the feature bits on failure (and the health-bar pass does not clear the vertical bit when its config flag is false).

At runtime, `RenderEntityScreenRing @ 0x00459b00` checks health-bar flag bit `0x2` and an entity eligibility helper before building screen-space ring geometry from the supplied coordinates and configured position, size, border, and color values. `IndexedPanelResourcePositionApply @ 0x00459f50` checks hunger flag bit `0x1`, converts the requested image index to an integer capped at 4, and draws the corresponding loaded resource at the supplied point plus the configured hunger offset. `PanelResourcePositionApplyIfEnabled @ 0x00459fd0` uses bubble flag bit `0x10` and applies the configured bubble offset. The rendering callers are visible in the entity/UI path; the exact gameplay meaning of every entity gate and bit remains dependent on surrounding state.

## Panel tables, bounds, and hit tests

`UiPanelNameTablesInit @ 0x0045a030` initializes fixed name tables and their scan counts: 12 panel names and 24 button names for each panel slot. `CaseInsensitiveStringTableLookup @ 0x0045a370` scans the 12 panel names, returning the matching index; `CaseInsensitiveIndexedStringLookup @ 0x0045a410` scans the 24 button names at a panel-indexed stride and returns that button's index. Finding 243 traces the config loader that resolves `Panels%ix%i` records through these names.

The panel state uses twelve `0x30`-byte category records. `DAT_00500134`, `DAT_00500138`, and `DAT_0050013c` are respectively the draw-record pointer, draw-record count, and category state word in the first record; each field repeats at the same offset for the other categories. `PanelTextureSlotsAllocate @ 0x0045ac30` stores a `0x2c`-byte-stride draw-record array and its count in the first two fields; rendering, hit testing, and per-item flag helpers use the matching category's array and count. `OverlayImageRecordLoad @ 0x0045a2e0` writes its supplied resource/state value into the state word and sets bit 0 on successful image or FLIC load. `PanelCategoryBoundsSet @ 0x0045a650` stores the two supplied coordinate pairs as bounds, clears the category's state field, then chooses one pair as the active position according to flag bit `0x2`. `DAT_00500128` and `DAT_0050012c` hold the active X/Y position; `IndexedPairSet/Get @ 0x0045a6e0/0x0045a700` write and read them, and `ManagerTick @ 0x0045a730` interpolates them toward the selected bound. `PanelCategoryInteractionStateReset @ 0x0045a270` restores saved pairs across the category records, sets/clears category bits, clears child-record interaction fields, and resets a related global. Its caller is the level teardown path; Finding 271 traces the panel-layout caller that supplies configured bounds.

`OverlayImageRecordLoad @ 0x0045a2e0` stores an image record in the indexed category slot. It first tries the bitmap/image loader; on success it applies the zero-to-zero (black) color key, records kind 0, and sets a record flag. If that path fails, it tries the FLIC loader, records kind 1, and sets the same flag on success. Finding 116 describes the color-key helper; the helper's use here and the record writes are direct observations. `PanelSlotCoordinateHitTest @ 0x0045a3b0` reads the record kind and resource. For kind 0 it adjusts the coordinates and calls the resource hit-test helper; when that helper reports a hit, it returns whether the category index is zero. A miss or a nonzero kind returns true. Its caller, `SimCategoryCellHitTest @ 0x0045b310`, calls it for active categories except 1 and 2 when an output pointer is supplied; a false return sets that output flag. This is not a plain hit/miss result. The behavior is directly visible in the pseudocode; its higher-level UI meaning remains unresolved.

## Scroll controls and text output

`ScrollPanelControlsInitialize @ 0x0045a240` initializes the scroll-info panel and a neighboring control, then registers two controls under category index 6. The specific scroll rectangles and callback behavior are traced in [Finding 262](262-scroll-panel-and-viewport-picking-helpers.md); this wrapper's direct role is setup sequencing.

`FUN_0045a460 @ 0x0045a460` is an unnamed formatted-text helper. When its centering flag is nonzero, it measures the formatted string and subtracts half its width; it then adds the selected category's stored position pair, rounds the coordinates, and calls the formatted text draw routine. `ScrollingAdvisorTextAppend @ 0x0045a590` appends formatted text to the supplied text-output object. `UITextOutputPositionAndDraw @ 0x0045a5b0` combines the record's base coordinates with offsets in its associated record, updates the output object's position, and draws it. `UITextOutputObjectRelease @ 0x0045a620` releases that object, while `UIRecordCallbackForward @ 0x0045a630` forwards its dereferenced handle and two arguments to a UI-record helper. Findings 351 and 382 trace the output object and formatted draw helper.

`PanelImageSlotEntryAllocate @ 0x0045a500` allocates a 16-byte slot record, creates the image resource from supplied geometry/config, stores the converted coordinates, and points the record at the indexed category. The allocation's coordinate units and flag meanings are not established by this helper alone (see Finding 278).

## Assigned-function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x00459530` | `RenderThreePassUpdate` | Calls the health-bar, hunger-image/position, and bubble-position configuration passes in order. |
| `0x00459560` | `HealthBarConfigLoadAndShadeDerive` | Reads health-bar geometry/colors, derives clamped border shades, and sets feature/orientation bits. |
| `0x00459900` | `HungerImagesPositionConfigLoad` | Loads hunger images, reads their configured position, and sets flag bit `0x1` on success. |
| `0x004599c0` | `LoadObjectHungerMeterImages` | Loads the indexed `HungerImage_%i` resources and applies a zero-to-zero color key to each loaded image. |
| `0x00459a50` | `BubbleImagesPositionConfigLoad` | Reads bubble coordinates and sets flag bit `0x10` when parsing succeeds. |
| `0x00459b00` | `RenderEntityScreenRing` | Builds/draws an entity's screen-space gauge rings when the feature bit and entity gate pass. |
| `0x00459f50` | `IndexedPanelResourcePositionApply` | Draws one of up to five hunger images at the supplied position plus the configured offset. |
| `0x00459fd0` | `PanelResourcePositionApplyIfEnabled` | Draws the supplied resource at the supplied position plus the bubble offset when enabled. |
| `0x0045a030` | `UiPanelNameTablesInit` | Initializes 12 panel names, 24 button names, and their table counts. |
| `0x0045a240` | `ScrollPanelControlsInitialize` | Initializes scroll controls and registers two controls under category 6. |
| `0x0045a270` | `PanelCategoryInteractionStateReset` | Restores category coordinate pairs and clears category/child interaction state. |
| `0x0045a2e0` | `OverlayImageRecordLoad` | Stores an indexed overlay record, applies a zero-to-zero color key to bitmap resources, and falls back to FLIC loading. |
| `0x0045a370` | `CaseInsensitiveStringTableLookup` | Finds a panel name in the fixed table and returns its zero-based index. |
| `0x0045a3b0` | `PanelSlotCoordinateHitTest` | For kind-0 records, tests adjusted coordinates and returns `categoryIndex == 0` on a hit; its caller uses false to set an output flag. |
| `0x0045a410` | `CaseInsensitiveIndexedStringLookup` | Finds a button name in the selected panel's fixed 24-entry table. |
| `0x0045a460` | `FUN_0045a460` | Measures/optionally centers formatted text, applies category offsets, and draws it. |
| `0x0045a500` | `PanelImageSlotEntryAllocate` | Allocates an image-slot record with resource, converted coordinates, and category pointer. |
| `0x0045a590` | `ScrollingAdvisorTextAppend` | Appends formatted text to a scrolling-advisor output object. |
| `0x0045a5b0` | `UITextOutputPositionAndDraw` | Applies record-relative position offsets, sets output position, and draws. |
| `0x0045a620` | `UITextOutputObjectRelease` | Releases the stored output-object handle. |
| `0x0045a630` | `UIRecordCallbackForward` | Forwards a dereferenced UI-record handle and two arguments. |
| `0x0045a650` | `PanelCategoryBoundsSet` | Stores two bounds pairs, resets state, and selects an active pair using a flag. |
| `0x0045a6e0` | `IndexedPairSet` | Stores the supplied pair in the indexed category record. |
| `0x0045a700` | `IndexedPairGet` | Reads the pair from the indexed category record. |

## Open questions

- Which rendering sites consume all health-bar and bubble flags, and what are the exact world-to-screen/unit conventions for their offsets?
- What do the remaining category flags and the `0x30`-stride record fields represent beyond the bounds, selected pair, loaded resource, and hit-test behavior established here?
- When the bitmap attempt in `OverlayImageRecordLoad` fails, which FLIC state transitions and cleanup paths own the loaded stream?
- What are the expected coordinate units and allocation flags passed to `PanelImageSlotEntryAllocate` at each caller?
- The coordinate units and the complete set of consumers of the active panel position are not established beyond the config, manager, render, and reset paths traced here.

Evidence is the checked-in pseudocode and the caller/config traces linked above. The exact in-game appearance and interaction were not runtime-tested in this investigation.
