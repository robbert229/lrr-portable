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
#include "gods98/record.hpp"
#include "gods98/input.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "render.hpp"
#include "gods98/sound.hpp"
#include "gods98/view.hpp"

static GameContext* s_selection_active_context;
// GLOBAL: LEGORR 0x004ff298
static uint32_t s_selection_active_count;
// GLOBAL: LEGORR 0x004fef2c
static EntitySlot* s_selection_active_records[SELECTION_MAX];

void selection_active_context_sync(GameContext* ctx)
{
    int i;
    if (ctx == NULL) {
        if (s_selection_active_context != NULL) return;
        s_selection_active_count = 0u;
        memset(s_selection_active_records, 0,
               sizeof(s_selection_active_records));
        return;
    }
    if (ctx != s_selection_active_context) return;
    s_selection_active_count = (uint32_t)ctx->selection_count;
    for (i = 0; i < SELECTION_MAX; ++i) {
        s_selection_active_records[i] = ctx->selection[i];
    }
}

void selection_active_context_bind(GameContext* ctx)
{
    s_selection_active_context = ctx;
    selection_active_context_sync(ctx);
}

int selection_add(GameContext* ctx, EntitySlot* e)
{
    if (ctx == NULL || e == NULL) {
        return 0;
    }
    /* Retail SelectionCandidateFilter uses FUN_00452610(entity, NULL),
     * which tests the +0x3e8 selected bit rather than scanning the array. */
    if ((e->flags_3e8 & ENTITY_SELECTED) != 0u) return 0;
    if (ctx->selection_count >= SELECTION_MAX) {
        return 0;
    }
    ctx->selection[ctx->selection_count++] = e;
    e->flags |= ENTITY_SELECTED;
    e->flags_3e8 |= ENTITY_SELECTED;
    selection_active_context_sync(ctx);
    return 1;
}

void selection_remove(GameContext* ctx, const EntitySlot* e)
{
    uint32_t index;
    if (ctx == NULL || e == NULL) {
        return;
    }
    if (!selection_contains_entity_target(ctx, e, &index)) return;
    ((EntitySlot*)e)->flags &= ~ENTITY_SELECTED;
    ((EntitySlot*)e)->flags_3e8 &= ~ENTITY_SELECTED;
    ctx->selection[index] = ctx->selection[ctx->selection_count - 1];
    --ctx->selection_count;
    selection_active_context_sync(ctx);
}

/* SelectionCandidateFilter (FUN_004526e0) operates on the active selection
 * globals. The command-layer adapter binds and publishes the GameContext. */
/* The reference callback at 0x0044c500 clears the transient selection marker
 * on every filtered-pool entity and clears the engine-state dirty bit when
 * that marker was present. Its body is visible in the reference executable's
 * code at this address, though omitted from the checked-in decompile. */
// FUNCTION: LEGORR 0x0044c500
static int selection_candidate_filter_pool_refresh_callback(
    EntitySlot* entity, void* payload)
{
    if ((entity->flags_3ec & 1u) != 0u) {
        uint32_t* state = engine_current_state_word();
        if (state != NULL) {
            *state &= ~0x10000000u;
        }
    }
    entity->flags_3ec &= ~1u;
    (void)payload;
    return 0;
}

#if !defined(_MSC_VER) || !defined(_M_IX86)
/* EntityTypeComponentGet (FUN_0044a200) indexes the retail entity record as
 * 32-bit words. EntitySlot is a port sidecar whose attached-record fields are
 * native-width on this build, so read them through their named members (the
 * same +0x0c/+0x10/+0x14/+0x18/+0x1c/+0x20 slots) instead of by word index. */
static void* selection_entity_type_component(const EntitySlot* e)
{
    const void* record = NULL;
    switch (e->type) {
    case 1:
        record = e->attached_direction_record_type1;
        return record != NULL
            ? (void*)(uintptr_t)scene_object_resource_get(record) : NULL;
    case 2:
        record = e->attached_direction_record_type2;
        break;
    case 3:
        record = e->attached_direction_record_type3;
        break;
    case 4:
        record = e->attached_direction_record_type4;
        break;
    case 10:
        record = e->attached_direction_record_type10;
        break;
    default:
        if ((e->flags_3e8 & 0x00020000u) != 0u)
            return (void*)e->type_operation_fallback_1c;
        return (void*)e;
    }
    return record != NULL
        ? (void*)(uintptr_t)animation_object_handle_get(record) : NULL;
}
#endif

