# Finding 497 — Render Flags and Checkpoint Progress Fields

`0x0050608C` stores the integer supplied to the `--flags` command-line option. `RenderFeatureFlagsGet` returns this value.

`0x00552100` begins the transient-effect texture handle array loaded from numbered bitmaps; emitters select handles using the configured count at `0x00552130`. `0x00552144` stores the indexed view-blit X coordinate, paired with Y at `0x00552148`; the render helper adds the selected view's offset before positioning the surface.

`0x005529C4` begins a 102-dword block copied into the checkpoint progress record. The whole block is copied, but its individual field meanings are not fully recovered.

`0x00554ADC` and `0x00554EFC` are floating values incremented by one and copied into checkpoint progress. `0x00555120` is an accumulated float copied into that record. Their measured quantities are unresolved. `0x00555328` and `0x0055532C` are integer counters that are incremented and copied to the record, with their domain meanings unresolved.

`0x00555738` accumulates reward score, and `0x0055573C` counts score additions. Checkpoint refresh stores the score total divided by the count as the average and also records the count.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
