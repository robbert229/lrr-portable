# 35 — Paired-scene updates can skip the primary channel

The type-1 activity updater can own two DDiScene channels. Under a wrapper flag,
if the requested action is absent from the primary scene and no current primary
action can be retained, it sets a suppression bit and skips the primary
draw-step. A present companion channel is still advanced and drawn.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-shutdown-reachability.bundle`
(33 records). Focused IDs: `ev_7fec5899952b84c0f471166cd4a7b2ef493cedfd23521aec268f47ce11844eb7`
(`FUN_00447c40` type dispatcher), `ev_38685e504de3c713468ba05dc65eb3ae0011b8c1c04347dd198254e728eeab6b`
(`FUN_0046c3f0` paired-channel updater), `ev_54685b4639c266087fbaea3a5d39cf90fcf38dbcc979cee47a70be03b3ccb700`
(`FUN_00473370` action-name setter), and
`ev_078f3992b12dc973a613d27b15937e60c4abde2c3dfc23927b8ecafc01ceedbd`
(`FUN_00473340` action-name presence check).

## Observed update order

For wrapper type 1, `FUN_00447c40` calls
`FUN_0046c3f0(wrapper+0x0c, requested_name, requested_time)`. The updater:

1. Calls `FUN_00473370(primary, requested_name)` and clears wrapper flag bit
   `0x4`.
2. If the primary setter returns zero and the wrapper's prior flag word had bit
   `0x8`, it calls `FUN_00473340(primary, requested_name)`. If that also returns
   zero, it sets wrapper flag bit `0x4`.
3. If the updater argument's companion pointer at `+0x0c` is non-null, attempts
   the same name there and ORs that result into its return value.
4. Calls `FUN_00475140(primary, requested_time)` only when wrapper bit `0x4`
   is clear. If the companion pointer is nonzero, it always calls
   `FUN_00475140(companion, requested_time)`.

The names and meaning of wrapper bits `0x4` and `0x8` are not established here.
The observed condition is enough to show that primary suppression does not
short-circuit companion stepping.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Type-1 activity dispatch uses `FUN_0046c3f0` | observed (`FUN_00447c40`) |
| The wrapper's bit-`0x4` condition suppresses the primary draw-step | observed (`FUN_0046c3f0`) |
| A non-null companion still receives the draw-step under that condition | observed complete body (`FUN_0046c3f0`) |
| The semantic names/intent of wrapper bits `0x4` and `0x8` | unknown |