// FUNCTION: LEGORR 0x004526e0
int selection_candidate_filter_active(void* entity, int mode, int param_3)
{
    EntitySlot* e = (EntitySlot*)entity;
    if (e->attach_304 != NULL) {
        e = (EntitySlot*)e->attach_304;
    }
    if (selection_candidate_eligibility_check(e) == 0) {
        return 0;
    }
    if (mode == 0
        && selection_active_candidate_already_selected_or_marked(e) != 0) {
        selection_clear_marker_and_reset_hud(e);
        return 0;
    }
    if (selection_contains_entity_active_target(e, NULL) != 0u
        || s_selection_active_count >= SELECTION_MAX) {
        return 0;
    }

    (void)entity_pool_find_by_callback_filtered_current(
        selection_candidate_filter_pool_refresh_callback, NULL);
    s_selection_active_records[s_selection_active_count] = e;
    ++s_selection_active_count;
    e->flags_3e8 |= ENTITY_SELECTED;
    if (e->type != 4
        && (e->flags_3e0 & 0x24002000u) == 0u
        && (e->flags_3e4 & 0x100u) == 0u) {
        entity_action_sequence_reset(e, 0);
        entity_activity_records_refresh(e);
        if (e->type == 2 && param_3 != 0) {
            ViewContext* view = engine_current_primary_view_get();
            if (view != NULL) {
                float facing[3];
                float target[2];
                cam_get_facing(&view->cam_follow, facing);
                target[0] = -facing[0];
                target[1] = -facing[1];
                entity_state_reset(e, 0, 0);
                unported_ledger_stubs::
                    ledger_stub_00444fb0_EntityTurnToVectorActivityIssue(
                        e, target);
            }
        }
    }
    if ((e->flags_3e8 & 0x80u) != 0u
        && ((entity_stats_word_get(e) & 0x20000u) == 0u
            || e->owner_36c != NULL)) {
#if defined(_MSC_VER) && defined(_M_IX86)
        sound_submission_route_wrapper(entity_type_component_get(e),
                                       0x11, 0, 1, NULL);
#else
        sound_submission_route_wrapper(selection_entity_type_component(e),
                                       0x11, 0, 1, NULL);
#endif
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00452be0
void selection_clear_active(void)
{
    uint32_t index = 0;
    if (s_selection_active_count != 0u) {
        EntitySlot** record = s_selection_active_records;
        do {
            EntitySlot* entity = *record;
            if ((entity->flags_3e8 & ENTITY_SELECTED) != 0u) {
                entity->flags_3ec |= 2u;
            }
            entity->flags_3e8 &= ~ENTITY_SELECTED;
            entity->flags_3ec &= ~1u;
            ++index;
            ++record;
        } while (index < s_selection_active_count);
    }
    s_selection_active_count = 0u;
}

void selection_clear(GameContext* ctx)
{
    int i;
    if (ctx == NULL) {
        return;
    }
    selection_active_context_bind(ctx);
    for (i = 0; i < ctx->selection_count; ++i) {
        EntitySlot* entity = ctx->selection[i];
        entity->flags &= ~ENTITY_SELECTED;
    }
    selection_clear_active();
    ctx->selection_count = 0;
}

// FUNCTION: LEGORR 0x00452c50
int selection_entity_remove_active(EntitySlot* entity)
{
    uint32_t index;
    if (s_selection_active_count != 0u) {
        if (selection_contains_entity_active_target(entity, &index) != 0u) {
            if (index == 0u) {
                command_context_post_to_hud(0, NULL);
            }
            --s_selection_active_count;
            s_selection_active_records[index] =
                s_selection_active_records[s_selection_active_count];
            entity->flags_3e8 &= ~ENTITY_SELECTED;
            return 1;
        }
    }
    return 0;
}

int selection_count(const GameContext* ctx)
{
    return ctx != NULL ? ctx->selection_count : 0;
}

int selection_has_any(const GameContext* ctx)
{
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

EntitySlot* const* selection_record_array(const GameContext* ctx)
{
    return ctx != NULL ? ctx->selection : NULL;
}

void selection_count_reset(GameContext* ctx)
{
    /* Corpus SelectionRecordCountReset @ 0x452600 — DAT_004ff298 = 0,
     * array entries (and their selected flags) untouched. */
    if (ctx != NULL) {
        ctx->selection_count = 0;
        if (ctx == s_selection_active_context) {
            selection_active_count_reset();
        }
    }
}

/* The reference reads its active selection globals. The portable runtime
 * keeps those globals inside the currently bound GameContext. */
// FUNCTION: LEGORR 0x004525c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
EntitySlot* selection_active_entity_get(void)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [s_selection_active_count]
        mov ecx, dword ptr [s_selection_active_records]
        neg eax
        sbb eax, eax
        and eax, ecx
        ret
    }
#else
    return s_selection_active_count != 0u
        ? s_selection_active_records[0] : NULL;
#endif
}

// FUNCTION: LEGORR 0x00452580
EntitySlot* const* selection_active_context_record_get(int* count_out)
{
    if (s_selection_active_count != 0u) {
        if (count_out != NULL)
            *count_out = (int)s_selection_active_count;
        return s_selection_active_records;
    }
    if (count_out != NULL)
        *count_out = 0;
    return NULL;
}

// FUNCTION: LEGORR 0x004525e0
EntitySlot* const* selection_active_records_get(void)
{
    return s_selection_active_records;
}

// FUNCTION: LEGORR 0x004525f0
uint32_t selection_active_count_get(void)
{
    return s_selection_active_count;
}

// FUNCTION: LEGORR 0x004525b0
int selection_active_has_any_get(void)
{
    return s_selection_active_count != 0u;
}

// FUNCTION: LEGORR 0x00452600
void selection_active_count_reset(void)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov dword ptr [s_selection_active_count], 0
    }
