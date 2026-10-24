# Finding 399 — CRT Numeric and Floating-Point Runtime

This investigation maps the 83 functions assigned to `runtime-numeric-and-floating-point`. The canonical pseudocode is in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); direct call relationships below come from its callsites. Microsoft CRT names are retained where the corpus identifies them, while semantic interpretations of generic helpers and opaque globals stay qualified. Earlier findings 260, 264, 320, and 382–386 cover a number of the same routines; this pass brings the complete assigned area together and makes its major data paths explicit.

## Runtime setup and scalar math

| Address | Name | Observed behavior | Caller/state flow |
| --- | --- | --- | --- |
| `0x004798a0` | `thunk_FUN_0048e170` (unresolved corpus alias; local body is an LCG step) | Advances shared state with `state = state * 0x343fd + 0x269ec3`, then returns bits 16–30. This is the observed LCG step used by the shared pseudo-random path. | Called by terrain, map-loading, entity, audio, and UI routines. Finding 147 documents its shared seed and the `RandomNext` relationship; the address is classified as compiler/runtime pseudo-random support in the coverage ledger. |
| `0x0048d920` | `__fpmath` | Initializes the math dispatch table, stores the result of a processor-feature probe, and attempts to set default x87 precision; the update path ends at an empty stub, so no x87 write is visible. | Called during CRT entry from `0x0048f000`; calls `CrtMathDispatchTableInitialize`, `CrtFloatingPointFeatureProbe`, and `__setdefaultprecision`. |
| `0x0048d940` | `CrtMathDispatchTableInitialize` | Stores six code pointers for scientific/fixed formatting and decimal-point normalization. | Called by `__fpmath`; the pointers are later consumed by floating-point formatting code. |
| `0x0048e060` | `X87FloatToLongLongRound` | Converts the x87 stack value to a signed 64-bit integer using the current x87 rounding behavior. | Used broadly by game and UI code when converting x87 results to integers. |
| `0x0048e090` | `CrtAsin` | Reads exponent/special-value information from the x87 input and transfers into the adjacent `CrtAcosCore` path; this body does not expose a separate ordinary return. | Called from vector-angle and geometry paths. The alias is preserved from the corpus; the exact instruction-level entry relationship warrants comparison with the shipped binary before assigning a more specific standalone formula. |
| `0x0048e0ad` | `CrtAcosCore` | For finite input in range, computes an arccos-like result using `fpatan(sqrt((1-x)(1+x)), x)`; boundary and exceptional cases go through CRT math-error handling. | Called by `CrtAsin`; consults the exponent helper, precision-mode helper, and one-argument error path. The export identity is not established by this pseudocode alone. |
| `0x0048e1aa` | `CrtMathIntrinsicDispatch` | Enters the two-operand x87 intrinsic dispatcher. | Called by animation/entity update paths; delegates to `__cintrindisp2`. |
| `0x0048e280` | `CrtRoundToIntegralWithMathErrorHandling` | Rounds an input to an integral x87 result; distinguishes finite, infinity, NaN, and other exceptional encodings and routes errors with operation code `0x0b`. | Used by text/UI and surface code; calls the x87 round helper and the math-result handlers. |
| `0x0048e480` | `CrtFloatingPointRuntimeInitialize` | Invokes the configured `__fpmath` callback when present and initializes two callback ranges. | Called from CRT entry at `0x0048f000`; callback meaning is inferred from the subsequent runtime setup. |
| `0x0048e680` | `CrtDoubleAbsoluteValue` | Returns absolute value for ordinary finite inputs, preserves/corrects infinity cases, and routes NaN/unsupported encodings through CRT math-error handlers. | Called by view-context update code; uses `DoubleClassify` and FPU-status helpers. |
| `0x0048e870` | `DoubleIsFinite` | Tests whether the double exponent field is not all ones; infinities and NaNs therefore return false. | Used by movement, model orientation, and surface geometry routines. |
| `0x0048e8e0` | `CrtRoundToIntegralWithMathErrorHandlingVariant` | Performs the same broad rounding and exceptional-value handling as `0x0048e280`, but reports operation code `0x0c`. | Called by `FloatToRoundNearestInt`; shares the x87 round and math-error helpers. |
| `0x0048e9e0` | `__alldiv` | Implements signed 64-bit integer division from 32-bit halves, normalizing operand signs and correcting the quotient after scaled division. | Used by video-duration frame-bound calculation. |
| `0x0048ea90` | `__allmul` | Implements signed 64-bit multiplication from 32-bit halves, including high-word cross-products. | Used by the formatted-scan numeric conversion path. |

