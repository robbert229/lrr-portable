# Finding 264 — CRT Exception and Extended-Float Helpers

Five additional runtime procedures are now named from their direct behavior
and resolved caller chains.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x495160 | CxxExceptionHandlerSearch | Recognizes MSVC C++ exception records (`0xe06d7363`, magic `0x19930520`), validates catch-frame state, walks try-block and catchable-type descriptors for the current instruction address, and transfers matching handlers. Called on the search path of 0x495090. Exact exported CRT identity is not claimed. |
| 0x495ee0 | CrtCommandLineArgumentsInit | Obtains the startup command line or module path, calls 0x495f80 in a sizing pass and an output pass, allocates argv storage, then stores argc/argv globals. Called from CRT startup at 0x48f000. |
| 0x49a1b0 | CrtExtendedFloatToDecimal | Accepts an 80-bit extended value, handles zero/infinity/NaN, scales by powers of ten, emits requested significant digits, and rounds the result; the digit count is capped at 21. Its caller at 0x497ec0 feeds printf floating-point conversion handlers. |
| 0x49a540 | Extended80Multiply | Multiplies 80-bit extended operands using limb arithmetic, combines signs and exponents, normalizes and rounds, and handles zero, overflow, and subnormal outputs. Used by 0x49a1b0 and the power-of-ten scaling routine. |
| 0x49a800 | ExtendedFloatPowerOfTenScale | Scales or initializes an extended-float accumulator using a signed small integer power-of-ten table; used by 0x49a1b0's decimal conversion path. |

**Evidence.** Direct decompiles and caller/callee records are in
[legorr-crt-second-pass-2026-10-23.bundle](../evidence/legorr-crt-second-pass-2026-10-23.bundle).
Key decompiles are
`ev_4b60db0fe347d50cbe1232cb29032f17e45b4b14f68bbe72686bf0f5411baa4b`
(exception search),
`ev_7e4b1f3490745ea187f722be709639dea3e19ebe1264dfb60a0669e4f2924c28`
(argv initialization),
`ev_f3d397788c8de1280dc6e42c59e0efd89d57572d8c2d6f5348be2fff6eb40dcf`
(extended-to-decimal),
`ev_dc0c6beb83fb0cd74f4b2819546b39a83b393f32ab11baac1c5372a02c1bcf4a`
(extended multiply), and
`ev_4aa106005186bc02103e278861f24f9b3775b3e5a78c35f5cba9455e972f5ca5`
(power-of-ten scale). All observations are static analysis of the shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
