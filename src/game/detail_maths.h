// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-011's copy of gods98 Maths.h's inline vector helpers (modulus,
 * subtract, cross product, scale, normalise) over Detail_Vector3F
 * (finding 615). The original unit expands them in place, so they have no
 * address of their own. Only detail.c includes this file, after its types.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). Their bodies keep Maths.h's
 * parenthesised operands, (a->x - b->x) and (v->x * v->x): without them
 * MSVC 5.0 schedules detail_set_mesh_frame's two subtractions together
 * and spills more. The cross product is Maths.h's left-handed one,
 * through a temporary, so r may alias a or b. */
#ifndef LEGORRR_GAME_DETAIL_MATHS_H
#define LEGORRR_GAME_DETAIL_MATHS_H

static __inline float detail_vector3d_modulus(const Detail_Vector3F* v)
{
    return (float)sqrt((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}

static __inline Detail_Vector3F* detail_vector3d_subtract(
    Detail_Vector3F* r, const Detail_Vector3F* a, const Detail_Vector3F* b)
{
    r->x = (a->x - b->x);
    r->y = (a->y - b->y);
    r->z = (a->z - b->z);
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

static __inline Detail_Vector3F* detail_vector3d_scale(
    Detail_Vector3F* r, const Detail_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline Detail_Vector3F* detail_vector3d_normalise(Detail_Vector3F* r)
{
    detail_vector3d_scale(r, r, 1.0f / detail_vector3d_modulus(r));
    return r;
}

#endif /* LEGORRR_GAME_DETAIL_MATHS_H */