## Precision, feature probing, and decimal formatting

| Address | Name | Observed behavior | Caller/state flow |
| --- | --- | --- | --- |
| `0x00490210` | `__setdefaultprecision` | Computes/attempts to update selected x87 control-word fields through `CrtFpuControlWordUpdateMask`; the visible call chain ends at an empty stub, so no x87 write is established. | Called by `__fpmath`; masks the precision-related bit before attempting to apply the default control state. |
| `0x00490230` | `ProcessorFeatureFallbackFalse` | Returns zero. | Fallback used when `IsProcessorFeaturePresent` cannot be resolved. |
| `0x00490280` | `CrtFloatingPointFeatureProbe` | Dynamically resolves `KERNEL32!IsProcessorFeaturePresent`, calls it with selector 0 when available, and otherwise uses the zero fallback. | Called by `__fpmath`; the return is stored in a runtime global. |
| `0x004902b0` | `CrtNormalizeDecimalPoint` | Finds the decimal point in a generated numeric string and replaces it with the locale decimal character, shifting the remaining text to preserve the terminator. | Installed as one of the math formatting dispatch pointers by `CrtMathDispatchTableInitialize`. |
| `0x00490400` | `CrtScientificFloatFormatAppend` | Builds a signed scientific-format mantissa, inserts the locale decimal character when needed, and appends an `e`/`E` exponent with sign and decimal digits. | Called through `CrtScientificFloatFormatGuarded`; uses locale data and decimal-mantissa rounding. |
| `0x00490540` | `CrtFixedFloatFractionFormat` | Builds a signed fixed-format integer and fractional portion, pads leading fractional zeroes, and inserts the locale decimal character. | Called through `CrtFixedDecimalFormatGuarded`; uses locale data and decimal-mantissa rounding. |
| `0x00490640` | `CrtFloatDecimalFormatPrepare` | Converts the input to decimal digits, derives the decimal exponent, trims trailing zeroes when appropriate, then selects fixed or scientific formatting based on the exponent and requested precision. | Entry for the two formatting wrappers below. |
| `0x004906f0` | `CrtScientificFloatFormatGuarded` | Sets the shared formatting-mode flag, invokes the scientific formatter, then clears the flag. | Called by `CrtFloatDecimalFormatPrepare`. The flag acts as a guard for the shared conversion state. |
| `0x00490720` | `CrtFixedDecimalFormatGuarded` | Sets the shared formatting-mode flag, invokes the fixed formatter, then clears the flag. | Called by `CrtFloatDecimalFormatPrepare`. |

## x87 operation dispatch and exception handling

