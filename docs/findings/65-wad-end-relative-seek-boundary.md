# Finding 65 — WAD end-relative seeks bypass the range clamp

*Target: shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.*
*Provider: Ghidra 12.1.4 via REA. Evidence: `docs/evidence/legorr-wad-stream-contract.bundle`.*

The WAD branch of `FUN_0047f850` handles three seek-origin values. Absolute
(0) and current-relative (1) seeks clamp the cursor into `[0, entryLength]`.
End-relative (2) instead assigns `entryLength + offset` and returns without
clamping. The generic file-to-memory helper uses end-relative seek to measure a
file and then seeks back to zero, confirming the conventional origin mapping
for that call site (`ev_8a19036ea1a2a1b56c0f49c671a0debe37d49951f7135bd834043b3772f08029`,
`ev_8a576e71604114cae565a89bc4bb0349e7a02fa9b18129e9b65f1c2aab8d6503`).

This is observable as a difference between the three WAD seek branches. A
positive end-relative offset can leave the cursor beyond EOF; a sufficiently
negative one can put it before the buffer. **Derived risk, not runtime-tested:**
the WAD read branch only clips when `entryLength < cursor + requestedBytes`.
When the cursor already exceeds the entry length, it computes
`entryLength - cursor` and stores that result in an unsigned byte count before
copying from `data + cursor`. That arithmetic can turn a negative remainder
into a very large copy length. The shipped binary's code therefore does not
establish safe reads after an out-of-range end-relative seek
(`ev_a0a065d16b58f66810fb67b194228dbe8704b71b84aa9c0b096408511b91c8b5`).

The evidence establishes the branch asymmetry and the derived arithmetic path,
not that a normal shipped caller supplies a positive out-of-range offset or
that a particular malformed input reaches the copy. The CRT seek branch is
delegated to a separate file helper and is outside this finding.

