# 66 — Front-end option state round-trips through six save contexts

Finding 23 identifies the menu callbacks. This trace follows the associated
runtime globals into their selected slot context and back. It establishes a
per-slot data flow; it does not establish a separate machine-wide preferences
file or assign user-facing meanings to every stored dword.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-options-context-expansion.bundle`.

## Slot context and option globals

`FUN_00417e50(index)` returns `DAT_00557080 + index * 0xb8` only for indices
0 through 5. `FUN_00417e90()` returns `DAT_005574d0`, and
`FUN_00417e70()` returns the context for that index unless the global is
`-1`, in which case it returns null. These routines establish six fixed
context records and a current-context selector, but do not by themselves
establish how each index is presented in the UI
(`ev_2baef27b8a2348be98fbda71b002ffce1171bfacc35fc180d59c9c6b4b8da3c6`,
`ev_406441f71cb4969be4448b4641df254100fb55a72d41cfb8432d9ab1db205203`,
`ev_b45fa0ab5bf5981806f9ddf19008b4217a200b398f91129a7fe30ccd3d8635a9`).

When `FUN_00416940(param_1, param_2)` is called with `param_1 != 0`, it reads
the current context's dwords at `+0x0c`, `+0x08`, and `+0x10` into runtime
globals `DAT_00557030`, `DAT_00557034`, and `DAT_0055703c` only when
`DAT_00557028 != -1` and the context lookup succeeds. Otherwise it assigns
`1`, `8`, and `5` to those globals. With `param_1 == 0`, this function does
not load or reset those three option globals. A separate `param_2` gate clears
front-end state (`ev_c035268f6d178d8e76b3f4b7db78510a2c359df0e9509835d724f90f03fe4001`).

The reverse helper, `FUN_00416910`, writes `DAT_00557030`, `DAT_00557034`,
and `DAT_0055703c` into the current context at `+0x0c`, `+0x08`, and `+0x10`
respectively. Its sole direct caller is `FUN_00462030`; that caller invokes it
before building a save-context snapshot (`ev_3fb0da29c99a1ff5688a0183cddd24a6213011d96345077d6b8c4459383bbab8`,
`ev_4ea4fda60b0730708a939482d161fcb1dcd9b3b140ffcd22a9a36a3a236e7d6a`,
`ev_aeeb9678a4ef9ab1a01ffd9ad9909647030e1a81034276de21816af6a19ce8eb`).

The level-load helper `FUN_00415150` applies three fields from the selected
context: `+0x08` is passed to `FUN_00414de0`, `+0x10` to `FUN_00414d30`, and
`+0x04` to `FUN_00414df0`; its sole direct caller is `LevelLoad @ 0x429890`.
The `+0x04` value is not among the three fields written by `FUN_00416910`, so
this trace leaves its producer and relation to the option globals open
(`ev_f94384262b6d46ce6216bbacd306fe05c747567c9ab313b711b51b382f7bf76b`,
`ev_657865462610d17ae00177f2d5c0b4d118c39c6b3fffce683d650621e0a16bc8`).

## Boundaries

- The `+0x08`, `+0x0c`, and `+0x10` slot fields are observed as raw data-flow
  locations. Callback names and neighboring menu code suggest option state,
  but this pass does not map each field to a player-facing setting or unit.
- The data path is static analysis of the shipped executable. It does not
  observe a user changing a setting, a specific slot being selected, or a
  save succeeding.
- Finding 67 follows the snapshot into the slot-file write path. The path
  here does not establish a standalone global preferences store.
