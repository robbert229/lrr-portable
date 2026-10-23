# Finding 245 — Region-Route, Light-Config, and Surface Helpers

Thirteen more anonymous procedures now carry names grounded in their own
instruction sequences, config keys, and the already-named procedures around
them — six of the twelve candidates have a ledger-named or ported caller
directly, the other seven trace back through their callee and constant
evidence alone.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x409600 | TerrainDescriptorTilesResetAndFree | Finds the descriptor with `TerrainDescriptorFindExact`, and while `+0x1c != +0x20`, per tile pair at `+0x14` clears cell-record bit 17 (`&= 0xffefffff`), zeroes `cell+0x10`, and re-inits through `CellInitPass`; for duplicate tiles also clears bit 15. Then per `+0xcc` item: type `9` gets `item[0xfa] \|= 0x800000` and `-1.0f` at `+0x340`, others get `CmdCreateTask(item, 0)`; closes with `CmdCancelByCoords` and the ported unlink/`EngineHeapFree` pair at 0x4099f0. Sole caller `UiActionDispatch`. |
| 0x411f10 | PercentSplitSurfaceBlit | Draws `param_4/param_5` as a percentage rect pair: `SurfaceBlitRect(surface+0x1c, rect[x, 0, pct])` and `SurfaceBlitRect(surface+0x18, rect[x+pct, 0, 100.0-pct])` with the 20.0f strip height constant, plus conditional segment blits at `x-100.0`/`x-104.0` and record-driven target selection via `MenuPointerRectangleHitTest`. Sole caller `MenuTick`. |
| 0x412750 | MenuHoverStatusTextDraw | Operates only when the window matches the focused window (`*DAT_00556fc4 + 0xc`) or the secondary slot; runs the hover-index state machine over `DAT_004a1f50/-1/-2` with `DAT_004db8c8` save/restore, redrawing the status buffer via `TextBufferReset`, `TextBufferAppendFormat` of float buffer fields `+0x8a`/`+0xca` and `TextBufferDrawWrapped`, and emits `"%s %d"` labels (`FormattedStringOutput`) for every `+0x14 == 6` child entry. Sole caller `MenuTick`. |
| 0x41ab00 | InterfaceBuildImagesConfigLoad | Allocates the three `DAT_004dd734/73c/744` (and `+4`-offset `DAT_004dd738/740/748`) catalog arrays sized `DAT_0055627c`/`DAT_00556288`, and per built-in entry name looks up the `"InterfaceBuildImages"` section through `ConfigEntryFind` + `ConfigPathValueGet`; on a hit splits the path at `":"` (`BlockTextTokenSplit`) and loads the three triplet surfaces with `BitmapSurfaceLoad`, zeroing all three on a miss; the name scan at `FUN_00485e30` records catalog indices into `DAT_004dd99c/004dd9a0`. Sole caller `InterfaceImagesConfigLoad`. |
| 0x44c1b0 | EntityHeadingStepMove | Reads the position with `EntityTransformPositionGet`, steps by heading fields `+0x3b4/+0x3b8` times the step, maps through `CurrentLevelHeightGet`/`TerrainGridPick`, and gates with `TerrainCellRouteAndOccupancyCheck`; on failure normalizes the heading, snaps it to the nearest axis (`±0.5` thresholds writing `±1.0`/`0.0`), rescales to the original magnitude, and retries once. Success fires `FUN_00445d70` event hooks and `EntityWorldPositionSet`; the movement budget at `+0x3bc` is decremented by the traveled distance and flag bit 15 is cleared on blocked or exhausted steps. No static callers (indirect dispatch). |
| 0x44c9c0 | LightEffectsFadeConfigLoad | Reads `LightEffects/FadeRGBMin`, `FadeRGBMax` (`ConfigEntryFind` + `ConfigRgbVectorRead`) and the `RandomRangeForTimeBetweenFades`/`RandomRangeForFadeTimeFade`/`RandomRangeForHoldTimeOfFade` pair strings (`ConfigStringCopy`, `BlockTextTokenSplit` on `":"`, `ParseFloat` ×2), scales all ranges `×25.0`, and hands twelve values to the sink `FUN_0044cc10`; returns 0 on any missing key. Caller `FUN_0044c7c0`. |
| 0x44cca0 | LightEffectsMoveConfigLoad | Same parser skeleton for the move group: `RandomRangeForTimeBetweenMoves`, `RandomRangeForSpeedOfMove`, `RandomRangeForDistOfMove` (×25) and three-float `MoveLimit`, fed to sink `FUN_0044cef0`. Caller `FUN_0044c7c0`, running back-to-back with the fade loader. |
| 0x44e8c0 | SurfaceVertexSpanValidate | For each unique grid pair (consecutive duplicates skipped) collects the surface vertices with `TerrainCellSurfaceVerticesGet`, checks the six edge-index pairs for a span above `param_5` (early `return 0`), and keeps the min/max `local_28` coordinates only over corner cells whose map byte carries flag `& 8`; finally `return 0` when any corner hit and `param_4 < max-min`, else 1. Sole caller `BuildingPlacementFootprintRenderAndValidate`. |
| 0x45d110 | RegionContainmentScanUpdate | After the slot-8 state gates `FUN_0045ab30`/`FUN_0045ab90`, reads the base position via `FUN_0045a700` and scans the region-entry table `DAT_005013a4` (`DAT_0050111c` keys → `DAT_00500f6c` records), testing `(x,y)` containment inside origin+`DAT_0050118c` offset plus record `+0x4/+0x8` sizes. A hit sets `DAT_005013bc \|= 1`, stores bounds via `FUN_0045d370`, and (unchanged region, mode 0) re-shows the overlay through `UiTextEntryLoad(7)` + `TimedEffectUpdateRequestSet(7)`; region changes go through `FUN_0045d300` with entry-event `FUN_0045d3c0`, and no hit falls back to region id `0x1b` returning 0. Sole caller `MainSimInteractionAndCellUpdate`. |
| 0x4702b0 | EntitySurfaceQuadSegmentIntersect | Builds the entity surface quad by transforming four `±0.5` scaled half-extent offsets from the `+0x334+0xa0` vector through the object vtable `+0x108` (`FUN_00475070`), optionally rejects starts behind the segment via `FUN_00470630`, then per edge solves the 2D intersection with `FUN_00479af0`, range-checks the hit against the edge span on the dominant axis, and writes the point to `param_4`. Callers `0x471d20`/`0x471f00` dispatch it behind the stats-flag bit `0x100`. |
| 0x476bf0 | SurfacePixelRowsFlipInPlace | Pulls the surface descriptor through vtable `+0x60`, computes row bytes `(width*bpp)>>3`, allocates a row-sized scratch with `EngineHeapAlloc`, and for `i < height/2` swaps first row against last row through the buffer with `EngineHeapFree` tail. No static callers (indirect dispatch). |
| 0x479630 | SurfaceTriangleAreaCalculate | Gates on the triangle normal (`SurfaceTriangleNormalCalculate`, computing only when the sign is negative — backface cull), measures all three edge lengths with `SQRT`, keeps the longest edge, negates its direction, and returns `len[i] * 0.5 * fsin(acos(dot(dir_i, dir_next))) * len[i+1]` — half-rectangle sine-area on the two non-longest edges, with the acos resolved through the wrapper `FUN_0048e090`. Sole caller `0x40b470`, which sums two such triangles sharing an edge for a projected-quad area. |
| 0x47d520 | Palette8To16BitSurfaceExpand | Depth-gated (`+0x54` switch; only 16-bit implemented): derives per-channel shifts from `PopCount32`/`FindFirstSetBit32` of the three surface masks (`8 - popcount`), then per texel indexes the palette-LUT triple by byte index and composes one 16-bit word per pixel into the surface buffer, finishing with the ported row-flip `FUN_0047d740`. Sole caller `0x47d490`, the DIB-info blit dispatcher. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-17.bundle](../evidence/legorr-engine-helpers-expansion-17.bundle):
`ev_50ac433a55f0fe9d174a1fccee1298b2eddb40d022f7beef590478017d4dd352`
(batch decompile of all thirteen addresses). Caller observations are the
matching `procedure_callers` records: `ev_a11ae39e0fb25d751ae828095013e54d2f74cc6c150ee9abbef6158693ea8db2`
(0x409600), `ev_09431621fa586c7bdaeb5216ba0f9d6568e8e96cd4183640be3e3a7b1b271f8b`
(0x411f10), `ev_7b44550bba50b24af17db64720cca81a5147b509aadc814e7ed4ad9a2ae85bf2`
(0x412750), `ev_a432138bb2c6b08bed45852b1d8813ba9a040ddbd1ec0ec1e8d21fb6dfebb97c`
(0x41ab00), `ev_00e3d846367311b5e0100861911ad39abd9ae0235afc0c2c955f757bca14af24`
(0x44c1b0), `ev_cf7731ce47f4629eb72c20bd5c0a6f701ad6d47d92b4555fa17cae1434175b3f`
(0x44c9c0), `ev_928779cfdd2ddafd3bcc6a4a2b3843a3401a102b03e14bef6fb41af5fdf84a5c`
(0x44cca0), `ev_ff62a4ce908cd2cc0634102047b553c3d9d9fe95cde0a1a0dd4517a8d1ba1955`
(0x44e8c0), `ev_6f275ea9776143020bb91bea3aecef2e998603cd954855c29448f0c6a9db929a`
(0x45d110), `ev_201d0cb6f607dbca45ff46536bcefc028651d7a6a44daa790adfa1f21343e84e`
(0x4702b0), `ev_4bcfaf0a2c9dccd6a4cbde239eed25abb9849337ac8a07a30b612792041926f6`
(0x476bf0), `ev_2fd9f5d38acb850ce260bac1c66edaaaaa907bc3b992f6d3d81512fe0908cb5a`
(0x479630), and `ev_16eb0bbff3776fc41ad07f869e4ca07b03fb936b19417ecf7ac211740f77fb40`
(0x47d520). Caller identities were read from each record's `raw_result.value`.
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** Nine behavior-traced candidates were kept for the next
entity/clone band: 0x446e40 (`EntityNearestConnectedTerrainCellRouteSelect`
route-commit wrapper), 0x44bc30 (entity-motion-to-scene-node sync),
0x448a60 (kind-0xb neighbor-cell move command handler), 0x465f40 (instance
quad-vertex build), 0x471a60 (steering velocity update), 0x487a00/0x4899f0
(clone / recursive-clone pair behind 0x489660), 0x441ed0 (face-target action
start), and 0x40b000 (sector-table positional record collect, domain term
not evidenced). 0x474760 stays anonymous pending vtable `+0x48/+0x80/+0xa0`
semantics — its body is pure interface indirection with unreliable element
counts. The CRT cluster 0x493–0x49Bxxxx (deferred in finding 243, 0x4957d0
among them) and the entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`
keep their status, as do the pair-list builders, `DAT_00544858`, and the
unresolved set carried from findings 240–244 (`0x4802d0`/`0x480390`/`0x480570`,
`0x418a00`/`0x45c0d0`, `0x44d280`, `0x473460`/`0x473480`/`0x4734a0`).
