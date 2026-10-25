// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_view — validation for src/gods98/view.cpp (ViewContextTick @ 0x435f5c).
 *
 * Covers the pure vector helpers, the record-backed camera-handle
 * primitives, the glide-to-stop tail, the mode-1 grid-edge clamp, and the
 * mode-3 velocity path via the test blend hook.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "view.hpp"
#include "record.hpp"

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

    /* MenuOverlayListCount (FUN_00412170): chain over pointer slots. */
    {
        typedef struct OverlayNode {
            char     link_pad[0x18];
            void*    next;
        } OverlayNode;
        OverlayNode chain[3] = { { {0}, NULL }, { {0}, NULL }, { {0}, NULL } };
        chain[0].next = &chain[1];
        chain[1].next = &chain[2];
        struct {
            char link_pad[0x40];
            void* head;
        } menu;
        memset(&menu, 0, sizeof(menu));
        menu.head = NULL;
        CHECK(menu_overlay_list_count(&menu) == 0, "empty chain counts zero");
        menu.head = &chain[0];
        CHECK(menu_overlay_list_count(&menu) == 3, "three-node chain");
        chain[1].next = NULL;
        CHECK(menu_overlay_list_count(&menu) == 2, "chain shortened by a tail");
        CHECK(menu_overlay_list_count(NULL) == 0, "NULL menu counts zero");
    }

    /* TerrainGridBorderInsetGet (FUN_00435f28): DAT_005560ac getter. */
    {
        const int saved = g_view_grid_edge;
        g_view_grid_edge = 3;
        CHECK(view_grid_edge_get() == 3, "the edge getter reads the global");
        g_view_grid_edge = 0;
        CHECK(view_grid_edge_get() == 0, "zero reads back zero");
        g_view_grid_edge = saved;
    }

    /* Camera-selection cluster (2026-10-24): the config setter, the
     * follow-mode camera leaves, and the pointer-pan accumulator. */
    {
        float save_follow = g_view_follow_scale;
        int save_edge = g_view_grid_edge;
        float save_cap = g_view_pan_cap, save_scale = g_view_pan_scale;
        CHECK(g_view_pan_cap == 0.0f && g_view_pan_scale == 0.0f,
              "the pan globals start zero (unfed)");

        ViewContext v = {};
        v.mode = 1;
        v.offset[0] = 0.0f; v.offset[1] = 0.0f; v.offset[2] = 0.0f;
        EngineContext ctx = {};

        /* The point set: follow mode writes cam_follow x/y (z zero). */
        selection_record_point_set(&v, 12.5f, -3.0f);
        float got[3] = { 9, 9, 9 };
        cam_get_pos(&v.cam_follow, got);
        CHECK(got[0] == 12.5f && got[1] == -3.0f && got[2] == 0.0f,
              "the follow-mode point set writes the follow camera");
        v.mode = 2;
        selection_record_point_set(&v, 99.0f, 99.0f);
        cam_get_pos(&v.cam_follow, got);
        CHECK(got[0] == 12.5f, "the non-follow mode ignores the point set");
        v.mode = 1;

        /* Position + terrain: unbound level answers zero z, constant 1. */
        float out[3] = { 7, 7, 7 };
        CHECK(camera_get_position_with_terrain_height(&v, &ctx, out) == 1
                  && out[2] == 0.0f,
              "the terrain read follows the follow camera (z stub)");
        out[0] = 7;
        v.mode = 3;
        CHECK(camera_get_position_with_terrain_height(&v, &ctx, out) == 1
                  && out[0] == 7.0f,
              "non-follow leaves the output alone (corpus shape)");
        v.mode = 1;

        /* The pan update: scale 2, cap 5. Move 1 along x three times. */
        camera_view_context_set(5.0f, 1.0f, 2.0f, 0);
        CHECK(g_view_follow_scale == 1.0f && g_view_grid_edge == 0,
              "the setter lands the follow scale and edge");
        float step[3] = { 1.0f, 0.0f, 0.0f };
        camera_pointer_pan_update(&v, step, 1.0f);
        CHECK(v.offset[0] == 2.0f && v.pan_accum == 2.0f,
              "the first move scales by amount * pan scale");
        step[0] = 1.0f;
        camera_pointer_pan_update(&v, step, 1.0f);
        CHECK(v.offset[0] == 6.0f && v.pan_accum == 4.0f,
              "the accumulator compounds into the offsets");
        step[0] = 1.0f;
        camera_pointer_pan_update(&v, step, 1.0f);
        CHECK(v.pan_accum == 5.0f && v.offset[0] == 11.0f,
              "the cap clamps the accumulator (and stays clamped)");

        /* Zero-length vector: the corpus's unguarded leaf (1/0 * 0), so
         * the components go NaN and the offset keeps its value. */
        float flat[3] = { 0.0f, 0.0f, 0.0f };
        float offset_save = v.offset[0];
        camera_pointer_pan_update(&v, flat, 1.0f);
        CHECK(flat[0] != flat[0] && flat[1] != flat[1]
                  && flat[2] != flat[2],
              "the zero vector goes NaN (corpus's unguarded leaf)");
        CHECK(v.offset[0] != v.offset[0],
              "the NaN move poisons the offsets (corpus shape)");
        (void)offset_save;
        v.offset[0] = 0.0f; v.offset[1] = 0.0f; v.offset[2] = 0.0f;

        camera_view_context_set(0.0f, save_follow, 0.0f, save_edge);
    }

    /* The camera-controller bounds band + cursor pan reset (2026-10-24). */
    {
        char cam[0xc0];
        memset(cam, 0, sizeof(cam));
        camera_tilt_bounds_set(cam, 1.0f, 2.0f);
        CHECK(*(const uint8_t*)(cam + 0xb8) == 1u, "the tilt gate bit rises");
        float lo = 0.0f, hi = 0.0f;
        __builtin_memcpy(&lo, cam + 0x7c, 4);
        __builtin_memcpy(&hi, cam + 0x80, 4);
        CHECK(lo == 1.0f && hi == 2.0f, "the tilt bounds land");
        camera_scalar_bounds_set(cam, 3.0f, 4.0f);
        camera_distance_bounds_set(cam, 5.0f, 6.0f);
        CHECK(*(const uint8_t*)(cam + 0xb8) == 7u, "the three gate bits stack");
        __builtin_memcpy(&lo, cam + 0x88, 4);
        __builtin_memcpy(&hi, cam + 0x8c, 4);
        CHECK(lo == 3.0f && hi == 4.0f, "the scalar bounds land");

        camera_distance_set(cam, 5.5f); /* inside the distance bounds */
        float d = 0.0f;
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 5.5f, "an in-range distance lands");
        camera_distance_set(cam, 9.0f); /* out of range: gate holds */
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 5.5f, "the gate withholds an out-of-range value");

        camera_zoom_update(cam, 0.5f); /* 5.5 + 0.5 = 6.0, at the edge */
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 6.0f, "zoom accumulates through the gate");
        camera_zoom_update(cam, 3.0f); /* 6.0 + 3.0 out of range */
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 6.0f, "the zoom keeps the last accepted value");

        /* bit 8 overrides the gate entirely */
        cam[0xb8] |= 8;
        camera_distance_set(cam, 99.0f);
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 99.0f, "the override bit lets anything through");
        cam[0xb8] &= 7;

        camera_tilt_bounds_set(NULL, 1.0f, 2.0f); /* guarded */
        camera_distance_set(NULL, 1.0f); /* guarded */
        camera_zoom_update(NULL, 1.0f); /* guarded */

        ViewContext v;
        memset(&v, 0, sizeof(v));
        v.pan_accum = 5.0f;
        cursor_screen_ray_context_reset(&v);
        CHECK(v.pan_accum == 0.0f, "the cursor ray reset drops the pan");
        cursor_screen_ray_context_reset(NULL); /* guarded */
    }

    /* CurrentLevelTerrainGridPick (2026-10-24). */
    {
        EngineContext ec = {};
        int gx = -1, gy = -1;
        current_level_terrain_grid_pick(&ec, 3.7f, 4.9f, &gx, &gy);
        CHECK(gx == 3 && gy == 4, "the pick truncates to cells");
        current_level_terrain_grid_pick(&ec, 1.0f, 2.0f, NULL, NULL);
        CHECK(gx == 3 && gy == 4, "null outputs keep the last write");
    }

    /* The adapter + scalar-to-one wrappers (2026-10-24). */
    {
        EngineContext ec = {};
        grid_operation_argument_adapter(1.5f, 2.5f, &ec); /* height discarded */
        grid_operation_argument_adapter(1.0f, 1.0f, NULL); /* guarded */
        set_global_scalar_to_one(); /* the modeled ramp is inert */
        CHECK(1, "the wrappers keep their corpus shapes");
    }

    /* The view math band (2026-10-24). */
    {
        float f = 3.5f;
        float_negate_absolute_value(&f);
        CHECK(f == -3.5f, "the negate flips a positive");
        f = -2.0f;
        float_negate_absolute_value(&f);
        CHECK(f == 2.0f, "the negate flips a negative");
        float_negate_absolute_value(NULL); /* guarded */

        float v2[2] = { 3.0f, 0.0f };
        vector2_set_length(v2, 2.0f);
        CHECK(v2[0] == 2.0f && v2[1] == 0.0f, "the length lands on x");
        float v3[2] = { 0.0f, 4.0f };
        vector2_set_length(v3, 1.0f);
        CHECK(v3[0] == 0.0f && v3[1] == 1.0f, "the unit y stays put");
        vector2_set_length(v3, 0.0f); /* guarded divide */
        CHECK(v3[1] == 1.0f, "a zero length keeps the vector");

        char cam[0xc0];
        memset(cam, 0, sizeof(cam));
        camera_distance_bounds_set(cam, 0.0f, 100.0f);
        camera_distance_set(cam, 10.0f);
        float d = 0.0f;
        camera_zoom_step_adjust(cam, 1, 1.0f);
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 13.0f, "the +3x step zooms out");
        camera_zoom_step_adjust(cam, 0, 1.0f);
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 10.0f, "the -3x step zooms back");
        camera_zoom_step_adjust(cam, 7, 1.0f); /* inert direction */
        __builtin_memcpy(&d, cam + 0x90, 4);
        CHECK(d == 10.0f, "other directions stay put");
    }

    /* SelectionRecordPointGet (2026-10-24): the follow-mode readback. */
    {
        ViewContext v = {};
        cam_set_pos(&v.cam_follow, 12.5f, -3.0f, 4.0f);
        v.mode = 1;
        float out[3] = { 0.0f, 0.0f, 0.0f };
        selection_record_point_get(&v, out);
        CHECK(out[0] == 12.5f && out[1] == -3.0f && out[2] == 4.0f,
              "the follow record reads the camera back");
        v.mode = 0;
        out[0] = 9.0f;
        selection_record_point_get(&v, out);
        CHECK(out[0] == 9.0f, "the non-follow gate answers nothing");
        selection_record_point_get(NULL, out); /* guarded */
    }

    /* The selected-point x transform (2026-10-24): mode-selected view. */
    {
        EngineContext ctx = {};
        ViewContext v = {};
        v.mode = 1;
        ctx.context_mode = 1;
        ctx.views[0] = &v;
        selection_record_point_set(&v, 7.5f, -3.5f);
        CHECK(selected_global_point_transform_and_return_x(&ctx) == 7.5f,
              "the in-game follow view answers its x");
        ctx.context_mode = 0; /* editor: the view-2 window is unbound */
        CHECK(selected_global_point_transform_and_return_x(&ctx) == 0.0f,
              "the unbound editor view answers zero");
        selected_global_point_transform_and_return_x(NULL); /* guarded */
    }

    /* RendererNodeRelativePositionSet (2026-10-24). */
    {
        CameraHandle cam;
        memset(&cam, 0, sizeof cam);
        cam.pos[0] = 1.0f;
        cam.pos[1] = 2.0f;
        char node[0x30];
        memset(node, 0, sizeof node);
        void* h = &cam;
        __builtin_memcpy(node + 0x28, &h, sizeof h);
        renderer_node_relative_position_set(node, 5.0f, 3.0f);
        CHECK(cam.inuse == 1 && cam.pos[0] == 6.0f && cam.pos[1] == 5.0f
                  && cam.pos[2] == 0.0f,
              "the node handle rides the offset with a zero z");
        renderer_node_relative_position_set(NULL, 1.0f, 1.0f); /* guarded */
        memset(node, 0, sizeof node); /* clear handle */
        renderer_node_relative_position_set(node, 1.0f, 1.0f); /* cam guard */
    }

    /* Matrix/ray math leaves (2026-10-24) — FUN_00479ab0 / FUN_004798e0 /
     * FUN_00479e70 / FUN_00479e40 / FUN_00479eb0 / FUN_00479d50 /
     * FUN_00479da0 / FUN_00479df0 / FUN_0040bd60. */
    {
        float out[3];
        const float origin[3] = { 1.0f, 2.0f, 3.0f };
        const float dir[3] = { 2.0f, 0.0f, -1.0f };
        point_along_ray(out, origin, dir, 1.5f);
        CHECK(out[0] == 4.0f && out[1] == 2.0f && out[2] == 1.5f,
              "point_along_ray scales and offsets per component");
    }
    {
        /* Ray (0,0,0)→(1,1,0) versus the plane through (2,2,2) with the
         * (1,1,1) normal: both direction vectors ride their normalizers
         * into the plane solve, answering the exact hit (3,3,0). */
        float out[3];
        const float p[3] = { 0.0f, 0.0f, 0.0f };
        const float da[3] = { 1.0f, 1.0f, 0.0f };
        const float q[3] = { 2.0f, 2.0f, 2.0f };
        const float db[3] = { 1.0f, 1.0f, 1.0f };
        int hit = view_ray_intersection_test(out, p, da, q, db);
        CHECK(hit == 1 && fabsf(out[0] - 3.0f) < 1e-5f
                  && fabsf(out[1] - 3.0f) < 1e-5f && fabsf(out[2]) < 1e-5f,
              "view_ray_intersection_test solves the scaled ray/plane");
        const float pa[3] = { 1.0f, 0.0f, 0.0f };
        const float pb[3] = { 0.0f, 1.0f, 0.0f };
        CHECK(view_ray_intersection_test(out, p, pa, q, pb) == 0,
              "orthogonal ray and plane normal answer zero");
    }
    {
        float m[16];
        float got[16];
        const float tr[3] = { 5.0f, 6.0f, 7.0f };
        matrix4_set_identity(m);
        CHECK(m[0] == 1.0f && m[5] == 1.0f && m[10] == 1.0f && m[15] == 1.0f
                  && m[1] == 0.0f && m[12] == 0.0f,
              "matrix4_set_identity raises the diagonal only");
        matrix4_identity_translation_set(m, tr);
        CHECK(m[12] == 5.0f && m[13] == 6.0f && m[14] == 7.0f
                  && m[15] == 1.0f && m[0] == 1.0f && m[1] == 0.0f,
              "the translation triad lands in row 3");
        matrix4_copy(got, m);
        CHECK(memcmp(got, m, sizeof got) == 0, "matrix4_copy moves 16 cells");
    }
    {
        float m[16];
        matrix4_set_rotation_x(m, 0.0f);
        CHECK(m[0] == 1.0f && m[5] == 1.0f && m[6] == 0.0f && m[9] == 0.0f
                  && m[10] == 1.0f && m[2] == 0.0f,
              "matrix4_set_rotation_x at zero is the identity");
        matrix4_set_rotation_x(m, 3.1415926535f / 2.0f);
        CHECK(fabsf(m[5]) < 1e-6f && fabsf(m[6] + 1.0f) < 1e-6f
                  && fabsf(m[9] - 1.0f) < 1e-6f && fabsf(m[10]) < 1e-6f
                  && m[0] == 1.0f && m[2] == 0.0f,
              "the rows 1-2 block at half-pi is (c,-s / s,c)");
        matrix4_set_rotation_y(m, 3.1415926535f / 2.0f);
        CHECK(fabsf(m[0]) < 1e-6f && fabsf(m[2] - 1.0f) < 1e-6f
                  && fabsf(m[8] + 1.0f) < 1e-6f && fabsf(m[10]) < 1e-6f,
              "the rows 0/2 block at half-pi is (c,+s / -s,c)");
        matrix4_set_rotation_z(m, 3.1415926535f / 2.0f);
        CHECK(fabsf(m[0]) < 1e-6f && fabsf(m[1] + 1.0f) < 1e-6f
                  && fabsf(m[4] - 1.0f) < 1e-6f && fabsf(m[5]) < 1e-6f
                  && m[10] == 1.0f,
              "the rows 0-1 block at half-pi is (c,-s / s,c)");
    }
    {
        float m[16];
        perspective_projection_matrix_set(m, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
        CHECK(m[0] == 4.0f && m[5] == 1.5f && m[1] == 5.0f
                  && m[10] == 1.0f && m[15] == 1.0f && m[7] == 0.0f
                  && m[12] == 0.0f,
              "the perspective quotients land with the zeros");
    }

    /* RandomReachableGridPointSelect (2026-10-24): the ray-descending
     * grid pick, faithful plane geometry and plane-z quirk. */
    {
        EngineContext ec = {};
        float hit[3] = { 1.0f, 1.0f, 1.0f };
        const float down[3] = { 0.0f, 0.0f, 1.0f };
        const float p[3] = { 0.0f, 0.0f, -100.0f };
        int gx = -1, gy = -1;
        random_reachable_grid_point_select(&ec, p, down, hit, &gx, &gy, 1);
        CHECK(hit[0] == 0.0f && hit[1] == 0.0f && hit[2] == -50.0f
                  && gx == 0 && gy == 0,
              "the first descent lands on the z=-50 plane");
        random_reachable_grid_point_select(&ec, p, down, hit, &gx, &gy, 2);
        CHECK(hit[2] == 0.0f && gx == 0 && gy == 0,
              "the second iteration drops the plane onto the height");
        random_reachable_grid_point_select(&ec, p, down, hit, &gx, &gy, 0);
        CHECK(gx == 0 && gy == 0, "rounds of zero grid-picks the buffer");
    }
    {
        /* Bound voxel: the plane descends onto the surface byte. */
        static uint8_t lvl[0x284];
        static uint8_t vox[4 * 0x48];
        memset(lvl, 0, sizeof lvl);
        memset(vox, 0, sizeof vox);
        const int w = 2, h = 2;
        memcpy(lvl + 0x14, &w, sizeof w);
        memcpy(lvl + 0x18, &h, sizeof h);
        const void* vp = vox;
        memcpy(lvl + 0xb8, &vp, sizeof vp);
        vox[3 * 0x48 + 0x04] = 7; /* cell (1,1) surface byte */
        EngineContext ctx = {};
        ctx.level_block = lvl;
        float hit[3];
        const float down[3] = { 0.0f, 0.0f, 1.0f };
        const float p[3] = { 1.0f, 1.0f, -10.0f };
        int gx = -1, gy = -1;
        random_reachable_grid_point_select(&ctx, p, down, hit, &gx, &gy, 1);
        CHECK(hit[0] == 1.0f && hit[1] == 1.0f && hit[2] == -50.0f
                  && gx == 1 && gy == 1,
              "the descent hits the z=-50 plane off the surface byte");
        random_reachable_grid_point_select(&ctx, p, down, hit, &gx, &gy, 2);
        CHECK(hit[2] == 7.0f && gx == 1 && gy == 1,
              "the plane rides the voxel height for the second round");
    }

    if (s_failures == 0) {
        printf("view_test: PASS\n");
        return 0;
    }
    printf("view_test: %d failure(s)\n", s_failures);
    return 1;
}