#else
    s_selection_active_count = 0u;
#endif
}

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

/* These menu predicates have direct ports in the entity module. The
 * callback parameters remain overrides for hosts with additional runtime
 * state, while the normal game path can use the live EntitySlot/config APIs. */
static int selection_target_cell_valid_default(GameContext* ctx,
                                                EntitySlot* entity)
{
    EntitySlot* target;
    void* resource;
    float position[3];
    int cell_x, cell_y;
    if (ctx == NULL || ctx->level == NULL || entity == NULL ||
        (entity_capability_flags_get(entity) & 1u) == 0u ||
        (entity->flags_3e0 & 0x400u) == 0u || entity->carry_link_308 == NULL)
        return 0;
    target = (EntitySlot*)entity->carry_link_308;
    if (target->type != 1) return 0;

    /* EntityActionTargetCellIsValid @ 0x43fdc0 resolves the source's visual
     * scene, asks DDiScene for its position, converts that position to a map
     * cell, and tests that same cell through the linked type-1 target. */
    resource = entity->visual_resource_handles[0];
    if (resource == NULL && entity_type1_scene_resource_bind(entity))
        resource = entity->scene_resource_type1.scene_resource;
    if (resource == NULL) {
        entity_visual_resource_get_or_create(entity);
        resource = entity->visual_resource_handles[0];
        if (resource == NULL && entity_type1_scene_resource_bind(entity))
            resource = entity->scene_resource_type1.scene_resource;
    }
    if (resource == NULL ||
        !ddiscene_object_position_get(resource, NULL, position) ||
        !terrain_world_xy_to_cell(ctx->level->ptr_terrain, position[0],
                                  position[1], &cell_x, &cell_y)) return 0;
    return terrain_cell_route_and_occupancy_check(
        target, ctx->level, cell_x, cell_y, cell_x, cell_y, 1);
}

static int selection_route_marker_candidate_default(EntitySlot* entity,
                                                       int include_secondary)
{
    return entity_route_marker_candidate_find(entity, include_secondary)
        != NULL;
}

static int selection_vehicle_unload_eligible_default(EntitySlot* entity,
                                                       int unused)
{
    (void)unused;
    return entity_vehicle_unload_cell_find_if_eligible(entity, NULL);
}

