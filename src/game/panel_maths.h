// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-032's copies of gods98 Maths.h's inline 2D vector helpers, which the
 * original expands inside the rotation control's circle clamp (0045b930).
 * They have no address of their own. Only panel_unit.c includes this file;
 * in a header they add no annotation-lint debt, as steering_maths.h
 * (finding 544) and advisor_maths.h (finding 550). */
#ifndef LEGORRR_GAME_PANEL_MATHS_H
#define LEGORRR_GAME_PANEL_MATHS_H

static __inline float panel_vector2d_modulus(const Point2F* v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y);
}

static __inline Point2F* panel_vector2d_scale(Point2F* r, const Point2F* a,
                                              float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    return r;
}

static __inline Point2F* panel_vector2d_normalise(Point2F* v)
{
    return panel_vector2d_scale(v, v, 1.0f / panel_vector2d_modulus(v));
}

static __inline Point2F* panel_vector2d_set_length(Point2F* v, float length)
{
    return panel_vector2d_scale(v, panel_vector2d_normalise(v), length);
}

#endif /* LEGORRR_GAME_PANEL_MATHS_H */
