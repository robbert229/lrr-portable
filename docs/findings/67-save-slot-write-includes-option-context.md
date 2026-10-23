# 67 — Save-slot writes include the option context in their fixed header

The front-end helper writes three option-state globals into the selected slot
context before the save path snapshots that context. The save routine then
writes a fixed header and per-level records through the file abstraction. This
connects the option data flow in finding 66 to a save-file write path; it does
not imply that the executable observed a successful save during this static
analysis.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-options-context-expansion.bundle`.

## Snapshot and file route

`FUN_00462030` calls `FUN_00416910` to copy the three globals into the active
context, then calls `FUN_00418040` to build a local snapshot. The snapshot
copies `0x2e` dwords (`0xb8` bytes) from the context and allocates/copies the
context's per-level data region using `0x3190` bytes per record
(`ev_aeeb9678a4ef9ab1a01ffd9ad9909647030e1a81034276de21816af6a19ce8eb`,
`ev_3921d653759132baa3b6baf05c25d315baba920c6895dae6a2c247e72edd4dfc`).

The slot writer `FUN_00417bd0` constructs the path `Saves\%i.sav`. Its
existing-file branch passes a `0xb0`-byte context header to `FUN_0047fa50`
and, when the slot has level records, writes `0x3190` bytes per record. The
write wrapper checks the file-handle class and forwards to `FUN_0048f500`. The
same function contains a new-file branch that writes a `0xb4`-byte initialized
header before any level records. After the update branch, it also constructs
`Saves\%i.osf` and passes that path to `FUN_004593e0`; this finding does not
assign the helper's meaning or the sidecar's role
(`ev_19ce61f1ac3af7e6f0b04a7f14e36493db5ecd3792e2ecfb8bee668538ac6416`,
`ev_bfe58fbf2b3d5307fdda218fc7efa2ad3e2787e277f1a694fc90641b5bd5c551`).

Because finding 66 locates the three option-state dwords within the first
`0xb0` bytes of that context, their values are part of the header buffer sent
to the write wrapper. The analysis establishes the static serialization path
and path formats, not that a particular save file exists or that a runtime
write completes.

## Boundaries

- The `.osf` path's role is unknown in this trace.
- The snapshot copies `0xb8` context bytes, while the existing-file write
  branch sends `0xb0` bytes. The option offsets described in finding 66 fall
  inside both spans; the remaining eight context bytes' role is not assigned.
- No runtime save/load was performed, and no on-disk save was compared with
  the serialized buffer.
