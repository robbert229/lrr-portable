// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-048's copy of gods98 Maths.h's inline vector helpers over
 * Vehicle_Vector3F (finding 590). The original unit expands them in place,
 * so they have no address of their own; Maths_Vector3DScale is the
 * exception, which the original calls out of line (Advisor.c's COMDAT copy,
 * 00401720, vec_scale). Only vehicle_model.c includes this file, after
 * Vehicle_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_VEHICLE_MODEL_MATHS_H
#define LEGORRR_GAME_VEHICLE_MODEL_MATHS_H

static __inline float vehicle_vector3d_dot(const Vehicle_Vector3F* a,
                                           const Vehicle_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float vehicle_vector3d_modulus(const Vehicle_Vector3F* v)
{
    return (float)sqrt(vehicle_vector3d_dot(v, v));
}

static __inline Vehicle_Vector3F* vehicle_vector3d_add(
    Vehicle_Vector3F* r, const Vehicle_Vector3F* a, const Vehicle_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline Vehicle_Vector3F* vehicle_vector3d_subtract(
    Vehicle_Vector3F* r, const Vehicle_Vector3F* a, const Vehicle_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline Vehicle_Vector3F* vehicle_vector3d_normalise(
    Vehicle_Vector3F* r)
{
    return (Vehicle_Vector3F*)vec_scale((float*)r, (const float*)r,
                                        1.0f / vehicle_vector3d_modulus(r));
}

static __inline Vehicle_Vector3F* vehicle_vector3d_scale(
    Vehicle_Vector3F* r, const Vehicle_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

/* Maths_RayEndPoint: p + t * v. */
static __inline Vehicle_Vector3F* vehicle_ray_end_point(
    Vehicle_Vector3F* r, const Vehicle_Vector3F* p, const Vehicle_Vector3F* v,
    float t)
{
    r->x = p->x + (t * v->x);
    r->y = p->y + (t * v->y);
    r->z = p->z + (t * v->z);
    return r;
}

/* gods98's left-handed form, (a.z * b.y - a.y * b.z, ...). */
static __inline Vehicle_Vector3F* vehicle_vector3d_cross_product(
    Vehicle_Vector3F* r, const Vehicle_Vector3F* a, const Vehicle_Vector3F* b)
{
    r->x = a->z * b->y - a->y * b->z;
    r->y = a->x * b->z - a->z * b->x;
    r->z = a->y * b->x - a->x * b->y;
    return r;
}

#endif /* LEGORRR_GAME_VEHICLE_MODEL_MATHS_H */
