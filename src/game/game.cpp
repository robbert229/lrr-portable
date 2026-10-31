// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "game.hpp"
#include "cmd.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "savegame.hpp"
#include "crt/crt.hpp"

const char* game_layer_name(void)
{
    return "game layer (level/mission/save)";
}

/* Root identity string used in the reconstruction log. */
const char* game_identity(void)
{
    return "lrr-portable game layer — finding 07/08/22";
}

static EntitySlot* game_ddiscene_owner_resolve(uint32_t owner_token,
                                               void* context)
{
    GameContext* ctx = (GameContext*)context;
    int table;
    if (ctx == NULL || owner_token == 0u) return NULL;
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        int index;
        if (count < 0) count = 0;
        if (count > ENTITY_CAP) count = ENTITY_CAP;
        for (index = 0; index < count; ++index) {
            EntitySlot* entity = &ctx->entity[table][index];
            if (entity->ddiscene_owner_token == owner_token) return entity;
        }
    }
    return NULL;
}

static void* game_font_resource_cache_load(const char* name)
{
    return bitmap_font_resource_load(name);
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
    save_init(ctx);
    string_cache_font_loader_set(game_font_resource_cache_load);
    entity_pool_callback_context_set(ctx);
    cmd_context_set(ctx);
    unported_ledger_stubs::entity_ddiscene_callbacks_install(
        game_ddiscene_owner_resolve, ctx);
}
