// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "game.hpp"
#include "savegame.hpp"

const char* game_layer_name(void)
{
    return "game layer (level/mission/save)";
}

/* Root identity string used in the reconstruction log. */
const char* game_identity(void)
{
    return "lrr-portable game layer — finding 07/08/22";
}

void game_context_init(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    save_init(ctx);
}
