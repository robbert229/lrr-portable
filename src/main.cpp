// SPDX-License-Identifier: LGPL-3.0-or-later
/* Shared game-shell stub — backend-neutral.
 *
 * Proven behavior: under Wine/Proton (winapi) or natively (native), writes
 * legoRR-stub.log alongside the running directory and exits 0. This is the
 * seam consumer; the platform entry points (src/platform entry.cpp files)
 * call it.
 */
#include <cstdio>

#include "gods98/gods98.hpp"
#include "game/game.hpp"

int LegorrrMain(void)
{
    /* GUI-subsystem / SDL builds have no console; write an observable log. */
    FILE* log = fopen("legoRR-stub.log", "w");
    if (log == nullptr) {
        return 1;
    }

    fprintf(log, "lrr-portable reconstruction stub\n");
    fprintf(log, "  gods98 engine: %s\n", gods98_engine_name());
    fprintf(log, "  game layer:   %s\n", game_layer_name());

    fclose(log);
    return 0;
}