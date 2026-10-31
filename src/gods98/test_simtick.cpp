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
#include "message.hpp"

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

static int s_keep_n = 0;
static int keep_poke(int /*entry*/, int /*a*/, int /*b*/)
{
    ++s_keep_n;
    return 0;
}

static int s_tree_n = 0;
static int tree_poke(int /*idx*/, int /*bits*/)
{
    ++s_tree_n;
    return 0;
}

static int s_pick_n = 0;
static int pick_poke(int /*x*/, int /*y*/)
{
    ++s_pick_n;
    return 0;
}

static int s_spawn_n = 0;
static int spawn_poke(int /*slot*/)
{
    ++s_spawn_n;
    return 0;
}

static int s_scene_n = 0;
static void scene_bcast(int /*word*/)
{
    ++s_scene_n;
}

static int minion_dist(int /*h*/, float /*s*/) { return 7; }
static int s_minion_acts = 0, s_minion_fails = 0;
static void minion_act(int /*h*/) { ++s_minion_acts; }
static void minion_fail(void* /*node*/) { ++s_minion_fails; }

int main(void)
{
    EngineContext ctx = {};
    EngineState st = {};
    ctx.state_block = &st;
    InputState in = {};
    input_reset(&in);
    ctx.input = &in;

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

    /* ---- tranche 8 ------------------------------------------------- */
    /* cooldown tick (FUN_0041a2c0) decs the message bus cooldown. */
    message_init_defaults();
    message_set_flags(0x4u);
    message_config_set(3, "Sounds\\x.wav", 4.0f, 0);
    simtick_cooldown_tick(&ctx, 1.5f);
    CHECK(message_cooldown(3) > 2.4f && message_cooldown(3) < 2.6f,
          "cooldown: decremented by dt");
    simtick_cooldown_tick(&ctx, 10.0f);
    CHECK(message_cooldown(3) < 0.0f, "cooldown: overshoot goes negative");
    message_config_set(3, "Sounds\\x.wav", 0.0f, 0);
    simtick_cooldown_tick(&ctx, 1.0f);
    CHECK(message_cooldown(3) == 0.0f, "cooldown: zero stays zero");

    /* sound keep-alive (FUN_0047aba0): gate + hook. */
    s_keep_n = 0;
    simtick_set_keepalive_hook(keep_poke);
    simtick_set_sound_guards(1, 1);
    simtick_sound_keepalive(&ctx);
    CHECK(s_keep_n == 1, "keepalive: poked when sound on + active");
    simtick_set_sound_guards(0, 1);
    simtick_sound_keepalive(&ctx);
    CHECK(s_keep_n == 1, "keepalive: gated off");

    /* editor steps (FUN_0044cfa0). */
    simtick_set_step_hook(NULL);
    simtick_set_editor_state(0, 1);
    simtick_editor_steps(&ctx);
    CHECK(simtick_editor_steps_run() == 5, "editor: five steps");
    simtick_set_editor_state(0x100u, 1);
    simtick_editor_steps(&ctx);
    CHECK(simtick_editor_steps_run() == 0, "editor: gated by bit 0x100");
    simtick_set_editor_state(0, 0);
    simtick_editor_steps(&ctx);
    CHECK(simtick_editor_steps_run() == 0, "editor: gated by inactive");

    /* bit-field tree (FUN_0049cf90). */
    simtick_set_tree_hook(tree_poke);
    s_tree_n = 0;
    int treebase[16] = {};
    treebase[8 + 1] = 2; /* low short of entry 0's +1 int = 2 */
    simtick_bitfield_tree(treebase, 1);
    CHECK(s_tree_n == 1, "tree: code matched (low short 2)");
    simtick_set_tree_hook(NULL);
    s_tree_n = 0;
    simtick_bitfield_tree(treebase, 1);
    CHECK(s_tree_n == 0, "tree: counts only via hook");

    /* ---- tranche 10 ------------------------------------------------- */
    /* FUN_00425d90 mouse world pick: gate bits + distance threshold. */
    ctx.state = 2u | 0x1000u;
    simtick_set_pick_hook(pick_poke);
    s_pick_n = 0;
    ctx.input->mouse_x = 60;
    ctx.input->mouse_y = 40;
    simtick_mouse_world(&ctx, 0.1f);
    CHECK(simtick_mouse_picks() == 1 && s_pick_n == 1,
          "mouse: pick fired past the distance threshold");
    ctx.state = 0x100u;
    simtick_set_pick_hook(NULL);
    simtick_mouse_world(&ctx, 0.1f); /* gated by 0x100 */
    CHECK(simtick_mouse_picks() == 1, "mouse: gated by state 0x100");

    /* FUN_004402220 frame counter bumps monotonically. */
    {
        uint32_t f0 = simtick_frame();
        simtick_frame_counter(&ctx);
        CHECK(simtick_frame() == f0 + 1, "frame: counter incremented");
    }

    /* FUN_0040eab0 ambient spawn: fires after the accumulator threshold. */
    simtick_set_spawn_hook(spawn_poke);
    s_spawn_n = 0;
    simtick_ambient_spawn(&ctx, 0.6f);
    simtick_ambient_spawn(&ctx, 0.6f);
    CHECK(simtick_spawn_calls() == 1 && s_spawn_n == 1,
          "spawn: fired after ~1 s of accumulation");
    simtick_set_spawn_hook(NULL);

    /* FUN_00449c00 render step increments the ordered-step census. */
    {
        int r0 = simtick_render_steps();
        simtick_render_step(&ctx);
        CHECK(simtick_render_steps() == r0 + 1, "renderstep: one");

        /* FUN_00426520 vu meter clears the debug bit 0x400 each frame. */
        ctx.debug |= 0x400u;
        ctx.input->cur[0x2d] = 0;
        simtick_vu_meter(&ctx, 0.1f);
        CHECK((ctx.debug & 0x400u) == 0, "vumeter: debug bit cleared");
        ctx.debug = 0;
    }

    /* ---- tranche 9 ------------------------------------------------- */
    /* FUN_004665c0 single post-step. */
    int steps_before = simtick_post_steps_count();
    simtick_post_step(&ctx);
    CHECK(simtick_post_steps_count() == steps_before + 1, "poststep: one");

    /* FUN_004313b0 minion states: flag transitions on the distance hook. */
    {
        static uintptr_t m1[7] = { 7, 0, 0, 0, 0, 0, 0 }; /* fresh -> act */
        static uintptr_t m2[7] = { 8, 0, 0, 1u | 2u, 0, 0, 0 }; /* done -> fail */
        m1[5] = (uintptr_t)m2;
        st.minion_list = m1;
        simtick_set_minion_hooks(minion_dist, minion_act, minion_fail);
        simtick_minion_states(&ctx, 0.1f, 1.0f);
        CHECK(simtick_minion_actions() == 1, "minion: fresh node acts");
        CHECK((m1[3] & 1u) != 0, "minion: act flag set");
        CHECK(simtick_minion_failures() == 1, "minion: done node fails");
        st.minion_list = NULL;
        simtick_set_minion_hooks(NULL, NULL, NULL);
    }

    /* FUN_00424600 grid cell pass clears the cell flags. */
    {
        uint8_t g[2 * 0x48];
        memset(g, 0, sizeof(g));
        st.grid_w = 2;
        st.grid_h = 1;
        st.grid_cells = g;
        *(uint32_t*)(g + 0x48 + 8) = 0x800u | 0x2000000u; /* 0x1000 clear -> dispatches */
        int xs[1] = { 1 }, ys[1] = { 0 };
        simtick_cell_seed(1, xs, ys);
        simtick_grid_cell_pass(&ctx);
        CHECK((*(uint32_t*)(g + 0x48 + 8) & (0x800u | 0x1000u | 0x2000000u))
              == 0, "gridcell: flags cleared");
        CHECK(simtick_cell_dispatches() == 1, "gridcell: dispatched");
        st.grid_cells = NULL;
    }

    /* FUN_0043b270 scene pass: the bit-8 broadcast fires, and the expired
     * 125 s re-arm broadcast also fires in the same pass (corpus order). */
    simtick_set_scene_broadcast(scene_bcast);
    simtick_set_scene_word(0x8u);
    simtick_scene_pass(&ctx, 1.0f);
    CHECK(simtick_scene_broadcasts() == 2, "scene: bit-8 + timer-reset fires");
    CHECK((simtick_scene_word() & 8u) == 0, "scene: bit 8 cleared");
    simtick_set_scene_broadcast(NULL);

    /* FUN_0046f550 sfx bank: negative timer releases the entry. */
    {
        struct { uint32_t i0, i1, i2; float t; } s[2] = {
            { 1, 2, 3, -1.0f }, { 4, 5, 6, 5.0f },
        };
        const void* entries[2] = { &s[0], &s[1] };
        simtick_sfx_seed(2, entries);
        simtick_sfx_bank(&ctx, 1.0f);
        CHECK(simtick_sfx_releases() == 1, "sfx: negative timer released");
        CHECK(s[0].i0 == 0 && s[1].t == 4.0f, "sfx: cleared + decremented");
        simtick_sfx_seed(0, NULL);
    }

    /* SimulationGlobalWordSet (2026-10-24): the faithful blind store. */
    simulation_global_word_set(0xABCDu);
    CHECK(simtick_mute_word() == 0xABCDu, "simtick: the word store lands");
    simulation_global_word_set(0u);

    /* The blocked-route and timer-word bit folds (2026-10-24). */
    {
        simtick_set_editor_state(0x1600u, 1);
        simtick_route_position_blocked_state_set(1);
        CHECK((simtick_route_position_state_peek() & 0x600u) == 0x200u,
              "blocked raises bit 9 and drops bit 10");
        simtick_set_editor_state(0x1600u, 1);
        simtick_route_position_blocked_state_set(0);
        CHECK(simtick_route_position_state_peek() == 0x400u,
              "the unblocked mask drops bits 9 and 12, keeping bit 10");

        simtick_manager_set_timer_word(0x1012u);
        simtick_global_ui_flags_clear_bits12();
        CHECK(simtick_global_ui_flags_word() == 0x1010u,
              "the clear keeps every other bit");
    }

    /* The scene-transform copy + motion-cancel bit (2026-10-24). */
    {
        const float cur[3] = { 1.5f, -2.5f, 3.25f };
        simtick_scene_transform_current_set(cur);
        simtick_set_editor_state(0x180u, 0);
        simtick_scene_position_reset_and_effect_motion_cancel();
        const float* cfg = simtick_scene_transform_config_peek();
        CHECK(cfg[0] == 1.5f && cfg[1] == -2.5f && cfg[2] == 3.25f,
              "the config triple copies the current transform");
        CHECK(simtick_route_position_state_peek() == 0x100u,
              "the cancel clears the motion gate bit");
        simtick_scene_transform_current_set(NULL); /* guarded */
    }

    /* The axis-tolerance window (2026-10-25): the per-axis compares about
     * the config triple with the installed tolerance triple. */
    {
        const float cfg[3] = { 10.0f, 20.0f, 30.0f };
        simtick_scene_transform_current_set(cfg);
        simtick_scene_position_reset_and_effect_motion_cancel();
        const float tol[3] = { 0.5f, 1.0f, 2.0f };
        simtick_vector_tolerance_install(tol);
        const float inside[3] = { 10.4f, 21.0f, 28.0f };
        CHECK(simtick_vector_within_axis_tolerance(inside) == 1,
              "the inside vector lands");
        const float outside_x[3] = { 10.6f, 21.0f, 28.0f };
        CHECK(simtick_vector_within_axis_tolerance(outside_x) == 0,
              "the first axis drops out of the window");
        const float outside_y[3] = { 10.0f, 22.1f, 28.0f };
        CHECK(simtick_vector_within_axis_tolerance(outside_y) == 0,
              "the second axis drops out of the window");
        const float outside_z[3] = { 10.0f, 21.0f, 27.0f };
        CHECK(simtick_vector_within_axis_tolerance(outside_z) == 0,
              "the third axis drops out of the window");
        CHECK(simtick_vector_within_axis_tolerance(NULL) == 0,
              "the null vector guards (dev)");
        const float zero[3] = { 0.0f, 0.0f, 0.0f };
        simtick_vector_tolerance_install(zero);
        CHECK(simtick_vector_within_axis_tolerance(cfg) == 1,
              "the zero tolerance rides exact matches");
        simtick_vector_tolerance_install(NULL); /* guarded: stays installed */
        CHECK(simtick_vector_within_axis_tolerance(cfg) == 1,
              "the guarded install leaves the window");
        simtick_scene_transform_current_set(zero);
        simtick_scene_position_reset_and_effect_motion_cancel();
        simtick_vector_tolerance_install(zero);
    }

    if (s_failures == 0) {
        printf("simtick_test: PASS\n");
        return 0;
    }
    printf("simtick_test: %d failure(s)\n", s_failures);
    return 1;
}
