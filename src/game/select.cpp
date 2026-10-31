// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <string.h>

#include "cmd.hpp"
#include "game.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "select.hpp"
#include "ai.hpp"
#include "frontend.hpp"
#include "gods98/engine.hpp"
#include "gods98/view.hpp"

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

// FUNCTION: LEGORR 0x00452be0
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

// FUNCTION: LEGORR 0x004525f0
int selection_count(const GameContext* ctx)
{
    /* Corpus SelectionRecordCountGet @ 0x4525f0 — DAT_004ff298. */
    return ctx != NULL ? ctx->selection_count : 0;
}

// FUNCTION: LEGORR 0x004525b0
int selection_has_any(const GameContext* ctx)
{
    /* Corpus SelectedEntitySetHasAny @ 0x4525b0 — DAT_004ff298 != 0. */
    return ctx != NULL && ctx->selection_count != 0;
}

/* SelectedObjectOutlinesDraw (FUN_00424830): draw the first selected entry
 * green, then the rest yellow. The scene-marker builder is renderer-owned. */
// FUNCTION: LEGORR 0x00424830
void selected_object_outlines_draw(
    const GameContext* ctx, SelectedObjectOutlineDrawFn draw,
    void* context)
{
    const int count = selection_count(ctx);
    EntitySlot* const* entries = selection_record_array(ctx);
    int i;
    if (draw == NULL || entries == NULL) return;
    for (i = 0; i < count; ++i) {
        if (i == 0) {
            draw(context, entries[i], 0.0f, 1.0f, 0.0f);
        }
        else {
            draw(context, entries[i], 1.0f, 1.0f, 0.0f);
        }
    }
}

// FUNCTION: LEGORR 0x004525e0
EntitySlot* const* selection_record_array(const GameContext* ctx)
{
    /* Corpus SelectionRecordArrayGet @ 0x4525e0 — &DAT_004fef2c. */
    return ctx != NULL ? ctx->selection : NULL;
}

// FUNCTION: LEGORR 0x00452600
void selection_count_reset(GameContext* ctx)
{
    /* Corpus SelectionRecordCountReset @ 0x452600 — DAT_004ff298 = 0,
     * array entries (and their selected flags) untouched. */
    if (ctx != NULL) {
        ctx->selection_count = 0;
    }
}

// FUNCTION: LEGORR 0x004525c0
EntitySlot* selection_first(const GameContext* ctx)
{
    if (ctx == NULL || ctx->selection_count == 0) {
        return NULL;
    }
    return ctx->selection[0];
}

/* SelectedEntitiesAnyMatch (FUN_0041f420): the corpus fetches count and
 * array through 0x4525f0/0x4525e0, then returns on the first callback hit.
 * The port routes through selection_count/selection_record_array, whose own
 * NULL guards cover ctx, and hands each entry to the predicate blind — the
 * predicates NULL-check it themselves. */
// FUNCTION: LEGORR 0x0041f420
int selected_entities_any_match(const GameContext* ctx,
                                int (*predicate)(EntitySlot*, uint32_t),
                                uint32_t argument)
{
    int count = selection_count(ctx);
    EntitySlot* const* entries = selection_record_array(ctx);
    unsigned i = 0;
    EntitySlot* const* p = entries;
    while (i < (unsigned)count) {
        if (predicate(*p, argument) != 0) {
            return 1;
        }
        ++i;
        ++p;
    }
    return 0;
}

typedef struct EntityActionRequest {
    uint32_t type;
    uint32_t subtype;
    uint32_t stats_mask;
    uint32_t state_mask;
    uint32_t flags3f0_mask;
    uint32_t mode;
} EntityActionRequest;

static int entity_action_request_matches(EntitySlot* entity, void* payload)
{
    const EntityActionRequest* request = (const EntityActionRequest*)payload;
    if (entity == NULL || request == NULL) {
        return 0;
    }
    if (entity->type == 4 && entity_shared_eligibility_check(entity, 0) == 0) {
        return 0;
    }
    if ((request->mode & 1u) != 0u
        && ((uint32_t)entity->type != request->type
            || (uint32_t)entity->etype != request->subtype)) {
        return 0;
    }
    if ((request->mode & 2u) != 0u
        && (entity->flags_3f0 & request->flags3f0_mask) == 0u) {
        return 0;
    }
    if (request->stats_mask != 0u
        && (entity_stats_word_get(entity) & request->stats_mask) == 0u) {
        return 0;
    }
    if (request->state_mask != 0u
        && (entity_state_flags_get(entity) & request->state_mask) == 0u) {
        return 0;
    }
    return 1;
}