static int selection_unload_cell_available_default(EntitySlot* entity,
                                                     int unused)
{
    (void)unused;
    return entity_record_flag20_query(entity, NULL);
}

static int selection_building_upgrade_allowed_default(EntitySlot* entity)
{
    return building_upgrade_resource_admission_check(entity, NULL);
}

static int selection_entity_action_prerequisite_default(EntitySlot* entity,
                                                         int action)
{
    return entity_action_kind_prerequisite_check(entity, action);
}

static int selection_building_upgrade_limit_default(EntitySlot* entity)
{
    uint32_t maximum;
    if (entity == NULL) return 0;
    maximum = building_upgrade_admission_check(entity->type, entity->etype);
    return maximum > (uint32_t)entity->indexed_record_slot_330 + 1u;
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
            return (building_upgrade_allowed != NULL
                ? building_upgrade_allowed(entity)
                : selection_building_upgrade_allowed_default(entity)) != 0;
        }
        if (queue_action_request_type2(ctx) == 0) {
            return 0;
        }
        int (*action_ready)(EntitySlot*, int) =
            entity_action_kind_prerequisite != NULL
                ? entity_action_kind_prerequisite
                : selection_entity_action_prerequisite_default;
        if (action == 0x42) {
            return action_ready(entity, 1) != 0
                || action_ready(entity, 0) != 0
                || action_ready(entity, 2) != 0
                || action_ready(entity, 3) != 0;
        }
        return action_ready(
            entity, action == 0x43 ? 1 : action == 0x44 ? 0
                : action == 0x45 ? 2 : 3) != 0;
    }
    if (action == 0x40) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL && (entity->flags_3e0 & 0x400u) == 0u
            && (building_upgrade_limit_allows != NULL
                ? building_upgrade_limit_allows(entity)
                : selection_building_upgrade_limit_default(entity)) != 0
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
            && (route_marker_candidate != NULL
                ? route_marker_candidate(entity, 1)
                : selection_route_marker_candidate_default(entity, 1)) != 0;
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
                ? vehicle_unload_eligible(entity, 0)
                : selection_vehicle_unload_eligible_default(entity, 0)) != 0
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
            && (entity_unload_cell_available != NULL
                ? entity_unload_cell_available(entity, 0)
                : selection_unload_cell_available_default(entity, 0)) == 0) {
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
            && (target_cell_valid != NULL
                ? target_cell_valid(entity)
                : selection_target_cell_valid_default(ctx, entity)) == 0) {
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
        return (descriptors != NULL
                   ? terrain_cell_predicate_check(descriptors, target)
                   : terrain_cell_predicate_check_current(target)) == 0
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
        return (descriptors != NULL
                   ? terrain_cell_predicate_check(descriptors, target)
                   : terrain_cell_predicate_check_current(target)) == 0
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
void selection_state_tick_active(void)
{
    EntitySlot** sel = s_selection_active_records;

    /* Pass 1: classify the whole selection into the highest class any
     * entry justifies. Deviations: the widened owner mirror stands in
     * for the corpus's +0x36c pointer word (pointer-word convention),
     * and the corpus's byte read at +0x3ec lands on the flag word's low
     * byte in little-endian. */
    int cls_none = 0; /* the class-none shape: every entry is stale */
    int cls_two = 0, cls_bad = 0, cls_clean = 0, cls_flag = 0;
    int cls_four = 0, cls_eleven = 0;
    int i;
    for (i = 0; i < (int)s_selection_active_count; ++i) {
        EntitySlot* e = sel[i];
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
    for (i = 0; i < (int)s_selection_active_count; ) {
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
            --s_selection_active_count;
            sel[i] = sel[s_selection_active_count];
            e->flags_3e8 &= ~ENTITY_SELECTED;
            /* No index advance: the swapped-in entry gets revisited. */
        } else {
            ++i;
        }
    }
}

void selection_state_tick(GameContext* ctx)
{
    int i;
    if (ctx == NULL) {
        return;
    }
    selection_active_context_bind(ctx);
    selection_state_tick_active();
    ctx->selection_count = (int)s_selection_active_count;
    for (i = 0; i < ctx->selection_count; ++i) {
        ctx->selection[i] = s_selection_active_records[i];
    }
}

/* Squared distance between a placed entity and grid cell (cx,cy). */
static int pick_dist2(const EntitySlot* e, const LevelState* level,
                       float cx, float cy, float* distance)
{
    float grid[2];
    float dx, dy;
    if (!entity_position_grid_coordinates_get(e, level, grid)) return 0;
    dx = grid[0] - cx;
    dy = grid[1] - cy;
    *distance = dx * dx + dy * dy;
    return 1;
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
            float d;
            if (e->type != 0 && pick_dist2(e, ctx->level, cx, cy, &d) &&
                d <= best_d) {
                best_d = d;
                best = e;
            }
        }
    }
    return best;
}

