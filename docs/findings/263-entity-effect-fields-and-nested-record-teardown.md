# Finding 263 — Entity Effect Fields and Nested Record Teardown

Two simulation helpers have direct behavioral evidence and named caller
context.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x44c150 | EntityPairScalarFieldsSet | Accepts entity type words 1, 2, or 3; resets entity state through 0x441b30(entity, 0, 1); stores a pair at entity offsets +0x3b4/+0x3b8 and a scalar at +0x3bc; then sets bit 0x4000 at +0x3e4. `EffectApplyByKindBranch` calls it with a 1.5-scaled position pair in its type-2 branch; another caller provides float bit patterns for 2.0, 1.0, and 40.0. |
| 0x44e670 | NestedPointerRecordDestroy | Releases two object/interface pointers through the known object-release helper 0x472a40 and frees two nested allocations through 0x48db80. Called during teardown at 0x42f258 inside 0x42f0c0. |

**Evidence.** Direct behavior for 0x44c150 is in
`ev_db4fc6d3552d692b554be5273e88fc50b80bc08b2d72dfdf029e2344ec6b0b74`;
resolved caller sites and context are in
`ev_f4afdec5d700cba5c431fd7facf07914690866afd44fae0331094328cefbb86e`.
The 0x44e670 decompile and teardown caller are in
[legorr-sim-terrain-next-followup-2026-10-23.bundle](../evidence/legorr-sim-terrain-next-followup-2026-10-23.bundle):
`ev_967c5aeda4a9cde640cbf82a52f8bf3b0da54e0bdfce120a87359f2c18561171`
and `ev_4c7949d1e74667991b4ca1fdd4a4fd2ede2153639968aba509bbb995277171c9`.
All observations use static Ghidra analysis of the shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
