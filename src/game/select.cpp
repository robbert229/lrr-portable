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

/* SelectionStateTick (FUN_004528c0) — see select.hpp. */
void selection_state_tick(GameContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus strides its global band blindly */
    }
    EntitySlot** sel = ctx->selection;
    int* count = &ctx->selection_count;

    /* Pass 1: classify the whole selection into the highest class any
     * entry justifies. Deviations: the widened owner mirror stands in
     * for the corpus's +0x36c pointer word (pointer-word convention),
     * and the corpus's byte read at +0x3ec lands on the flag word's low
     * byte in little-endian. */
    int cls_none = 0; /* the class-none shape: every entry is stale */
    int cls_two = 0, cls_bad = 0, cls_clean = 0, cls_flag = 0;
    int cls_four = 0, cls_eleven = 0;
    for (int i = 0; i < *count; ++i) {
        EntitySlot* e = sel[i];
        if (e == NULL) {
            continue; /* deviation: the corpus strides the slot blindly */
        }
        const int type = e->type;
        const uint32_t stats = entity_stats_word_get(e);
        if (type == 2 && e->owner_36c == NULL) {
            cls_two = 1; /* kind 2 unowned: the class wins, scan done */
            break;
        }
        if ((stats & 0x20000u) == 0u) {
            if (type == 1) {
                if ((stats & 0x2000000u) == 0u
                    && (e->flags_3ec & 0x40u) == 0u) {
                    cls_clean = 1;
                } else {
                    cls_bad = 1;
                }
            } else if (type == 4) {
                cls_four = 1;
            } else if (type == 0xb && (e->flags_3e4 & 0x2000000u) != 0u) {
                cls_eleven = 1;
            }
        } else if (e->owner_36c == NULL) {
            cls_flag = 1;
        } else if ((stats & 0x2000000u) == 0u
                   && (e->flags_3ec & 0x40u) == 0u) {
            cls_clean = 1;
        } else {
            cls_bad = 1;
        }
    }

    /* The class ladder posts one HUD context and silences the lower
     * classes (the corpus's own chain, contexts 9..0xe). */
    if (cls_two) {
        command_context_post_to_hud(9, NULL);
        cls_bad = cls_clean = cls_flag = cls_four = cls_eleven = 0;
    } else if (cls_bad) {
        command_context_post_to_hud(10, NULL);
        cls_clean = cls_flag = cls_four = cls_eleven = 0;
    } else if (cls_clean) {
        command_context_post_to_hud(0xb, NULL);
        cls_flag = cls_four = cls_eleven = 0;
    } else if (cls_flag) {
        command_context_post_to_hud(0xc, NULL);
        cls_four = cls_eleven = 0;
    } else if (cls_four) {
        command_context_post_to_hud(0xd, NULL);
        cls_eleven = 0;
    } else if (cls_eleven) {
        command_context_post_to_hud(0xe, NULL);
    } else {
        command_context_post_to_hud(0, NULL);
        cls_none = 1;
    }

    /* Pass 2: entries outside the chosen class drop out — swap-with-last
     * compaction that revisits the swapped-in slot (the corpus's index
     * quirk) — and every dropped entity loses the selected bit. The
     * class-none shape drops everything (quirk pinned). */
    for (int i = 0; i < *count; ) {
        EntitySlot* e = sel[i];
        const int type = e->type;
        const uint32_t stats = entity_stats_word_get(e);
        const int bit17 = (stats & 0x20000u) != 0u;
        const int bit25 = (stats & 0x2000000u) != 0u;
        const int bit6 = (e->flags_3ec & 0x40u) != 0u;
        const int owned = e->owner_36c != NULL;
        int drop;
        if (cls_two) {
            drop = (type != 2 || owned);
        } else if (cls_bad) {
            drop = bit17 ? ((!bit25 && !bit6) || !owned) : (type != 1);
        } else if (cls_clean) {
            drop = bit17 ? ((bit25 || bit6) || !owned) : (type != 1);
        } else if (cls_flag) {
            drop = !bit17 || owned;
        } else if (cls_four) {
            drop = (type != 4);
        } else if (cls_eleven) {
            drop = type != 0xb || (e->flags_3e4 & 0x2000000u) == 0u;
        } else {
            drop = 1; /* the class-none shape removes every entry */
        }
        if (drop) {
            *count = *count - 1;
            sel[i] = sel[*count];
            e->flags_3e8 &= ~ENTITY_SELECTED;
            /* No index advance: the swapped-in entry gets revisited. */
        } else {
            ++i;
        }
    }
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
