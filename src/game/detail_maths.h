// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-011's copy of gods98 Maths.h's inline vector helpers (dot, modulus,
 * subtract, cross product) over Detail_Vector3F (finding 615). The original
 * unit expands them in place, so they have no address of their own. Only
 * detail.c includes this file, after its types.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). The cross product is Maths.h's
 * left-handed one, through a temporary, so r may alias a or b. */
#ifndef LEGORRR_GAME_DETAIL_MATHS_H
#define LEGORRR_GAME_DETAIL_MATHS_H

static __inline float detail_vector3d_dot(const Detail_Vector3F* a,
                                          const Detail_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float detail_vector3d_modulus(const Detail_Vector3F* v)
{
    return (float)sqrt(detail_vector3d_dot(v, v));
}

static __inline Detail_Vector3F* detail_vector3d_subtract(
    Detail_Vector3F* r, const Detail_Vector3F* a, const Detail_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline Detail_Vector3F* detail_vector3d_cross_product(
    Detail_Vector3F* r, const Detail_Vector3F* a, const Detail_Vector3F* b)
{
    Detail_Vector3F v;

    v.x = (a->z * b->y) - (a->y * b->z);
    v.y = (a->x * b->z) - (a->z * b->x);
    v.z = (a->y * b->x) - (a->x * b->y);
    *r = v;
    return r;
}

#endif /* LEGORRR_GAME_DETAIL_MATHS_H */