static int entity_action_request_find(GameContext* ctx,
                                     const EntityActionRequest* request)
{
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, (void*)request);
}

static int selected_entity_activity_present(EntitySlot* entity,
                                            uint32_t activity)
{
    return ai_activity_scan(entity, (int)activity);
}

static int selected_entity_activity_missing(EntitySlot* entity,
                                            uint32_t activity)
{
    return entity != NULL && ai_activity_scan(entity, (int)activity) == 0;
}

static int selected_entity_action_state_matches(EntitySlot* entity,
                                                uint32_t unused)
{
    (void)unused;
    return entity != NULL && (entity->flags_3e0 & 0x400u) != 0u
        && (entity->flags_3e0 & 0x2000u) == 0u;
}

static int selected_entity_progress_below_100(EntitySlot* entity,
                                              uint32_t unused)
{
    (void)unused;
    return entity != NULL && entity->progress_344 < 100.0f;
}

static int selected_entity_field340_below_100(EntitySlot* entity,
                                              uint32_t unused)
{
    (void)unused;
    return entity != NULL && entity->field_340 < 100.0f;
}

static int selected_action_14_15_nonroute_matches(EntitySlot* entity)
{
    if (entity == NULL || (entity_capability_flags_get(entity) & 1u) != 0u) {
        return 0;
    }
    if (entity->type == 2) {
        return (entity->flags_3e0 & 0x400u) == 0u
            && (entity->flags_3e8 & 0x60u) == 0x60u;
    }
    if ((entity->flags_3e8 & 0x60u) != 0x60u) {
        return 0;
    }
    uint32_t limit = entity->field_328;
    const uint32_t deposit_count = entity_stats_word_at_ec(entity);
    if (deposit_count < limit) {
        limit = deposit_count;
    }
    return (uint32_t)entity->carry_state_324 < limit;
}

/* QueueActionRequestType2 (FUN_0041f230): the callback tests state +0x148
 * bit 2 across all live entities, after the pool's +0x3e8 filter. */
