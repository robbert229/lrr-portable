# 76 — Entity teardown drains the attached command chain by flags

The command-chain head at entity `+0x2f0` is not freed uniformly when the
entity teardown path reaches `FUN_00403a50`. With the global `DAT_004b8060`
bit `0x2` clear, each linked record is routed by flag bits: some records are
reclassified onto one of two global lists, while the rest go through the
command-record free helper. The entity head is cleared only after the chain has
been walked, then a separate pool-wide callback scan runs.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-entity-destruction-chain-and-payload-expansion.bundle`.

## Drain policy

`FUN_00403a50` has one resolved direct caller, `FUN_00437540`, the entity
teardown routine. If `DAT_004b8060 & 2` is set, the helper returns without
walking or clearing the entity chain. Otherwise, it saves each record's
`+0x60` next pointer before changing that record, then routes by the word at
`+0x5c` (`ev_d75cc5274f8e68bd43f17a337eac45fdd773767988e4deb1da93fb0e12efeeb6`,
`ev_1019b4257018e0db8c5aed431dd62d32b13cc4877553429e4bf9757ef1b8dff5`).

| Chain-record condition | Observed disposition |
| --- | --- |
| bit `0x10` set and bit `0x20` clear | `FUN_00404030` links the record onto `DAT_004b3354`, writes `0x44bb8000` at `+0x14`, changes flags to `(old & ~1) | 0x10`, maps its saved `+0x4c` index through `DAT_004b32e8` into `+0x1c`, then writes the index back to `+0x4c` (`ev_8d32153244d12608882d2a98080f0a21d2ae767df6d04b9974bd3901d6192d39`). |
| otherwise, bit `0x2000` set and bit `0x20` clear | `FUN_00404070` links the record onto `DAT_004b3358`, writes the same `+0x14` value, and clears flag bits `0` and `4` (`ev_25583b39c26d7e113ff403d4886ed85be6e30753bf7d631092ad430c0a0ebdc0`). |
| all other combinations | `FUN_00406480(record, 0)` runs the command cleanup, conditionally frees the extension pointer at `+0x40` when record flag `0x200` is clear, and links the record onto `DAT_004b31f8` (`ev_d9d44e85b70fc60a2646eb26b1c50d3843a0d6924e418691c6b76ce552e5d9d4`). |

After processing the chain, the helper writes zero to entity `+0x2f0` and
calls `FUN_00406540` with callback `LAB_00403ad0`. The walker scans allocated
record blocks (stride `0x68`); it calls the supplied callback only for entries
whose `+0x64` link points back to that same entry, and stops if the callback
returns nonzero (`ev_d75cc5274f8e68bd43f17a337eac45fdd773767988e4deb1da93fb0e12efeeb6`,
`ev_187d4a339b5e07c13bc6c61a676206b77b6259f979e7f099d14d154a585b7264`).
The callback's domain meaning is not assigned here.

## Boundary

The flag tests and list/free-list writes are observed; names such as “pending,”
“persistent,” or “cancelled” would be semantic guesses and are not used. The
analysis identifies this helper's resolved direct caller, but it does not prove
the allocator origin or exclusive ownership of each record and extension
pointer, nor does it resolve indirect callback behavior.
