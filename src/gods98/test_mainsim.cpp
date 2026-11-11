// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_mainsim — validation for src/gods98/mainsim.cpp (MainSimStep @
 * 0x4250c0, finding 04).
 *
 * Feeds synthetic key events through the input tables and checks the
 * debug/cheat toggles (state flag flips), the edge outputs o1/o2/o3, and the
 * quit gate.
 */
#include <stdio.h>
#include <string.h>

#include "input.hpp"
#include "mainsim.hpp"
#include "view.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* capture cur edges via a fresh InputState; hold=1 for a held key. */
static void press(InputState* in, int key)
{
    input_reset(in);
    PlatformEvent e = { PLATFORM_EVENT_KEYDOWN, key, 0, 0, 0 };
    input_capture_events(in, &e, 1);
}

int main(void)
{
    EngineState st = {};
    InputState  in = {};
    input_reset(&in);
    EngineContext ctx = {};
    ctx.state_block = &st;
    ctx.input = &in;

    uint32_t o1 = 0, o2 = 0, o3 = 0;

    /* debug toggles are gated by the debug word 0x10. */
    press(&in, MNSIM_K_FRAME_RATE);
    CHECK(main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3) == 1,
          "frame: runs, no quit");
    CHECK((ctx.state & 0x40u) == 0, "gate: toggle disabled without debug 0x10");

    ctx.debug |= 0x10u;
    press(&in, MNSIM_K_FRAME_RATE);
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK((ctx.state & 0x40u) != 0, "frame: toggle sets 0x40");
    press(&in, MNSIM_K_FRAME_RATE);
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK((ctx.state & 0x40u) == 0, "frame: toggle clears 0x40");

    press(&in, MNSIM_K_MEMORY);
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK((ctx.state & 0x80u) != 0, "memory: sets 0x80");

    press(&in, MNSIM_K_SOUNDFX);
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK((ctx.state & 0x10u) != 0, "soundfx: sets 0x10");

    /* edge outputs. */
    press(&in, '1');
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK(o1 == 1 && o2 == 0 && o3 == 0, "outs: o1 on 1");
    press(&in, '2');
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK(o1 == 0 && o2 == 1 && o3 == 0, "outs: o2 on 2");
    press(&in, '3');
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK(o1 == 0 && o2 == 0 && o3 == 1, "outs: o3 on 3");

    /* air cheat (held 'a') drains the air counter. */
    st.air_beat = 10;
    input_reset(&in);
    in.cur[MNSIM_K_AIRCHEAT] = 1; /* held */
    main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3);
    CHECK(st.air_beat < 10, "aircheat: drains air");

    /* held zoom keys (Finding 101): '-' adds and '=' subtracts
     * 3 * scale * 25 to the controller distance, in-game mode only. */
    {
        float d = 0.0f;
        camera_distance_bounds_set(ctx.camera_controller, 100.0f, 300.0f);
        camera_distance_set(ctx.camera_controller, 200.0f);
        input_reset(&in);
        in.cur[MNSIM_K_ZOOM_OUT] = 1;
        main_sim_step(&ctx, 0.04f, &o1, &o2, &o3);
        memcpy(&d, ctx.camera_controller + 0x90, sizeof(d));
        CHECK(d == 200.0f, "zoom: inert outside in-game context mode");

        ctx.context_mode = 1;
        main_sim_step(&ctx, 0.04f, &o1, &o2, &o3);
        memcpy(&d, ctx.camera_controller + 0x90, sizeof(d));
        CHECK(d > 202.9f && d < 203.1f, "zoom: '-' zooms out by 3/frame");

        input_reset(&in);
        in.cur[MNSIM_K_ZOOM_IN] = 1;
        main_sim_step(&ctx, 0.08f, &o1, &o2, &o3);
        memcpy(&d, ctx.camera_controller + 0x90, sizeof(d));
        CHECK(d > 196.9f && d < 197.1f, "zoom: '=' zooms in by 3/frame");

        ctx.state |= 0x100000u; /* frozen time gates the camera keys */
        main_sim_step(&ctx, 0.04f, &o1, &o2, &o3);
        memcpy(&d, ctx.camera_controller + 0x90, sizeof(d));
        CHECK(d > 196.9f && d < 197.1f, "zoom: gated while time frozen");
        ctx.state &= ~0x100000u;

        camera_distance_set(ctx.camera_controller, 101.0f);
        main_sim_step(&ctx, 0.08f, &o1, &o2, &o3);
        memcpy(&d, ctx.camera_controller + 0x90, sizeof(d));
        CHECK(d == 101.0f, "zoom: step past MinDist is refused");
        ctx.context_mode = 0;
    }

    /* quit gate. */
    press(&in, MNSIM_K_QUIT);
    CHECK(main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3) == 0,
          "quit: q returns 0");
    press(&in, '9');
    CHECK(main_sim_step(&ctx, 1.0f, 1.0f / 30.0f, &o1, &o2, &o3) == 1,
          "quit: ordinary key keeps running");

    if (s_failures == 0) {
        printf("mainsim_test: PASS\n");
        return 0;
    }
    printf("mainsim_test: %d failure(s)\n", s_failures);
    return 1;
}