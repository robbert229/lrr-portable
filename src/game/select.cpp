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
EntitySlot* selection_active_entity_get(void)
{
    __asm {
        mov eax, dword ptr [s_selection_active_count]
        mov ecx, dword ptr [s_selection_active_records]
        neg eax
        sbb eax, eax
        and eax, ecx
        ret
    }
}
#else
EntitySlot* selection_active_entity_get(void)
{
    return s_selection_active_count != 0u
        ? s_selection_active_records[0] : NULL;
}
#endif

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
