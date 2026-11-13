// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009's copy of gods98 Maths.h's inline vector helpers (add, scale,
 * cross product, ray end point) over DamageFont_Vector3F (finding 609). The
 * original unit expands them in place, so they have no address of their
 * own. Only screen_effect_quads.c includes this file, after
 * DamageFont_Vector3F.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). */
#ifndef LEGORRR_GAME_SCREEN_EFFECT_QUADS_MATHS_H
#define LEGORRR_GAME_SCREEN_EFFECT_QUADS_MATHS_H

static __inline DamageFont_Vector3F* damage_font_vector3d_add(
    DamageFont_Vector3F* r, const DamageFont_Vector3F* a,
    const DamageFont_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

static __inline DamageFont_Vector3F* damage_font_vector3d_scale(
    DamageFont_Vector3F* r, const DamageFont_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline DamageFont_Vector3F* damage_font_vector3d_cross_product(
    DamageFont_Vector3F* r, const DamageFont_Vector3F* a,
    const DamageFont_Vector3F* b)
{
    r->x = a->y * b->z - a->z * b->y;
    r->y = a->z * b->x - a->x * b->z;
    r->z = a->x * b->y - a->y * b->x;
    return r;
}

static __inline DamageFont_Vector3F* damage_font_ray_end_point(
    DamageFont_Vector3F* r, const DamageFont_Vector3F* p,
    const DamageFont_Vector3F* v, float t)
{
    r->x = p->x + v->x * t;
    r->y = p->y + v->y * t;
    r->z = p->z + v->z * t;
    return r;
}

#endif /* LEGORRR_GAME_SCREEN_EFFECT_QUADS_MATHS_H */
