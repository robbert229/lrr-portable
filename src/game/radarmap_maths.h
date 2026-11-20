// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.h's inline Vector2DScale and Vector2DAdd, as RadarMap.c expands them.
 *
 * Included by radarmap.c where the helpers' definitions sat, so the unit's
 * token order (and MSVC's symbol numbering, finding 538) is unchanged. */
#ifndef LEGORRR_GAME_RADARMAP_MATHS_H
#define LEGORRR_GAME_RADARMAP_MATHS_H

static __inline Point2F* radarmap_vector2d_scale(Point2F* r, Point2F* a, float f)
{
    r->x = f * a->x;
    r->y = f * a->y;
    return r;
}

static __inline Point2F* radarmap_vector2d_add(Point2F* r, Point2F* a, Point2F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    return r;
}

#endif
