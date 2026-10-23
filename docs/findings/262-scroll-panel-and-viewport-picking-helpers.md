# Finding 262 — Scroll Panel and Viewport Picking Helpers

Five UI/rendering procedures now have names supported by direct decompiles and
resolved callers. They include a callback that uses the existing
`TerrainPointPickResolve` mapper; that callback's application-specific meaning
remains unknown, so its name describes only the dispatch path.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x45b740 | ScrollInfoPanelInitialize | Initializes the `Interface_InfoPanel_scrollInfo_b` panel, creates two scroll-state rectangles (50×12 and 0x50×0x12), builds a 0x28-byte record, and registers a roughly 44×43 widget. Sole caller: 0x45a240. |
| 0x45e660 | RectContainsPointHalfOpen | Tests `left <= x < left + width` and `top <= y < top + height`. Called from 0x425d90 and 0x426520. |
| 0x45e820 | ViewportPointCallbackDispatch | Uses the existing `TerrainPointPickResolve` mapping at 0x45e6d0, derives a vertical step from the cell query, invokes a callback through 0x4377b0, and optionally returns the callback's pair. The callback's exact object-level semantics are not resolved. Sole caller: 0x425d90. |
| 0x463770 | ScrollRectResourceRecordCreate | Allocates a 0x1c-byte record, stores rectangle bounds and two converted resource fields, and writes one of two pointer slots. Sole caller: 0x45b740. |
| 0x463810 | ScrollRectPayloadRecordCreate | Allocates a 0x28-byte record containing bounds, a converted resource handle, and two additional values. Sole caller: 0x45b740. |

**Evidence.** Direct decompiles and caller records are in
[legorr-ui-render-audit-2026-10-23.bundle](../evidence/legorr-ui-render-audit-2026-10-23.bundle).
Key decompile records are
`ev_24770cf6fe9845db10074e499cf5d82ce4a299f3b3b6067f1829a6766356760b`
(panel initializer),
`ev_415767c4f0576dd12049b19fad0db867e6a45ce04ba1cdc4ca9f36bfcd5d688e`
(half-open rectangle test),
`ev_420c2dafd16307a196526909359d1d0ff8156c1f4dfaf11b6cdfa0fb90fcf480`
(callback dispatch). The panel constructors' caller records are
`ev_067463ec1683414c4610318da44c5a28d87abdd5ff2f8f0f5c6092f9293ead6d`
and `ev_d9fa327294bed5c76a02ee84c39826e42c9766fa9755c96b2d7a5ce7a892fb81`.
All observations concern the shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
