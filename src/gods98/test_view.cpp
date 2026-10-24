// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_view — validation for src/gods98/view.cpp (ViewContextTick @ 0x435f5c).
 *
 * Covers the pure vector helpers, the record-backed camera-handle
 * primitives, the glide-to-stop tail, the mode-1 grid-edge clamp, and the
 * mode-3 velocity path via the test blend hook.
 */
#include <stdio.h>
#include <string.h>

#include "view.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

int main(void)
{
    /* vector helpers */
    float s[3] = { 2, 3, 4 }, d[3], a[3] = { 1, 2, 3 }, b[3] = { 4, 5, 6 };
    vec_scale(d, s, 2.0f);
    CHECK(d[0] == 4.0f && d[1] == 6.0f && d[2] == 8.0f, "vec_scale");
    vec_add(d, a, b);
    CHECK(d[0] == 5.0f && d[1] == 7.0f && d[2] == 9.0f, "vec_add");
    CHECK(vec_dist(a, b) > 5.19f && vec_dist(a, b) < 5.20f, "vec_dist");
    CHECK(vec_dot(a, b) == 32.0f, "vec_dot");
    CHECK(vec_len(a) > 3.741f && vec_len(a) < 3.742f, "vec_len");
    vec_sub(d, b, a);
    CHECK(d[0] == 3.0f && d[1] == 3.0f && d[2] == 3.0f, "vec_sub");
    {   /* x * y = z (corpus right-hand-order cross convention). */
        float x[3] = { 1, 0, 0 }, y[3] = { 0, 1, 0 };
        vec_cross(d, x, y);
        CHECK(d[0] == 0.0f && d[1] == 0.0f && d[2] == 1.0f, "vec_cross");
    }
    {
        float n[3] = { 3, 0, 4 };
        vec_normalize(n);
        CHECK(vec_len(n) > 0.9999f && vec_len(n) < 1.0001f, "vec_normalize");
    }
    {
        float p[2] = { 1, 2 }, q[2] = { 4, 6 };
        CHECK(vec2_dist(p, q) > 4.99f && vec2_dist(p, q) < 5.01f, "vec2_dist");
        CHECK(vec2_dist_alt(p, q) == vec2_dist(p, q), "vec2_dist_alt agrees");
    }
    CHECK(vec_exact_equal(a, b) == 0, "different vectors unequal");
    CHECK(vec_exact_equal(a, a) == 1, "identical vectors equal");

    /* 4x4 matrices: zero-fill, and left×right order (row-major). */
    {
        float m[16];
        matrix4_set_zero(m);
        int z = 1;
        for (int i = 0; i < 16; ++i) {
            z &= m[i] == 0.0f;
        }
        CHECK(z, "matrix4_set_zero clears every cell");
    }
    {
        const float A[16] = { 1, 2, 0, 0,  0, 3, 0, 0,  0, 0, 1, 0,
                              0, 0, 0, 1 };
        const float B[16] = { 0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,
                              0, 0, 0, 1 };
        float out[16];
        matrix4_multiply(out, B, A); /* corpus (out, right, left) = A×B */
        CHECK(out[0] == 2.0f && out[1] == 1.0f && out[2] == 0.0f &&
                  out[3] == 0.0f, "A×B row 0");
        CHECK(out[4] == 3.0f && out[5] == 0.0f, "A×B row 1");
        CHECK(out[8] == 0.0f && out[9] == 0.0f && out[10] == 1.0f &&
                  out[11] == 0.0f && out[15] == 1.0f, "A×B rows 2/3");
        matrix4_multiply(out, A, B);
        CHECK(out[0] == 0.0f && out[1] == 3.0f && out[2] == 0.0f,
              "B×A differs: multiplication order is corpus's");
    }

    /* PointInPolygon2D: square ring as edge-start/edge-end pairs. */
    {
        const float start[8] = { 1, -1,  1, 1,  -1, 1,  -1, -1 };
        const float end[8]   = { 1, 1,  -1, 1,  -1, -1,  1, -1 };
        const float inside[2] = { 0, 0 };
        const float outside[2] = { 2, 0 };
        CHECK(point_in_polygon_2d(inside, start, end, 4) == 1,
              "center point answers inside");
        CHECK(point_in_polygon_2d(outside, start, end, 4) == 0,
              "far point answers outside");
        CHECK(point_in_polygon_2d(inside, start, end, 0) == 0,
              "zero edges answer even parity");
    }

    /* geometry helpers */
    CHECK(point_distance_within_radius(0, 0, 3, 4, 5.1f) == 1,
          "distance under radius");
    CHECK(point_distance_within_radius(0, 0, 3, 4, 5.0f) == 0,
          "strictly under: equality is not under");
    {
        float rect[4] = { 10, 20, 30, 40 }; /* x, y, w, h */
        CHECK(rect_contains_point_half_open(rect, 10, 20) == 1,
              "origin corner inside");
        CHECK(rect_contains_point_half_open(rect, 40, 20) == 0,
              "right edge exclusive");
        CHECK(rect_contains_point_half_open(rect, 39, 59) == 1,
              "just inside the far corner");
        CHECK(rect_contains_point_half_open(rect, 10, 60) == 0,
              "top edge exclusive");
    }
    /* Offset-point rect family: same half-open test on a bounds quad with
     * the float offsets truncated (the corpus's __ftol pair). */
    {
        const float rect[4] = { 5, 8, 55, 33 }; /* x_min, y_min, x_max, y_max */
        CHECK(rect_contains_offset_point(rect, 0.0f, 0.0f, 5, 8) == 1,
              "lower corner is inside");
        CHECK(rect_contains_offset_point(rect, 0.0f, 0.0f, 55, 33) == 0,
              "upper edges are exclusive");
        CHECK(rect_contains_offset_point(rect, 0.9f, 0.9f, 6, 9) == 1,
              "offset 0.9 truncates to 0: same answer as zero");
        CHECK(rect_contains_offset_point(rect, 2.7f, 2.7f, 8, 11) == 1,
              "truncated offset 2 shifts the corner back in");
        CHECK(rect_contains_offset_point(rect, 3.7f, 0.0f, 8, 11) == 1,
              "just inside after truncation");
        CHECK(rect_contains_offset_point(rect, 3.8f, 0.0f, 58, 11) == 0,
              "truncating past the exclusive edge answers outside");
        CHECK(rect_contains_offset_point(rect, -1.0f, 0.0f, 4, 8) == 1,
              "negative offset shifts the test point up");
        CHECK(rect_contains_point_by_converted_coords(rect, 2.0f, 3.0f, 7,
                                                      11) == 1,
              "the region-route copy agrees");
    }
    {
        float rect_mem[4] = { 5, 8, 55, 33 };
        const float* slot = rect_mem; /* stands in for the record +0x14 */
        CHECK(rect_contains_point_from_object_bounds(&slot, 1.5f, 1.5f, 6,
                                                     9) == 1,
              "bounds read through the slot");
        CHECK(rect_contains_point_from_object_bounds(&slot, 1.5f, 1.5f, 56,
                                                     33) == 0,
              "the slot copy stays half-open");
        CHECK(rect_contains_point_from_object_bounds(NULL, 0.0f, 0.0f, 6,
                                                     9) == 0,
              "NULL slot answers outside");
        const float* null_slot = NULL;
        CHECK(rect_contains_point_from_object_bounds(&null_slot, 0.0f, 0.0f,
                                                     6, 9) == 0,
              "NULL rect answers outside");
    }
    {
        float p[3] = { 0, 0, 0 }, d[3] = { 0, 1, 0 }, q[3] = { 0, 5, 0 },
              n[3] = { 0, 1, 0 };
        float t;
        CHECK(ray_plane_intersection_parameter(&t, p, d, q, n) == 1 && t == 5.0f,
              "orthogonal ray-plane hit at t 5");
        float dd[3] = { 1, 0, 0 };
        CHECK(ray_plane_intersection_parameter(&t, p, dd, q, n) == 0,
              "parallel ray answers 0");
    }
    {
        float p1[2] = { 0, 0 }, d1[2] = { 1, 0 }, p2[2] = { 5, -3 },
              d2[2] = { 0, 1 }, hit[2];
        float* r = line_intersection_2d_compute(hit, p1, d1, p2, d2);
        CHECK(r == hit && hit[0] == 5.0f && hit[1] == 0.0f,
              "perpendicular lines meet at (5,0)");
        float d3[2] = { 1, 0 };
        CHECK(line_intersection_2d_compute(hit, p1, d1, p2, d3) == NULL,
              "parallel lines answer NULL");
    }
    {
        float c[3] = { 0, 0, 0 }, o[3] = { 0, 0, -10 }, dir[3] = { 0, 0, 1 };
        CHECK(sphere_ray_intersection_discriminant_positive(c, 4, o, dir) == 1,
              "ray aiming through the sphere discriminants positive");
        float off[3] = { 5, 0, -10 };
        CHECK(sphere_ray_intersection_discriminant_positive(c, 4, off, dir) == 0,
              "missed ray discriminants non-positive");
    }

    /* camera-handle primitives */
    CameraHandle c = {};
    cam_offset(&c, 1, 2, 3);
    CHECK(c.inuse == 1 && c.pos[0] == 1 && c.pos[1] == 2 && c.pos[2] == 3,
          "cam_offset");
    cam_set_pos(&c, 10, 20, 30);
    float out[3];
    cam_get_pos(&c, out);
    CHECK(out[0] == 10 && out[1] == 20 && out[2] == 30, "cam_set/get_pos");
    float rot[3] = { 0, 1, 0 };
    cam_set_facing(&c, rot);
    cam_get_facing(&c, out);
    CHECK(out[0] == 0 && out[1] == 1 && out[2] == 0, "cam facing");

    EngineState st = {};
    EngineContext tctx = {};
    tctx.state_block = &st;
    st.grid_w = 32;
    st.grid_h = 32;

    /* glide-to-stop tail: motion timer expired -> velocity snaps. */
    ViewContext tv = {};
    tv.mode = 3;
    tv.step = 8.0f;
    tv.timer_num = 0;
    tv.timer_acc = 5;
    tv.velocity[0] = 4.0f;
    view_tick(&tv, &tctx, 1.0f / 30.0f, 1.0f);
    CHECK(tv.velocity[0] == 0.0f, "tail: timer expired snaps velocity");

    /* tail decay: timer budget left -> decays toward step. */
    ViewContext dv = {};
    dv.mode = 3;
    dv.step = 8.0f;
    dv.timer_num = 10;
    dv.timer_acc = 0;
    dv.velocity[0] = 4.0f;
    view_tick(&dv, &tctx, 1.0f / 30.0f, 1.0f);
    /* decay = (10-0)/10 * 8 = 8 -> vel = 4 * 8 = 32; corpus scales velocity. */
    CHECK(dv.velocity[0] == 32.0f, "tail: velocity decays by (num-acc)/num*step");

    /* mode 1: follow camera clamps at the grid edge. */
    g_view_follow_scale = 1.0f;
    g_view_grid_edge = 4;
    ViewContext fv = {};
    fv.mode = 1;
    fv.offset[0] = 30.0f; /* beyond grid_w(32) - edge(4) = 28 */
    fv.offset[1] = 3.0f;
    view_tick(&fv, &tctx, 1.0f / 30.0f, 1.0f);
    CHECK(fv.cam_world.inuse == 1, "mode1: camera touched");

    /* mode 3 velocity path via the test blend hook. */
    g_view_test_blend = 1;
    ViewContext mv = {};
    mv.mode = 3;
    mv.follow_id = 1; /* corpus mode-3 gate: needs a target id */
    mv.step = 8.0f;
    mv.timer_num = 10;
    mv.timer_acc = 0;
    mv.velocity[0] = 2.0f;
    mv.velocity[1] = 1.0f;
    mv.velocity[2] = 0.5f;
    cam_set_pos(&mv.cam_world, 5, 5, 0);
    view_tick(&mv, &tctx, 1.0f / 30.0f, 1.0f);
    CHECK((mv.flags & 0x10) != 0, "mode3: dirty flag set");
    /* body repositions to the blend target (0,0,0), then the tail slides by
     * the decayed velocity (decay = 8 -> vel {16,8,4}) => {-16,-8,-4}. */
    CHECK(mv.cam_world.pos[0] == -16.0f && mv.cam_world.pos[1] == -8.0f
          && mv.cam_world.pos[2] == -4.0f, "mode3: camera re-positioned + glide");
    g_view_test_blend = 0;

    /* mode 2 with no object target idles (object_lookup stub). */
    ViewContext lv = {};
    lv.mode = 2;
    view_tick(&lv, &tctx, 1.0f / 30.0f, 1.0f);
    CHECK(lv.cam_world.inuse == 1 && lv.velocity[0] == 0.0f
          && lv.velocity[1] == 0.0f && lv.velocity[2] == 0.0f,
          "mode2: idles without target");

    /* MenuPointerRectangleHitTest (FUN_004112e0): closed-edge widget
     * rectangle test — the menu row band from finding 23. */
    CHECK(menu_pointer_rectangle_hit_test(50, 10, 0, 0, 100, 20) == 1,
          "widget: inside the row band");
    CHECK(menu_pointer_rectangle_hit_test(0, 0, 0, 0, 100, 20) == 1,
          "widget: lower corners inclusive");
    CHECK(menu_pointer_rectangle_hit_test(100, 20, 0, 0, 100, 20) == 1,
          "widget: upper corners inclusive");
    CHECK(menu_pointer_rectangle_hit_test(101, 20, 0, 0, 100, 20) == 0,
          "widget: one past width fails");
    CHECK(menu_pointer_rectangle_hit_test(100, 21, 0, 0, 100, 20) == 0,
          "widget: one past height fails");
    CHECK(menu_pointer_rectangle_hit_test(-1, 10, 0, 0, 100, 20) == 0,
          "widget: below the origin fails");
    CHECK(menu_pointer_rectangle_hit_test(50, 10, 40, 5, 10, 5) == 1,
          "widget: offset rectangle contains");

    if (s_failures == 0) {
        printf("view_test: PASS\n");
        return 0;
    }
    printf("view_test: %d failure(s)\n", s_failures);
    return 1;
}