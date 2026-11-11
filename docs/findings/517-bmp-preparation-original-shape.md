# Finding 517 — BMP preparation (G98-026) in original shape

Issue #414. The ADR 0005 scaffold moves the two functions at
`00489c30`–`00489d90` into `src/gods98/g98_026.c` and its C boundary header.
Before is the baseline at `c7fea709`; after is the matching build.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00489c30` | `indexed_bitmap_palette_data_prepare` | 32.40% | 70.54% |
| `00489d70` | `object_field_38_release_target` | 100.00% | 100.00% |

## Boundary and contracts

The dossier shows no data attributed to either function. Both are called by
`00474050` and `0047d9d0`, which prepare the same 15-slot BMP record then
release its palette at +0x38. The preceding scene functions use `00505914`
and have unrelated callers; the following DirectX diagnostics use their own
state and strings. This supports retaining the mapped two-function leaf,
though its exact boundary remains low confidence: no data step proves the start,
and 16-byte alignment and NOP padding do not distinguish C objects (finding
505). The spec records the evidence rather than claiming higher confidence.

The target routine copies the declared palette without bounds or allocation
checks, leaves the channel fields alone for non-indexed images, and ignores
its size argument. The portable texture decoder validates input before calling
it; short Windows headers are rejected before accessing their fields. The
record uses widened pointers on native hosts. The target's unaligned BMP
loads follow the original x86 representation. The separate portable release
wrapper retains its existing guarded interface.

## Callers and entropy

The two original callers are not mapped in this reconstruction, so no
before/after scores exist for them. Their portable texture path still passes
the real-WAD tests. `surface_bitmap24_bit_surface_convert` (`0047d800`) is a
neighbour of one caller, not a caller or a function in this unit; its score
remains 29.34%. The existing declarations retain their positions and receive
C linkage in place. The full gate reports no drops, lost mappings or newly
mapped functions. The only changes are the unit's 38.14-point gain and
`text_buffer_draw_wrapped` rising 5.96% to 5.99%; net +38.17 points across
common functions in both scopes.

## Validation

`make reccmp-check` passes against the previous baseline. `make match-progress`
regenerated both reports inside the developer distrobox. Full accuracy changed
54.23% → 54.24%, game-only 49.91% → 49.93%, exact-function bytes remain 41,053,
and `.text` bytes identical at original addresses changed 18,225 → 18,216;
the last measure depends on image layout. The regenerated unit map has zero
data conflicts. The native build and all native tests pass, including
`c_boundaries` and texture tests with shipped WADs. The new source passes the
GNU89 declaration-after-statement check.
