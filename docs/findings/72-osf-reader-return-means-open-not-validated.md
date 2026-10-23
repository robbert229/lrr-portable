# 72 — The `.osf` reader reports success after open, even when validation fails

The objective-sidecar reader clears its active table before opening the file.
When open succeeds, it reads the two-dword header and accepts entries only if
the first dword equals `0x534f5252` and the remaining file size is exactly
`count * 0x14` bytes. It returns `1` after closing any successfully opened
file, even when the header or size gate rejected the contents; the return value
therefore records the open path, not successful table restoration.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-checkpoint-reader-expansion.bundle`.

## Parse gate and result

`FUN_00459470` first calls `FUN_004592a0`, which zeros the current count and
capacity and frees/nulls the current table (`ev_ba2448ef43bb719dc56a2cc1b287b21cf8a6703a1564392f195bddde326f4053`).
If the path opens, it gets the file size, reads two 4-byte units, compares the
first to `0x534f5252`, and requires `fileSize - 8 == count * 0x14`. Only that
branch allocates and reads the entries, clones the loaded table to the shadow
snapshot, and clears the dirty flag (`ev_021e2fe21000999a95a77ebd08483338e2894ce93ceb742cd4d664d6cbd597c0`,
`ev_58e2e70c0dc52e169028ba177e09179b95370b51f65757d07162fd2a6f641b10`).

After either validation outcome, the open-file branch closes the handle and
sets the function result to `1`; only failure to open leaves the initialized
result at `0`. The result does not distinguish a loaded table from a rejected
header/length. The read helper can return fewer units than requested, and the
reader does not inspect those return values; no runtime corrupt-file behavior
was tested (`ev_021e2fe21000999a95a77ebd08483338e2894ce93ceb742cd4d664d6cbd597c0`,
`ev_a0a065d16b58f66810fb67b194228dbe8704b71b84aa9c0b096408511b91c8b5`).

`FUN_00429890` constructs the active slot's `Saves\\%i.osf` path and calls
this reader during level loading when the sidecar dirty check is clear and a
slot is selected (`ev_22cae969db6b45e81e1404b6739355a517a1413e3123007025500e7da2e3c359`).
The direct xrefs to the active table pointer remain within the sidecar
management region; no direct objective-entity lookup is established by this
trace (`ev_3e0a9f79d8a5e7b239d93524b534906977cc6000c66a22b48833731b735714a4`).

## Boundary

The comparison constant is reported as stored, without assigning an ASCII
interpretation. This analysis did not execute the parser against malformed
files or establish whether another indirect consumer uses the table.
