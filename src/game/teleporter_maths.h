// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-044's copy of gods98 Maths.h's inline 2D vector helpers
 * (Maths_Vector2DModulus, Maths_Vector2DDistance) over Teleporter_Point2F.
 * The original unit expands them in place, so they have no address of
 * their own. Only teleporter.c includes this file, after teleporter.h.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_TELEPORTER_MATHS_H
#define LEGORRR_GAME_TELEPORTER_MATHS_H

static __inline float teleporter_vector2d_modulus(const Teleporter_Point2F* v)
{
    return (float)sqrt((v->x * v->x) + (v->y * v->y));
}

static __inline float teleporter_vector2d_distance(const Teleporter_Point2F* a,
                                                   const Teleporter_Point2F* b)
{
    Teleporter_Point2F c;
    c.x = a->x - b->x;
    c.y = a->y - b->y;
    return teleporter_vector2d_modulus(&c);
}

#endif /* LEGORRR_GAME_TELEPORTER_MATHS_H */
