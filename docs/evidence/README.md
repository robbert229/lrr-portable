# Evidence Bundle

`legorr-rev.bundle` is an REA evidence bundle (JSON) exported from the Ghidra analysis
session for `LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).

It contains 236 recorded observations — function decompilations, address lookups,
xref queries, and byte reads — each tied to the target digest, so findings 01–22 can be
re-verified: import the bundle into a session (`import_evidence_bundle`) or inspect the
`records` array directly. No game asset bytes are included; entries reference offsets
and produce analysis text only.
