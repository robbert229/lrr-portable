# 91 — Optional `.ol` health defaults zero to 100 before entity initialization

The `.ol` parser reads `health` with an optional-value lookup. When the key is
absent, it passes the NUL byte at `0x4b8a14` to the numeric conversion path
(`FUN_0048d980` → `FUN_004907f0`). The parser then replaces any resulting zero
with `100.0`. After entity creation succeeds, it casts that value to an
integer and writes it to the entity field at byte offset `+0x340`
(`piVar9[0xd0]`). Thus an omitted health value and an explicitly parsed zero
both reach the same 100 default. The body shows no range clamp for nonzero
values.

The field key and the optional lookup are separate from the required `type`,
`xpos`, `ypos`, and `heading` checks documented in finding 75. Finding 83
covers the parser's other optional entry field, `driving`; the complete parser
body exposes no additional optional entry-field lookup. This is static
analysis: it does not establish how a particular level uses health or assign a
broader semantic name to entity offset `+0x340`.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-olistfile-health-field.bundle`.

Focused IDs: `ev_55645c200e2f7eb8fc3d34b41a3eeccb0d4e41b65b3c496829a3556fd5c2ccdb`
(`FUN_0042e020`, optional lookup, zero default, and entity write),
`ev_35aea9a74a4e5819e5382bc5133a8a120e031239c962a8d6d867cfc27d81cfb9`
(field-key bytes),
`ev_d62e685b719e7223c59df68c73531d2b1acab059f738365abf8f8b1955e36249`
(`0x4b8a14` fallback bytes),
`ev_924a5cb8960771ec4f88e67301947409585f96d88aea02b900faa26c0e434801`
(`FUN_0048d980` numeric conversion), and
`ev_fd776f9094425ce5656cc0f84243cc0c9314e5aa455dcdc2c7a60ddae809cfc9`
(`FUN_004907f0` numeric parse result).

| Claim | State |
| --- | --- |
| `health` is optional and is converted numerically | observed (`FUN_0042e020`, conversion helpers) |
| A missing key supplies a NUL fallback; a zero result becomes `100.0` | observed (parser plus fallback bytes) |
| A successfully created entity receives the value cast to integer at `+0x340` | observed (`FUN_0042e020`) |
| Nonzero range validation or the field's full gameplay meaning | unknown |
