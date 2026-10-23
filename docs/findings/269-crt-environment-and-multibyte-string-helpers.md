# Finding 269 — CRT Environment and Multibyte String Helpers

Four CRT helpers in the environment mutation path now have names grounded in
their direct string and allocation behavior. The environment lookup is also
corroborated by its use to read the `TZ` variable during time-zone setup.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x49ae10 | CrtEnvironmentVariableLookup | Scans the CRT environment pointer array for a case-insensitive `NAME=` key and returns its value pointer, or null. If the narrow array is absent it can synchronize through the wide-to-ANSI helper. Time-zone initialization calls it with the bytes `TZ\0`. |
| 0x49b4c0 | CrtEnvironmentEntryFind | Scans null-terminated environment entries, compares the key case-insensitively, checks the `=` boundary, and returns a match index or negative insertion index. `CrtPutEnvUpdate` uses the result for add/replace/delete handling. |
| 0x49b540 | CrtEnvironmentArrayClone | Counts a null-terminated environment array, allocates a new pointer table, duplicates each string, and appends a null terminator. Called by `CrtPutEnvUpdate` when cloning the saved array. |
| 0x49b5b0 | CrtMultibyteCharacterSearch | Searches for a character token while respecting DBCS lead-byte pairs when multibyte mode is active; otherwise delegates to single-byte search. `CrtPutEnvUpdate` uses it to locate `=`. |

The `CrtPutEnvUpdate` caller at 0x49b2b0 is named in finding 261. The exact
Microsoft CRT export identities remain unclaimed; these labels describe the
observed operations.

**Evidence.** Direct pseudocode and the related environment mutation/caller
records are in
[legorr-crt-fourth-pass-2026-10-23.bundle](../evidence/legorr-crt-fourth-pass-2026-10-23.bundle):
`ev_2bcb78f3f658b3059cf60db0bcf10c27408833b0651c546ed7bfaed607156360`
(direct batch),
`ev_bae397470fa3acc59a2ddf96ecf8bd303c40b1f2b98328fb88aa1455884f6cf5`
(environment lookup caller),
`ev_eab5a93848aa1b6cffb25b053ede0dc5fb5c4ddddec13ccb0c134fa1d271ea95`
(entry search caller),
`ev_24ac96aa3d1720fd4f80d8f641f490c98cd645a557d59ce7cc0258d270745685`
(array clone caller), and
`ev_6a1c22b758683e2387489e490aadc8c0a170ff694197b7345ca9e2854e1a44d6`
(multibyte search caller). Additional timezone and environment-key evidence
is included in that bundle. All observations are static Ghidra analysis of
shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
