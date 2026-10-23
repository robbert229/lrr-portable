# Finding 135 — The Stream Marker Persists Across Later Variants in One Samples Entry

`FUN_00464d00` initializes its local stream-mode value to zero before iterating the comma-split paths. When a path begins with `@`, the parser sets that value to one and passes it to `FUN_0047a870`. The loop does not clear it before processing the next path. Therefore, within one multi-value `Samples` entry, an `@`-marked path changes the loader mode for every later path as well; earlier paths retain the mode they had when processed.

The extracted shipped `Lego.cfg` has no comma-separated `Samples` value containing `@`: its streamed assets are registered as one path per alias. So this stateful behavior is established for the parser, while its practical effect on shipped content is not observed. Whether the marker was intended to apply to the remainder of a list or this is an implementation quirk remains unknown.

**Evidence.** The `FUN_00464d00` decompilation in `docs/evidence/legorr-samples-config.bundle` (`ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af`) shows the single initialization before the item loop, the conditional `uVar7 = 1` on `@`, and the later call `FUN_0047a870(pcVar8, uVar7, cVar1 == '*', uVar4)`. The config check used `Lego.cfg` from `reference/LegoRR1.wad`; source and extracted-config digests are recorded in finding 134.
