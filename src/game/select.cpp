// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <string.h>

#include "cmd.hpp"
#include "game.hpp"
#include "entity.hpp"
#include "select.hpp"
#include "gods98/engine.hpp"

int selection_add(GameContext* ctx, EntitySlot* e)
{
    if (ctx == NULL || e == NULL) {
        return 0;
    }
    /* Dedupe (FUN_00452610 analog). */
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] == e) {
            return 0;
        }
    }
    if (ctx->selection_count >= SELECTION_MAX) {
        return 0;
    }
    ctx->selection[ctx->selection_count++] = e;
    e->flags |= ENTITY_SELECTED;
    return 1;
}

void selection_remove(GameContext* ctx, const EntitySlot* e)
{
    if (ctx == NULL || e == NULL) {
        return;
    }
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] == e) {
            ctx->selection[i]->flags &= ~ENTITY_SELECTED;
            ctx->selection[i] = ctx->selection[ctx->selection_count - 1];
            --ctx->selection_count;
            return;
        }
    }
}

void selection_clear(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    for (int i = 0; i < ctx->selection_count; ++i) {
        ctx->selection[i]->flags &= ~ENTITY_SELECTED;
    }
    ctx->selection_count = 0;
}

int selection_count(const GameContext* ctx)
{
    /* Corpus SelectionRecordCountGet @ 0x4525f0 — DAT_004ff298. */
    return ctx != NULL ? ctx->selection_count : 0;
}

int selection_has_any(const GameContext* ctx)
{
    /* Corpus SelectedEntitySetHasAny @ 0x4525b0 — DAT_004ff298 != 0. */
    return ctx != NULL && ctx->selection_count != 0;
}

EntitySlot* const* selection_record_array(const GameContext* ctx)
{
    /* Corpus SelectionRecordArrayGet @ 0x4525e0 — &DAT_004fef2c. */
    return ctx != NULL ? ctx->selection : NULL;
}

void selection_count_reset(GameContext* ctx)
{
    /* Corpus SelectionRecordCountReset @ 0x452600 — DAT_004ff298 = 0,
     * array entries (and their selected flags) untouched. */
    if (ctx != NULL) {
        ctx->selection_count = 0;
    }
}

EntitySlot* selection_first(const GameContext* ctx)
{
    if (ctx == NULL || ctx->selection_count == 0) {
        return NULL;
    }
    return ctx->selection[0];
}

/* Squared distance between a placed entity and grid cell (cx,cy). */
static float pick_dist2(const EntitySlot* e, float cx, float cy)
{
    float dx = e->x - cx;
    float dy = e->y - cy;
    return dx * dx + dy * dy;
}

EntitySlot* selection_pick(GameContext* ctx, float cx, float cy, float tol)
{
    if (ctx == NULL) {
        return NULL;
    }
    EntitySlot* best = NULL;
    float best_d = tol * tol;
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            EntitySlot* e = &ctx->entity[t][i];
            float d = pick_dist2(e, cx, cy);
            if (d <= best_d) {
                best_d = d;
                best = e;
            }
        }
    }
    return best;
}

int selection_cmd_move(GameContext* ctx, float tx, float ty)
{
    if (ctx == NULL) {
        return 0;
    }
    int n = 0;
    for (int i = 0; i < ctx->selection_count; ++i) {
        cmd_produce(5, ctx->selection[i], 0, (int)tx, (int)ty);
        ++n;
    }
    return n;
}

void selection_cmd_add(GameContext* ctx, EntitySlot* e)
{
    if (ctx != NULL && e != NULL) {
        cmd_produce(8, e, 0, 0, 0);
    }
}

int selection_contains_entity(const GameContext* ctx, const EntitySlot* e)
{
    if (ctx == NULL || e == NULL) {
        return 0;
    }
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] == e) {
            return 1;
        }
    }
    return 0;
}

int selection_candidate_already_selected_or_marked(const GameContext* ctx,
                                                   const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if (selection_contains_entity(ctx, e) != 0) {
        return 1;
    }
    return (e->flags_3ec & 2u) != 0u ? 1 : 0;
}

/* CurrentLevelStateFlagsContainMask (FUN_0041f720) — see select.hpp. */
int current_level_state_flags_contain_mask(const GameContext* ctx,
                                           uint32_t mask)
{
    const EntitySlot* e = selection_first(ctx);
    if (e == NULL) {
        return 0; /* deviation: the corpus reads *(NULL + 0x3f0) blindly */
    }
    return (e->flags_3f0 & mask) != 0u;
}

/* SelectionClearMarkerAndResetHud (FUN_0044c4a0) — see select.hpp. The
 * marker raise lands on the DAT_00556ecc engine state word, which the
 * port threads through the engine handle (deviation: the corpus strides
 * the global word). The clear request always runs. */
void selection_clear_marker_and_reset_hud(struct EngineContext* engine,
                                          EntitySlot* e)
{
    if (e != NULL && (e->flags_3ec & 2u) != 0u) {
        if ((entity_stats_word_get(e) & 0x20000u) != 0u) {
            if ((entity_state_flags_get(e) & 0x20000u) == 0u
                || e->owner_36c != NULL) {
                if (engine != NULL) {
                    engine->state |= 0x10000000u;
                }
                e->flags_3ec |= 1u;
            }
        }
    }
    selection_clear_request_and_hud_reset(); /* FUN_0041e750 */
}
