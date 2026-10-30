# Finding 272 — CRT File Resize and Signal Helpers

Three CRT helpers now have names grounded in direct behavior and resolved
caller paths. The signal numbers are left numeric because their exact symbolic
mapping is not established by this binary evidence.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x49a890 | Win32FileDescriptorResize | Validates a CRT descriptor, saves its position, seeks to the end, truncates with `SetEndOfFile` or zero-extends in `0x1000`-byte writes, then restores position and text/binary state. Its caller 0x499240 uses it to remove a trailing Ctrl-Z byte in a text append/open path. |
| 0x49aa10 | CrtSignalHandlerDispatch | Dispatches configured handler slots for numeric signal-like codes. Handler value 1 is ignored, null selects default exit behavior, and other values are called; code 8 also adjusts CRT FPU state. The `_abort` routine calls it with 22 before exiting. |
| 0x49ab90 | CrtSignalHandlerRecordFind | Exact match for VC5 `LIBC.LIB`'s private `_siglookup` in `winsig.obj`. `_raise` references this helper, so linking the public `_raise` entry pulls it in; its table globals are supplied by `fpexcept.obj`. The local table/search and hooks remain only in non-MSVC builds. |

These labels describe observed mechanics. The signal constants remain numeric
because their symbolic mapping is unconfirmed. `_siglookup` is a private
`winsig.obj` helper rather than a public CRT export; the public `_raise` entry
pulls in the object that contains it.

**Evidence.** Direct decompiles and callers are in
[legorr-crt-fifth-pass-2026-10-23.bundle](../evidence/legorr-crt-fifth-pass-2026-10-23.bundle):
`ev_9c4a88030e6fb298e32dec824c086c53c6dd046ba3801b675b0d0c46650f78cc`
(file resize),
`ev_6b8a9f54b1aee2b70cba953129567751833069eddc9f9b399649cb6bef3fde70`
(resize caller),
`ev_ebcf276fab07e13e5014f4b67ee7f81821e854af136082f7792ff7ec5cce13bf`
(signal dispatch),
`ev_40d5dfc5473a08c9daa606d51b0564e8876bd188e441102c59a3711db115161a`
(`_abort` caller),
`ev_44875d8466623a4195e140dee70cea78a07aeda408f329532f7639dc119a2278`
(record lookup), and
`ev_923a5774146e2c041ec2a6f46c6169efa20f733a020d7aec72f1aaab31b0f10a`
(lookup caller). All observations are static analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