| Address | Name | Observed behavior | Caller/state flow |
| --- | --- | --- | --- |
| `0x00491db0` | `__trandisp1` | Builds one-operand x87 status/control metadata and dispatches through an operation-indexed function table. | Used by one-operand intrinsic and transcendental dispatch wrappers. Ghidra could not recover the indirect jump table, so individual table cases are unresolved. |
| `0x00491e17` | `__trandisp2` | Builds two-operand x87 status/control metadata and dispatches through the operation-indexed function table. | Used by two-operand intrinsic and transcendental dispatch wrappers; the indirect table is likewise unresolved. |
| `0x00491f80` | `CrtOneArgumentMathErrorHandlingStart` | Calls `FUN_00498ce0` and returns an extended result; the recovered helper signature takes three arguments, but this call site supplies none explicitly, so the intended data flow and entry/alias relationship remain unresolved. | Called by exceptional one-argument math paths. |
| `0x00491f97` | `__startOneArgErrorHandling` | Same visible helper call/return shape as `CrtOneArgumentMathErrorHandlingStart`. | Called by the acos-like exceptional path and `__math_exit`; exact duplicate/export relationship is unresolved. |
| `0x0049200c` | `CrtFpuPrecisionModeCodeGet` | Returns code 7 when the incoming control/status value has bit `0x80000`, otherwise code 1. | Called by the acos-like path to select a math-error precision mode. |
| `0x00492068` | `DoubleExponentBitsGetOrSpecialValue` | Returns the double exponent bits for ordinary values, but returns the supplied high word unchanged when the exponent is all ones. | Called by `CrtAsin` before dispatch into its adjacent math core. |
| `0x0049208b` | `__math_exit` | Calls the one-argument error handler only when the saved control word and x87 status meet the observed invalid-operation condition. | Called by `CrtAcosCore`; condition interpretation follows the saved control/status bits. |
| `0x00492170` | `__cintrindisp2` | Runs the two-operand intrinsic dispatcher, sets `DAT_00549168`, and invokes the result/error dispatcher variant. | `CrtFloatingPointExceptionResultDispatchVariant` preserves the existing exception-context operand fields while this flag is set. |
| `0x004921ae` | `__cintrindisp1` | Runs the one-operand intrinsic dispatcher, sets `DAT_00549168`, and invokes the result/error dispatcher variant. | Called from `__ctrandisp1`-style paths; the precise operation selection is encoded in the table. |
| `0x004921e4` | `__ctrandisp2` | Loads two double arguments into x87 extended values, dispatches the two-operand operation, then processes its exception/result state. | Calls `__fload`, `__trandisp2`, and `CrtFloatingPointExceptionResultDispatch`. |
| `0x0049221e` | `CrtFloatingPointExceptionResultDispatch` | Clears the shared dispatch marker, classifies x87 status and exceptional results, builds the handler arguments, and routes them through the CRT numeric/error helper. | Called by two- and one-operand transcendental dispatchers. It may scale subnormal or overflowing extended results before dispatch. |
| `0x00492225` | `CrtFloatingPointExceptionResultDispatchVariant` | Performs the same broad exceptional-result preparation as `0x0049221e`, but preserves the caller's shared dispatch-marker state. | Called by the `__cintrindisp1/2` intrinsic wrappers. |
| `0x00492355` | `__ctrandisp1` | Loads one double argument into x87 extended format, dispatches its operation, then processes the resulting exception state. | Calls `__fload`, `__trandisp1`, and `CrtFloatingPointExceptionResultDispatch`. |
| `0x00492381` | `__fload` | Converts IEEE double bits to an x87 extended value, explicitly expanding the significand for exponent-all-ones values. | Used by one- and two-argument transcendental dispatch wrappers. |
| `0x00492bc6` | `CrtDoubleRemainderReduction` | Performs exponent-aligned subtraction/reduction steps for finite operands in the extended-precision remainder path. | Called by `CrtFmodExtendedPrecision` for special exponent/significand cases. The full edge-case contract is not inferred beyond the visible branch conditions. |
| `0x00492dcc` | `CrtFmodExtendedPrecision` | Computes an x87 remainder directly for ordinary operands and routes subnormal/special cases to the reduction helper. | Calls `CrtDoubleRemainderReduction`. |
| `0x00493150` | `X87RoundToIntegral` | Returns the x87 `ROUND` result for a double input. | Shared by both public rounding wrappers. |
| `0x00493170` | `CrtMathExceptionHandleSingleResult` | When `DAT_004b28e0` is zero, sends the result through `CrtMathExceptionDispatch`; otherwise stores `EDOM` (`0x21`) in CRT `errno` at `DAT_005490d0`, invokes the control-word helper, and returns the input result. | Called by the rounding and absolute-value wrappers for special cases. Finding 237 maps the shared error global and adjacent Win32 error storage. |
| `0x004931c0` | `CrtMathExceptionHandleResultWithStatus` | Converts the status/result into an integer form, raises a structured floating-point exception when requested, then dispatches through the registered math handler or updates CRT error/FPU state. | Called by rounding/absolute-value wrappers and the formatted scanner. Uses `CrtDoubleToIntegerRound`, `CrtFloatingPointExceptionRaise`, and `CrtMathExceptionDispatch`. |
| `0x00493280` | `CrtFloatingPointExceptionRaise` | Maps x87 exception bits to Windows floating-point exception codes, builds an exception record/context from the FPU state, calls `RaiseException`, then copies handler-adjusted control/status/result data back. | Called by `CrtMathExceptionHandleResultWithStatus` and numeric scan conversion. Finding 260 gives the prior focused evidence; exact Windows structure semantics follow the observed field writes. |
| `0x004935c0` | `CrtDoubleToIntegerRound` | Converts the supplied double according to operation flags and x87 rounding control, handles underflow/overflow/special cases, updates output bits, and reports whether conversion completed without an unhandled condition. | Called by `CrtMathExceptionHandleResultWithStatus` and formatted numeric scanning; uses `DoubleFrexpDecompose` and FPU status helpers. |
| `0x004938f0` | `CrtMathExceptionDispatch` | Looks up the registered handler by operation code, builds a callback argument record, clears FPU status as required, invokes the handler, and falls back to CRT error-code state when no handler handles it. | Called by the two math-result handlers; calls `CrtMathExceptionHandlerGet` and `CrtMathErrorCodeSet`. |
| `0x004939a0` | `CrtMathErrorCodeSet` | Stores `EDOM` (`0x21`) for operation 1 and `ERANGE` (`0x22`) for operation codes 2–3 in CRT `errno` at `DAT_005490d0`. | Used by math exception and numeric conversion paths; see Finding 237 for corroborating file-error assignments. |
| `0x004939d0` | `CrtMathExceptionHandlerGet` | Searches a fixed table of operation codes and returns the associated registered callback, or null if absent. | Called by `CrtMathExceptionDispatch`. |
| `0x00493a00` | `CrtMathExceptionCodeFromStatus` | Maps selected x87 status bits to CRT math exception codes, checking overflow/underflow/inexact/invalid-related masks in priority order. | Called by the status-bearing math-result handler. |

