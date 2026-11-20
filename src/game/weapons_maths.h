// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-050's copy of gods98 Maths.h's inline vector helpers (dot, modulus,
 * add, subtract, scale, normalise, ray end point and distance) over
 * Weapons_Vector3F, and the 2D modulus over a float pair. The
 * original unit expands them in place, so they have no address of their
 * own. Only weapons.c includes this file, after Weapons_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_WEAPONS_MATHS_H
#define LEGORRR_GAME_WEAPONS_MATHS_H

/* The products in parentheses, as mesh_maths.h's: MSVC 5.0 keeps a
 * parenthesised float subexpression as written (finding 661). */
static __inline float weapons_vector3d_dot(const Weapons_Vector3F* a,
                                           const Weapons_Vector3F* b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

static __inline float weapons_vector3d_modulus(const Weapons_Vector3F* v)
{
    return (float)sqrt(weapons_vector3d_dot(v, v));
}

/* Maths_Vector2DModulus. 0x00471370 stores each component of its ray and
 * squares it from that slot, as this expansion does (finding 681). The
 * unit keeps its 2D points as float pairs, so v is one. */
static __inline float weapons_vector2d_modulus(const float* v)
{
    return (float)sqrt(v[0] * v[0] + v[1] * v[1]);
}

static __inline Weapons_Vector3F* weapons_vector3d_subtract(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, const Weapons_Vector3F* b)
{
    r->x = (a->x - b->x);
    r->y = (a->y - b->y);
    r->z = (a->z - b->z);
    return r;
}

static __inline Weapons_Vector3F* weapons_vector3d_add(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, const Weapons_Vector3F* b)
{
    r->x = (a->x + b->x);
    r->y = (a->y + b->y);
    r->z = (a->z + b->z);
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

/* weapons_fire_lazer's ray: a scale whose x product is cast to float. The
 * cast changes nothing in value; it makes MSVC 5.0 load the factor twice and
 * all three components before the first store, the original's x87 order
 * (plain stores give a load-multiply-store per component). It stands for an
 * original helper or macro whose exact form is unknown, and has the plain
 * scale's three parameters so the unit's symbol count is unchanged. */
static __inline Weapons_Vector3F* weapons_vector3d_scale_cast(
    Weapons_Vector3F* r, const Weapons_Vector3F* a, float f)
{
    r->x = (float)(a->x * f);
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
