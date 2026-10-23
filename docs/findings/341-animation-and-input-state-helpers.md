# Finding 341 — Animation and Input-State Helpers

Three helpers advance wrapped animation frame offsets, interpolate a keyframe location, and toggle interface input state.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x482040 | AnimationTrackApplyWrappedFrameOffset | Updates enabled animation records by adding a frame offset, wrapping by each record’s frame count, and applying the resulting frame. |
| 0x483810 | InputActionStateToggle | Calls interface input-state slots and tracks/toggles the value associated with the supplied input index. |
| 0x487cb0 | AnimationTrackFindKeyframeAndFraction | Binary-searches animation records by timestamp and returns the keyframe index and interpolation fraction. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct analysis for `0x482040` is in `legorr-engine-helper-audit-text-io-expansion-9.bundle`, record `ev_52089e3fd8e434de24a204f8237644aafe21e6af89c3a88348f6c1f8f2559683`. Direct evidence for `0x483810` is in `legorr-engine-helper-audit-high-expansion-6.bundle`, record `ev_7a32ee86033f5cb891e5c82fc7d437226177208c1309bc81cc5b2869e35687ec`; the related render-expansion record is `ev_77d33b453abb5e0818a3c20d7632b32618254e6b6e550b92fed8257ce6b6b323`. No direct retained procedure record was found for `0x487cb0`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
