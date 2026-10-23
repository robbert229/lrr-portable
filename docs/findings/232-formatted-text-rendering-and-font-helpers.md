# Finding 232 — Formatted Text Rendering and Font Helpers

The text-rendering call chain shows that several remaining anonymous routines
format and draw text through the engine's font atlas, including inline bitmap
tokens.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x401c50 | FormattedTextPrintfWidth | Variadic wrapper that formats and draws text, returning the measured maximum line width. Fourteen callers span menu, UI, diagnostics, and reward text. |
| 0x401c80 | FormattedTextPrintf | Variadic wrapper that formats and draws text with caller-provided position/style arguments. Its callers include scene-object and UI text paths. |
| 0x47a150 | FormattedTextFormatDraw | Passes a format string and argument list to the shared renderer, optionally writing the measured width through an output pointer. |
| 0x47a1e0 | FormattedTextDrawAndMeasure | Expands %b to an inline bitmap token, handles newlines and tab stops, draws glyphs and embedded bitmaps, and returns maximum line width while optionally reporting line count. |
| 0x47a470 | BitmapFontGlyphDrawAndMeasure | Maps a supported character to its font-atlas cell, optionally draws that glyph at the requested coordinates, and returns its advance width. |

**Evidence.** The direct decompiles are in
ev_7290f0003287568f5038e51ace3fddf0aa9e9ed9fa461113280a710be7d0854e and
ev_31c487b2dd170e87945cec36934fa9d299586443f19c04c8f48fb09629f66bfe,
preserved in
[legorr-engine-helpers-expansion-7.bundle](../evidence/legorr-engine-helpers-expansion-7.bundle).
Caller records for the wrappers and glyph routine are in the same bundle. The
source is shipped reference/LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f.
