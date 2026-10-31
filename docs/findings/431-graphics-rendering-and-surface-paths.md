# Finding 431 — Graphics, Rendering, and Pixel Surface Paths

Issue #189 assigns 126 procedures spanning device setup, render-state control, model and terrain geometry, bitmap conversion, DirectDraw surfaces, software pixel drawing, and display-mode selection. The repository already contains operation-level maps for each assigned address. This finding ties those maps into the main resource and frame flows, records the handoffs between hardware-facing and software paths, and provides a complete cross-reference to the existing address-level explanations. It does not infer undocumented Direct3D state labels from opaque flags or interface slots.

## Device and frame lifecycle

The setup path begins with display-mode enumeration and filter selection, then constructs the DirectDraw and Direct3DRM interfaces and their primary/secondary surfaces. `DirectDrawEnumerateAndStoreCallbackResult` records candidate device data; `DirectDrawDeviceCreate` and `DirectDrawEnumeratedDeviceCreate` construct a selected device; `DirectDrawRendererInit`, `DirectDrawDeviceInitialize`, and `Direct3DRMDeviceInit` set up the renderer-facing objects. `Direct3DRenderPreferencesApply`, `RendererWindowModeApply`, and the display-mode helpers connect configuration and dialog choices to that setup. Findings [11](11-model-and-scene-formats.md), [48](48-directdraw-renderer-setup.md), [111](111-directdraw-windowed-exclusive-surfaces.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [282](282-display-resolution-helper-names.md), [309](309-geometry-and-directdraw-surface-helpers.md), and [315](315-pooled-record-and-display-mode-helpers.md) trace these handoffs. The static paths show how a mode is selected and applied, but not which mode wins on each supported runtime/display configuration.

`RendererBeginFrameOnce` and `RenderSetup` gate and prepare a frame. The model path sets up the viewport, synchronizes render state, traverses render records, and dispatches model/child material passes; the end path clears the model list, flushes cached material state, and restores device state. Finding [49](49-model-render-pass-dispatch.md) establishes the order of the two child passes and preserves the opacity bit meanings as unresolved. Findings [48](48-directdraw-renderer-setup.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md), [372](372-incident-messages-options-and-resource-helpers.md), and [415](415-model-lightwave-and-animation-asset-pipeline.md) cover viewport setup, render-state application, texture binding, and the asset/material inputs. The scene layer's begin/end calls and current-object state are described in finding 25.

Renderer state is split between cached state words, interface calls, and explicit restoration. Fog enable/color, fog mode, fog parameters, clip distance, object state, render preferences, and material flushes update different portions of that path. Findings [285](285-render-collision-fog-and-segment-helpers.md), [311](311-renderer-fog-and-mesh-color-setters.md), [346](346-screen-surface-allocation-and-clip-distance.md), [49](49-model-render-pass-dispatch.md), and [309](309-geometry-and-directdraw-surface-helpers.md) document the observed writes and callers. The exact semantic names for several render-state values remain unknown, so this map retains their numeric values and interface-call boundaries.

## Geometry and material data

The model geometry path starts with parsed LWO layer/polygon data, creates mesh records, remaps/scales vertex coordinates, and copies indexed geometry into render records. `MaterialPass` and `BindModelTextures` then connect mesh material data to texture resources. Findings [11](11-model-and-scene-formats.md), [13](13-texture-system.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [265](265-token-directory-scan-and-mesh-coordinate-helpers.md), [288](288-render-object-and-mesh-helpers.md), [372](372-incident-messages-options-and-resource-helpers.md), and [415](415-model-lightwave-and-animation-asset-pipeline.md) supply the parser, vertex, mesh-record, and texture-binding details. `LwoLayerPolygonVertexCount` delegates the count query to the layer's mesh/indexed-record interface; the polygon vertex reader and terrain emitters use the same interface boundary for data retrieval and output.

The terrain path builds per-cell quad records, emits the four-corner vertex data, updates cell surface properties, and later copies indexed geometry to the renderer. The dirty-normal pass revisits that emitted geometry after neighbor changes. Findings [54](54-terrain-grid-emits-per-cell-quad-vertices.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), and [372](372-incident-messages-options-and-resource-helpers.md) trace those producers and consumers. Whether every vertex attribute follows the same winding/coordinate convention across imported LWO meshes and live terrain is still a focused verification question.

Matrix utilities provide the shared transform foundation: multiplication, identity/zero initialization, copy, translation, and axis rotations feed scene-node and render-interface calls. Findings [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), [301](301-scene-geometry-and-resource-slot-helpers.md), [309](309-geometry-and-directdraw-surface-helpers.md), [323](323-camera-math-and-matrix-primitives.md), [358](358-matrix-window-surface-and-heap-helpers.md), and [372](372-incident-messages-options-and-resource-helpers.md) document the field layouts and callers. The two procedures named `Matrix4SetRotationZ` at `0x00479d50` and `0x00479df0` write different matrix index pairs; their distinct row/column convention should be preserved rather than collapsed based on the shared name.

## Surface formats and pixel paths

The current port has concrete LWO/UV parsing and path-keyed model reuse, but its
`Model` value is still the parser's compact `PNTS`/`POLS`/`SURF` representation.
It is not the reference's 32-bit layer object consumed by `MaterialPass`, nor
does it own the generated `0x70` render records, Direct3DRM materials/textures,
or the interface methods used to submit and release them. `material_pass_execute`
therefore remains an operation seam: the missing piece is the scene/render
owner that converts parsed layers and their TIMG bindings into renderer objects
and supplies per-record apply/submit methods. The parsed model cache and mesh
record allocator can be exercised independently, but those components alone
cannot draw a loaded model.

DirectDraw surface descriptors expose width, height, pitch, bit depth, and channel masks through interface queries. `DirectDrawSurfacePixelRead` bounds-checks coordinates, locks the surface, reads a value using pitch and pixel depth, and unlocks it. The software drawing routines use a shared lock/configure path, cache the channel masks and shifts, select a writer for 8-, 16-, 24-, or 32-bit formats, and reset that state on unlock. Polyline drawing uses a clipped Bresenham walk; span records fill horizontal intervals; circle drawing samples points and clips them to surface bounds. Findings [234](234-game-file-streams-and-surface-pixels.md), [309](309-geometry-and-directdraw-surface-helpers.md), [358](358-matrix-window-surface-and-heap-helpers.md), and [385](385-stream-surface-avi-and-route-state-helpers.md) provide the operation-level behavior and evidence.

Bitmap handling branches on the source/destination format. The assigned helpers decode palettized data, expand 8-bit palette indices to 16-bit pixels, convert 24-bit bitmap data, encode a requested RGB color using either a palette lookup or channel masks, pack/unpack RGB values, and save/load bitmap surfaces. Population-count and first-set-bit helpers support channel-mask normalization. Row-flip helpers derive row bytes from width and bit depth, then swap opposing rows through a temporary row buffer. Findings [23](23-front-end-and-menu.md), [229](229-wwad-archive-and-render-resource-helpers.md), [230](230-engine-input-selection-and-resource-helpers.md), [234](234-game-file-streams-and-surface-pixels.md), [245](245-region-route-light-config-and-surface-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [309](309-geometry-and-directdraw-surface-helpers.md), [358](358-matrix-window-surface-and-heap-helpers.md), and [393](393-string-list-and-small-record-utilities.md) cover these transformations. The available static paths do not establish behavior for every malformed bitmap header or every unusual channel-mask layout.

Surface presentation and cleanup form a separate device boundary: windowed rendering blits the secondary surface to the primary; fullscreen rendering flips; restore and release paths handle lost surfaces, clippers, palette and texture-memory queries, and the DirectDraw object set. Findings [20](20-verify-pass.md), [111](111-directdraw-windowed-exclusive-surfaces.md), [172](172-avi-frames-blit-to-render-target.md), [205](206-configured-video-blit-fills-render-surface.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [355](355-bit-pointer-surface-and-window-wrappers.md), and [384](384-crt-formatting-com-cleanup-and-spawn-helpers.md) trace those boundaries. Their documented evidence distinguishes observed interface calls from assumed DirectDraw semantics.

## Remaining questions

- Which runtime-specific device and render-state values correspond to each cached numeric field and opaque vtable slot?
- How do display-mode filters behave when the enumerated mode list contains duplicate dimensions or uncommon bit depths?
- Which malformed or unsupported bitmap formats fail cleanly, and which leave partially initialized surface state?
- Do imported mesh vertices and generated terrain quads share the same coordinate/winding convention in every render mode?

These are investigation questions; the current pass does not establish a concrete implementation slice, so no implementation follow-up issue was created.

## Assigned function cross-reference

The following cross-reference covers the 126 addresses assigned by Issue #189. Each linked finding contains the prior behavior-level explanation and, where available, the evidence records for that procedure. The machine-generated function-area and coverage-ledger rows already existed for all 126 assigned addresses; this finding updates the README findings index/range only.

| Address | Function | Existing behavior map |
| --- | --- | --- |
| `0x00473b40` | `SceneRenderLockToggle` | [03](03-engine-tick.md), [25](25-ddscene-object-runtime.md), [39](39-advisor-request-reset-sounds.md), [40](40-advisor-transition-worker.md) |
| `0x00473dc0` | `RendererFogEnableAndColorApply` | [311](311-renderer-fog-and-mesh-color-setters.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x00473e10` | `RendererFogColorSet` | [285](285-render-collision-fog-and-segment-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x00473e70` | `RendererFogModeSet` | [311](311-renderer-fog-and-mesh-color-setters.md) |
| `0x00473ea0` | `RendererFogParametersSet` | [311](311-renderer-fog-and-mesh-color-setters.md) |
| `0x00473ec0` | `RenderObjectStateApplyToEntries` | [294](294-render-segment-and-object-state-helpers.md), [255](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md) |
| `0x004748f0` | `MeshRecordAllocateOrAppend` | [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x00474ae0` | `TerrainDetailEntryGetAtIndex` | [375](375-entity-ui-and-render-tree-helpers.md) |
| `0x00474b30` | `LwoLayerEnabledSet` | [235](235-camera-projection-lwo-layer-state-and-surface-normals.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x00474c40` | `LwoLayerPolygonVertexCount` | [233](233-terrain-rendering-and-stream-primitives.md) |
| `0x00474cc0` | `LwoLayerPolygonVertexDataRead` | [233](233-terrain-rendering-and-stream-primitives.md), [372](372-incident-messages-options-and-resource-helpers.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) |
| `0x00474d30` | `TerrainCellQuadVertexEmit` | [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [54](54-terrain-grid-emits-per-cell-quad-vertices.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [244](244-config-file-text-layout-and-record-emit-helpers.md) |
| `0x00474e30` | `TerrainSurfacePropertySet` | [375](375-entity-ui-and-render-tree-helpers.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x00474ff0` | `SceneMeshColorSet` | [311](311-renderer-fog-and-mesh-color-setters.md), [255](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md) |
| `0x00475020` | `SceneMeshColorAndAlphaSet` | [288](288-render-object-and-mesh-helpers.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x00476bf0` | `SurfacePixelRowsFlipInPlace` | [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x00476fb0` | `RendererObjectSetClipDistance` | [346](346-screen-surface-allocation-and-clip-distance.md) |
| `0x00477fa0` | `RendererBeginFrameOnce` | [02](02-startup-and-main-loop.md), [20](20-verify-pass.md) |
| `0x004780b0` | `RendererWindowModeApply` | [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md), [384](384-crt-formatting-com-cleanup-and-spawn-helpers.md), [398](398-application-startup-window-loop.md) |
| `0x004781d0` | `Direct3DRMDeviceInit` | [412](412-platform-import-thunks.md), [11](11-model-and-scene-formats.md), [15](15-compat-audit.md), [22](22-research-crumbs.md) |
| `0x00478330` | `Direct3DRenderPreferencesApply` | [309](309-geometry-and-directdraw-surface-helpers.md), [247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md) |
| `0x004788d0` | `Direct3DRenderStateSetCached` | [233](233-terrain-rendering-and-stream-primitives.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x00478940` | `D3DMaterialStateFlush` | [49](49-model-render-pass-dispatch.md) |
| `0x00479ce0` | `Matrix4Multiply` | [309](309-geometry-and-directdraw-surface-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x00479d50` | `Matrix4SetRotationZ` | [301](301-scene-geometry-and-resource-slot-helpers.md), [323](323-camera-math-and-matrix-primitives.md) |
| `0x00479da0` | `Matrix4SetRotationY` | [323](323-camera-math-and-matrix-primitives.md) |
| `0x00479df0` | `Matrix4SetRotationZ` | [323](323-camera-math-and-matrix-primitives.md), [301](301-scene-geometry-and-resource-slot-helpers.md) |
| `0x00479e40` | `MatrixIdentityAndTranslationSet` | [358](358-matrix-window-surface-and-heap-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x00479e70` | `Matrix4SetIdentity` | [323](323-camera-math-and-matrix-primitives.md) |
| `0x00479ea0` | `Matrix4SetZero` | [323](323-camera-math-and-matrix-primitives.md) |
| `0x00479eb0` | `Matrix4Copy` | [323](323-camera-math-and-matrix-primitives.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x0047b0d0` | `IndexedRecordStride134WordGet` | [358](358-matrix-window-surface-and-heap-helpers.md) |
| `0x0047c0c0` | `Vector3ExactEqual` | [163](163-view-provider-feeds-directsound-listener.md) |
| `0x0047c1c0` | `DirectDrawEnumerateAndStoreCallbackResult` | [355](355-bit-pointer-surface-and-window-wrappers.md), [412](412-platform-import-thunks.md) |
| `0x0047c2e0` | `DirectDrawDeviceCreate` | [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [412](412-platform-import-thunks.md), [15](15-compat-audit.md) |
| `0x0047c4b0` | `DirectDrawEnumeratedDeviceCreate` | [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [412](412-platform-import-thunks.md) |
| `0x0047c610` | `DirectDrawRendererInit` | [412](412-platform-import-thunks.md), [15](15-compat-audit.md), [48](48-directdraw-renderer-setup.md), [111](111-directdraw-windowed-exclusive-surfaces.md) |
| `0x0047c8d0` | `RenderSurfacePresent` | [02](02-startup-and-main-loop.md), [20](20-verify-pass.md), [172](172-avi-frames-blit-to-render-target.md), [205](205-directshow-video-uses-shared-render-presentation.md) |
| `0x0047c8f0` | `BitmapFileWrite` | [08](08-save-and-checkpoints.md) |
| `0x0047cc20` | `RenderSurfaceRestoreFromPrimary` | [172](172-avi-frames-blit-to-render-target.md) |
| `0x0047cc50` | `WindowedRenderSurfaceBlit` | [172](172-avi-frames-blit-to-render-target.md), [205](205-directshow-video-uses-shared-render-presentation.md) |
| `0x0047ccf0` | `DirectDrawObjectsRelease` | [02](02-startup-and-main-loop.md), [20](20-verify-pass.md) |
| `0x0047cd50` | `QueryTextureMem` | [13](13-texture-system.md) |
| `0x0047cdd0` | `SurfaceDescriptionGet` | [355](355-bit-pointer-surface-and-window-wrappers.md), [309](309-geometry-and-directdraw-surface-helpers.md), [386](386-spatial-ui-com-and-crt-runtime.md), [413](413-configured-directshow-video-path.md) |
| `0x0047ce20` | `ProjectionOverlayDrawSubmit` | [375](375-entity-ui-and-render-tree-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [398](398-application-startup-window-loop.md) |
| `0x0047cee0` | `DirectDrawClipperSetRegion` | [48](48-directdraw-renderer-setup.md) |
| `0x0047d000` | `PalettizedBitmapSurfaceDecode` | [243](243-config-loader-file-descriptor-and-crt-helpers.md) |
| `0x0047d2d0` | `SurfacePixelValueEncode` | [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x0047d3f0` | `BitCount32` | [355](355-bit-pointer-surface-and-window-wrappers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [364](364-bit-count-cursor-and-directory-helpers.md) |
| `0x0047d410` | `RendererRecordTableInitialize` | [358](358-matrix-window-surface-and-heap-helpers.md) |
| `0x0047d470` | `ImageSurfaceRelease` | [08](08-save-and-checkpoints.md), [274](274-priority-image-selection-and-scene-registry-teardown.md), [393](393-string-list-and-small-record-utilities.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) |
| `0x0047d490` | `SurfacePixelFormatQuery` | [358](358-matrix-window-surface-and-heap-helpers.md), [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0047d520` | `Palette8To16BitSurfaceExpand` | [245](245-region-route-light-config-and-surface-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md) |
| `0x0047d700` | `PopCount32` | [230](230-engine-input-selection-and-resource-helpers.md), [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0047d720` | `FindFirstSetBit32` | [230](230-engine-input-selection-and-resource-helpers.md), [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0047d740` | `SurfacePixelRowsFlipWithScratch` | [246](246-entity-move-route-command-and-object-clone-helpers.md), [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0047d800` | `Bitmap24BitSurfaceConvert` | [229](229-wwad-archive-and-render-resource-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md) |
| `0x0047d9d0` | `BitmapSurfaceLoad` | [247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md), [08](08-save-and-checkpoints.md), [09](09-reward-task-manager.md), [23](23-front-end-and-menu.md) |
| `0x0047db90` | `RGBChannelsPack` | [23](23-front-end-and-menu.md) |
| `0x0047dbf0` | `ImageSurfaceBlitTransformed` | [09](09-reward-task-manager.md), [116](116-reward-button-color-key-setup.md), [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x0047dcb0` | `SurfaceBlitRect` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [04](04-sim-input-and-events.md), [08](08-save-and-checkpoints.md) |
| `0x0047de60` | `DirectDrawSurfaceDescriptionGet` | [309](309-geometry-and-directdraw-surface-helpers.md), [386](386-spatial-ui-com-and-crt-runtime.md), [413](413-configured-directshow-video-path.md) |
| `0x0047ded0` | `DirectDrawSurfaceUnlock` | [385](385-stream-surface-avi-and-route-state-helpers.md) |
| `0x0047def0` | `DirectDrawSurfacePixelFormatGet` | [309](309-geometry-and-directdraw-surface-helpers.md) |
| `0x0047df50` | `PixelChannelMaskGet` | [384](384-crt-formatting-com-cleanup-and-spawn-helpers.md) |
| `0x0047dfa0` | `DirectDrawSurfacePixelRead` | [309](309-geometry-and-directdraw-surface-helpers.md) |
| `0x0047e2d0` | `RGBChannelsUnpack` | [23](23-front-end-and-menu.md) |
| `0x0047e440` | `BitmapSurfaceSave` | [08](08-save-and-checkpoints.md) |
| `0x0047e510` | `DirectDrawDisplayModeSetup` | [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), [414](414-avi-and-flic-playback-routes.md) |
| `0x0047e710` | `DirectDrawSurfaceDescriptorInitialize` | [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x0047e7e0` | `BitCount32ByPosition` | [364](364-bit-count-cursor-and-directory-helpers.md) |
| `0x00480ac0` | `MaterialPass` | [11](11-model-and-scene-formats.md), [13](13-texture-system.md), [19](19-uv-grammar.md), [401](401-task-callback-record-allocator.md) |
| `0x00481820` | `BindModelTextures` | [13](13-texture-system.md), [394](394-runtime-engine-memory-management.md), [11](11-model-and-scene-formats.md), [244](244-config-file-text-layout-and-record-emit-helpers.md) |
| `0x00481fa0` | `MeshVertexCoordinateRemap` | [265](265-token-directory-scan-and-mesh-coordinate-helpers.md) |
| `0x004821a0` | `IndexedRecordEntryRead` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x00482220` | `MeshRecordAllocateAndFill` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md) |
| `0x00482350` | `IndexedRecordField64Set` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x00482370` | `MeshVerticesScaleApply` | [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x004823e0` | `TerrainCellQuadVerticesWrite` | [54](54-terrain-grid-emits-per-cell-quad-vertices.md) |
| `0x00482470` | `IndexedGeometryDataCopyToOutput` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x00482500` | `InstanceQuadVertexDataFill` | [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md) |
| `0x004826c0` | `MeshVertexRecordFill` | [251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md), [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) |
| `0x00482780` | `IndexedRecordFlag6CBit0Get` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x004827a0` | `IndexedRecordFlag6CBit0Set` | [373](373-cursor-simulation-renderer-and-file-helpers.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) |
| `0x00482ac0` | `ModelRenderRecordStateSetup` | [49](49-model-render-pass-dispatch.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x00482b50` | `RenderStateFlagsApply` | [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [372](372-incident-messages-options-and-resource-helpers.md), [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) |
| `0x00482cb0` | `RendererCommandModeAndPairSend` | [373](373-cursor-simulation-renderer-and-file-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x00482ce0` | `RendererCommandNodePrepend` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x00482d30` | `ModelRenderListClear` | [13](13-texture-system.md), [49](49-model-render-pass-dispatch.md) |
| `0x00482d60` | `ModelRenderPassDispatch` | [25](25-ddscene-object-runtime.md), [49](49-model-render-pass-dispatch.md), [129](129-effect-item-flags-reach-model-render-dispatch.md) |
| `0x00483140` | `IndexedRecordInterfaceRelease` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x00483170` | `IndexedRecordTransformInitialize` | [372](372-incident-messages-options-and-resource-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x00483240` | `IndexedRecordTransformCopyIn` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x00483270` | `RenderObjectColorVectorSet` | [226](226-omitted-simulation-terrain-resource-text-procedures.md), [372](372-incident-messages-options-and-resource-helpers.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) |
| `0x00483400` | `IndexedRecordDataAddressGet` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x00483420` | `ScreenEffectItemOpacitySet` | [126](126-screen-space-effect-lane-dispatch-and-opacity-update.md), [129](129-effect-item-flags-reach-model-render-dispatch.md) |
| `0x00483540` | `Matrix4x4IdentityInitialize` | [372](372-incident-messages-options-and-resource-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x00483580` | `RenderInterfaceCapabilityCheck` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x00483600` | `Direct3DViewportSetup` | [22](22-research-crumbs.md), [48](48-directdraw-renderer-setup.md), [49](49-model-render-pass-dispatch.md) |
| `0x00483690` | `MatrixMethodPairApply` | [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md), [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x004838b0` | `D3DRenderStateSync` | [49](49-model-render-pass-dispatch.md) |
| `0x00483940` | `Direct3DRenderStateRestore` | [49](49-model-render-pass-dispatch.md) |
| `0x004839c0` | `ModelRenderChildPassDispatch` | [49](49-model-render-pass-dispatch.md) |
| `0x00483a70` | `ModelChildRenderEligibilityCheck` | [49](49-model-render-pass-dispatch.md) |
| `0x00483a90` | `ModelRenderChildMaterialPass` | [49](49-model-render-pass-dispatch.md), [129](129-effect-item-flags-reach-model-render-dispatch.md), [229](229-wwad-archive-and-render-resource-helpers.md) |
| `0x00483b00` | `WorldObjectRenderStateApply` | [375](375-entity-ui-and-render-tree-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x00483b70` | `Direct3DDeviceStateApply` | [229](229-wwad-archive-and-render-resource-helpers.md) |
| `0x00484ba0` | `PackedTripletTableToWordBuild` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x004850c0` | `IndexedWordTableLookup` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x00485e80` | `RenderSetup` | [20](20-verify-pass.md), [02](02-startup-and-main-loop.md) |
| `0x00485fb0` | `ViewportRectangleGet` | [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x00485ff0` | `SurfaceDrawPolyline` | [233](233-terrain-rendering-and-stream-primitives.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) |
| `0x00486090` | `SurfaceFillPixelSpans` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486210` | `SurfaceFillPixelSpanRecords` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486390` | `SurfaceDrawCirclePixels` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x004864d0` | `SurfaceColorPackToPixel` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486550` | `SurfaceLockConfigurePixelWriter` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486650` | `SurfaceUnlockResetPixelWriter` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486690` | `SurfacePixelWriterSelectByFormat` | [234](234-game-file-streams-and-surface-pixels.md) |
| `0x00486720` | `BresenhamLineWalkClipped` | [233](233-terrain-rendering-and-stream-primitives.md) |
| `0x00489c30` | `IndexedBitmapPaletteDataPrepare` | [13](13-texture-system.md), [23](23-front-end-and-menu.md) |
| `0x0049d030` | `DirectDrawDeviceInitialize` | [02](02-startup-and-main-loop.md), [15](15-compat-audit.md), [282](282-display-resolution-helper-names.md), [20](20-verify-pass.md) |
| `0x0049d5e0` | `DisplayModeDialogSetModeAndRefreshChoices` | [315](315-pooled-record-and-display-mode-helpers.md), [279](279-crt-and-display-helper-names.md), [282](282-display-resolution-helper-names.md) |
| `0x0049d630` | `DisplayModeListPopulate` | [279](279-crt-and-display-helper-names.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [282](282-display-resolution-helper-names.md), [398](398-application-startup-window-loop.md) |
| `0x0049d8d0` | `DisplayModeMatchesFilterTuple` | [282](282-display-resolution-helper-names.md) |
| `0x0049d950` | `DisplayModeNameIndexFind` | [282](282-display-resolution-helper-names.md) |
