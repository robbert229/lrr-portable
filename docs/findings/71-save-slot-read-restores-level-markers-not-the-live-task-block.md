# 71 — Save-slot reads restore level markers, not the live task block

The `.sav` reader loads each slot's fixed-size per-level records into the slot
context, but the traced restore walk consumes only each record's low dirty bit
to update a separate level-list marker. It does not copy the checkpoint payload
into the live reward/task block. This narrows the “load” behavior documented in
finding 08: the observed path restores campaign/menu state, not an in-progress
mission snapshot.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-checkpoint-reader-expansion.bundle`.

## Reader and tree walk

`FUN_00417a90` opens `Saves\\%i.sav`, reads the `0xb0`-byte slot header, then
allocates `recordCount * 0x3190` bytes and reads that many per-level records
into the slot context (`ev_2c4a344de75b036dcc7038bd17212038781e19d5cc7f38a63f0875010c0a46e0`).
The read is used by save-menu refresh and checkpoint-commit callers as well as
the score scan, so the presence of these records in memory alone does not show
that they are replayed into a running level (`ev_d67036dcdd5a7049ac956ead2056aaa4cf28dcbe46f34ecc8469becafc8a998b`).

The named `SavTreeLoad` wrapper delegates to `FUN_004172f0` and then advances
the list state (`ev_31b85aad15b12c268f04e56b2f8239c588e93b13e2748201b95ee77f472e9dd1`).
For each visited tree node, that walker reads only `recordIndex * 0x3190`'s
first dword and tests bit 0. It uses the result to set or clear bit 2 (`0x4`)
in the corresponding `0x20`-byte level-list entry, increments the list index,
and recurses into child nodes. No checkpoint payload bytes are copied by this
walk (`ev_ed7ea9896bfe7f0a79aed0c00cf96149ccb69c8a2144d52f5e7b73cdbe12f3eb`).

The live reward/task block is separately allocated and zero-filled by
`FUN_0045f030`, followed by names and config-driven loaders; the checkpoint
builder later copies that live block *to* its blob. The direct references to
the task-block global are confined to its initialization, teardown, and
checkpoint-building/display routines (`ev_2bf978b0e0ae4430fa6ec2e212504fefc9c40f1cb7011483f153d38efde3bd49`,
`ev_5174eb185e609be145be6d2fa2cd3268652cde8d1dc9be8a222c382a4ddd4efa`,
`ev_ec7e4664ecbafde4dbf392c1807289d5c3df4690403f82bd00b0f3941a6047b4`).
Level loading has a distinct `.osf` sidecar call, discussed in finding 72;
its body does not call the slot-record reader (`ev_22cae969db6b45e81e1404b6739355a517a1413e3123007025500e7da2e3c359`).

## Boundary

This is a static trace of the save-slot reader, the tree restore walk, and the
direct task-block references. It establishes what those paths read and write;
it does not prove that no other indirect or unclassified route can use a
checkpoint payload. The runtime meaning of the list-entry bit remains the
existing finding's UI/progression interpretation.
