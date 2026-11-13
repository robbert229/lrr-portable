// SPDX-License-Identifier: LGPL-3.0-or-later
/* Roof.c's inline vector helpers (gods98 Maths.h's Maths_Vector3D* inlines:
 * add, dot, modulus, scale) over Roof_Vector3F. The original unit expands
 * them in place, so they have no address of their own. Only roof.c includes
 * this file, after Roof_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_ROOF_MATHS_H
#define LEGORRR_GAME_ROOF_MATHS_H

static __inline Roof_Vector3F* roof_vector3d_add(Roof_Vector3F* r,
                                                 const Roof_Vector3F* a,
                                                 const Roof_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline float roof_vector3d_dot(const Roof_Vector3F* a,
                                        const Roof_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float roof_vector3d_modulus(const Roof_Vector3F* v)
{
    return (float)sqrt(roof_vector3d_dot(v, v));
}

static __inline Roof_Vector3F* roof_vector3d_scale(Roof_Vector3F* r,
                                                   const Roof_Vector3F* a,
                                                   float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline Roof_Vector3F* roof_vector3d_normalise(Roof_Vector3F* r)
{
    return roof_vector3d_scale(r, r, 1.0f / roof_vector3d_modulus(r));
}

#endif /* LEGORRR_GAME_ROOF_MATHS_H */
