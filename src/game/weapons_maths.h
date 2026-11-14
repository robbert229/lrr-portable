// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-050's copy of gods98 Maths.h's inline vector helpers (dot, modulus,
 * add, subtract, scale, normalise, ray end point and distance) over Weapons_Vector3F. The
 * original unit expands them in place, so they have no address of their
 * own. Only weapons.c includes this file, after Weapons_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_WEAPONS_MATHS_H
#define LEGORRR_GAME_WEAPONS_MATHS_H

static __inline float weapons_vector3d_dot(const Weapons_Vector3F* a,
                                           const Weapons_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float weapons_vector3d_modulus(const Weapons_Vector3F* v)
{
    return (float)sqrt(weapons_vector3d_dot(v, v));
}

static __inline Weapons_Vector3F* weapons_vector3d_subtract(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, const Weapons_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline Weapons_Vector3F* weapons_vector3d_add(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, const Weapons_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline Weapons_Vector3F* weapons_vector3d_scale(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline Weapons_Vector3F* weapons_vector3d_normalise(Weapons_Vector3F* r)
{
    return weapons_vector3d_scale(r, r, 1.0f / weapons_vector3d_modulus(r));
}

/* gods98's Maths_RayEndPoint scales the ray into a local vector, then adds
 * the origin. The local is what gives the callers their larger frame and
 * x87 order (finding 636); r may be p. */
static __inline Weapons_Vector3F* weapons_ray_end_point(
    Weapons_Vector3F* r, const Weapons_Vector3F* p, const Weapons_Vector3F* v,
    float t)
{
    Weapons_Vector3F scaled;

    weapons_vector3d_scale(&scaled, v, t);
    return weapons_vector3d_add(r, p, &scaled);
}

static __inline float weapons_vector3d_distance(const Weapons_Vector3F* a,
                                                const Weapons_Vector3F* b)
{
    Weapons_Vector3F d;

    weapons_vector3d_subtract(&d, a, b);
    return weapons_vector3d_modulus(&d);
}

#endif /* LEGORRR_GAME_WEAPONS_MATHS_H */