// FUNCTION: LEGORR 0x0041F230
int queue_action_request_type2(GameContext* ctx)
{
    const EntityActionRequest request = { 0, 0, 0, 2u, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestValue (FUN_0041f270): search state +0x148 for the
 * supplied mask. */
// FUNCTION: LEGORR 0x0041F270
int queue_action_request_value(GameContext* ctx, uint32_t state_mask)
{
    const EntityActionRequest request = { 0, 0, 0, state_mask, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestFlag100000 (FUN_0041f2b0): search stats word +0x144
 * for bit 0x100000. */
// FUNCTION: LEGORR 0x0041F2B0
int queue_action_request_flag100000(GameContext* ctx)
{
    const EntityActionRequest request = { 0, 0, 0x100000u, 0, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestPair (FUN_0041f2f0): mode 1 compares both leading
 * entity words with the two request values. The corpus at 0x41f2f0 clears
 * the inactive masks, ORs mode 1 into a zero word, then supplies the pair. */
// FUNCTION: LEGORR 0x0041F2F0
int queue_action_request_pair(GameContext* ctx, int entity_type,
                              uint32_t entity_subtype)
{
    EntityActionRequest request;
    request.subtype = 0u;
    request.stats_mask = 0u;
    request.state_mask = 0u;
    request.flags3f0_mask = 0u;
    uint32_t mode = 0u;
    mode |= 1u;
    request.mode = mode;
    request.type = (uint32_t)entity_type;
    request.subtype = entity_subtype;
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, &request);
}

/* QueueActionRequestIndex (FUN_0041f340): mode 2 asks whether any entity
 * has the requested bit in its +0x3f0 flags word. Keep the 0x41f340
 * zero-mask / zero-word-then-OR setup shared with QueueActionRequestPair. */
// FUNCTION: LEGORR 0x0041F340
int queue_action_request_index(GameContext* ctx, uint32_t flags_mask)
{
    EntityActionRequest request;
    request.subtype = 0u;
    request.stats_mask = 0u;
    request.state_mask = 0u;
    request.flags3f0_mask = 0u;
    uint32_t mode = 0u;
    mode |= 2u;
    request.mode = mode;
    request.flags3f0_mask = flags_mask;
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, &request);
}

/* ObjectInteractionFlagsAllow (FUN_0041f100): flags are checked in the
 * corpus's priority order. Only the first present capability bit selects
 * a state mask; no capability bits means the action is allowed directly. */
// FUNCTION: LEGORR 0x0041f100
int object_interaction_flags_allow(GameContext* ctx, int type, int row)
{
    const uint32_t capabilities = stats_table_word_at_148(type, row);
    uint32_t required_stats;
    if ((capabilities & 0x1000u) != 0u) {
        required_stats = 0x10u;
    }
    else if ((capabilities & 0x800u) != 0u) {
        required_stats = 0x20u;
    }
    else if ((capabilities & 0x2000u) != 0u) {
        required_stats = 0x40u;
    }
    else if ((capabilities & 0x400000u) != 0u) {
        required_stats = 0x80000000u;
    }
    else {
        return 1;
    }
    const EntityActionRequest request = { 0, 0, required_stats, 0, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

typedef struct QueueCellRequest {
    const LevelState* level;
    int x;
    int y;
} QueueCellRequest;

static int queue_placement_cell_matches(EntitySlot* entity, void* payload)
{
    const QueueCellRequest* cell = (const QueueCellRequest*)payload;
    return cell != NULL
        && terrain_cell_allows_placement(entity, cell->level, cell->x, cell->y);
}

/* QueueCallbackRequest3E0 (FUN_0041f400): its callback forwards each live
 * entity and the requested cell to TerrainCellAllowsPlacement (FUN_00432a20). */
// FUNCTION: LEGORR 0x0041F400
int queue_callback_request_3e0(GameContext* ctx, int x, int y)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    QueueCellRequest cell = { ctx->level, x, y };
    return entity_pool_find_by_callback_filtered(
        ctx, queue_placement_cell_matches, &cell);
}

static int queue_walkable_cell_matches(EntitySlot* entity, void* payload)
{
    const QueueCellRequest* cell = (const QueueCellRequest*)payload;
    if (entity == NULL || cell == NULL || cell->level == NULL) {
        return 0;
    }
    if ((entity_stats_word_get(entity) & 0x20000u) != 0u
        && entity->owner_36c == NULL) {
        return 0;
    }
    if ((entity->flags_3e8 & 2u) == 0u) {
        return 0;
    }
    const uint8_t* voxel = level_voxel(cell->level, cell->x, cell->y);
    if (voxel == NULL) {
        return 0;
    }
    const int terrain_type = (int)(signed char)voxel[2];
    if (terrain_type < 0 || terrain_type >= AI_STATS_TERRAIN_TYPES
        || entity_stats_float_at24_indexed(entity, terrain_type) == 0.0f) {
        return 0;
    }
    return level_walkable(cell->level, cell->x, cell->y);
}

/* QueueCallbackRequest390 (FUN_0041f3c0): LAB_0041f390 invokes
 * TerrainCellWalkabilityQuery(entity, x, y, 0, 0). This preserves that
 * call's entity flag/owner and per-terrain speed gates before the simple
 * cell walkability predicate. */
// FUNCTION: LEGORR 0x0041F3C0
int queue_callback_request_390(GameContext* ctx, int x, int y)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    QueueCellRequest cell = { ctx->level, x, y };
    return entity_pool_find_by_callback_filtered(
        ctx, queue_walkable_cell_matches, &cell);
}

/* TutorialPathMenuAction switch cases before the final runtime gate. */
static int tutorial_path_menu_action_precheck_raw(
    GameContext* ctx, const EngineContext* engine,
    TerrainDescriptorNode* descriptors,
    TerrainCellSidecarPresenceQuery sidecar_query,
    int (*target_cell_valid)(EntitySlot*),
    int (*route_marker_candidate)(EntitySlot*, int),
    int (*vehicle_unload_eligible)(EntitySlot*, int),
    int (*entity_unload_cell_available)(EntitySlot*, int),
    int (*selected_action_14_15_route_predicate)(EntitySlot*),
    int (*building_upgrade_allowed)(EntitySlot*),
    int (*entity_action_kind_prerequisite)(EntitySlot*, int),
    int (*building_upgrade_limit_allows)(EntitySlot*),
    int action, int x, int y)
{
    if (action != 0 && action != 1 && action != 5 && action != 6 && action != 7
        && action != 0xb
        && action != 0xc && action != 0xd && action != 0xe && action != 0x10
        && action != 0x9
        && action != 0x1d && action != 0x1e && action != 0x1f
        && action != 0x21 && action != 0x22 && action != 0x23
        && action != 0x24 && action != 0x25
        && action != 0x26 && action != 0x27
        && action != 0x16 && action != 0x17
        && action != 0x18
        && action != 0x19
        && action != 0x1a
        && action != 0x1b && action != 0x1c
        && action != 0x14 && action != 0x15
        && action != 0x20
        && action != 0x29 && action != 0x30 && action != 0x31
        && action != 0x32 && action != 0x33 && action != 0x34
        && action != 0x35 && action != 0x36 && action != 0x37
        && action != 0x38 && action != 0x39 && action != 0x3a
        && action != 0x3b && action != 0x3c && action != 0x3d
        && action != 0x3e && action != 0x3f && action != 0x2d
        && action != 0x2e && action != 0x40 && action != 0x41 && action != 0x42
        && action != 0x43 && action != 0x44 && action != 0x45
        && action != 0x46) {
        return 1; /* the reference switch's default branch is a no-op */
    }
    if (action == 0x1b || action == 0x1c || action == 0x20) {
        return 1;
    }
    if (action == 0x14 || action == 0x15) {
        if (ctx == NULL) {
            return 0;
        }
        for (int i = 0; i < ctx->selection_count; ++i) {
            EntitySlot* const entity = ctx->selection[i];
            if (entity == NULL) {
                continue;
            }
            if ((entity_capability_flags_get(entity) & 1u) != 0u) {
                if ((entity->flags_3e0 & 0x400u) == 0u
                    && (entity->flags_3ec & 0x10000u) == 0u
                    && selected_action_14_15_route_predicate != NULL
                    && selected_action_14_15_route_predicate(entity) != 0) {
                    return 1;
                }
            }
            else if (selected_action_14_15_nonroute_matches(entity) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (action == 0) {
        return map_store_kind_peek() != 0xfu;
    }
    if (action >= 0x41 && action <= 0x46) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if (action == 0x41) {
            return building_upgrade_allowed != NULL
                && building_upgrade_allowed(entity) != 0;
        }
        if (queue_action_request_type2(ctx) == 0
            || entity_action_kind_prerequisite == NULL) {
            return 0;
        }
        if (action == 0x42) {
            return entity_action_kind_prerequisite(entity, 1) != 0
                || entity_action_kind_prerequisite(entity, 0) != 0
                || entity_action_kind_prerequisite(entity, 2) != 0
                || entity_action_kind_prerequisite(entity, 3) != 0;
        }
        return entity_action_kind_prerequisite(
            entity, action == 0x43 ? 1 : action == 0x44 ? 0
                : action == 0x45 ? 2 : 3) != 0;
    }
    if (action == 0x40) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL && (entity->flags_3e0 & 0x400u) == 0u
            && building_upgrade_limit_allows != NULL
            && building_upgrade_limit_allows(entity) != 0
            && queue_action_request_flag100000(ctx) != 0;
    }
    if (ctx == NULL) {
        return 0;
    }

    if (action == 1) {
        int barracks_type = 0;
        int barracks_row = 0;
        (void)entity_type_row_for_name(
            "Barracks", &barracks_type, &barracks_row);
        return object_interaction_flags_allow(ctx, 2, 0) != 0
            && ensure_cell_entries_loaded(2u, 0, 0) != 0
            && entity_state_pair_predicate(
                   ctx, 2, 0, barracks_type, barracks_row) != 0;
    }

    if (action == 0x2d || action == 0x2e) {
        if (engine == NULL || engine->views[2] == NULL) {
            return 0;
        }
        const int required_view_mode = (action == 0x2d) ? 0 : 1;
        return selection_first(ctx) != current_selection_context_get()
            || engine->views[2]->a5 != required_view_mode;
    }

    if (action == 0x9) {
        return selection_has_any(ctx) == 0
            || active_entity_list_contains_code(ctx, 1) != 0
            || queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x1e) {
        return active_entity_list_contains_code(ctx, 0) != 0
            || queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x30) {
        return queue_action_request_flag100000(ctx);
    }
    if (action == 0x1f) {
        return selected_entities_any_match(
            ctx, selected_entity_progress_below_100, 0u);
    }
    if (action == 0x1a) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity->flags_3e0 & 2u) == 0u
            && (entity_stats_word_get(entity) & 0x20000u) != 0u
            && (entity->flags_3ec & 0x40u) == 0u
            && route_marker_candidate != NULL
            && route_marker_candidate(entity, 1) != 0;
    }
    if (action == 0x19) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL || (entity->flags_3e0 & 1u) != 0u
            || (entity_stats_word_get(entity) & 0x20000u) == 0u) {
            return 0;
        }
        const uint32_t stats_flags = entity_stats_word_get(entity);
        return (stats_flags & 0x2000000u) != 0u
            || (vehicle_unload_eligible != NULL
                && vehicle_unload_eligible(entity, 0) != 0)
            || (entity->flags_3ec & 0x40u) != 0u;
    }
    if (action == 0x18) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }

        int request_index = 2;
        uint32_t request_state_mask = 8u;
        const uint32_t capabilities = entity_capability_flags_get(entity);
        if ((capabilities & 0x100u) != 0u) {
            request_index = 1;
            request_state_mask = 4u;
        }
        else {
            const uint32_t stats_flags = entity_stats_word_get(entity);
            if ((stats_flags & 0x2000000u) != 0u) {
                if ((stats_flags & 0x1000000u) != 0u) {
                    request_index = 1;
                    request_state_mask = 4u;
                }
                else {
                    request_index = 4;
                    request_state_mask = 0x10u;
                }
            }
        }

        const int has_unload_capability = (capabilities & 0x20u) != 0u;
        const int has_unload_override = (entity->flags_3ec & 0x40u) != 0u;
        if ((!has_unload_capability || has_unload_override)
            && request_index == 2 && !has_unload_override) {
            return 0;
        }
        if (has_unload_capability && !has_unload_override
            && (entity_unload_cell_available == NULL
                || entity_unload_cell_available(entity, 0) == 0)) {
            return 0;
        }
        if ((entity->flags_3e4 & 0x10u) != 0u) {
            return 0;
        }
        return queue_action_request_index(ctx, (uint32_t)request_index) != 0
            || (queue_action_request_pair(ctx, 2, 0u) != 0
                && queue_action_request_value(ctx, request_state_mask) != 0);
    }
    if (action == 0x21) {
        return selected_entities_any_match(
            ctx, selected_entity_activity_present, 6u);
    }
    if (action == 0x22 || action == 0x23) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity_state_flags_get(entity) & 0x400u) == 0u
            && (entity->flags_3e8 & 0x20000000u) != 0u;
    }
    if (action == 0x24) {
        if (selected_entities_any_match(
                ctx, selected_entity_field340_below_100, 0u) == 0) {
            return 0;
        }
        if (queue_action_request_index(ctx, 0x10u) != 0) {
            return 1;
        }
        return queue_action_request_pair(ctx, 2, 0u) != 0
            && queue_action_request_value(ctx, 0x40u) != 0;
    }
    if (action == 0x25) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity_stats_word_get(entity) & 0x70u) != 0u;
    }
    if (action == 0x16 || action == 0x17) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if ((entity->flags_3e0 & 0x400u) != 0u
            && entity->carry_link_308 != NULL
            && ((const EntitySlot*)entity->carry_link_308)->type == 9) {
            return 0;
        }
        if ((entity_capability_flags_get(entity) & 1u) != 0u
            && (target_cell_valid == NULL
                || target_cell_valid(entity) == 0)) {
            return 0;
        }
        return selected_entities_any_match(
            ctx, selected_entity_action_state_matches, 0u);
    }
    if (action == 0x26 || action == 0x27) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if (action == 0x26
            && entity_action_kind_supported(entity, 6, 0) == 0) {
            return 0;
        }
        return entity_action_kind_supported(entity, 7, 0);
    }
    if (action >= 0x31 && action <= 0x38) {
        static const uint8_t activities[8] = { 0, 1, 2, 3, 4, 5, 7, 6 };
        const uint32_t activity = activities[action - 0x31];
        return selected_entities_any_match(
                   ctx, selected_entity_activity_missing, activity) != 0
            && queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x39) {
        if (queue_action_request_value(ctx, 0xfcu) == 0) {
            return 0;
        }
        const EntitySlot* entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        const uint32_t flags = entity->flags_3f0;
        return (flags & 0x3fu) != 0x3fu;
    }
    if (action >= 0x3a && action <= 0x3f) {
        static const uint32_t current_masks[6] = {
            4u, 0x10u, 0x20u, 1u, 2u, 8u
        };
        static const uint32_t request_masks[6] = {
            0x10u, 0x40u, 0x80u, 4u, 8u, 0x20u
        };
        EntitySlot* const entity = selection_first(ctx);
        const int index = action - 0x3a;
        return entity != NULL && ctx->level != NULL
            && current_level_state_flags_contain_mask(
                   ctx, current_masks[index]) == 0
            && queue_action_request_value(ctx, request_masks[index]) != 0
            && entity_stats_float_atf4(entity)
                   >= engine_float_field260_get(ctx->level);
    }

    if (action == 0x1d) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        return (entity->flags_3e8 & 2u) != 0u
            && (entity->flags_3e0 & 0x400u) == 0u;
    }

    if (ctx->level == NULL) {
        return 0;
    }

    if (action == 6) {
        return level_cell_occupied_at(ctx->level,
                                      (int)map_store_pair_word(0),
                                      (int)map_store_pair_word(1));
    }

    if (action == 5) {
        const uint32_t target[2] = {
            map_store_pair_word(0), map_store_pair_word(1)
        };
        return terrain_cell_predicate_check(descriptors, target) == 0
            && level_cell_occupied_at(ctx->level, (int)target[0],
                                      (int)target[1]) == 0
            && ensure_cell_entries_loaded(0xfu, 0, 0) != 0;
    }

    if (action == 7) {
        const uint32_t target[2] = {
            map_store_pair_word(0), map_store_pair_word(1)
        };
        const uint8_t* voxel = level_voxel(
            ctx->level, (int)target[0], (int)target[1]);
        return terrain_cell_predicate_check(descriptors, target) == 0
            && voxel != NULL && voxel[2] != 6u;
    }

    if (action == 0xe) {
        return level_cell_secondary_flag_8(ctx->level, x, y) == 0u
            && terrain_cell_has_occupied_or_flagged_neighbor(
                   ctx->level, x, y, sidecar_query) != 0
            && ensure_cell_entries_loaded(0xbu, 0, 0) != 0;
    }

    if (action == 0x10) {
        const uint8_t* voxel = level_voxel(ctx->level, x, y);
        if (voxel == NULL) {
            return 0;
        }
        uint32_t cell_flags;
        __builtin_memcpy(&cell_flags, voxel + 0x08, sizeof(cell_flags));
        return (cell_flags & 0x40100000u) != 0u
            && level_cell_flag_8000(ctx->level, x, y) == 0u
            && level_cell_occupied_at(ctx->level, x, y) == 0
            && level_cell_occupied_or_flag(ctx->level, x, y, 1) == 0u;
    }

    if (action == 0x29) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        int cell_x = -1;
        int cell_y = -1;
        entity_position_to_terrain_cell(entity, ctx->level, &cell_x, &cell_y);
        if (entity->owner_36c != NULL
            && terrain_cell_route_and_occupancy_check(
                   (const EntitySlot*)entity->owner_36c, ctx->level,
                   cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            return 0;
        }
        if ((entity_capability_flags_get(entity) & 1u) != 0u
            && (entity->flags_3e0 & 0x400u) != 0u
            && entity->carry_link_308 != NULL
            && terrain_cell_route_and_occupancy_check(
                   (const EntitySlot*)entity->carry_link_308, ctx->level,
                   cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            return 0;
        }
        return (entity_state_flags_get(entity) & 0x2000000u) != 0u
            || (entity->flags_3ec & 0x40u) != 0u;
    }

    if (cell_four_neighbor_walk_check(ctx->level, x, y) == 0) {
        return 0;
    }

    if (action == 0xb) {
        if (selection_has_any(ctx) == 0) {
            return queue_callback_request_390(ctx, x, y);
        }
        if (active_entity_list_contains_code(ctx, 0) == 0
            && queue_action_request_flag100000(ctx) == 0) {
            return 0;
        }
        for (int i = 0; i < ctx->selection_count; ++i) {
            if (ctx->selection[i] != NULL
                && level_walkable(ctx->level, x, y) != 0) {
                return 1;
            }
        }
        return 0;
    }

    if (action == 0xd) {
        const uint8_t* voxel = level_voxel(ctx->level, x, y);
        if (voxel == NULL || voxel[2] == 1 || voxel[2] == 11
            || level_cell_walkable_flag(ctx->level, x, y) == 0
            || level_cell_flag_reserved(ctx->level, x, y) != 0u
            || queue_action_request_flag100000(ctx) == 0) {
            return 0;
        }
        if (queue_action_request_index(ctx, 8u) != 0u) {
            return 1;
        }
        return queue_action_request_pair(ctx, 2, 0u) != 0
            && queue_action_request_value(ctx, 0x20u) != 0;
    }

    if (level_cell_flag_20_bounded(ctx->level, x, y) != 0u
        || level_cell_mask_c0(ctx->level, x, y) != 0u
        || terrain_cell_allows_placement(NULL, ctx->level, x, y) == 0) {
        return 0;
    }
    if (selection_has_any(ctx) == 0) {
        return queue_callback_request_3e0(ctx, x, y);
    }
    if (active_entity_list_contains_code(ctx, 2) == 0
        && queue_action_request_flag100000(ctx) == 0) {
        return 0;
    }
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] != NULL
            && terrain_cell_allows_placement(ctx->selection[i], ctx->level,
                                             x, y) != 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041D040
