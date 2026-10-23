# 61 — Gameplay cursor coordinates route through an unprojected view ray

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-cursor-routing-expansion.bundle`](../evidence/legorr-cursor-routing-expansion.bundle).
This follows the client-relative cursor globals established in finding [60](60-win32-cursor-sampling.md)
into one gameplay camera path.

`FUN_00426420` runs this path only when `DAT_00556ec8 == 1` and the `0x1000`
bit returned by `FUN_00456240(0)` is clear. It compares `DAT_0076ab24/ab28`
against an inset rectangle defined by `DAT_00556e98` and the stored width and
height globals. A cursor inside that rectangle takes `FUN_00435f16` and returns;
a cursor outside it proceeds through a ray-based route. The bit and helper names
are not symbolically identified in the binary. [Evidence:
`ev_8761a64ae9fcd212d43808b274b938e2ac3b0ef3e4184bc7805f2599ea903559`]

On the outside-rectangle path, the function passes the stored cursor x/y to
`FUN_00429470`. That helper calls the camera wrapper `FUN_00477290` twice with
the same screen x/y and different depth values (0 and 1), subtracts the resulting
3D points, normalizes the direction, and calls `FUN_004798e0` with the ray data.
The wrapper itself dispatches through the camera-like object at `param_1+0x10`
using vtable offset `+0x54`. If the intersection helper returns nonzero,
`FUN_00426420` forms a coordinate delta from its outputs and sends it to
`FUN_00436b87` with the active view context. The direction and call chain support
screen-to-world unprojection and camera movement; the exact camera API contract
and what geometry `FUN_004798e0` intersects remain unnamed. [Evidence:
`ev_8761a64ae9fcd212d43808b274b938e2ac3b0ef3e4184bc7805f2599ea903559`,
`ev_6f43825c35ecf8376c42169b1a1168e8daf5865a946daca0d253908ed1ce5fd7`,
`ev_d684e14e49b3e8a65a3dc8fbeef8091050d83aeefab39776bb4db3064263aff7`]

## Caveats

- This identifies a screen-coordinate-to-ray path and its use by a camera helper;
  it does not establish whether the hit geometry is terrain, a plane, or another
  scene object.
- The `0x1000` state bit and the inset rectangle's user-facing meaning are
  unresolved.
- This is static analysis of the shipped executable, not a runtime trace.
