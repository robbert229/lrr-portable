// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-020's copy of gods98 Maths.h's inline vector helpers (add, subtract,
 * scale, interpolate), over LwsVector3. The original unit expands them in
 * place, so they have no address of their own. Only lws.c includes this
 * file, after its types.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). */
#ifndef LEGORRR_GODS98_LWS_MATHS_H
#define LEGORRR_GODS98_LWS_MATHS_H

static __inline LwsVector3* lws_vector3d_add(LwsVector3* r, const LwsVector3* a,
                                             const LwsVector3* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline LwsVector3* lws_vector3d_subtract(LwsVector3* r,
                                                  const LwsVector3* a,
                                                  const LwsVector3* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

static __inline LwsVector3* lws_vector3d_scale(LwsVector3* r,
                                               const LwsVector3* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline LwsVector3* lws_vector3d_interpolate(LwsVector3* r,
                                                     const LwsVector3* a,
                                                     const LwsVector3* b,
                                                     float f)
{
    lws_vector3d_subtract(r, b, a);
    lws_vector3d_scale(r, r, f);
    lws_vector3d_add(r, a, r);
    return r;
}

static __inline LwsVector3* lws_try_interp1(LwsVector3* r, const LwsVector3* a,
                                            const LwsVector3* b, float f)
{
    r->x = b->x - a->x;
    r->y = b->y - a->y;
    r->z = b->z - a->z;
    r->x = r->x * f;
    r->y = r->y * f;
    r->z = r->z * f;
    r->x = a->x + r->x;
    r->y = a->y + r->y;
    r->z = a->z + r->z;
    return r;
}

static __inline void lws_try_interp2(LwsVector3* r, LwsVector3* a,
                                     LwsVector3* b, float f)
{
    r->x = b->x - a->x;
    r->y = b->y - a->y;
    r->z = b->z - a->z;
    r->x *= f;
    r->y *= f;
    r->z *= f;
    r->x = a->x + r->x;
    r->y = a->y + r->y;
    r->z = a->z + r->z;
}

#endif /* LEGORRR_GODS98_LWS_MATHS_H */
