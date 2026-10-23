# Finding 275 — SceneAct Frame Record Clone or Create

The helper `SceneActFrameRecordCloneOrCreate` at `0x489660` allocates a
0x20-byte frame-record block and follows
one of two paths selected by the input record's `+0xc` field. A nonzero value
copies the eight dwords, clones the referenced object through `0x487a00`, and
clears the new record's first field. A zero value initializes a fresh record,
retains its owner through interface slot `+0x38`, initializes an associated
interface through slot `+0x10`, allocates an array sized by the input count,
and registers a callback through `0x489910`. The optional output receives the
input record's `+0x1c` value.

The scene animation assembler `DDiSceneActFrameRecordParse` at `0x475d10`
calls this helper when its optional frame-record argument is non-null. A
second caller at `0x473690` is also present in the direct analysis record. The
frame-record fields and interface
slot meanings remain unresolved, so the name describes the clone-or-create
behavior without assigning a more specific rendering role.

**Evidence.** Direct pseudocode is in
[legorr-engine-helpers-expansion-22.bundle](../evidence/legorr-engine-helpers-expansion-22.bundle),
record `ev_e0c58e5bb04ee83c0327854fd9e50249b5fc8c65c5e199787622b70bf41060d8`.
Direct analysis and the scene animation caller are in
[legorr-ui-render-third-pass-2026-10-23.bundle](../evidence/legorr-ui-render-third-pass-2026-10-23.bundle),
records `ev_d3e7694128a176bf999d70fae1ccfc450bf7ae69423540f2da83e32fae50b7ac`
and `ev_6a9a133b060dc50c093e1d51f804207de841587f236d1c1e2656821547921806`.
All observations are static Ghidra analysis of the shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
