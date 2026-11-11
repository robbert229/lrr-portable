# Finding 532 — Config's Direct3DRM prefix and remaining score drift

Issue #481; builds on findings [524](524-d3drm-header-constants-in-rdata.md)
and [529](529-unit-relative-data-offsets.md). The original G98-006 Config contribution begins at
`0049e8e8` with the four Direct3DRM constants, then the four annotated
floats at `0049e8f8`–`0049e904`. The SDK's `d3drm.h` includes
`d3drmdef.h`, whose four `static const` definitions reproduce that prefix.
The original Config include chain is not established by the reconstructed
headers or decompile; the matching build includes the SDK directly. The
native and reference builds retain their current portable declarations.

A matching build on the pre-#482 tree followed by `unit_data_offsets.py` reported:

| Unit | Before `.rdata` | After `.rdata` |
| --- | --- | --- |
| GAME-003 | partial, 3 anchors, first `0049e048`: `14/0` | same |
| G98-006 | partial, 4 anchors, first `0049e8f8`: `10/0` | full, 4 anchors |
| G98-007 | partial, 7 anchors, first `0049e920`: `10/0` | same |
| G98-019 | partial, 4 anchors, first `0049ea20`: `10/0` | same |

After rebasing onto #480, #482 and #483, `make unit-data-offsets` reports
the same four section results. Offsets are original/rebuilt hexadecimal. `objdump -s -j .rdata` on
`config.c.obj` shows `01 00 00 00 02 00 00 00 04 00 00 00 ff ff ff ff`
at offset zero, followed by the four floats. The section-wide at-original-address `.rdata` comparison remains
1,482 / 11,232 bytes on the rebased tree, unchanged from main; this is a
unit-relative improvement, not a claim of absolute placement.
`make reccmp-check` passes with no score or instruction-count changes in
the accepted Config-only build. The three progress files were regenerated.

An experiment including the SDK in all four units and explicitly placing the
unreferenced `1.0f` at `0049e044` after Bezier's SDK constants made all four
objects begin with the expected sequence. It failed the score gate, however:

| Function | Score before → experiment | Classifier and instruction delta |
| --- | --- | --- |
| `004065f0` cubic_bezier_point_evaluate | 56.88 → 53.21 | drift; +0 |
| `00479ab0` point_along_ray | 100.00 → 91.30 | drift-review; +0, owner review |
| `00479af0` line_intersection_2d_compute | 86.67 → 81.67 | drift-review; +0, owner review |
| `00485ea0` viewport_subrectangle_set | 100.00 → 97.40 | drift-review; +0, owner review |
| `004869a0` Draw_Pixel24 | 100.00 → 71.43 | drift-review; +0, owner review |
| `00479630` surface_triangle_area_calculate | 42.30 → 62.93 | improved |

Those experimental changes were removed. A deferred-initializer experiment
preserved both Draw scores in one probe but did not consistently preserve
scores across the other units. The original Bezier word is an unreferenced
IEEE-754 `1.0f` (finding 514); no available evidence establishes its original
declaration or shows that a header emitted it. The remaining work needs an
include chain or other evidence that restores those three prefixes while
satisfying the score gate; this finding does not claim it has been found.
