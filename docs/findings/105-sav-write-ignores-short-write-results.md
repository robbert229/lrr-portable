# 105 — Slot-save success ignores partial write results

`SavWriteSlot` returns success after it opens a slot and issues its writes, without
checking whether the file-write helper wrote every requested item. The lower
file layer returns a partial item count on a short or failed write, so the
slot-level return value does not establish that the `.sav` contents or `.osf`
sidecar were fully persisted.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-save-write-results-and-count-reset.bundle`.

## Write result path

`FUN_00417bd0` (`SavWriteSlot`) checks that the slot number is below six and
that the file open returned a nonzero handle. In the new-slot route it writes
a `0xb4`-byte initialized header and then one `0x3190`-byte record per current
level; in the existing-slot route it writes a `0xb0`-byte header and, when
present, the record array. Both routes call `FUN_0047fa50` for writes, close
the `.sav` handle, and return `1` without branching on those write-call
results. The existing-slot route also calls the `.osf` writer; that call does not gate
the `SavWriteSlot` return (`ev_6f9375d8aac886f1c09efc546729e2fd0909661ed9369674386dbe3ba8b06adc`).

`FUN_0047fa50` rejects an invalid file-handle class with `0`, otherwise returns
the result of `FUN_0048f500` (`ev_73508c7e7ac4388f7994355cc9f263feaf7b47828ccbb2b5fa87265140d7ef97`).
That lower writer computes requested bytes as item-size × item-count. On its
file-stream path, a failed or short byte write sets the stream error bit and
returns the number of complete items written; it returns the requested item
count only when the requested bytes are handled (`ev_1829104c31f50f01d7d14a17bfdfd934e343609c360d5c08ad33a5505152d7af`).

Thus, `SavWriteSlot`'s `1` indicates that the selected open/write/close control
path reached its success return. It is not a full-write confirmation. The
higher-level `FUN_00417fc0` caller also discards `SavWriteSlot`'s result
(`ev_78cd95279c78b99c451333d718d964a1eacd77d7078ebe64ea200da888a23d08`).

## Limits

This is static control-flow evidence. No short write or filesystem error was
forced at runtime, so the finding does not establish that a shipped save is
truncated or that the underlying provider will produce a partial write in a
given environment. It establishes that the save routine does not propagate the
available write status when one occurs.