## Floating-point representation and decimal conversion

| Address | Name | Observed behavior | Caller/state flow |
| --- | --- | --- | --- |
| `0x00493a40` | `DoubleBuildWithExponent` | Rebuilds an x87 extended result from a double significand and supplied exponent while preserving the sign and fraction bits. | Called by `DoubleFrexpDecompose`. |
| `0x00493a80` | `DoubleClassify` | Returns distinct codes for positive infinity, negative infinity, quiet-NaN-shaped, and signaling-NaN-shaped encodings; finite values return zero. | Used by rounding and absolute-value error paths. The bit-pattern class mapping is visible in this helper. |
| `0x00493ae0` | `DoubleFrexpDecompose` | Produces a normalized significand and writes the corresponding exponent to the output; zero returns exponent 0, and subnormals are shifted until normalized. | Used by `CrtDoubleToIntegerRound`; delegates representation assembly to `DoubleBuildWithExponent`. |
| `0x00493be0` | `FpuStatusWordGetVariantA` | Returns the current x87 status word. | Used when constructing Windows floating-point exception state. |
| `0x00493bf0` | `FpuStatusWordGetVariantB` | Returns the current x87 status word, matching the adjacent variant's visible behavior. | Called by exception-record construction after the first status read. |
| `0x00493c10` | `FpuControlWordUpdateMasked` | Returns the current x87 control word; despite the alias, this body does not update or mask it. | Used by rounding/error handlers to save the current state. Rename is deferred pending broader alias/index reconciliation. |
| `0x00493ca0` | `LocaleCharacterMapString` | Selects ANSI or Unicode locale mapping based on runtime capability; if only the wide API works, converts input to wide characters, maps it, converts output back, and frees temporary buffers. | Called by case conversion and locale-aware character helpers. |
| `0x00497770` | `CrtFpuControlWordUpdate` | Computes a masked/merged control-word value and returns it. It passes the value to `0x00497870`, whose body is an empty return, so no x87 control-word write is visible on this path. | Called by `CrtFpuControlWordUpdateMask`; uses `CrtControlFlagsTranslate`. The no-op callee is also assigned in issue #215. |
| `0x004977b0` | `CrtFpuControlWordUpdateMask` | Clears mask bit `0x80000` from the second argument, then delegates the update to `CrtFpuControlWordUpdate`. | Called by `__setdefaultprecision`. |
| `0x004977d0` | `CrtControlFlagsTranslate` | Translates x87 exception, rounding, precision, and related control bits into the CRT's internal control-word representation. | Called by `CrtFpuControlWordUpdate`. The numeric bit mapping is explicit; higher-level policy names are not assigned here. |
| `0x00497900` | `UInt96HasNoBitsAtOrAbove` | Checks whether a three-word unsigned value has any set bits at or above the specified bit position. | Used by `UInt96ClearFromBit` to decide whether rounding requires a carry. |
| `0x00497970` | `UInt96IncrementAtBit` | Adds one at a selected bit position in a three-word value and propagates carry toward the high word. | Used by `UInt96ClearFromBit` for rounding. |
| `0x004979e0` | `UInt96ClearFromBit` | Clears the selected bit and all more-significant bits; when discarded bits require rounding, increments the preceding retained position and reports carry. | Used by `CrtExtended80ToBinaryFloat`. |
| `0x00497a80` | `UInt96Copy` | Copies three 32-bit words between 96-bit work values. | Used by extended80 conversion. |
| `0x00497aa0` | `UInt96Zero` | Clears all three 32-bit words of a 96-bit value. | Used for zero/subnormal conversion cases. |
| `0x00497ab0` | `UInt96IsZero` | Returns true only when all three words are zero. | Used by `CrtExtended80ToBinaryFloat` to distinguish zero from denormal inputs. |
| `0x00497ad0` | `UInt96ShiftRight` | Shifts a three-word value right by a bit count, carrying bits between words and clearing words shifted past the end. | Used while aligning and rounding extended80 significands. |
| `0x00497b90` | `CrtExtended80ToBinaryFloat` | Converts an extended80 representation using a format descriptor: handles zero/denormal inputs, aligns and rounds the 96-bit significand, adjusts exponent/sign, and writes 32- or 64-bit output according to the descriptor. | Called by both format wrappers. Depends on the 96-bit helpers above. |
| `0x00497d60` | `CrtExtendedFloatConvertFormatA` | Calls `CrtExtended80ToBinaryFloat` with the format table at `0x004b28a8`. | Called by `ParseEntityHealthValue` and the double-to-format-A wrapper. |
| `0x00497d80` | `CrtExtendedFloatConvertFormatB` | Calls `CrtExtended80ToBinaryFloat` with the format table at `0x004b28c0`. | Called by the double-to-format-B wrapper. |
| `0x00497da0` | `CrtDoubleToExtendedFloatConvertA` | Converts a double input to a temporary extended80 value, then converts it using format A. | Calls `CrtExtendedFloatConvertFormatA`. |
| `0x00497de0` | `CrtDoubleToExtendedFloatConvertB` | Converts a double input to a temporary extended80 value, then converts it using format B. | Calls `CrtExtendedFloatConvertFormatB`. |
| `0x00497e20` | `CrtDecimalMantissaRoundAndNormalize` | Copies/pads decimal digits to a requested length, rounds upward when the next digit exceeds 4, propagates runs of 9s, and adjusts the stored exponent when the leading digit grows. | Used by fixed and scientific formatting helpers. |
| `0x00497f40` | `DoubleToExtended80` | Expands a double's sign, exponent, and significand into the 80-bit x87 layout, normalizing denormals and preserving zero/infinity encodings. | Used by the locale/formatting conversion path. |
| `0x00498bf0` | `__aulldiv` | Implements unsigned 64-bit division from 32-bit halves using a normalized divisor and quotient correction. | Used by the engine formatter. |
| `0x00498c60` | `__aullrem` | Implements unsigned 64-bit remainder using normalized division and subtract/carry correction. | Used by the engine formatter. |
| `0x00498f30` | `__allshl` | Shifts a 64-bit value left by the count in `CL`; counts above 63 produce zero, and counts below 32 combine cross-word bits. | Used by formatted scanning. |
| `0x00499fb0` | `CrtUnsignedAddWithCarry` | Adds two 32-bit unsigned values, stores the result, and returns whether the addition carried. | Primitive used by 96-bit increment/add/multiply helpers. |
| `0x00499fe0` | `UInt96Add` | Adds two three-word unsigned values, propagating carry from low to middle and high words. | Used in decimal digit accumulation and extended-float conversion. |
| `0x0049a050` | `ExtendedFloat96BitShiftLeftOne` | Shifts a three-word value left one bit with carry propagation across all words. | Used by decimal digit accumulation, extended-to-decimal conversion, and multiplication. |
| `0x0049a080` | `ExtendedFloat96BitShiftRightOne` | Shifts a three-word value right one bit with carry propagation across all words. | Used by extended-to-decimal conversion and multiplication. |
| `0x0049a0b0` | `CrtDecimalDigitSequenceToExtended96` | Accumulates a decimal digit sequence into a 96-bit value by repeated multiply-by-10/add-digit, then normalizes it and stores an exponent. | Called by `NumberStringScanParse`; uses the 96-bit add and shift helpers. |
| `0x0049a1b0` | `CrtExtendedFloatToDecimal` | Converts an extended80 value to decimal digits, handling zero, infinity, and NaN; scales by powers of ten, emits requested significant digits (capped at 21), rounds, and stores sign/exponent metadata. | Called from CRT locale/format conversion; uses `ExtendedFloatPowerOfTenScale`, `Extended80Multiply`, and 96-bit helpers. Finding 264 contains the prior focused evidence. |
| `0x0049a540` | `Extended80Multiply` | Multiplies extended80 operands using limb products and carry propagation, combines signs/exponents, normalizes and rounds, and emits zero or infinity for observed underflow/overflow cases. | Used by decimal conversion and power-of-ten scaling. Finding 264 contains the prior focused evidence. |
| `0x0049a800` | `ExtendedFloatPowerOfTenScale` | Multiplies an extended80 accumulator by table entries selected in groups of three exponent bits; negative powers select a separate table, and initialization mode clears the leading word before scaling. | Used by decimal digit parsing and `CrtExtendedFloatToDecimal`; prior behavior is documented in Finding 264. |

