// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-003 — original translation unit (text 004065f0-00406990, .rdata
 * 0049e048-0049e050; finding 505). Cubic Bezier sampling and arc-length
 * helpers.
 *
 * Rebuilt in the original's shape (issue #417, ADR 0005): one file for the
 * unit, its seven functions in original order and the C-compatible subset,
 * so the matching build compiles it as C. The native port compiles it as
 * C++. The unit has no .bss or .data. The functions keep the port's names;
 * bezier.h gives the corpus names. */
#include <math.h>

#include "bezier.h"

/* The unit's .rdata, in original order. Like Maths.c, the unit does not
 * reproduce the 16-byte {1, 2, 4, -1} table at 0049e034 that opens its block
 * (finding 505), nor the unreferenced 1.0 after it at 0049e044. The table
 * is d3drmdef.h's (finding 524), but the toolchain's DirectX 3 d3drm.h
 * shifts the symbol counter so that cubic_bezier_point_evaluate falls
 * (finding 646). */
// GLOBAL: LEGORR 0x0049e048
static const float s_bezier_three = 3.0f;
// GLOBAL: LEGORR 0x0049e04c
static const float s_bezier_zero = 0.0f;
/* BezierCurveSamplesBuild's 1.0. The function uses the literal: MSVC hoists
 * 1.0f / (count - 1) out of the loop only for a constant, as the original
 * does, and not for a const variable. This copy keeps the table's layout. */
// GLOBAL: LEGORR 0x0049e050
static const float s_bezier_one = 1.0f;

/* The control points c1 = p0 + 3 * p1 and c2 = p2 - 3 * p3 are computed
 * before r->x is written: the original reloads only p0 and p2 after that
 * store. The order of the local declarations and of the r->x terms sets
 * MSVC's x87 register choice (finding 506); this order scored best. */
// FUNCTION: LEGORR 0x004065f0
void cubic_bezier_point_evaluate(Point2F* r, const Point2F* p0,
                                 const Point2F* p1, const Point2F* p2,
                                 const Point2F* p3, float t)
{
    Point2F c1, c2;
    float tCubed, ax, cx, tSquared, cy, bx, by, ay;

    tSquared = t * t;
    tCubed = tSquared * t;

    c1.x = p0->x + p1->x * s_bezier_three;
    c1.y = p0->y + p1->y * s_bezier_three;
    c2.x = p2->x - p3->x * s_bezier_three;
    c2.y = p2->y - p3->y * s_bezier_three;

    cx = s_bezier_three * (c1.x - p0->x);
    bx = s_bezier_three * (c2.x - c1.x) - cx;
    ax = p2->x - p0->x - cx - bx;
    r->x = (ax * tCubed) + (cx * t) + (bx * tSquared) + p0->x;

    cy = s_bezier_three * (c1.y - p0->y);
    by = s_bezier_three * (c2.y - c1.y) - cy;
    ay = p2->y - p0->y - cy - by;
    r->y = (ay * tCubed) + (by * tSquared) + (cy * t) + p0->y;
}

// FUNCTION: LEGORR 0x00406730
float vec2_dist_alt(const Point2F* a, const Point2F* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return (float)sqrt(dx * dx + dy * dy);
}

// FUNCTION: LEGORR 0x00406760
Point2F* vector2_set_length(Point2F* v, float length)
{
    float scale = (float)sqrt(v->x * v->x + v->y * v->y) / length;
    v->x /= scale;
    v->y /= scale;
    return v;
}

// FUNCTION: LEGORR 0x004067b0
float polyline_segment_lengths_total(BezierCurve* curve)
{
    unsigned int i;
    float total = 0.0f;
    float dist;

    curve->distances[0] = 0.0f;
    for (i = 1; i < curve->count; i++) {
        dist = vec2_dist_alt(&curve->points[i - 1], &curve->points[i]);
        curve->distances[i] = dist;
        total += dist;
    }
    return total;
}

// FUNCTION: LEGORR 0x00406820
void bezier_curve_samples_build(BezierCurve* curve, const Point2F* p0,
                                const Point2F* p1, const Point2F* p2,
                                const Point2F* p3, unsigned int count)
{
    unsigned int i;

    if (count >= BEZIER_MAXPOINTS) count = BEZIER_MAXPOINTS;
    curve->count = count;
    curve->points[0] = *p0;
    for (i = 1; i < count; i++) {
        cubic_bezier_point_evaluate(&curve->points[i], p0, p1, p2, p3,
                                    (float)i * (1.0f / (float)(count - 1)));
    }
}

// FUNCTION: LEGORR 0x004068c0
unsigned int path_arc_length_interpolate(const BezierCurve* curve,
                                         float distance, Point2F* r)
{
    unsigned int i;
    float dist = s_bezier_zero;
    float frac;
    Point2F delta;

    for (i = 1; i < curve->count; i++) {
        dist += curve->distances[i];
        if (distance < dist) break;
    }

    if (i < curve->count) {
        *r = curve->points[i - 1];
        frac = (distance - (dist - curve->distances[i])) /
               curve->distances[i];
        delta.x = curve->points[i].x - curve->points[i - 1].x;
        delta.y = curve->points[i].y - curve->points[i - 1].y;
        delta.x *= frac;
        delta.y *= frac;
        r->x = delta.x + curve->points[i - 1].x;
        r->y = delta.y + curve->points[i - 1].y;
        return i;
    }

    *r = curve->points[i - 1];
    return i;
}

/* Called only from LegoObject (0044a070). It has no data, so its unit is set
 * by the boundary of finding 505, which ends GAME-003 after it. */
// FUNCTION: LEGORR 0x00406980
int scene_object_mode_is_one(const void* scene_object)
{
    return *(const int*)((const char*)scene_object + 0x28) == 1;
}
