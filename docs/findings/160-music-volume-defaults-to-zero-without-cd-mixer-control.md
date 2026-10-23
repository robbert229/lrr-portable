# Finding 160 — Music Volume Defaults to Zero Without a CD Mixer Control

The Options initializer sets `DAT_00557038`, the `Music_Volume` slider value, from `FUN_00414e10`. That getter starts with zero and returns a converted mixer reading only if `FUN_004789a0` reports success. The underlying `FUN_004789c0` getter reports success only after it reads a volume control from a source line whose component type is `0x1005` (the compact-disc mixer source, finding 159).

If there are no mixer devices or none exposes a matching CD-source volume control, the lookup returns failure and the slider remains zero. The initialization path provides no nonzero fallback. This documents code behavior for a missing mixer control; it does not establish which controls a given Windows audio driver exposes.

| Initialization condition | `Music_Volume` value |
| --- | ---: |
| Mixer CD-source volume lookup succeeds | Derived from the mixer reading |
| No mixer device or no matching CD-source volume control | 0 |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00414e10` and its Options initializer in `FUN_00415220` are in `docs/decompiles/full/LEGORR.full.c`; they are recorded in `docs/evidence/legorr-options-context-expansion.bundle` (`ev_e96d3527e95453ee43682a867419b9f9319ee9b78e0be605179c92980adc55d4`, `ev_ec2a2ba36674b50e709598f83eb1dc97173306ca6adda9f6f55068a67d351ad0`). The mixer getter is `FUN_004789c0` in the same decompile. The slider label and range are in `Lego.cfg` inside `reference/LegoRR1.wad` (SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`).