## Data paths and boundaries

- **CRT initialization → math/format state:** CRT entry `0x0048f000` calls `CrtFloatingPointRuntimeInitialize`; when configured, it enters `__fpmath`, which sets the dispatch table, probes the processor feature, and attempts to set default x87 precision. That precision update computes a new word but calls the no-op `0x00497870` stub, so the checked-in path shows no actual control-word write. The formatting table connects scientific/fixed emission and decimal-point localization.
- **Math operation → exception/error policy:** scalar wrappers and intrinsic/transcendental dispatchers collect x87 status and route exceptional values through result dispatch, integer conversion, Windows `RaiseException`, or a registered CRT math handler. When a handler is absent or declines, the path updates CRT math-error state and invokes observed FPU status/control helpers; their broader state-restoration contract is unresolved. The static pseudocode does not establish all external CRT structure meanings.
- **Decimal parsing/formatting → extended precision:** scanner input builds a 96-bit decimal accumulator; the extended conversion helpers translate between decimal digits, extended80, and target binary formats. Formatter output converts an extended value to decimal digits, rounds/pads them, and emits fixed/scientific text with the active locale separator.
- **Shared random state:** the address `0x004798a0` advances the same LCG state used by game code and documented in Finding 147. It is included in this assigned runtime area by the coverage ledger, but its visible role is pseudo-random generation rather than floating-point math.

