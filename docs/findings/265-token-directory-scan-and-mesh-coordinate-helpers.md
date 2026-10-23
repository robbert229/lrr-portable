# Finding 265 — Token Directory Scan and Mesh Coordinate Helpers

Three functions from the startup/shutdown directory walk and one mesh helper
have mechanically grounded names. The token comparator's return value is
ignored by the observed caller, so its intended effect remains uncertain.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4802d0 | FileTokenListLoadAndTreeScan | Opens a path in read mode, scans whitespace-delimited `%s` tokens into 1024-byte local storage, copies tokens into 0x100-byte global slots, increments the token count, invokes the recursive directory walk, then closes the stream. It is called by 0x48b2f0 in the WinMain shutdown path. The file's meaning is unknown. |
| 0x480390 | DirectoryTreeScan | Builds `root\\*.*`, walks entries with `FindFirstFileA`/`FindNextFileA`, skips `.` and `..`, recurses into directories, and passes ordinary entries to 0x480570. Called by 0x4802d0 and recursively by itself. |
| 0x480570 | DirectoryTokenListCompare | Performs case-insensitive comparison against the loaded token slots and returns on a match. The observed caller ignores its result and the routine has no visible state mutation, so it does not establish a working filter or its purpose. |
| 0x481fa0 | MeshVertexCoordinateRemap | Processes a count of vertex-like records into two floats per record, selecting axes, scale, and offset from flag bits. Called by geometry-array builder 0x480ac0. Its UV/texture role is not established. |

**Evidence.** The token and directory decompiles plus caller chains are in
[legorr-token-directory-scan-second-pass-2026-10-23.bundle](../evidence/legorr-token-directory-scan-second-pass-2026-10-23.bundle).
Direct decompiles are
`ev_07c639a2e63d928d33ebaf89211aaf25078eb6384def9d2d8e589f5cae749d3e`
(0x4802d0),
`ev_752e068f812b6827a4a98f675cfdd3138ed5ccb6a97a2471888a6741543e7a3a`
(0x480390), and
`ev_9b0181045ea72ad08c95414a7af08133d625f159afc15db7179bc02f57e06a17`
(0x480570). Caller-chain evidence is
`ev_dab0eeeb42997d0b8f6a70f83b3b33f2b0d631471c4e432d655868d720bf00c7`.
The mesh helper and its caller are
`ev_f0b34105266933dd0d18f5c0b9b26d7b57ab4fdd7d2f6a2c2d63294b18258462`
and `ev_211b6a53e572c56eb197725ebfef2f1c5cd6910febf37b3788b5dc33a44046a5`.
All observations are static Ghidra analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
