# Finding 323 — Camera Math and Matrix Primitives

Ten routines cover angle parsing, linked-list release, ray/plane arithmetic, a quadratic intersection discriminant, and basic 4×4 matrix operations. Names stay at the operation level where caller context does not establish higher-level domain meaning.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x479110 | ReadAngleSettingRadians | Reads a configured angle in degrees and converts it to radians. |
| 0x479240 | ReleaseLinkedObjectList | Releases the head payload and walks the linked list, releasing each node. |
| 0x479a30 | RayPlaneIntersectionParameter | Computes the parameter from plane-point minus ray-origin dotted with the normal, divided by ray-direction dotted with the normal; returns false on a zero denominator. |
| 0x479ab0 | PointAlongRay | Writes `origin + t × direction`. |
| 0x479c10 | SphereRayIntersectionDiscriminantPositive | Tests whether a quadratic discriminant-like value is positive; the exact parameter meanings are not established. |
| 0x479da0 | Matrix4SetRotationY | Initializes identity and writes the sine/cosine entries for a Y-axis rotation. |
| 0x479df0 | Matrix4SetRotationZ | Initializes identity and writes the sine/cosine entries for a Z-axis rotation. |
| 0x479e70 | Matrix4SetIdentity | Writes a 4×4 identity matrix. |
| 0x479ea0 | Matrix4SetZero | Clears 16 matrix elements. |
| 0x479eb0 | Matrix4Copy | Copies 16 elements from one matrix to another. |

Bodies and available callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The matrix and camera relationships are visible in caller pseudocode. No matching direct retained analysis records were found for these addresses. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
