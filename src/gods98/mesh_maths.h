// SPDX-License-Identifier: LGPL-3.0-or-later
/* Mesh.c's inline vector helpers (gods98 Maths.h's Maths_Vector3D* inlines:
 * add, subtract, dot, modulus, scale, normalise) over Mesh_Vector3F. The
 * original unit expands them in place, so they have no address of their
 * own. Only mesh_unit.c includes this file, after Mesh_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GODS98_MESH_MATHS_H
#define LEGORRR_GODS98_MESH_MATHS_H

static __inline Mesh_Vector3F* mesh_vector3d_add(Mesh_Vector3F* r,
                                                 Mesh_Vector3F* a,
                                                 Mesh_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline Mesh_Vector3F* mesh_vector3d_subtract(Mesh_Vector3F* r,
                                                      Mesh_Vector3F* a,
                                                      Mesh_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

/* The parentheses matter: MSVC 5.0 keeps a parenthesised float product as
 * written, and with them each inlined Mesh_ParseLWO normalise copies y and
 * z to stack temporaries as the original does (finding 661). */
static __inline float mesh_vector3d_dot(Mesh_Vector3F* a, Mesh_Vector3F* b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

static __inline float mesh_vector3d_modulus(Mesh_Vector3F* v)
{
    return (float)sqrt(mesh_vector3d_dot(v, v));
}

static __inline Mesh_Vector3F* mesh_vector3d_scale(Mesh_Vector3F* r,
                                                   Mesh_Vector3F* a,
                                                   float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline Mesh_Vector3F* mesh_vector3d_normalise(Mesh_Vector3F* r)
{
    return mesh_vector3d_scale(r, r, 1.0f / mesh_vector3d_modulus(r));
}

#endif /* LEGORRR_GODS98_MESH_MATHS_H */
