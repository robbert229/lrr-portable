# Finding 220 — AVI Timeline Advances When the Movie Surface Depth Is Unsupported

The credits AVI row-copy helper writes pixels only for its 15-bit and 16-bit
surface-format cases. If the movie surface's stored format selector matches
neither case, the helper returns without copying. A non-null frame fetch still
causes `FUN_0047eb20` to update the time-derived frame index, and the credits
loop subsequently blits the movie surface. Under that condition, playback
continues to advance while the displayed surface retains its previous pixels
(or its initial contents if no supported frame was copied yet).

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_0047e9e0` (`0x47e9e0`) enters its row-copy loops only when the surface
state's selector resolves to 15 or 16; there is no fallback copy branch. The
helper has no status result. `FUN_0047eb20` (`0x47eb20`) calls it after a
non-null `AVIStreamGetFrame` result and then calculates and stores the next
frame index without checking whether the helper copied rows. In
`FUN_0040a0c0` (`0x40a0c0`), the loop calls `FUN_0047ed00` after each frame
update, and that function blits the movie surface whenever the movie object is
initialized. Finding 216 documents that frame retrieval is opened with
`AVIGETFRAMEF_BESTDISPLAYFMT`; this finding does not establish which surface
format a particular runtime selects.
