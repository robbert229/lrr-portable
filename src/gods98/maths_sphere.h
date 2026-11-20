// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.c's copy of gods98 Maths.h's inline Maths_Vector3DDotProduct
 * (OpenLRR engine/core/Maths.h, marked <inlined>), which the original
 * expands twice inside Maths_RaySphereIntersection (00479c10), as
 * OpenLRR's version of that function does; the unit's Maths_Vector3DScale
 * copy is maths_vector3d_scale in maths.c. It has no address of its own.
 * Only maths.c includes this file, just before that function, so the
 * functions above keep their symbol counter (finding 538); in a header it
 * adds no annotation-lint debt, as maths_maths.h. */
#ifndef LEGORRR_GODS98_MATHS_SPHERE_H
#define LEGORRR_GODS98_MATHS_SPHERE_H

static __inline float maths_vector3d_dot_product(const Vector3F* a, const Vector3F* b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

#endif /* LEGORRR_GODS98_MATHS_SPHERE_H */
