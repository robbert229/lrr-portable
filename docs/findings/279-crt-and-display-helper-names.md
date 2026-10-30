# Finding 279 — CRT and Display Helper Names

Nine more routines have behavior-based names. Their pseudocode and callers
support these operation-level labels; no exact CRT export identity is claimed.

| Address | Name | Observed behavior and remaining uncertainty |
| --- | --- | --- |
| 0x495df0 | CrtEnvironmentPointerArrayBuild | Copies non-empty-name entries from the startup environment block into separately allocated strings and a null-terminated pointer array, frees the source block, and stores the array. The exact CRT environment API relationship is unknown. |
| 0x496ad0 | CrtGetStringTypeACompat | Uses `GetStringTypeA` where available or converts input and calls `GetStringTypeW`. Caller use is character classification; exact CRT export identity is not asserted. |
| 0x496f50 | CrtCalendarFieldsToTimeScalar | Converts calendar-like fields to a scalar using cached timezone and optional DST status; rejects out-of-range years. Epoch origin and exact field names remain unresolved. |
| 0x497b90 | CrtExtended80ToBinaryFloat | Converts x87 80-bit input to selected 32/64-bit binary float output with exponent, mantissa, and sign handling. Descriptor and rounding semantics remain partly unknown. |
| 0x497e20 | `__fptostr` | Exact VC5 `LIBC.LIB` `_fptostr.obj` match; MSVC builds link the runtime implementation directly, while other builds retain the equivalent local body. It produces requested decimal digits, pads, rounds, propagates carry, and adjusts the exponent when the leading digit overflows. |
| 0x497ec0 | `__fltout` | Exact VC5 `LIBC.LIB` `cfout.obj` match outside relocation slots; converts a double to the CRT's cached decimal record. MSVC builds link it directly, while portable builds keep the equivalent local implementation. |
| 0x497da0 | `__atodbl` | Exact VC5 `LIBC.LIB` `intrncvt.obj` match outside relocation slots; parses a numeric string and converts the extended80 result to binary64. |
| 0x497de0 | `__atoflt` | Exact VC5 `LIBC.LIB` `intrncvt.obj` match outside relocation slots; parses a numeric string and converts the extended80 result to binary32. |
| 0x48d980 | `_atof` | Exact VC5 `LIBC.LIB` `atof.obj` match outside relocation slots; handles DBCS whitespace and calls the CRT numeric conversion helper. |
| 0x48d940 | `__cfltcvt_init` | Exact VC5 `LIBC.LIB` `fpinit.obj` match outside relocation slots; initializes the CRT floating-point conversion dispatch table. |
| 0x48e380 | `_toupper` | Exact VC5 `LIBC.LIB` `toupper.obj` match outside relocation slots; performs locale-aware uppercase conversion with DBCS handling. |
| 0x49a1b0 | `_$I10_OUTPUT` | Exact VC5 `LIBC.LIB` `x10fout.obj` match outside relocation slots. Converts an extended80 value to decimal digits and metadata; MSVC builds link the runtime directly. |
| 0x499fe0 | `___add_12` | Exact VC5 `LIBC.LIB` `mantold.obj` helper, linked directly for MSVC builds. |
| 0x49a050 | `___shl_12` | Exact VC5 `LIBC.LIB` `mantold.obj` helper, linked directly for MSVC builds. |
| 0x49a080 | `___shr_12` | Exact VC5 `LIBC.LIB` `mantold.obj` helper, linked directly for MSVC builds. |
| 0x49a0b0 | `___mtold12` | Exact VC5 `LIBC.LIB` `mantold.obj` match outside relocation slots; accumulates decimal digits into the extended96 representation and is linked directly for MSVC builds. |
| 0x49a540 | `___ld12mul` | Exact VC5 `LIBC.LIB` `tenpow.obj` helper, linked directly for MSVC builds. |
| 0x49a800 | `___multtenpow12` | Exact VC5 `LIBC.LIB` `tenpow.obj` helper, linked directly for MSVC builds. |
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
