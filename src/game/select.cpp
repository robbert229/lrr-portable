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
#include "select_from_legoobject.hpp"
#include "select_from_message_unit.hpp"

static GameContext* s_selection_active_context;

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

int selection_count(const GameContext* ctx)
{
    return ctx != NULL ? ctx->selection_count : 0;
}

int selection_has_any(const GameContext* ctx)
{
    return ctx != NULL && ctx->selection_count != 0;
}

EntitySlot* const* selection_record_array(const GameContext* ctx)
{
    return ctx != NULL ? ctx->selection : NULL;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

EntitySlot* selection_first(const GameContext* ctx)
{
    if (ctx == NULL || ctx->selection_count == 0) {
        return NULL;
    }
    return ctx->selection[0];
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void selection_cmd_add(GameContext* ctx, EntitySlot* e)
{
    if (ctx != NULL && e != NULL) {
        cmd_produce(1, e, 1, NULL);
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int selection_contains_entity(const GameContext* ctx, const EntitySlot* e)
{
    uint32_t index;
    return selection_contains_entity_target(ctx, e, &index);
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
#endif /* !LEGORRR_BACKEND_WINAPI */

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
