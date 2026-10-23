# Finding 211 — Advisor Sound Token Is Latched Before the Audio Gate

When `FUN_00456830` resolves a known `#name#` sound token, it compares the
token's table index with `DAT_00555dbc`. If they differ, the function stores
the new index in `DAT_00555dbc` before calling `FUN_00465370`, the sound-enabled
predicate. The call to `FUN_0047ad70` that submits the sound is inside the
predicate's enabled branch. The remembered index is therefore updated even
when sound is disabled and no playback submission occurs.

Because the parser skips the sound block while the resolved index equals the
remembered one, a token first encountered while sound is disabled is not
submitted later if the same token remains active when sound is re-enabled. A
different sound-token index must pass through first to change the remembered
index. This is a conditional static control-flow result; it does not establish
how long a particular advisor token stays active in a level.

Finding 94 documents the token lookup and repeat suppression. The additional
point here is the ordering of the cache write and the audio-enabled check.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
In `FUN_00456830` at `0x456830`, the differing-index branch writes
`DAT_00555dbc = uVar6` before calling `FUN_00465370`; the `FUN_0047ad70`
submission is nested inside the subsequent nonzero test. `FUN_00465370` at
`0x465370` returns `DAT_00502a2c & 1`. The decompiled functions are in
`docs/decompiles/full/LEGORR.full.c`; finding 94 cites the inline-markup
evidence bundle.
