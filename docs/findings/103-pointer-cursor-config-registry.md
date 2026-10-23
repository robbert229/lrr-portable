# 103 — Pointer cursor config uses a fixed action-name registry

The cursor subsystem recognizes a fixed set of 56 `Pointer_*` names, from
`Pointer_Standard` and `Pointer_Blank` through command-specific and surface-type
variants. Its initializer fills that name table, then the cursor loader maps
each supplied config entry to a slot with a case-insensitive name comparison.
The loader accepts either a bitmap pointer resource or an animated cursor
resource with two stored hotspot coordinates.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-pointer-cursor-config-and-override.bundle`.
Focused IDs: `ev_53a0e1d0b9c20e515ad95cd3a86a8faef76c17292a0ee08f5a5abfe7bec2d23a`
(`FUN_0045c830`), `ev_e4d4b47c8f83e2816af4599d927a4190e715ddc897f322e960bb74173f7d57f1`
(`FUN_0045ca70`), `ev_2f82d72f3d45507b66672a516f8d9cb31f760b312847c1c79edc7d4b79f3c72d`
(`FUN_0045cbd0`), and `ev_a2ae4d538e8dda137abd5b4a7a3fb4b6d0d309ea572f33253d354246a5bbe3c7`
(loader caller inventory).

`FUN_0045c830` assigns 56 pointer-name strings into the registry. The loader
`FUN_0045ca70` walks config entries, resolves recognized names through
`FUN_0045cbd0`, and parses each entry's data. A one-field parse loads a bitmap
surface and marks the slot static. The other parse path loads an animation
resource and stores two additional values beside the slot; the draw helper
uses these values as cursor offsets. Both the name-table initializer and the
loader have the message/config initializer `FUN_0041fb50` as their sole
resolved caller.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| 56 fixed `Pointer_*` names populate the registry | observed in `FUN_0045c830` |
| Name lookup is case-insensitive and bounded by the registry | observed in `FUN_0045cbd0` |
| Loader distinguishes a bitmap surface from an animated resource with two offsets | observed in `FUN_0045ca70` |
| Exact shipped config values and assets for every cursor name | not enumerated in this pass |