int selection_cmd_move(GameContext* ctx, float tx, float ty)
{
    if (ctx == NULL || ctx->level == NULL ||
        !(tx >= 0.0f && ty >= 0.0f &&
          tx < (float)level_width(ctx->level) &&
          ty < (float)level_height(ctx->level))) {
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
        cmd_produce(1, e, 1, NULL);
    }
}

int selection_contains_entity_target(const GameContext* ctx,
                                     const EntitySlot* e,
                                     uint32_t* index_out)
{
    if (index_out == NULL) {
        return e != NULL && (e->flags_3e8 & 0x200000u) != 0u;
    }
    if (ctx == NULL || e == NULL) return 0;
    int count = ctx->selection_count;
    if (count < 0) count = 0;
    if (count > SELECTION_MAX) count = SELECTION_MAX;
    for (int i = 0; i < count; ++i) {
        if (ctx->selection[i] == e) {
            *index_out = (uint32_t)i;
            return 1;
        }
    }
    return 0;
}

/* The retail entry point takes only the entity and optional index output;
 * its array and count are the active selection globals mirrored above. */
// FUNCTION: LEGORR 0x00452610
uint32_t selection_contains_entity_active_target(
    EntitySlot* entity, uint32_t* index_out)
{
    if (index_out != NULL) {
#if defined(_MSC_VER) && defined(_M_IX86)
        return (uint32_t)cmd_array_find_value_index(
            (const int*)s_selection_active_records,
            (unsigned)s_selection_active_count, (int)entity,
            (unsigned*)index_out);
#else
        uint32_t i;
        for (i = 0; i < s_selection_active_count; ++i) {
            if (s_selection_active_records[i] == entity) {
                *index_out = i;
                return 1u;
            }
        }
        return 0u;
#endif
    }
    return entity->flags_3e8 & 0x200000u;
}

int selection_contains_entity(const GameContext* ctx, const EntitySlot* e)
{
    uint32_t index;
    return selection_contains_entity_target(ctx, e, &index);
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
    const uint32_t mode = s_obj_sel_word;
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
    if (selection_contains_entity_target(ctx, entity, NULL) != 0) {
        selection_clear_request_and_hud_reset();
    }
}

int selection_candidate_already_selected_or_marked(const GameContext* ctx,
                                                   const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if (selection_contains_entity_target(ctx, e, NULL) != 0) {
        return 1;
    }
    return (e->flags_3ec & 2u) != 0u ? 1 : 0;
}

