# 110 — A delegated objective predicate searches entities for paired-field matches

After its timer, crystal, and ore checks, `FUN_00459050` delegates another predicate through `LAB_00459100`, passing the level-state address plus 0xbc to the registered-entity iterator. The iterator invokes the callback for candidates and returns when one passes.

The callback has two flag-gated paths. With level flag 0x400 set, it considers entity kinds 1 and 2, calls `FUN_004424f0`, and compares the helper's two outputs with level-state fields +0xfc and +0x100. With flag 0x1000 set, it compares candidate fields +0x4c and +0x50 with level-state fields +0x108 and +0x10c. These are existence checks over the iterator's candidate table.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-objective-entity-paired-predicate.bundle`. Focused records: `ev_1d5be4a8ee35d532f5533b2e28b4a59d9a30a156f8558af4ef63322faf41f169` (objective evaluator); `ev_bcc123a8785d5f592afd9919d0cd4ffc0cdadc630a982bd4ef43b3a4d29d20f3` (delegation); `ev_4862e283e32dbb4833b15f4bb262e95792ab3d7ff64a1ccd5a8751119db3b35e` (callback bytes); `ev_8783fc3f1cd6e5815a810bad6d03bbf20198420718f2f7cff6227ef38c0a3b79` (entity iterator); and `ev_081039914ca326fcde0b3d75fc2a11f11f201ad85b57f92dee34e3a1cba6bb09` (comparison helper).

**Limitations.** The meanings of the compared fields and helper outputs are unknown. Static evidence does not enumerate runtime configurations or establish which path a live level takes. Finding 45 documents the timer short-circuit; this finding closes one delegated predicate's mechanics.
