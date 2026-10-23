# Finding 147 — Sample Variant Choice Uses the Shared Seeded LCG

`FUN_00464e20` selects a sample variant with `FUN_0048e170() % variantCount`, then swaps the selected sample record with the alias's first record. It calls the generator whenever the alias has at least one registered variant, including aliases with only one variant. `FUN_0048e170` advances one global state word using `state = state * 0x343fd + 0x269ec3` and returns `(state >> 16) & 0x7fff`.

The state (`DAT_004afcc8`) is initialized to `1` in the shipped executable's `.data`. Level loading (`FUN_00429890`) reads a `Seed` value and calls `FUN_0048e160(seed)` only when the parsed value is nonzero. Thus a nonzero level seed resets the same global generator used for sample variants; an absent or zero `Seed` value does not reset it. A sample alias's chosen variant therefore depends on the shared generator's prior calls, not on a per-alias random sequence. Because other gameplay routines also call this generator, sound-alias resolution advances the same state and can shift later random results elsewhere in the process.

The shipped WADs contain 121 `.txt`, `.ol`, and `.cfg` entries; none contains a `Seed` key. For those text/config assets, no level-specific reseed is observed, so the generator begins from the binary's initialized state unless another external asset or runtime path provides a seed. This makes the stock startup state predictable from static data, while the exact later variant sequence depends on the order of all calls to the shared generator.

| Source | State effect |
| --- | --- |
| Executable `.data` initializer | Initial state `1` |
| `FUN_00429890` with nonzero `Seed` | Replace shared state with that value |
| `FUN_00429890` with absent/zero `Seed` | Do not reset state |
| `FUN_00464e20` sample selection | Advance shared state, reduce result modulo variant count, swap chosen variant to the head |
| Alias with one registered variant | Still consumes one shared generator value |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00464e20`, `FUN_0048e170`, `FUN_0048e160`, and the `FUN_00429890` seed read are in `docs/decompiles/full/LEGORR.full.c`; the global initializer is the four bytes `01 00 00 00` at VA `0x4afcc8`. The variant selector is recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_ee2cbb6e6321d2818c917957cc59e8e8ff32819e13e51842789c5ef43c5222dd`), and the LevelLoad body is recorded in `docs/evidence/legorr-checkpoint-reader-expansion.bundle` (`ev_22cae969db6b45e81e1404b6739355a517a1413e3123007025500e7da2e3c359`). The config asset inventory was searched from `reference/LegoRR0.wad` and `reference/LegoRR1.wad`; the `Lego.cfg` and executable hashes are recorded in finding 134.