// FUNCTION: LEGORR 0x00452690
int selection_active_candidate_already_selected_or_marked(EntitySlot* entity)
{
    if (selection_contains_entity_active_target(entity, NULL) == 0
        && (entity->flags_3ec & 2u) == 0u) {
        return 0;
    }
    return 1;
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

/* SelectionClearMarkerAndResetHud (FUN_0044c4a0) uses the active engine
 * state word, corresponding to DAT_00556ecc. */
// FUNCTION: LEGORR 0x0044c4a0
void selection_clear_marker_and_reset_hud(EntitySlot* e)
{
    if ((e->flags_3ec & 2u) != 0u) {
        if ((entity_state_flags_get(e) & 0x20000u) != 0u) {
            if ((entity_stats_word_get(e) & 0x20000u) == 0u
                || e->owner_36c != NULL) {
                uint32_t* state = engine_current_state_word();
                if (state != NULL) *state |= 0x10000000u;
                e->flags_3ec |= 1u;
            }
        }
    }
    selection_clear_request_and_hud_reset(); /* FUN_0041e750 */
}

/* Compatibility entry retained for callers and tests that own an explicit
 * engine context; the mapped retail entry above takes only the entity. */
void selection_clear_marker_and_reset_hud(struct EngineContext* engine,
                                          EntitySlot* e)
{
    if (e != NULL && (e->flags_3ec & 2u) != 0u) {
        if ((entity_state_flags_get(e) & 0x20000u) != 0u) {
            if ((entity_stats_word_get(e) & 0x20000u) == 0u
                || e->owner_36c != NULL) {
                if (engine != NULL) engine->state |= 0x10000000u;
                else {
                    uint32_t* state = engine_current_state_word();
                    if (state != NULL) *state |= 0x10000000u;
                }
                e->flags_3ec |= 1u;
            }
        }
    }
    selection_clear_request_and_hud_reset();
}

/* The ordinary floor alternatives at 0x4272b6..0x42744f. A recognized
 * floor owns a failed route/permission result too: it must not become an
 * unconditional move in the compatibility adapter below. Other action
 * modes are outside this slice. */
static int selection_floor_release(GameContext* ctx, EngineContext* engine,
                                    const int cell_xy[2])
{
    const uint8_t* cell;
    uint32_t flags;
    EntitySlot* first;
    int handled = 0;
    int mark_matching = 0;
    if (level_width(ctx->level) < 2 || level_height(ctx->level) < 2 ||
        (unsigned int)cell_xy[0] >= (unsigned int)level_width(ctx->level) - 1u ||
        (unsigned int)cell_xy[1] >= (unsigned int)level_height(ctx->level) - 1u)
        return 0;
    if ((engine->state & (0x40000u | 0x10000000u)) != 0u ||
        (global_state_low_word_get() & 0xffu) != 0u) return -1;
    cell = level_voxel(ctx->level, cell_xy[0], cell_xy[1]);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x0cu) != 0x0cu || selection_count(ctx) == 0)
        return -1;
    if ((flags & 0x400u) != 0u || (cell[0x0c] & 0x24u) != 0u)
        return 0;
    first = selection_first(ctx);
    if ((flags & 0x200000u) == 0u &&
        (entity_stats_word_get(first) & 0x10000u) != 0u &&
        terrain_cell_route_and_occupancy_check_current(first,
            cell_xy[0], cell_xy[1], cell_xy[0], cell_xy[1], 0) != 0) {
        handled = 1;
        mark_matching = 1;
    }
    if (handled == 0 &&
        terrain_cell_route_and_occupancy_check_current(first,
            cell_xy[0], cell_xy[1], cell_xy[0], cell_xy[1], 0) != 0)
        handled = 1;
    /* Route success owns the branch before permission is tested. */
    if (handled == 0 || terrain_cell_command_predicate(engine,
        cell_xy[0], cell_xy[1]) != 0) return 0;
    /* HUD reset reaches the x86 visibility owner through this record.
     * Refuse the whole operation while that scene owner is unavailable;
     * do not queue a move and pretend the dependent reset succeeded. */
    if (global_refresh_record_peek() == NULL) {
        return 0;
    }
    cmd_produce(8, NULL, 0, cell_xy);
    if (mark_matching != 0) cmd_mark_matching(12, 0, cell_xy);
    media_action_feedback_sound_dispatch(0);
    selection_clear_request_and_hud_reset();
    return 1;
}

/* Ordinary selected wall action and wall context release. The special
 * placement modes remain owned by the existing dispatcher. */
