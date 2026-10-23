# Finding 279 — CRT and Display Helper Names

Nine more routines have behavior-based names. Their pseudocode and callers
support these operation-level labels; no exact CRT export identity is claimed.

| Address | Name | Observed behavior and remaining uncertainty |
| --- | --- | --- |
| 0x495df0 | CrtEnvironmentPointerArrayBuild | Copies non-empty-name entries from the startup environment block into separately allocated strings and a null-terminated pointer array, frees the source block, and stores the array. The exact CRT environment API relationship is unknown. |
| 0x496ad0 | CrtGetStringTypeACompat | Uses `GetStringTypeA` where available or converts input and calls `GetStringTypeW`. Caller use is character classification; exact CRT export identity is not asserted. |
| 0x496f50 | CrtCalendarFieldsToTimeScalar | Converts calendar-like fields to a scalar using cached timezone and optional DST status; rejects out-of-range years. Epoch origin and exact field names remain unresolved. |
| 0x497b90 | CrtExtended80ToBinaryFloat | Converts x87 80-bit input to selected 32/64-bit binary float output with exponent, mantissa, and sign handling. Descriptor and rounding semantics remain partly unknown. |
| 0x497e20 | CrtDecimalMantissaRoundAndNormalize | Produces requested decimal digits, pads, rounds and propagates carry, adjusting the exponent when the leading digit overflows. Formatting-path differences remain open. |
| 0x499da0 | CrtDaylightTransitionBoundaryCompute | Computes fixed-day or week/weekday transition boundaries and stores day/millisecond values with timezone and wrap adjustments. Rule fields remain unnamed. |
| 0x49b660 | CrtStringDuplicate | Null-safe exact-size heap string copy, returning null for null input or allocation failure. No library export name is inferred. |
| 0x49d630 | DisplayModeListPopulate | Enumerates display modes, applies registered predicates, populates a dialog list, and shows/disables the control when no mode qualifies. Predicate meanings remain unknown. |
| 0x49dba0 | CrtUnsignedIntegerFormatRadix | Emits digits by division/modulo in the requested radix, optionally prepends a minus sign, then reverses the buffer. Radices above 36 are not validated. |

**Direct pseudocode and callers.** Corpus bodies and callers are in
[LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x495df0` runs during
startup; `0x496ad0` is called from `0x48f1d0`; `0x497b90` is called through
binary-float wrappers `0x497d60` and `0x497d80`; `0x497e20` is used by decimal
formatters `0x490400`, `0x490540`, and `0x490640`; `0x49b660` is called by
environment-array clone `0x49b540`; `0x49d630` is called by `0x49d5e0`; and
`0x49dba0` is wrapped by signed-decimal helper `0x49db60`.

Retained REA evidence for `0x496f50` is in
[legorr-crt-third-pass-2026-10-23.bundle](../evidence/legorr-crt-third-pass-2026-10-23.bundle):
direct record `ev_84dd1791867ee43bef7dca9ed06f1a4fb33854146a2b3c3786d290a14f13e076`
and sole caller record `ev_d69ef25a0116669dec6021e125c60402eb8e43ede1eac470478538f5b85d0f94`
(`0x48f970`). The daylight-boundary helper `0x499da0` is in the same bundle,
direct record `ev_6de38b90d3c03832e9b884f8c7d7e15837844f5b3ca0f2c9a15d90c765af4d07`;
caller record `ev_425989870ebdf2f02d98ba70995d113842674b0b0a32d488e6586889169159d4`
confirms `CrtDaylightSavingTimeCheck` (`0x499b30`). All observations are
static analysis of shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
