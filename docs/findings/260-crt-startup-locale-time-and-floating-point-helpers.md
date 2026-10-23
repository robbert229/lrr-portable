# Finding 260 — CRT Startup, Locale, Time-Zone, and Floating-Point Helpers

Five CRT support routines in the deferred `0x49xxxx` cluster have names
grounded in their own instruction sequences and direct caller relationships.
Their names describe observed behavior; the exact Microsoft CRT export names
are not asserted.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x498000 | CrtMemoryMove | Copies a byte range with overlap handling: it chooses forward or backward traversal according to the source and destination ranges, matching `memmove` semantics. Used by callers at 0x4907c0 and 0x4957d0. |
| 0x4965d0 | CrtFileDescriptorTableInit | Initializes the 32-entry CRT descriptor table, imports inherited handles described by `STARTUPINFO`, and binds the standard streams. Called from CRT startup at 0x48f000. |
| 0x499880 | CrtTimeZoneStateInit | Reads time-zone environment state, calls `GetTimeZoneInformation`, and caches standard/daylight offsets and names. Reached through the one-time initialization wrapper at 0x499860. |
| 0x4962f0 | CrtLocaleCodePageTableUpdate | Selects a code page and builds byte-classification and lead-byte tables from internal tables or `GetCPInfo`; called during descriptor/locale setup at 0x4965c0. |
| 0x493280 | CrtFloatingPointExceptionRaise | Maps x87 exception/status bits to Windows exception codes, constructs an exception record, calls `RaiseException`, then copies handler-adjusted flags back. The behavior is clear; an exact CRT export identity is not established. |

**Evidence.** Direct decompiles and supporting caller observations are in
[legorr-crt-runtime-audit-2026-10-23.bundle](../evidence/legorr-crt-runtime-audit-2026-10-23.bundle).
The broad decompile batch is
`ev_468de84c18457a319ae27433f163c3dd722c4e3d8114758ac0ce66914613a69d`;
caller evidence includes
`ev_330381f0166620b5a6fbb23d9b42791b56785a808ef2546dac62c0ad3425e405`
(memory move),
`ev_38163f2423d8978bb27fb41466a0815f7f96596cf1f014e7d4c78303eab8457c`
(descriptor initialization), and
`ev_69339e62a15fa032d018dfa71b4451a940f02c87396c0e99d4ba2bfc034d7532`
(time-zone initialization). The floating-point routine's dedicated decompile
is `ev_c8a9ae60402ac3f98429ef1265575ad12a85968ccaf27c7c81e9adea7c5b1799`.
All observations concern the shipped `reference/LegoRR.exe`.
