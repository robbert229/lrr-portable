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
    /* Corpus activity-list capacity comes from the stats block (+0xcc,
     * FUN_0046a060 — instance fill TODO(corpus)); the port defaults it to
     * the corpus inline span so list installs behave (see game.hpp). */
    ctx->stats_activity_cap = AI_ACTIVITY_SLOTS;
    save_init(ctx);
}
