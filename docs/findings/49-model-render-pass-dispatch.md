# 49 — Model renderer dispatches two ordered child passes

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

The model-render entry `FUN_00482d60` (`0x482d60`, 269 bytes) is a list dispatcher around the Direct3D viewport setup documented in finding 48. It first calls `FUN_00483600` and `FUN_004838b0`, then walks a linked list of render records.

For each record, bit `0x1` at record `+0x2c` skips the item. Other records go through `FUN_00482ac0`. If bit `0x2` in the pointed-to object's `+0x2c` word is set, the dispatcher uses `FUN_004839c0` and does not run the child loops. Otherwise it traverses the object's child array twice. Each pass selects children for which `FUN_00483a70(child)` succeeds and child flags at `+0x6c` contain `0x80000`; the first calls `FUN_00483a90` only when child field `+0x64` is zero, and the second only when that field is nonzero. It finishes the record walk with `FUN_00482d30`, `FUN_00478940`, and `FUN_00483940`.

This establishes a stable two-pass ordering and an alternate dispatch branch. The semantics of the bits and `+0x64` field are not named by the static evidence; labels such as opaque/transparent or foreground/background would be speculation. The observed caller is the scene end path `FUN_00477150` at `0x477150`.

## Evidence

- `ev_d722ec9e85e35a60f03ef74728a5881604fcb8b9f863c83b5f024ae0b55211f8` — full dispatcher body and its caller/callee inventory.
- `ev_b16410b7dd86246e897f45c09c1efd0a81491f70682066842ba9b31e75a166b4` — viewport setup helper's incoming call from the dispatcher.

Both records are retained in [`legorr-engine-expansion.bundle`](../evidence/legorr-engine-expansion.bundle). The findings describe branch predicates and ordering, not visual material semantics.
