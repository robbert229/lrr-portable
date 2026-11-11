# Finding 510 — Maths unit (G98-007) rebuilt in original shape

Issue #395. This conversion follows the [ADR 0005](../adr/0005-original-shape-c-units.md)
recipe, after the [config tree](508-config-unit-original-shape.md) and the
[string helpers](509-string-helpers-unit-original-shape.md). It rebuilds the
gods98 vector, ray and matrix helpers in the original's shape. That is
original translation unit G98-007: text `00479500`–`00479ee0`, 20 functions,
including the two D3DRMVector wrappers
([finding 505](505-original-translation-units-and-link-order.md)).

All scores come from the *matching build* on `5fc15ad5` (#404) and this
change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 2,372 bytes) | 68.00% | **75.14%** |
| Exact functions | 11 | **12** of 20 |
| Mapped functions | 20 | 20 |
| Game-only mean (2,121 functions mapped in both builds) | 49.10% | 49.21% |
| Full mean (2,340 functions mapped in both builds) | 53.49% | 53.59% |

Byte weights are the original's code bytes, without padding.

The *before* column is not all C. Before this change, ten of the unit's
functions (1,295 bytes) were naked `__asm`, six of them exact. The
conversion replaces every one with C, so the unit now has no hand-written
assembly:

- The ten former asm functions went from 88.85% to 82.01% byte-weighted.
  Five rose or held, and five lost score (see
  [In-unit drops](#in-unit-drops)).
- The ten that were already C (1,077 bytes) rose from 42.93% to 66.88%.
  Seven are exact, up from five, and none fell.

## The unit

[`src/gods98/maths.c`](../../src/gods98/maths.c) and its C header
[`maths.h`](../../src/gods98/maths.h) hold the 20 functions in original
order. The functions keep the port's names, which the table maps to the
gods98 names.

| Address | Function (gods98) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00479500` | `d3drm_vector_random` (Maths_Vector3DRandom) | 11 | 66.67 | 75.00 |
| `00479510` | `d3drm_vector_rotate` (Maths_Vector3DRotate) | 26 | 85.71 | 90.00 |
| `00479530` | `surface_triangle_normal_calculate` (Maths_PlaneNormal) | 241 | 98.81 asm | 64.29 |
| `00479630` | `surface_triangle_area_calculate` (Maths_TriangleAreaZ) | 609 | 17.37 | 42.30 |
| `004798a0` | `rng_thunk_draw15` (Maths_Rand) | 5 | 100.00 asm | **100.00** |
| `004798b0` | `random_float_range` (Maths_RandRange) | 41 | 76.19 | **100.00** |
| `004798e0` | `view_ray_intersection_test` (Maths_RayPlaneIntersection) | 326 | 100.00 asm | 94.69 |
| `00479a30` | `ray_plane_intersection_parameter` (Maths_RayPlaneDistance) | 120 | 100.00 asm | 93.62 |
| `00479ab0` | `point_along_ray` (Maths_RayEndPoint) | 63 | 100.00 asm | **100.00** |
| `00479af0` | `line_intersection_2d_compute` (Maths_Vector2DIntersection) | 140 | 100.00 asm | 86.67 |
| `00479b80` | `point_in_polygon_2d` (Maths_PointInsidePoly) | 138 | 31.65 | **100.00** |
| `00479c10` | `sphere_ray_intersection_discriminant_positive` (Maths_RaySphereIntersection) | 204 | 100.00 asm | 49.35 |
| `00479ce0` | `matrix4_multiply` (Matrix_Mult) | 108 | 100.00 | **100.00** |
| `00479d50` | `matrix4_set_rotation_x` (Matrix_RotX) | 66 | 27.78 asm | **100.00** |
| `00479da0` | `matrix4_set_rotation_y` (Matrix_RotY) | 65 | 27.78 asm | **100.00** |
| `00479df0` | `matrix4_set_rotation_z` (Matrix_RotZ) | 65 | 27.78 asm | **100.00** |
| `00479e40` | `matrix4_identity_translation_set` (Matrix_Translation) | 37 | 100.00 | **100.00** |
| `00479e70` | `matrix4_set_identity` (Matrix_Identity) | 44 | 100.00 | **100.00** |
| `00479ea0` | `matrix4_set_zero` (Matrix_Zero) | 16 | 100.00 | **100.00** |
| `00479eb0` | `matrix4_copy` (Matrix_Copy) | 47 | 100.00 | **100.00** |

"asm" marks a function that was naked `__asm` before this change. The
rotation setters were asm but scored 27.78%, because their hand-written
listing did not reproduce the original's stack use.

### Shape

- **State.** The unit has no `.bss` and no `.data`.
- **`.rdata`.** The unit's float constants, at `0049e920`–`0049e938`, are
  annotated statics in original order: 1.0, 0.0, -1.0, 0.5, 1/32767, 2.0
  and 4.0.
  - Nothing reads the 2.0 at `0049e934`. Maths_RaySphereIntersection
    multiplies by `2.0f`, and MSVC turns each of those multiplications into
    `fadd st, st` but still emits the constant. The function uses the
    literal, and the static keeps the table's layout.
  - Zero and -1.0 are stored as immediates in the original
    (`mov [..], 0xbf800000`), so those stores use literals.
  - Like `config.c`, the unit does not reproduce the 16-byte
    `{1, 2, 4, -1}` table at `0049e910` that opens it. Finding 505 found
    that table in 19 gods98 objects. Its header of origin is not known.
- **Types.** The public API keeps `float*` parameters, which have the
  original's ABI. Inside the unit, `Vector3F` reproduces the original's
  struct copies, which compile to integer moves (`mov ebx, [ecx]`).
- **Inline helpers.** Three `static __inline` helpers stand in for
  Maths.h's `Maths_Vector3DModulus`, `_Scale` and `_Normalise`. MSVC 5.0
  always inlines them, as the original did. They took
  Maths_RayPlaneIntersection from 78.30% to 94.69%.
- **Maths_Rand** is `return rand();`, which MSVC compiles to `jmp _rand`.
  On other compilers, `crt/vc5names.h` maps `rand` to the port's VC5 LCG
  (`level_random_draw15`) and `_finite` to `double_is_finite`.
- **D3DRM.** The wrappers call `D3DRMVectorRandom` and `D3DRMVectorRotate`
  through a declaration without `dllimport`, so the call goes through the
  linker's import thunk, as in the original.
  - The native port has no D3DRM import. `view.cpp` now defines the port's
    emulation of the two DLL entries under those names, outside the
    Windows builds.
  - The remaining difference in both wrappers is the call operand. Reccmp
    does not resolve the original's thunk at `0049b6b6`/`0049b6bc` as an
    import.

### Callee contracts changed to the original

- `rng_thunk_draw15` returns `short`, the original's S16, where it returned
  `uint32_t`. Every caller now sign-extends, as the original's callers do
  (`movsx eax, ax` after each `call 004798a0`). All values are 0..0x7fff,
  so no caller's result changes.
- `d3drm_vector_random`, `d3drm_vector_rotate` and
  `surface_triangle_normal_calculate` return their output vector, so `eax`
  holds it on return, as in the original.
- `point_in_polygon_2d` takes an unsigned count, as the original's `jbe`
  shows.

The C++ declarations in `view.hpp` and `level.hpp`, and the local
declarations in `camera.cpp`, `cmd.cpp`, `entity.cpp`, `level.cpp`,
`sound.cpp`, `spawn.cpp`, `util.cpp` and `view.cpp`, gained `extern "C"`
in place. `test_c_boundaries.cpp` now checks `maths.h` against `view.hpp`
and `level.hpp`.

## In-unit drops

Five functions that were hand-written assembly score lower as C:

| Function | asm | C | What remains |
| --- | ---: | ---: | --- |
| `view_ray_intersection_test` | 100.00 | 94.69 | x87 scheduling of the sum in one modulus |
| `ray_plane_intersection_parameter` | 100.00 | 93.62 | operand order of one `fmul` |
| `line_intersection_2d_compute` | 100.00 | 86.67 | when the negation of `d2.y / det` is applied |
| `surface_triangle_normal_calculate` | 98.81 | 64.29 | the cross product's evaluation order and stack slots |
| `sphere_ray_intersection_discriminant_positive` | 100.00 | 49.35 | the original stores two components of `camera - center` over the copy of `line` |

All five recompile to the original's length or within six bytes of it.

The remaining differences are in register and stack-slot choice. They also
depend on the other functions in the file. MSVC 5.0's symbol numbering (the
entropy effect, finding 506) couples them. Two examples:

- Adding the three inline helpers moved `ray_plane_intersection_parameter`
  from 90.11% to 79.12%, with no change to its source.
- Removing one unused local in Maths_TriangleAreaZ moved
  `ray_plane_intersection_parameter` from 85.11% to 93.62%, and
  `line_intersection_2d_compute` from 92.68% to 84.55%.

Each function's source form was therefore picked last, in address order,
from the variants tried.

`surface_triangle_area_calculate` also calls `__CIacos`, but crt.cpp
annotates `0048e090` as `__CIasin` (finding 399). The routine at `0048e090`
computes `atan2(sqrt(1 - x²), x)` and loads π for x = -1. That is acos, and
VC5's LIBC.LIB has a separate `acos.obj` of the same shape. This change
does not correct the annotation, which a CRT issue should check.

## Callers and entropy

This change refreshes the *match baseline* under ADR 0005 entropy rule 2.

Outside the unit, one function rose:
`kind_six_random_sweep_dispatch` (`0040f0e0`), from 32.34% to 33.33%.

Three fell:

| Address | Function | before | after | Listing difference | Class |
| --- | --- | ---: | ---: | --- | --- |
| `00464110` | `rotated_pairs_scratch_fill` | 13.95 | 10.85 | registers, instruction order | drift, rule 2 |
| `0045e940` | `view_point_to_screen_geometry_transform` | 18.90 | 17.05 | registers, x87 stack order, one stack slot, branch offsets | drift, rule 2 |
| `0043ad50` | `predug_spawn_coordinate_select_and_create` | 15.83 | 15.77 | one added `movsx eax, ax`, branch offsets | callee contract |

- **The two `view.cpp` functions.** Their source is unchanged. `view.cpp`
  lost the unit's functions and statics, so its symbol numbering moved.
  Single-function listings from `5fc15ad5` and from this change differ only
  as the table shows. This is drift under rule 2.
- **`predug_spawn_coordinate_select_and_create` (`camera.cpp`).** Its
  source is also unchanged. The new instruction is the sign extension that
  the `short` return of Maths_Rand requires. The original has the same
  `movsx` after its call at `0043ad9d`. The listing is now closer to the
  original, but the score falls by 0.06 points because the diff aligns
  differently. Rule 2 does not cover an added instruction, so this is
  reported as a callee-contract exception, like `priorities_load` in
  finding 508.

**Net effect, both scopes:** 9 functions rose and 8 fell. Five of the falls
are inside the unit (above) and three outside. No function was newly
mapped or lost.

## Native port

`cmake -S . -B build && cmake --build build && ctest --test-dir build
--output-on-failure` builds with no errors, and all 31 tests pass,
including `c_boundaries`, `view` and `level`. `maths.c` passes
`gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only -Wall`.

Behaviour changes in the native port:

- `surface_triangle_area_calculate` lost the port's NULL-argument guard and
  now computes in single precision, as the original does.
- `ray_plane_intersection_parameter` and `line_intersection_2d_compute`
  lost the port's explicit NaN check. On x87, MSVC's `fcomp` treats an
  unordered (NaN) denominator as zero, so the matching build still returns
  0 or NULL, as the original does. Natively, a NaN denominator now
  computes a NaN result.
- `rng_thunk_draw15` returns the same 15-bit stream as before.

`make reccmp-lint` fails on `5fc15ad5` (4,332 unannotated functions) and on
this change (4,315). This change adds three to that list: the
`static __inline` helpers.

## Reproducing

    make reccmp-check                          # matching build + baseline gate
    make match-function SOURCE=src/gods98/maths.c
    python3 tools/translation_units.py build
