// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-003 (original translation unit GAME-003, finding 505): cubic Bezier
 * curves sampled into a point table, with the arc-length helpers that walk
 * it. The original file name is not attested ("BezierCurve.c" is likely).
 *
 * C-compatible header (ADR 0005). bezier.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). */
#ifndef LEGORRR_GAME_BEZIER_H
#define LEGORRR_GAME_BEZIER_H

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Most points a sampled curve holds; the builder clamps its count to it. */
#define BEZIER_MAXPOINTS 50

/* A curve sampled into BEZIER_MAXPOINTS points (0x25c bytes). distances[i]
 * is the length of the segment points[i - 1] -> points[i]; distances[0] is
 * zero. */
typedef struct BezierCurve {
    unsigned int count;                    /* +0x000 */
    Point2F points[BEZIER_MAXPOINTS];      /* +0x004 */
    float distances[BEZIER_MAXPOINTS];     /* +0x194 */
} BezierCurve;

/* CubicBezierPointEvaluate (FUN_004065f0): the point at t (0..1) of the
 * cubic from p0 to p2, whose control points are p0 + 3 * p1 and
 * p2 - 3 * p3 (p1 and p3 are the end tangents). */
void cubic_bezier_point_evaluate(Point2F* r, const Point2F* p0,
                                 const Point2F* p1, const Point2F* p2,
                                 const Point2F* p3, float t);

/* Vector2DistanceVariant (FUN_00406730): |a - b|. */
float vec2_dist_alt(const Point2F* a, const Point2F* b);

/* Vector2SetLength (FUN_00406760): scale v to `length`. A zero vector or
 * length divides by zero, as in the original. */
Point2F* vector2_set_length(Point2F* v, float length);

/* PolylineSegmentLengthsAndTotalCalculate (FUN_004067b0): fill
 * curve->distances and return the curve's total length. */
float polyline_segment_lengths_total(BezierCurve* curve);

/* BezierCurveSamplesBuild (FUN_00406820): sample the cubic (arguments as
 * cubic_bezier_point_evaluate) at `count` evenly spaced t, at most
 * BEZIER_MAXPOINTS. The distances are not updated. */
void bezier_curve_samples_build(BezierCurve* curve, const Point2F* p0,
                                const Point2F* p1, const Point2F* p2,
                                const Point2F* p3, unsigned int count);

/* PathArcLengthInterpolate (FUN_004068c0): the point `distance` along the
 * curve, interpolated within its segment. Returns the segment's end index;
 * at or past the end, r is the last point and the result is curve->count. */
unsigned int path_arc_length_interpolate(const BezierCurve* curve,
                                         float distance, Point2F* r);

/* SceneObjectModeIsOne (FUN_00406980, findings 373/435): the +0x28 mode
 * word equals 1. */
int scene_object_mode_is_one(const void* scene_object);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BEZIER_H */
