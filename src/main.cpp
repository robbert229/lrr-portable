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
#include "gods98/frameloop.hpp"
#include "game/game.hpp"

int LegorrrMain(void)
{
    /* GUI-subsystem / SDL builds have no console; write an observable log. */
    FILE* log = fopen("legoRR-stub.log", "w");
    if (log == nullptr) {
        return 1;
    }

    fprintf(log, "lrr-portable reconstruction scaffold\n");
    fprintf(log, "  gods98 engine: %s\n", gods98_engine_name());
    fprintf(log, "  game layer:   %s\n", game_layer_name());
    fclose(log);

    /* The visible-milestone frame loop (window + DDraw + real BMPs). Runs
     * wherever the shipped wads are reachable (cwd or $LEGORRR_WAD_DIR). */
    return LegorrrRun();
}