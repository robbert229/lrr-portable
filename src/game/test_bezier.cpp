// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_bezier — the GAME-003 unit (game/bezier.c, issue #417): the cubic
 * evaluator, the 2D distance and set-length helpers, the sample builder with
 * its count clamp, the segment-length table and the arc-length walk.
 *
 * The control points are small integers, so every intermediate is exact in
 * single precision and the checks compare exactly. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "game/bezier.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void set_point(Point2F* p, float x, float y)
{
    p->x = x;
    p->y = y;
}

int main(void)
{
    CHECK(sizeof(BezierCurve) == 0x25c, "the curve record is 0x25c bytes");
    CHECK(offsetof(BezierCurve, distances) == 0x194,
          "the distance table starts at +0x194");

    /* CubicBezierPointEvaluate: the ends of the curve. */
    {
        Point2F p0, p1, p2, p3, r;
        set_point(&p0, 0.0f, 0.0f);
        set_point(&p1, 1.0f, 5.0f);
        set_point(&p2, 2.0f, 10.0f);
        set_point(&p3, 3.0f, 15.0f);
        cubic_bezier_point_evaluate(&r, &p0, &p1, &p2, &p3, 0.0f);
        CHECK(r.x == 0.0f && r.y == 0.0f, "t zero lands on p0");
        cubic_bezier_point_evaluate(&r, &p0, &p1, &p2, &p3, 1.0f);
        CHECK(r.x == 2.0f && r.y == 10.0f, "t one lands on p2");
    }

    /* Vector2DistanceVariant and Vector2SetLength. */
    {
        Point2F a, b, v;
        set_point(&a, 1.0f, 2.0f);
        set_point(&b, 4.0f, 6.0f);
        CHECK(vec2_dist_alt(&a, &b) == 5.0f, "the 3-4-5 distance");
        set_point(&v, 3.0f, 0.0f);
        CHECK(vector2_set_length(&v, 2.0f) == &v, "set-length returns v");
        CHECK(v.x == 2.0f && v.y == 0.0f, "the length lands on x");
        set_point(&v, 0.0f, 4.0f);
        vector2_set_length(&v, 1.0f);
        CHECK(v.x == 0.0f && v.y == 1.0f, "the unit y stays on its axis");
    }

    /* BezierCurveSamplesBuild: count, first point, last sample, clamp. */
    {
        static BezierCurve curve;
        Point2F p0, p1, p2, p3;
        set_point(&p0, 0.0f, 0.0f);
        set_point(&p1, 1.0f, 1.0f);
        set_point(&p2, 2.0f, 2.0f);
        set_point(&p3, 3.0f, 3.0f);
        memset(&curve, 0, sizeof curve);
        bezier_curve_samples_build(&curve, &p0, &p1, &p2, &p3, 5u);
        CHECK(curve.count == 5u, "the count lands");
        CHECK(curve.points[0].x == 0.0f && curve.points[0].y == 0.0f,
              "the first point is p0");
        CHECK(curve.points[4].x == 2.0f && curve.points[4].y == 2.0f,
              "the last sample (t one) is p2");
        bezier_curve_samples_build(&curve, &p0, &p1, &p2, &p3, 0x33u);
        CHECK(curve.count == BEZIER_MAXPOINTS, "the count clamps to 50");
        CHECK(curve.points[49].x == 2.0f && curve.points[49].y == 2.0f,
              "the clamped build still ends on p2");
        memset(&curve, 0, sizeof curve);
        set_point(&p0, 7.0f, 8.0f);
        bezier_curve_samples_build(&curve, &p0, &p1, &p2, &p3, 1u);
        CHECK(curve.count == 1u && curve.points[0].x == 7.0f &&
              curve.points[0].y == 8.0f && curve.points[1].x == 0.0f,
              "a single-point build stores only p0");
    }

    /* PolylineSegmentLengthsAndTotalCalculate. */
    {
        static BezierCurve curve;
        memset(&curve, 0xff, sizeof curve);
        curve.count = 4u;
        set_point(&curve.points[0], 0.0f, 0.0f);
        set_point(&curve.points[1], 3.0f, 4.0f);
        set_point(&curve.points[2], 3.0f, 0.0f);
        set_point(&curve.points[3], 0.0f, 0.0f);
        CHECK(polyline_segment_lengths_total(&curve) == 12.0f,
              "the total sums the segments");
        CHECK(curve.distances[0] == 0.0f, "distance 0 clears");
        CHECK(curve.distances[1] == 5.0f && curve.distances[2] == 4.0f &&
              curve.distances[3] == 3.0f, "each segment's length lands");
        curve.count = 1u;
        CHECK(polyline_segment_lengths_total(&curve) == 0.0f,
              "a single point has no length");
    }

    /* PathArcLengthInterpolate. */
    {
        static BezierCurve curve;
        Point2F r;
        unsigned int seg;
        memset(&curve, 0, sizeof curve);
        curve.count = 4u;
        set_point(&curve.points[0], 0.0f, 0.0f);
        set_point(&curve.points[1], 3.0f, 4.0f);
        set_point(&curve.points[2], 3.0f, 8.0f);
        set_point(&curve.points[3], 0.0f, 12.0f);
        (void)polyline_segment_lengths_total(&curve); /* 5 + 4 + 5 */

        seg = path_arc_length_interpolate(&curve, 2.0f, &r);
        CHECK(seg == 1u && r.x == 1.2f && r.y == 1.6f,
              "the first segment interpolates the 3-4-5 slope");
        seg = path_arc_length_interpolate(&curve, 5.5f, &r);
        CHECK(seg == 2u && r.x == 3.0f && r.y == 4.5f,
              "the second segment starts from its own head");
        seg = path_arc_length_interpolate(&curve, 14.0f, &r);
        CHECK(seg == 4u && r.x == 0.0f && r.y == 12.0f,
              "the full length lands on the last point");
        seg = path_arc_length_interpolate(&curve, 0.0f, &r);
        CHECK(seg == 1u && r.x == 0.0f && r.y == 0.0f,
              "zero distance stays on segment one");
        curve.count = 1u;
        seg = path_arc_length_interpolate(&curve, 5.0f, &r);
        CHECK(seg == 1u && r.x == 0.0f && r.y == 0.0f,
              "a single-point curve answers its only point");
    }

    /* SceneObjectModeIsOne: the +0x28 word. */
    {
        unsigned char object[0x40];
        int mode = 1;
        memset(object, 0, sizeof object);
        memcpy(object + 0x28, &mode, sizeof mode);
        CHECK(scene_object_mode_is_one(object) == 1, "mode 1 is one");
        mode = 2;
        memcpy(object + 0x28, &mode, sizeof mode);
        CHECK(scene_object_mode_is_one(object) == 0, "mode 2 is not one");
    }

    if (s_failures == 0) {
        printf("bezier_test: PASS\n");
        return 0;
    }
    printf("bezier_test: %d failure(s)\n", s_failures);
    return 1;
}
