# 86 — Low-resource entities queue a type-0x11 target command after route checks

The type-2 branch in `FUN_0043f7d0` considers target routing when the entity's
`+0x344` field falls below one percent of `+0x340` times a global scale, its
delay field is greater than 125, it has no linked target at `+0x36c`, and no
type-`0x11` command is already present. It asks `FUN_00438c60` for a candidate,
then checks up to four neighboring cells with `FUN_004410f0`. On the first
successful route check, it creates a type-`0x11` command carrying the candidate
pointer through `FUN_00402f70`.

**Observations from** REA/Ghidra 12.1.4 over the shipped `source/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-energy-low-target-routing.bundle`](../evidence/legorr-energy-low-target-routing.bundle).
Focused records: `ev_bb9901580051455453170ce2dcc9ee4c4bfa01013424d91011afe8d18a967d59`
(`FUN_0043f7d0`), `ev_261b68dfb26d96719b82b4c38fc2dea4fdc67eefd144e375bc902decb86fdf74`
(`FUN_00438c60`), and
`ev_e41160f306eb2e3c609d1eb9a98dca8977f5fff4073103c6fca6cb1dc7b8dc40`
(`FUN_00402f70`).

The candidate-search callback passed into `FUN_00438c60` is not a separately
analyzed procedure, so the candidate's eligibility rule is unknown. The
evidence does not establish that the target is a power station, that this is a
recharge order, or that the entity gains resource after arrival. No runtime
behavior was observed.
