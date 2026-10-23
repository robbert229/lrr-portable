# Finding 328 — CRT Environment and Descriptor Helpers

Three CRT helpers convert a wide-string list to multibyte strings, compare counted strings, and update a descriptor flag. Numeric flag values remain explicit where their meaning is unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x49af60 | CrtWideEnvironmentStringListConvert | Converts the null-terminated wide-string list referenced by `DAT_00549100` to multibyte strings and passes each result to a registration helper. |
| 0x49af20 | CrtCountedStringCompare | Calls the existing locale-aware comparison wrapper with a byte count and returns a normalized comparison result. |
| 0x49aea0 | CrtFileDescriptorFlag80SetOrClear | Validates a descriptor and sets or clears record-byte mask `0x80` according to mode `0x4000` / `0x8000`; returns the prior flag state. |

Bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct retained pseudocode records for the string-list conversion and comparison are in `legorr-crt-config-sweep-2026-10-23.bundle`, records `ev_e74de17c3ae783272915216adce3a8704f6c06f638bb04494908b8cb94c5645a` and `ev_e7bebd31dc8170c310eca891fcfd11713507743b683d5741025fa49477c943b4`. No direct record was found for the descriptor flag helper; its caller in `Win32FileDescriptorResize` is visible in the checked-in pseudocode. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