int tutorial_path_menu_action(
    GameContext* ctx, const EngineContext* engine,
    TerrainDescriptorNode* descriptors,
    TerrainCellSidecarPresenceQuery sidecar_query,
    int (*target_cell_valid)(EntitySlot*),
    int (*route_marker_candidate)(EntitySlot*, int),
    int (*vehicle_unload_eligible)(EntitySlot*, int),
    int (*entity_unload_cell_available)(EntitySlot*, int),
    int (*selected_action_14_15_route_predicate)(EntitySlot*),
    int (*building_upgrade_allowed)(EntitySlot*),
    int (*entity_action_kind_prerequisite)(EntitySlot*, int),
    int (*building_upgrade_limit_allows)(EntitySlot*),
    int action, int x, int y)
{
    const int action_result = tutorial_path_menu_action_precheck_raw(
        ctx, engine, descriptors, sidecar_query, target_cell_valid,
        route_marker_candidate, vehicle_unload_eligible,
        entity_unload_cell_available, selected_action_14_15_route_predicate,
        building_upgrade_allowed, entity_action_kind_prerequisite,
        building_upgrade_limit_allows, action, x, y);
    if (action_result <= 0) {
        return action_result;
    }

    const uint32_t global_flags = global_state_low_word_get();
    if ((global_flags & 0x6u) != 0u) {
        return action == 0 || (action >= 0x1b && action <= 0x1c);
    }
    if ((global_flags & 0x18u) != 0u) {
        return action == 0 || (action >= 0x14 && action <= 0x15);
    }
    if ((global_flags & 0x60u) != 0u) {
        return action == 0 || (action >= 0x1d && action <= 0x1e);
    }
    return action_result;
}

