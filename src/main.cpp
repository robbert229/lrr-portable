// SPDX-License-Identifier: LGPL-3.0-or-later
/* Shared game shell — backend-neutral driver for the reconstruction.
 *
 * Proven behavior: under Wine/Proton (winapi) or natively (native), writes
 * legoRR-stub.log alongside the running directory and exits 0.
 *
 * As of the engine scaffold (2026-10-20) this drives the reconstruction's
 * frame tick directly — EngineInit/EngineTick/EngineShutdown in the same
 * shape the game's real WinMain frame loop uses (findings 02/03/20): a fixed
 * dt, the triplet sitting behind EngineContext. The platform entry points
 * (src/platform entry.cpp files) call it.
 */
#include <cstdio>

#include "gods98/gods98.hpp"
#include "gods98/engine.hpp"
#include "game/game.hpp"

int LegorrrMain(void)
{
    /* GUI-subsystem / SDL builds have no console; write an observable log. */
    FILE* log = fopen("legoRR-stub.log", "w");
    if (log == nullptr) {
        return 1;
    }

    /* Engine state + context, initialized to the corpus's startup posture:
     * normal app mode, in-game context, empty feature/state/debug words. */
    EngineState state = {};
    EngineContext ctx = {
        ENGINE_APP_NORMAL, /* app_mode    DAT_00505f88 */
        0,                 /* feature     DAT_00505f84 */
        0,                 /* state       DAT_00556ecc */
        0,                 /* debug       DAT_00556ed0 */
        1,                 /* context     1 = in-game  (DAT_00556ec8) */
        &state,            /* state_block DAT_005560d4 */
        0.0f,              /* exit_timer  DAT_00556efc */
        0,                 /* tickers_ran */
    };

    fprintf(log, "lrr-portable reconstruction scaffold\n");
    fprintf(log, "  gods98 engine: %s\n", gods98_engine_name());
    fprintf(log, "  game layer:   %s\n", game_layer_name());

    /* Run the frame tick a few times, the way the game's frame loop does:
     * dt in seconds, fixed 1/30 for the smoke run. */
    const float dt = 1.0f / 30.0f;
    for (int i = 0; i < 10; ++i) {
        int rc = EngineTick(&ctx, dt);
        fprintf(log, "  tick %2d: rc=%d dispatched=%d state=0x%08x air=%u exit_timer=%f\n",
                i, rc, ctx.tickers_ran, ctx.state, state.air_beat, ctx.exit_timer);
        if (rc == 0) {
            break;
        }
    }

    EngineShutdown(&ctx);
    fclose(log);
    return 0;
}