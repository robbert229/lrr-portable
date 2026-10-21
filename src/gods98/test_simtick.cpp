// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_simtick — validation for src/gods98/simtick.cpp (tranche 6 tickers).
 *
 * Covers the mute timer (FUN_0041ba10), the effect-table dispatch
 * (FUN_0040aa10), the bobbin list flag machine (FUN_004091e0), and the
 * grid marker sweep (FUN_0044eff0).
 */
#include <stdio.h>
#include <string.h>

#include "simtick.hpp"

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

    if (s_failures == 0) {
        printf("simtick_test: PASS\n");
        return 0;
    }
    printf("simtick_test: %d failure(s)\n", s_failures);
    return 1;
}