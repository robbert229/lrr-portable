# 117 — FallinMultiplier supplies the block-pointer modulus factor

LevelLoad parses the FallinMultiplier config key into DAT_00556f14, the global used in the
block-pointer hash expression documented in finding 07. Immediately after parsing, a zero value is
replaced with 1. This identifies the config source and the observed zero normalization for the
multiplier.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-rev.bundle. Focused record:
ev_7bfd230fa9893ca904b397ecb18b388426d52486a686ba04d7ac398c99e3ae0a (LevelLoad parses the key and
normalizes zero to one).

**Limits.** This does not establish the gameplay rationale for the multiplier or the parser result
when the key is missing. The modulo expression's behavior remains as documented in finding 07.
