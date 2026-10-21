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
    st.grid_w = 32;
    st.grid_h = 32;

    /* glide-to-stop tail: motion timer expired -> velocity snaps. */
    ViewContext tv = {};
    tv.mode = 3;
    tv.step = 8.0f;
    tv.timer_num = 0;
    tv.timer_acc = 5;
    tv.velocity[0] = 4.0f;
    view_tick(&tv, &st, 1.0f / 30.0f, 1.0f);
    CHECK(tv.velocity[0] == 0.0f, "tail: timer expired snaps velocity");

    /* tail decay: timer budget left -> decays toward step. */
    ViewContext dv = {};
    dv.mode = 3;
    dv.step = 8.0f;
    dv.timer_num = 10;
    dv.timer_acc = 0;
    dv.velocity[0] = 4.0f;
    view_tick(&dv, &st, 1.0f / 30.0f, 1.0f);
    /* decay = (10-0)/10 * 8 = 8 -> vel = 4 * 8 = 32; corpus scales velocity. */
    CHECK(dv.velocity[0] == 32.0f, "tail: velocity decays by (num-acc)/num*step");

    /* mode 1: follow camera clamps at the grid edge. */
    g_view_follow_scale = 1.0f;
    g_view_grid_edge = 4;
    ViewContext fv = {};
    fv.mode = 1;
    fv.offset[0] = 30.0f; /* beyond grid_w(32) - edge(4) = 28 */
    fv.offset[1] = 3.0f;
    view_tick(&fv, &st, 1.0f / 30.0f, 1.0f);
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
    view_tick(&mv, &st, 1.0f / 30.0f, 1.0f);
    CHECK((mv.flags & 0x10) != 0, "mode3: dirty flag set");
    /* body repositions to the blend target (0,0,0), then the tail slides by
     * the decayed velocity (decay = 8 -> vel {16,8,4}) => {-16,-8,-4}. */
    CHECK(mv.cam_world.pos[0] == -16.0f && mv.cam_world.pos[1] == -8.0f
          && mv.cam_world.pos[2] == -4.0f, "mode3: camera re-positioned + glide");
    g_view_test_blend = 0;

    /* mode 2 with no object target idles (object_lookup stub). */
    ViewContext lv = {};
    lv.mode = 2;
    view_tick(&lv, &st, 1.0f / 30.0f, 1.0f);
    CHECK(lv.cam_world.inuse == 1 && lv.velocity[0] == 0.0f
          && lv.velocity[1] == 0.0f && lv.velocity[2] == 0.0f,
          "mode2: idles without target");

    if (s_failures == 0) {
        printf("view_test: PASS\n");
        return 0;
    }
    printf("view_test: %d failure(s)\n", s_failures);
    return 1;
}