### CRT character-class table

`DAT_005492b8` is the base of a 257-byte character-class table, with byte
values indexed at `base + character + 1`. `CrtLocaleCodePageTableUpdate @
0x004962f0` clears the table before populating it from a built-in code-page
range table or `GetCPInfo`. On the API path, it marks each reported multibyte
lead-byte range with bit `0x04`; the same bit is checked by path parsing and
command-line scanning before consuming a second byte. `CrtCharacterClassFlagTest
@ 0x00495db0` tests a requested flag in this table and, when absent, consults a
separate 16-bit character table at `0x004afd72`. The other class-bit meanings
and the complete built-in table layout remain unresolved.

## Open questions and focused follow-up

- Confirm the exact entry/alias relationship for `0x004798a0` and `0x0048e170`; the local entry's body is an LCG step while its corpus name indicates a thunk.
- The one-operand/two-operand transcendental dispatchers contain unrecovered indirect jump tables. Decode the operation table and document supported operation IDs and result handlers.
- `0x00493c10` is assigned the alias `FpuControlWordUpdateMasked`, but its body only returns the current x87 control word. Reconcile this misleading alias across the coverage ledger, heading annotations, and dependent findings before renaming.
- Determine the precise `DAT_005490cc` and `DAT_005490c8` roles, plus the external CRT error-handler contract, from runtime evidence or authoritative CRT matching.
- Compare the floating-point paths with the shipped executable on boundary inputs (subnormal, signed zero, infinities, signaling/quiet NaNs, and rounding-mode transitions) before turning any suspected issues into correctness claims.
