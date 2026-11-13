// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-007's copy of gods98 Maths.h's inline 2D vector helpers (dot,
 * modulus, subtract, scale, normalise) over CollisionPoint2F. The original
 * unit expands them in place, so they have no address of their own. Only
 * collision.c includes this file, after CollisionPoint2F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_COLLISION_MATHS_H
#define LEGORRR_GAME_COLLISION_MATHS_H

static __inline float collision_vector2d_dot(const CollisionPoint2F* a,
                                             const CollisionPoint2F* b)
{
    return a->x * b->x + a->y * b->y;
}

static __inline float collision_vector2d_modulus(const CollisionPoint2F* v)
{
    return (float)sqrt(collision_vector2d_dot(v, v));
}

static __inline CollisionPoint2F* collision_vector2d_subtract(
    CollisionPoint2F* r, const CollisionPoint2F* a, const CollisionPoint2F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    return r;
}

static __inline CollisionPoint2F* collision_vector2d_scale(
    CollisionPoint2F* r, const CollisionPoint2F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    return r;
}

static __inline CollisionPoint2F* collision_vector2d_normalise(
    CollisionPoint2F* r)
{
    return collision_vector2d_scale(r, r,
                                    1.0f / collision_vector2d_modulus(r));
}

static __inline CollisionPoint2F* collision_ray_end_point(
    CollisionPoint2F* r, const CollisionPoint2F* p, const CollisionPoint2F* v,
    float t)
{
    r->x = p->x + v->x * t;
    r->y = p->y + v->y * t;
    return r;
}

#endif /* LEGORRR_GAME_COLLISION_MATHS_H */
