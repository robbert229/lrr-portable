// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-041's copy of gods98 Maths.h's inline vector helpers (dot, modulus,
 * add, subtract, scale, normalise, cross product) over SFX_Vector3F. The
 * original unit expands them in place, so they have no address of their
 * own. Only sfx.c includes this file, after SFX_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_SFX_MATHS_H
#define LEGORRR_GAME_SFX_MATHS_H

static __inline float sfx_vector3d_dot(const SFX_Vector3F* a,
                                       const SFX_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float sfx_vector3d_modulus(const SFX_Vector3F* v)
{
    return (float)sqrt(sfx_vector3d_dot(v, v));
}

static __inline SFX_Vector3F* sfx_vector3d_add(
    SFX_Vector3F* r, const SFX_Vector3F* a, const SFX_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline SFX_Vector3F* sfx_vector3d_subtract(
    SFX_Vector3F* r, const SFX_Vector3F* a, const SFX_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline SFX_Vector3F* sfx_vector3d_scale(
    SFX_Vector3F* r, const SFX_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline SFX_Vector3F* sfx_vector3d_normalise(SFX_Vector3F* r)
{
    return sfx_vector3d_scale(r, r, 1.0f / sfx_vector3d_modulus(r));
}

static __inline SFX_Vector3F* sfx_vector3d_cross_product(
    SFX_Vector3F* r, const SFX_Vector3F* a, const SFX_Vector3F* b)
{
    r->x = a->y * b->z - a->z * b->y;
    r->y = a->z * b->x - a->x * b->z;
    r->z = a->x * b->y - a->y * b->x;
    return r;
}

#endif /* LEGORRR_GAME_SFX_MATHS_H */
