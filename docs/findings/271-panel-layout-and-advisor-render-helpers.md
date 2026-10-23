# Finding 271 — Panel Layout and Advisor Render Helpers

Four panel and advisor helpers have names supported by their direct writes and
call contexts. Two are linked to configuration-driven panel layout; the exact
render-interface method behind the advisor fill remains unidentified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x45d060 | PrioritiesImagePositionsConfigLoad | Reads ten numbered `PrioritiesImagePositions` configuration pairs and stores them in a global two-float table. Its sole observed caller is 0x41fb50; the caller's specific semantic role remains unresolved. |
| 0x45a650 | PanelCategoryBoundsSet | Writes four integer inputs as two bound pairs into a category-indexed, `0x30`-stride table, clears a state field, then mirrors one pair according to flag bit 1. Its caller 0x4345f0 parses `Panels%ix%i` config entries and supplies the coordinates. |
| 0x45a270 | PanelCategoryInteractionStateReset | Restores saved coordinate pairs across twelve `0x30`-stride category records, updates category state bits, clears child status/flags, and clears global `0x500820`. Its caller is the level teardown path at 0x42f0c0. The category interpretation follows its pairing with the panel config writer. |
| 0x45c3f0 | AdvisorProgressBarFillDraw | When a target surface exists and progress is positive, clamps the percentage to 100, scales fill width, derives the top-left from category-3 bounds plus offsets, and calls render helper 0x47dcb0. Its caller is `AdvisorTextDisplayWorker` at 0x46ad00; the precise draw method semantics remain unknown. |

**Evidence.** Direct pseudocode and caller chains are in
[legorr-ui-render-fourth-pass-2026-10-23.bundle](../evidence/legorr-ui-render-fourth-pass-2026-10-23.bundle):
`ev_c616a249c498db1772dd5435f13a0cfe35adb91d5b271b61cd6f8cbf329678e3`
(priority positions),
`ev_61239ec3448d0c671e2ca78d239dd8580ed42a975e874be32e7d9f62bcf63995`
(its caller),
`ev_d276d706f5dff5ec406d96cb78f2a752020ce774ce13bbdb2d328a2e458a6328`
(panel bounds),
`ev_c6450d74294f07d0865f821b836609034072bcdbadc6f26ef7aa3318e774f4fa`
(config-driven caller),
`ev_f5ea01a1c5f3ae428cf5a895196f1fc773ffd20ea3714adb69cd213202d3ae26`
(panel reset),
`ev_246a639b9c77b9d1d75579bcd3b7eba45485c4fc80ff8c25c43d1e28d9c2935e`
(level teardown caller), and
`ev_023bc06d0aa14325a5d500f45ab543525added8defb7b03423f0a281302bad63`
(advisor fill). All observations use static analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
