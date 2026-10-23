# Finding 280 — Vector Math Helper Names

Eight anonymous math helpers have direct operation-level names supported by their pseudocode and caller use. The routine at `0x401720` was already ported and is not part of this naming batch.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x401240 | Vector2Distance | Returns the Euclidean distance between two 2D vectors. |
| 0x4013a0 | Vector3Length | Returns the Euclidean length of a 3D vector. |
| 0x401420 | Vector3Distance | Returns the Euclidean distance between two 3D vectors. |
| 0x401470 | Vector3CrossProduct | Writes the 3D cross product of its two input vectors. |
| 0x401500 | Vector3Add | Adds corresponding 3D vector components. |
| 0x4016c0 | Vector3Subtract | Subtracts the third vector from the second component by component. |
| 0x4016f0 | Vector3DotProduct | Returns the sum of pairwise 3D component products. |
| 0x401750 | Vector3Normalize | Scales a 3D vector by the reciprocal of its Euclidean length. Zero-length behavior is not guarded in the pseudocode. |

The full Ghidra bodies are retained in [LEGORR.full.c](../decompiles/full/LEGORR.full.c) and copied into the named corpus. Caller paths include `0x4248b0` for the 2D distance, cross product, add, subtract and normalize operations, and `0x435f5c` for 3D length, distance, subtract, dot product and normalization. The retained low-address and terrain audit bundles include direct analysis for the neighboring scale helper at `0x401720`; these eight labels follow their checked-in operation bodies. The executable is `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