/* SelectionStateTick (FUN_004528c0) — see select.hpp. */
// FUNCTION: LEGORR 0x004528c0
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
    int i;
    for (i = 0; i < *count; ++i) {
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
    for (i = 0; i < *count; ) {
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
        int pair[2];
        pair[0] = (int)tx;
        pair[1] = (int)ty;
        cmd_produce(5, ctx->selection[i], 0, pair);
        ++n;
    }
    return n;
}

void selection_cmd_add(GameContext* ctx, EntitySlot* e)
{
    if (ctx != NULL && e != NULL) {
        cmd_produce(8, e, 0, NULL);
    }
}

// FUNCTION: LEGORR 0x00452610
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

static int selected_map_coordinate_matches(const uint32_t coordinate[2])
{
    return coordinate != NULL
        && map_store_pair_word(0) == coordinate[0]
        && map_store_pair_word(1) == coordinate[1];
}

static void cancel_action_if_map_coordinate_matches(uint32_t mode,
                                                     uint32_t required_mode,
                                                     const uint32_t coordinate[2])
{
    if (mode == required_mode && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

/* SelectionMode7CancelAtPoint (FUN_0041cf80): cancel on a matching map
 * coordinate while objective selection mode 7 is active. */
// FUNCTION: LEGORR 0x0041CF80
void selection_mode7_cancel_at_point(const uint32_t coordinate[2])
{
    cancel_action_if_map_coordinate_matches(
        objective_selection_word_peek(), 7u, coordinate);
}

/* CancelActionForMatchingCoordinates (FUN_0041cfb0): modes 1 and 4 cancel
 * when the clicked coordinate matches the stored map target. */
// FUNCTION: LEGORR 0x0041CFB0
void cancel_action_for_matching_coordinates(const uint32_t coordinate[2])
{
    const uint32_t mode = objective_selection_word_peek();
    if ((mode == 1u || mode == 4u)
        && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

/* SelectionMode5CancelAtPoint (FUN_0041cfe0): the same coordinate match
 * cancels only while objective selection mode 5 is active. */
// FUNCTION: LEGORR 0x0041CFE0
void selection_mode5_cancel_at_point(const uint32_t coordinate[2])
{
    if (objective_selection_word_peek() == 5u
        && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

/* SelectionObjectCommandUpdate (FUN_0041cea0): in mode 12, find the
 * argument in the current selection. A singleton selection updates the HUD
 * action to 10 or 11 from the entity's action flags; a multi-selection
 * clears the pending command context. */
// FUNCTION: LEGORR 0x0041CEA0
void selection_object_command_update(GameContext* ctx, EntitySlot* entity)
{
    if (objective_selection_word_peek() != 12u || ctx == NULL
        || entity == NULL || ctx->selection_count <= 0) {
        return;
    }
    int found = 0;
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] == entity) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        return;
    }
    if (ctx->selection_count != 1) {
        selection_clear_request_and_hud_reset();
        return;
    }
    const uint32_t capabilities = entity_capability_flags_get(entity);
    const uint32_t action_flags = entity->flags_3ec;
    const int action = ((capabilities & 0x02000000u) != 0u
                        || (action_flags & 0x40u) != 0u) ? 10 : 11;
    command_context_post_to_hud(action, NULL);
}

/* SelectionTargetCommandUpdate (FUN_0041cf20): in modes 10, 18, and 11,
 * read the selection through the count/array helpers (a NULL context
 * reads as empty) and match the passed entity's owner against every
 * entry. A matching singleton posts HUD action 12; a matching
 * multi-selection clears it. Deviation: a NULL entity faults at the
 * corpus's blind [entity+0x36c] read. */
// FUNCTION: LEGORR 0x0041CF20
void selection_target_command_update(GameContext* ctx, EntitySlot* entity)
{
    const uint32_t mode = objective_selection_word_peek();
    if (mode == 10u || mode == 18u || mode == 11u) {
        const int count = selection_count(ctx);
        EntitySlot* const* records = selection_record_array(ctx);
        if (count > 0) {
            EntitySlot* const* p = records;
            int n = count;
            do {
                /* the corpus re-reads [entity+0x36c] on every pass */
                if (entity->owner_36c == (const void*)*p) {
                    if (count == 1) {
                        command_context_post_to_hud(12, NULL);
                    }
                    else {
                        selection_clear_request_and_hud_reset();
                    }
                }
                ++p;
                --n;
            } while (n != 0);
        }
    }
}

/* SelectionCancelIfCursorHitsObject (FUN_0041d010): only active in game
 * context, and only when the entity is selected or has the cursor-hit flag.
 * The corpus then queues command 3 and clears the HUD action context. */
// FUNCTION: LEGORR 0x0041D010
void selection_cancel_if_cursor_hits_object(EngineContext* engine,
                                             GameContext* ctx,
                                             EntitySlot* entity)
{
    if (engine == NULL || engine->context_mode == 0 || entity == NULL) {
        return;
    }
    if (selection_contains_entity(ctx, entity) != 0
        || (entity->flags_3e8 & 0x200000u) != 0u) {
        selection_clear_request_and_hud_reset();
    }
}

// FUNCTION: LEGORR 0x00452690
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
// FUNCTION: LEGORR 0x0041f720
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
// FUNCTION: LEGORR 0x0044c4a0
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
