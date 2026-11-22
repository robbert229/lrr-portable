// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.c's gods98 Maths.h inline helper Maths_Vector3DSubtract over the
 * unit's Vector3F. The original unit expands it in place, so it has no
 * address of its own. Only maths.c includes this file, after Vector3F.
 *
 * Through it surface_triangle_normal_calculate's two edges are
 * address-taken, which puts v2 below v1 in the frame as in the original.
 * It is a static __inline function, not a macro: as a macro the expansion
 * compiles differently (finding 544). In a header it adds no Reccmp lint
 * debt. */
#ifndef LEGORRR_GODS98_MATHS_MATHS_H
#define LEGORRR_GODS98_MATHS_MATHS_H

static __inline Vector3F* maths_vector3d_subtract(Vector3F* r, const Vector3F* a, const Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

#endif /* LEGORRR_GODS98_MATHS_MATHS_H */
