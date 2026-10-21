// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_camera — validation for src/gods98/camera.cpp (finding 03 §3).
 *
 * Tests the self-contained cluster logic against the EngineState model:
 * the move-list timer decrement (FUN_0042c440), the grid expiry scan +
 * message fire (FUN_0042cb70), the state root (FUN_00431560), and the
 * chip-spawn countdown (FUN_004261c0).
 */
#include <stdio.h>
#include <string.h>

#include "camera.hpp"
#include "engine.hpp"

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
    EngineState st = {};
    EngineContext ctx = {};
    ctx.state_block = &st;

    /* state root = state block. */
    CHECK(camera_state_root(&ctx) == &st, "state_root");

    /* move-list timers: two cells, floats at +0xc stride 0x4c. */
    uint8_t cells[2 * 0x4c];
    memset(cells, 0, sizeof(cells));
    float* a = (float*)(cells + 0xc);
    float* b = (float*)(cells + 0x4c + 0xc);
    *a = 2.0f;
    *b = 0.5f;
    st.move_list = cells;
    st.move_list_count = 2;
    camera_grid_timers(&ctx, 0.25f);
    CHECK(*a > 1.74f && *a < 1.76f, "grid_timers: decrement by dt");
    CHECK(*b > 0.24f && *b < 0.26f, "grid_timers: partial decrement");
    camera_grid_timers(&ctx, 1.0f);
    CHECK(!(*b > 0.0f), "grid_timers: clamps at zero");

    /* grid expiry: 2x1 grid, one busy cell, hint + busy for the spent check.
     * busy * scale14 * 0x19 must be < timer to fire the message. */
    uint8_t grid[2 * 0x48];
    memset(grid, 0, sizeof(grid));
    *(uint32_t*)(grid + 0x3c) = 2;                  /* busy */
    *(uint32_t*)(grid + 0x38) = 7;                  /* hint */
    st.grid_cells = grid;
    st.grid_w = 2;
    st.grid_h = 1;
    g_camera_scale_14 = 1.0f;                        /* threshold = 2*1*25 = 50 */
    float* t = (float*)(grid + 0x40);
    *t = 40.0f;
    camera_grid_expiry(&ctx, 0.0f);
    CHECK(*t > 39.9f && *t < 40.1f, "grid_expiry: below threshold untouched");
    *t = 51.0f;
    static int fired = 0;
    (void)fired;
    /* message_dispatch is a stub; just verify the expiry resets the timer. */
    camera_grid_expiry(&ctx, 0.0f);
    CHECK(*t == 0.0f, "grid_expiry: spent cell timer reset");
    camera_grid_expiry(&ctx, 0.0f);
    CHECK(*t == 0.0f, "grid_expiry: reset cell stays quiet (busy still set)");

    /* spawn countdown: skips type 0x14, otherwise counts down and re-arms. */
    st.spawn_type = 0x14;
    g_camera_countdown = -1.0f;
    st.spawn_interval = 3.0f;
    camera_spawn_countdown(&ctx, 0.1f);
    CHECK(g_camera_countdown < 0.0f, "spawn: type 0x14 skipped");
    st.spawn_type = 0x8;
    g_camera_countdown = 0.5f;
    camera_spawn_countdown(&ctx, 0.1f);
    CHECK(g_camera_countdown > 0.39f && g_camera_countdown < 0.41f,
          "spawn: decrement");
    camera_spawn_countdown(&ctx, 0.6f);
    CHECK(g_camera_countdown > 2.99f && g_camera_countdown < 3.01f,
          "spawn: re-arms to interval");

    if (s_failures == 0) {
        printf("camera_test: PASS\n");
        return 0;
    }
    printf("camera_test: %d failure(s)\n", s_failures);
    return 1;
}