static int selection_wall_release(GameContext* ctx, EngineContext* engine,
                                  const int cell_xy[2])
{
    const uint8_t* cell;
    uint32_t flags;
    uint32_t coord[2];
    if ((engine->state & (0x40000u | 0x10000000u)) != 0u ||
        (global_state_low_word_get() & 0xffu) != 0u) return -1;
    if (level_width(ctx->level) < 2 || level_height(ctx->level) < 2 ||
        (unsigned int)cell_xy[0] >= (unsigned int)level_width(ctx->level) - 1u ||
        (unsigned int)cell_xy[1] >= (unsigned int)level_height(ctx->level) - 1u)
        return 0;
    cell = level_voxel(ctx->level, cell_xy[0], cell_xy[1]);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x08u) == 0u && selection_active_has_any_get() != 0 &&
        selected_entities_any_predicate_match(cell_xy[0], cell_xy[1], 0) &&
        cell[2] != 1 && cell[2] != 9 && cell[2] != 11 &&
        (flags & 0x40u) == 0u) {
        if (terrain_cell_command_predicate(engine, cell_xy[0], cell_xy[1]) != 0)
            return 0;
        if (global_refresh_record_peek() == NULL) return 0;
        terrain_cell_command_create(cell_xy[0], cell_xy[1], 0, 1);
        cmd_mark_matching(8, 0, cell_xy);
        media_action_feedback_sound_dispatch(0);
        selection_clear_request_and_hud_reset();
        return 1;
    }
    if ((flags & 0x08u) != 0u) return -1;
    if ((flags & (0x40u | 0x400u)) != 0u ||
        (cell[0x0c] & 0x24u) != 0u || cell[2] == 1 ||
        cell[2] == 6 || cell[2] == 9) return 0;
    if (terrain_cell_command_predicate(engine, cell_xy[0], cell_xy[1]) != 0)
        return 0;
    /* The real refresh scene owner is required by the HUD transition. */
    if (global_refresh_record_peek() == NULL) return 0;
    coord[0] = (uint32_t)cell_xy[0];
    coord[1] = (uint32_t)cell_xy[1];
    cmd_produce(3, NULL, 0, NULL);
    media_action_feedback_sound_dispatch(4);
    command_context_post_to_hud(7, coord);
    return 1;
}

/* The frame UI arbiter owns the complete gesture and admits its release.
 * No catalog tuple, synthetic ray, cell picker fallback or forced route. */
int selection_pointer_world_dispatch(GameContext* ctx,
    EngineContext* engine, InputPointerSample* sample, int width, int height)
{
    EntitySlot* hit;
    float cell_x, cell_y;
    int signed_cell_x, signed_cell_y;
    if (sample == NULL || sample->domain != INPUT_POINTER_WORLD ||
        sample->transition != PLATFORM_MOUSE_UP || sample->admitted ||
        (sample->button != PLATFORM_MOUSE_LEFT &&
         sample->button != PLATFORM_MOUSE_RIGHT)) return 0;
    /* Consume even a miss before any picker/queue side effect. A second
     * producer cannot reinterpret this physical edge later in the tick. */
    sample->admitted = 1;
    if (ctx == NULL || ctx->level == NULL || engine == NULL ||
        engine->level_block != ctx->level || engine->context_mode != 1 ||
        (engine->state & (0x100u | 0x100000u | 0x08000000u)) != 0u ||
        ((engine_feature_word_peek() & 0x4000u) != 0u &&
         sample->button != PLATFORM_MOUSE_LEFT) || width <= 0 || height <= 0)
        return 0;
    hit = level_scene_pick_entity(ctx, ctx->level, width, height,
                                  sample->x, sample->y, 30.0f);
    if (hit != NULL) {
        selection_clear(ctx);
        return selection_add(ctx, hit);
    }
    if (!level_scene_mouse_cell(ctx, ctx->level, width, height,
                                sample->x, sample->y, &cell_x, &cell_y) ||
        !(cell_x >= 0.0f && cell_y >= 0.0f &&
          cell_x < (float)level_width(ctx->level) &&
          cell_y < (float)level_height(ctx->level))) return 0;
    /* Projection returns fractional corner-based grid coordinates. Convert
     * only validated hits to containing signed cells, never world XYZ or a
     * catalog type/id. Existing queue/route owners keep all later authority. */
    signed_cell_x = (int)cell_x;
    signed_cell_y = (int)cell_y;
    {
        int cell_xy[2];
        int result;
        cell_xy[0] = signed_cell_x;
        cell_xy[1] = signed_cell_y;
        result = selection_wall_release(ctx, engine, cell_xy);
        if (result >= 0) return result;
        result = selection_floor_release(ctx, engine, cell_xy);
        if (result >= 0) return result;
    }
    /* Other existing world modes are not covered by the floor slice. */
    if (terrain_cell_command_predicate(engine, signed_cell_x,
                                       signed_cell_y) != 0) return 0;
    return selection_cmd_move(ctx, (float)signed_cell_x, (float)signed_cell_y);
}
