# Finding 264 — CRT Exception and Extended-Float Helpers

Five additional runtime procedures are now named from their direct behavior
and resolved caller chains.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x495160 | CxxExceptionHandlerSearch | Recognizes MSVC C++ exception records (`0xe06d7363`, magic `0x19930520`), validates catch-frame state, walks try-block and catchable-type descriptors for the current instruction address, and transfers matching handlers. Called on the search path of 0x495090. Exact exported CRT identity is not claimed. |
| 0x495ee0 | CrtCommandLineArgumentsInit | Obtains the startup command line or module path, calls 0x495f80 in a sizing pass and an output pass, allocates argv storage, then stores argc/argv globals. Called from CRT startup at 0x48f000. |
| 0x491f80 | `__startTwoArgErrorHandling` | Exact VC5 `LIBC.LIB` `genexcep.obj` entry; begins the two-argument math-error handling frame. The adjacent `__startOneArgErrorHandling` entry begins at 0x491f97. |
| 0x498ce0 | `__87except` | Exact 0x120-byte VC5 `LIBC.LIB` `87except.obj` function. It dispatches the x87 math exception case, applies math-error handling, and restores the control word; called from the adjacent `__startTwoArgErrorHandling` and `__startOneArgErrorHandling` routines. The MSVC build now links the runtime body directly. |
| 0x493280 | `__raise_exc` | Exact VC5 `LIBC.LIB` `fpexcept.obj` FPIEEE exception builder; the MSVC build links this public runtime helper directly. It maps x87 exception bits to Windows exception codes, fills the exception record/context, raises the exception, and copies any handler-adjusted control word and result back. |
| 0x49a1b0 | `_$I10_OUTPUT` | Exact VC5 `LIBC.LIB` `x10fout.obj` body match outside relocation slots. Converts an 80-bit extended value to decimal digits, including zero/infinity/NaN and rounding behavior, with significant digits capped at 21. It is called by `__fltout @ 0x497ec0`; MSVC builds link it directly, while other builds retain the local implementation. |
| 0x49a540 | `___ld12mul` | Exact VC5 `LIBC.LIB` `tenpow.obj` match outside relocation slots. Multiplies extended80 values and handles normalization, rounding, zero, and overflow. |
| 0x49a800 | `___multtenpow12` | Exact VC5 `LIBC.LIB` `tenpow.obj` match outside relocation slots. Scales an extended80 accumulator using the CRT's signed power-of-ten tables. |

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
