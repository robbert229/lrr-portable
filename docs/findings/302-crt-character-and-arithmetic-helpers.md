# Finding 302 — CRT Character and Arithmetic Helpers

Seven anonymous runtime helpers now have operation-level names supported by their pseudocode and caller use.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x48f950 | CrtFindHandleClose | Wraps `FindClose` and records a CRT error on failure. |
| 0x491360 | CrtBufferedCharacterWrite | Writes one byte through a buffered stream, refills when full, and updates output count/error state. |
| 0x498b70 | CrtWideCharToMultibyte | Converts one wide character with fallback handling. |
| 0x498e00 | CrtMultibyteToWideChar | Converts one multibyte character and handles lead-byte state. |
| 0x4991f0 | CrtFileDescriptorHandleGet | Validates a descriptor-table slot and returns its OS handle or records `EBADF`. |
| 0x4997c0 | CrtCallocAligned | Checks multiplication overflow, rounds allocation size to 16 bytes, allocates memory, and zeroes it. |
| 0x499fb0 | CrtUnsignedAddWithCarry | Adds two unsigned values and returns carry/overflow information while storing the sum. |

Direct bodies and wrapper callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Caller evidence is retained in [legorr-engine-helper-audit-text-io-expansion-9.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle) for the buffered write, character conversions, handle close, and aligned allocation; the carry helper is called by unsigned arithmetic wrappers. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
