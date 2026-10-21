// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_simtick — validation for src/gods98/simtick.cpp (tranche 6 tickers).
 *
 * Covers the mute timer (FUN_0041ba10), the effect-table dispatch
 * (FUN_0040aa10), the bobbin list flag machine (FUN_004091e0), and the
 * grid marker sweep (FUN_0044eff0).
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "simtick.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int s_stopped_n = 0;
static int stop_reposition(int /*handle*/, int /*x*/, int /*y*/)
{
    ++s_stopped_n;
    return 0;
}

int main(void)
{
    EngineContext ctx = {};
    EngineState st = {};
    ctx.state_block = &st;

    /* mute timer: a word change resets the 25 s timer and re-arms. */
    simtick_set_mute_word(0);
    simtick_set_mute_hook(NULL);
    simtick_mute_timer(&ctx, 1.0f);
    CHECK(simtick_mute_engaged() == 0, "mute: idle, not engaged");
    simtick_set_mute_word(0x1234u);
    simtick_mute_timer(&ctx, 1.0f); /* word changed -> reset */
    CHECK(simtick_mute_engaged() == 0, "mute: reset on word change");
    /* let the timer run out without re-arm -> engages */
    for (int i = 0; i < 26; ++i) {
        simtick_mute_timer(&ctx, 1.0f);
    }
    CHECK(simtick_mute_engaged() != 0, "mute: engages after 25 s");

    /* effect table: only enabled entries fire. */
    SimtickEffect table[3] = {};
    table[0].words[7] = 1; /* enabled +7 byte bit 0 */
    table[1].words[7] = 0; /* disabled */
    table[2].words[7] = 1; /* enabled */
    simtick_effect_table(table, 3);
    simtick_table_dispatch(&ctx, 0.1f, 1.0f);
    CHECK(simtick_effect_last_fired() == 2, "table: two live entries fired");

    /* bobbins: an armed node advances through the spawn path; delayed+dead
     * flags route the die/clear paths. */
    SimtickBobbin n1 = {};
    SimtickBobbin n2 = {};
    n1.flags = 1;      /* armed */
    n1.next = &n2;
    n2.flags = 6;      /* delayed(2) + dead(4) */
    simtick_set_bobbin_head(&n1);
    int before = simtick_bobbin_advanced();
    simtick_bobbins(&ctx);
    CHECK(simtick_bobbin_advanced() - before == 2, "bobbin: both nodes walked");

    /* marker sweep: cells with +0x19 bit 4 are cleared via the callback. */
    uint8_t grid[2 * 0x28];
    memset(grid, 0, sizeof(grid));
    grid[0x19] = 4;          /* cell 0 marker set */
    grid[0x28 + 0x19] = 0;   /* cell 1 clear */
    st.sim_w = 2;
    st.sim_h = 1;
    st.sim_grid = grid;
    st.sim_marker_flags = 2; /* bit 2 enables the sweep */
    CHECK(simtick_marker_count() == 0, "sweep: idle before");
    simtick_marker_sweep(&ctx, 0.1f, 1.0f);
    CHECK(simtick_marker_count() == 1, "sweep: one marker fired");
    CHECK((grid[0x19] & 4u) == 0, "sweep: marker bit cleared");
    CHECK(st.sim_marker_flags == 0, "sweep: flag byte reset");
    simtick_marker_sweep(&ctx, 0.1f, 1.0f); /* disabled -> no re-run */
    CHECK(simtick_marker_count() == 1, "sweep: no re-run when disabled");

    /* ---- ManagerTick (0x45a730) ------------------------------------- */

    /* EMA converges toward the target (stable defaults). */
    SimtickManagerSlot mg[12] = {};
    memset(mg, 0, sizeof(mg));
    mg[0].mode = 0;
    mg[0].handle = 7;
    mg[0].target_a[0] = 100.0f;
    mg[0].target_a[1] = 50.0f;
    simtick_manager_set_slots(mg, 12);
    for (int i = 0; i < 60; ++i) {
        simtick_manager_tick(0, 0.1f, 1);
    }
    CHECK(fabsf(mg[0].smoothed[0] - 100.0f) < 1.0f
          && fabsf(mg[0].smoothed[1] - 50.0f) < 1.0f,
          "manager: smoothing converges");
    CHECK(simtick_manager_repositions() == 60, "manager: mode-0 repositioned");

    /* within the snap threshold the target is taken verbatim. */
    memset(mg, 0, sizeof(mg));
    simtick_manager_set_slots(mg, 12);
    mg[0].mode = 0;
    mg[0].smoothed[0] = 99.6f;
    mg[0].target_a[0] = 100.0f;
    simtick_manager_tick(0, 0.1f, 1);
    CHECK(mg[0].smoothed[0] == 100.0f, "manager: snap verbatim");

    /* source-B int pathway when flag bit 4. */
    memset(mg, 0, sizeof(mg));
    simtick_manager_set_slots(mg, 12);
    mg[0].mode = 1;
    mg[0].flags = 4u;
    mg[0].target_b[0] = 33;
    mg[0].target_b[1] = 44;
    simtick_manager_set_metrics(200, 100);
    simtick_manager_tick(0, 0.1f, 1);
    CHECK(simtick_manager_attaches() == 1, "manager: mode-1 attached");
    CHECK(mg[0].smoothed[0] > 2.0f && mg[0].smoothed[0] < 4.0f,
          "manager: source-B int target smoothed");

    /* index 9 stops all four timer bits (via the reposition hook). */
    memset(mg, 0, sizeof(mg));
    mg[9].mode = 1; /* attach mode so slot 9 itself doesn't call reposition */
    simtick_manager_set_slots(mg, 12);
    simtick_manager_set_reposition_hook(stop_reposition);
    s_stopped_n = 0;
    simtick_manager_set_timer_word(0xf0u); /* all four bits 0x10..0x80 */
    simtick_manager_tick(9, 0.1f, 1);
    CHECK(simtick_manager_timer_bit(0) == 0 && simtick_manager_timer_bit(1) == 0
          && simtick_manager_timer_bit(2) == 0 && simtick_manager_timer_bit(3) == 0,
          "manager: index-9 stops all timers");
    CHECK(s_stopped_n == 4, "manager: four timer stops via the reposition hook");
    simtick_manager_set_reposition_hook(NULL);

    /* stop-detect clears flag bit 3 when the smoothed coords settle near 0. */
    memset(mg, 0, sizeof(mg));
    simtick_manager_set_slots(mg, 12);
    mg[0].flags = 8u;
    mg[0].target_a[0] = mg[0].target_a[1] = 0.0f;
    simtick_manager_tick(0, 0.1f, 1);
    CHECK((mg[0].flags & 8u) == 0, "manager: stop-detect clears bit 3");

    if (s_failures == 0) {
        printf("simtick_test: PASS\n");
        return 0;
    }
    printf("simtick_test: %d failure(s)\n", s_failures);
    return 1;
}