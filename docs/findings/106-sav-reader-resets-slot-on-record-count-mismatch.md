# 106 — A save record-count mismatch reinitializes the slot

The slot reader compares the record count stored in the `.sav` header with the
current menu-level count. When they differ and the reader is called in mode
zero, it sends the slot through `SavWriteSlot(slot, 0)`, clears the in-memory
slot context, and reports a read miss. The slot scanner passes mode zero on its
initial read and retries a miss with mode zero, so this path attempts to
reinitialize and reread a slot whose record shape no longer matches the
current menu tree.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-save-write-results-and-count-reset.bundle`.

## Reader fallback

`FUN_00417a90` (`SavReadSlot`) opens `Saves\\<slot>.sav`, reads the `0xb0`-byte
header into the slot context, and counts the current menu-level tree. If the
header's record count (`context[6]`) differs from that current count and
`param_3 == 0`, the reader calls `FUN_00417bd0(slot, 0)`, clears `0x2e`
dwords (`0xb8` bytes) at the context pointer, and returns `0` rather than
reading the saved record array (`ev_1a30393712f0072b84031fd1ae98aeb864e5fcb3521728ff0c9588ae928a140f`).

The scanner `FUN_00417df0` pushes zero as the third argument before its initial
`SavReadSlot` call. If that call returns zero, it calls `SavReadSlot(slot,
context, 0)` again. The disassembly shows both the initial zero argument and
this explicit retry (`ev_1ff40ee7b34feca9a8a9f873ea01bc65a0943b81710149a214e9b108ca758a`).
The reader's zero-mode mismatch branch therefore triggers during the ordinary
slot scan, and the caller then attempts to read the freshly initialized slot.

## Limits

The code establishes a count-based reset and a retry attempt; it does not
identify why the counts differ or guarantee the rewrite and second read
succeed. The scanner ignores the retry's return value. The comparison concerns
the saved record count and the currently enumerated menu-level tree; calling it
a save-format version check would go beyond this evidence.
