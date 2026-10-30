# Finding 261 — CRT Argument, Environment, Error, and Conversion Helpers

Five previously anonymous runtime procedures have caller-supported names based
on their direct decompiles. The names describe observed behavior and do not
assert undocumented CRT export identities.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x495f80 | CrtCommandLineArgumentScan | Scans the module command line into argv, handling quoted argv[0], whitespace, DBCS lead-byte counts, and backslash/quote escaping. The argv builder at 0x495ee0 performs sizing and output passes, allocates storage, and stores argc/argv. |
| 0x4968f0 | CrtRuntimeErrorMessage | VC5 `LIBC.LIB` `crt0msg.obj` exports the exact `__NMSG_WRITE` body here. Depending on runtime mode it writes fixed text to stderr or constructs a `Runtime Error!` dialog containing the module name. |
| 0x49b2b0 | CrtPutEnvUpdate | Parses `NAME=VALUE`, updates the CRT environment pointer array by insert/replace/delete, and optionally calls `SetEnvironmentVariableA`. Called by the wide-environment conversion/update loop at 0x49af60. |
| 0x49afe0 | CrtCompareStringACompat | Uses `CompareStringA` when available; on the wide-only path, converts the byte strings using the selected code page and calls `CompareStringW`, including DBCS handling for short inputs. Wrapper 0x49af20 maps the API result to -1/0/1. |
| 0x4935c0 | CrtDoubleToIntegerRound | Converts a double to an integer using the x87 control and exception/status bits. The behavior is observed; the exact CRT export name is not established. |

**Evidence.** Direct decompiles and caller records are in
[legorr-crt-config-sweep-2026-10-23.bundle](../evidence/legorr-crt-config-sweep-2026-10-23.bundle).
The main decompile batch is
`ev_0606849bab068af8d83877bbca898402ec1efb1c8e086c10f663d0c02e9e472d`;
argument-parser caller evidence is
`ev_df4403557e301c33d3d4289d78084aeb9777991d5d3ba1bfe482c13e8f0df3e5`,
environment-update callers are
`ev_7ad85500238106078b170ed000ab7a7bda03bab5633ab9180b9e715672b14f79`,
and string-comparison callers are
`ev_30427cab1bf82ade50cdbd9cf1c08bc7d83dcc93f5dddc8777ea5564a4a46186`.
All observations are static Ghidra analysis of the shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
