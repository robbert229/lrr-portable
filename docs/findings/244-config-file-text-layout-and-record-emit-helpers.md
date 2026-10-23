# Finding 244 — Config-File, Text-Layout, and Record-Emit Helpers

Thirteen anonymous procedures now carry names grounded in their own instruction
sequences, the strings and globals they touch, and the already-named procedures
around them — every one of the twelve original candidates has at least one
ledger-named caller, and the bonus twelfth callee is a direct sibling of an
already-named glyph routine.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x41ada0 | InterfaceImagesConfigLoad | Iterates the `"InterfaceImages"` config section through `ConfigEntryFind`/`ConfigNestedPathChildFind`/`SortedEntryNextEqualKey`; when the section is absent it falls back to the `InterfaceBuildImages` (`FUN_0041ab00`) and `InterfaceBackButton` (`FUN_0041b1b0`) loaders, and per-entry it resolves image-slot indices against the 74-slot `DAT_004dced0` pointer table via `FUN_0041a9c0`. Called by `GameConfigInit`. |
| 0x44db50 | KeyValueEqualsFileLoad | Opens a file read-mode through `GameFileOpenDispatch` (mode string byte `'r'` at `DAT_004a1eac`), reads lines with `GameFileReadLineDispatch`, splits each at the `'='` byte stored at `DAT_004a52ac` (`BlockTextTokenSplit`, result must be exactly 2 tokens), duplicates the key with `StringDuplicate`, converts the value with `ParseIntegerWrapper`, and stores the pair into the zero-initialized 164-entry `DAT_00555e00` table; registers cleanup through `FUN_004802b0`. Called by `GameConfigInit`. |
| 0x463f00 | SceneQuadGridEmit | Builds a stack vertex table of ±1.0 float coordinates and per-face normals, then in a ten-step loop emits four-vertex quads through `TerrainCellQuadVertexEmit`, posts faces via `FUN_00474a60`, and finalizes with `SceneRenderLockToggle` on a freshly `EngineHeapAlloc`ed record. Called by `GameConfigInit`. |
| 0x418450 | HelpWindowInfoConfigStringsLoad | Formats `"HelpWindow::HelpWindowInfo::%sL%i"` per catalog name (`FormattedStringOutput`) over the `DAT_005561e8`/`DAT_005561ec`/`DAT_00556280`/`DAT_00556288` name tables and copies each entry's string through `ConfigEntryFind` + `ConfigStringCopy` into the `DAT_004db978`–`DAT_004dc0f8` destination blocks. Called by `HelpWindowResourcesInit`. |
| 0x485710 | TextWrapMeasureAndDrawLines | Two-phase wrapped-text routine for `TextBufferDrawWrapped`: the first loop walks inline text measuring glyph advance with `FUN_0047a520` and breaking lines on whitespace (`CharacterClassifyMask` mask 8 or the char-class table) once accumulated width passes the wrap width; the second loop draws each glyph through `BitmapFontGlyphDrawAndMeasure`, tracking the maximum height and folding the consumed line advance back into the buffer state. |
| 0x47a520 | BitmapFontGlyphMeasureWidth | Four-instruction wrapper that calls `BitmapFontGlyphDrawAndMeasure(font, 0, 0, ch, 0)` — zero draw origin, measure-only — returning the glyph advance. Its callers `TextBufferDrawWrapped` and `TextWrapMeasureAndDrawLines` consume it as the glyph-width oracle. |
| 0x481b80 | CounterFormatStringBuild | Builds `prefix<counter>suffix` names: copies the prefix, `FormattedStringOutput`s `"%d"` of the `*param_3` counter into the 16-byte buffer, appends the counter string and then `param_2`, and increments the counter — called by `BindModelTextures` for numbered texture-slot names. |
| 0x475a70 | SceneActionRecordActFrameScan | For scene records with field `+0x10 == 4`, walks the two action-record entry lists (`+0x10`, `+0x20` pointers), pulls entry strings through vtable `+0x4c` and counts through vtable `+0xc`, and compares each name against `"ActFrame"` (`StringDuplicate` + compare) to locate that action entry. Called by `DDiSceneObjectDestroy` and `EntityVisualResourceLoad`. |
| 0x496cc0 | Win32FileDescriptorWrite | Descriptor write for `GameFileWrite`: validates the fd against the same `DAT_0076ddc0`/`DAT_0076dec0` slot table as the named `Win32FileDescriptorSeek`/`Read`/`Open` siblings, honors append via `Win32FileDescriptorSeek(fd, 0, 2)`, runs `WriteFile` binary or LF→CRLF chunked text expansion into a 1 KB stack buffer, returns bytes written minus inserted CRs, and maps failures through `Win32ErrorMapToErrno` (including the ERROR_ACCESS_DENIED/EOF text-mode Ctrl-Z flag path). |
| 0x491b40 | EnginePoolChunkTryAllocate | In-block allocator over a pool block whose word-0 cursor/word-1 free-count header is walked with the zero-marker freelist scan; claims an aligned `param_3`-byte chunk when it fits before the `+0x3e` block end, retargeting the cursor and decrementing the free count, and returns the chunk pointer or 0. Sole static caller `EnginePoolAllocate`. |
| 0x4425f0 | SubobjectPoseDeltaNormalizeAndDispatch | Takes (entity, x, z) and, when the new planar position differs from the cached one in `param_1[0xa8..0xad]`, stores the new pair, forms the old-minus-new delta, normalizes it (`SQRT` of squares), scales by a unit step scalar, writes it back, and dispatches the subobject handles with `FUN_00406dc0`/`FUN_00406df0`/`AttachedModelOrientationUpdate`-family calls plus `CurrentLevelHeightGet`, flag-gated on the `+0xf8` bit-2 pose flag. Sole caller `PathStepAdvanceAndArrivalCheck`. |
| 0x408ec0 | KindNineNeighborEmitForFootprintCells | For each footprint cell pair of the `TerrainDescriptorFindExact` descriptor, skips duplicates, and emits eight kind-9 neighbor records through the `FUN_00406440(7)` command-record initializer (`FUN_00403750`) with unit-scale float offsets, per accumulated record index. Sole caller `BuildingPlacementPreviewUpdate`. |
| 0x46e610 | TerrainEdgeFlagMidpointUpdate | For every terrain-table entry (outer `DAT_0054beb0`, inner count at `DAT_00549934`), reads the cell's four surface vertices (`TerrainCellSurfaceVerticesGet`), averages opposite edge midpoints scaled by the level-size factors `puVar[2]`/`puVar[3]`, converts through `X87FloatToLongRound` → `TerrainGridPick`, and applies the picked grid cell to `TerrainCellEdgeFlagSetAndUpdate` with flag 1 then flag 0 — the per-entry sub-scan of the ported fx-bank ticker at 0x46e390. |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-16.bundle](../evidence/legorr-engine-helpers-expansion-16.bundle):
`ev_dbe042295398495edb6eff4da314caad1f8e9d559afc923aea65771a02727dc8`
(batch decompile of all thirteen addresses). Caller observations are the
matching `procedure_callers` records: `ev_bcd292decb77d961a66fb65fa73886e014e8df35e566315c2093752d84d81011`
(0x408ec0), `ev_2e1506abd196e86359e60000d0fd8f554f671eff1d1d3dd9ea2f8c1f5f88c3b4`
(0x418450), `ev_c1da058a2c992e7f3cc71d90b3d1cafaae965ced865db0e73b14ea247259c0b2`
(0x41ada0), `ev_2587ca3d4057d802566083618c17e5fbf4de21abb157ac45ca16a65025157760`
(0x4425f0), `ev_ddf7dc2126d3e236a5dab42a507eeffd1b5074d6873bf58bdafb892626f1f816`
(0x44db50), `ev_7db44ab170ba5412a9025f89ba8a57c8a2f2b5dc6cf40829fcf7972f30fc340d`
(0x463f00), `ev_932b790b6dd6ecb37b66c1bc99b222353a028b2ce7db9519ae17cd253422f7d4`
(0x46e610), `ev_ca27d52cb10ca455fe06ad68537d559456aaee92520c3d1626fafba7fabe8079`
(0x475a70), `ev_2bcc332e0dc3df8dc6d8fbd3606c455acf77dfedaf9e27a41a048026c5593195`
(0x47a520), `ev_419285fc66168ce4dbcfedd9e0779e0fb29817125fbbbbd9c513eb3cd7eae085`
(0x481b80), `ev_7c0001fd8747dcbb22b60e37f0315342df6fe3c4b69d1e5d99a02e5171c9764e`
(0x485710), `ev_6db082237c4c542cbb6cd5c17cd64c2d03decfc37218af770180aca35f2cbfb2`
(0x491b40), and `ev_2b019ad0cf2257b8b52a4fd00b0ffe49eb5b0731577a80cdb41b60625023f8e1`
(0x496cc0). Caller identities were read from each record's `raw_result.value`.
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** Five named-proximity candidates were deliberately left
for the next band: 0x45d110 (`MainSimInteractionAndCellUpdate` callee),
0x409600 (`UiActionDispatch` callee), 0x412750/0x411f10 (both `MenuTick`
callees), and 0x44e8c0 (`BuildingPlacementFootprintRenderAndValidate` callee).
The anonymous `FUN_00406440(7)` command-record slot allocator used by the
kind-9 emitter, the two interface-table scanners `FUN_0041a9c0`/`FUN_0041ab00`,
and the record post-call `FUN_00474a60` stay mechanical-unknown until their
own callers are traced. The CRT cluster 0x493–0x49Bxxxx (deferred in finding
243), the entity scalar accessors `FUN_00469cb0`/`ce0`/`d20`/`0x46a0a0`, and the
`DAT_00544858` resolve callback keep their status. The unresolved set carried
from findings 240–243 (`0x4802d0`/`0x480390`/`0x480570`, `0x418a00`/`0x45c0d0`,
`0x44d280`, `0x473460`/`0x473480`/`0x4734a0`) keeps its status.
