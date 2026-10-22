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
    options_defaults(&ctx->options);
    /* Corpus activity-list capacity comes from the stats block (+0xcc,
     * FUN_0046a060 — instance fill TODO(corpus)); the port defaults it to
     * the corpus inline span so list installs behave (see game.hpp). */
    ctx->stats_activity_cap = AI_ACTIVITY_SLOTS;
    /* Screen-space effect lane (finding 121): the lane starts empty and the
     * shake amplitude mirror of DAT_004a48b0 starts at 1.0 — the corpus
     * stores the same 0x3f800000 init word, and the callback shake branch
     * (0x43ce88) resets it there when it decays past zero. */
    ctx->shake_amp = 1.0f;
    ctx->shake_dx = ctx->shake_dy = 0.0f;
    ctx->shake_frame = 0;
    for (int i = 0; i < EFFECT_LANE_CAP; ++i) {
        ctx->effect_lane[i].active = 0;
    }
    save_init(ctx);
}
