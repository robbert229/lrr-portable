// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-016's copy of gods98 Maths.h's inline vector helpers (modulus, scale,
 * normalise, dot, cross, add, subtract), over SteeringVector3. The original
 * unit expands them in place, so they have no address of their own; maths.c
 * reproduces the same shapes for G98-007. Only steering.c includes this file,
 * after its .rdata: normalise reads the unit's 1.0 (0049e14c).
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). */
#ifndef LEGORRR_GAME_STEERING_MATHS_H
#define LEGORRR_GAME_STEERING_MATHS_H

static __inline float steering_vector3d_dot(const SteeringVector3* a,
                                            const SteeringVector3* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float steering_vector3d_modulus(const SteeringVector3* v)
{
    return (float)sqrt(steering_vector3d_dot(v, v));
}

static __inline SteeringVector3* steering_vector3d_add(SteeringVector3* r,
                                                       const SteeringVector3* a,
                                                       const SteeringVector3* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline SteeringVector3* steering_vector3d_subtract(SteeringVector3* r,
                                                            const SteeringVector3* a,
                                                            const SteeringVector3* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline SteeringVector3* steering_vector3d_scale(SteeringVector3* r,
                                                         const SteeringVector3* a,
                                                         float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline SteeringVector3* steering_vector3d_normalise(SteeringVector3* r)
{
    return steering_vector3d_scale(r, r, s_steering_one / steering_vector3d_modulus(r));
}

static __inline SteeringVector3* steering_vector3d_cross(SteeringVector3* r,
                                                         const SteeringVector3* a,
                                                         const SteeringVector3* b)
{
    SteeringVector3 v;

    v.x = a->y * b->z - a->z * b->y;
    v.y = a->z * b->x - a->x * b->z;
    v.z = a->x * b->y - a->y * b->x;
    *r = v;
    return r;
}

#endif /* LEGORRR_GAME_STEERING_MATHS_H */
