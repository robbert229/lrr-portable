// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-027 Message.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-027.csv. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
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
#include <stdlib.h>
#include <math.h>
#include "effects.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include "util.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "port_loop_ports.hpp"
#include "gods98/string.hpp"
#include "cmd_shared.hpp"
#include <float.h>
#include <stdint.h>
#include <ctype.h>
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "weapon.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "creature_scene.hpp"
#endif
#include "gods98/model.hpp"
#include "gods98/resource.hpp"
#include "upgrades.hpp"
#include "cmd_for_message_unit.hpp"

#define CMD_QUEUE_CAP 0x800

typedef struct QueueRec {
    int        opcode;
    const void* a1;    /* arg1: entity ptr for entity ops */
    int        a2;
    int        a3;
    int        a4;
} QueueRec;

// GLOBAL: LEGORR 0x004eaf20
static QueueRec s_queue[2][CMD_QUEUE_CAP];
// GLOBAL: LEGORR 0x004fef20
int      s_queued_count[2];
// GLOBAL: LEGORR 0x004fef28
int      s_ping; /* active producer block (DAT_004fef28) */

// GLOBAL: LEGORR 0x004fef2c
EntitySlot* s_selection_active_records[SELECTION_MAX];
// GLOBAL: LEGORR 0x004ff298
uint32_t s_selection_active_count;

/* --- random-cell producer / entity remove (see cmd.hpp) ----------------------
 * VC5 rand.obj supplies the shared stream in the classic x86 build; other
 * builds keep this subsystem's local LCG model. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_cmd_rand_state = 1;
#endif

namespace unported_ledger_stubs {
int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int cmd_rand15(void);
static void* selection_entity_type_component(const EntitySlot* e);
static const void* cmd_entity_owner_at(const EntitySlot* e);

/* --- queue-adjacent leaves (see cmd.hpp) -----------------------------------
 * CommandQueueCancelOpcode: corpus shape — the active bank only,
 * swap-with-last removals, forward index never rechecking the slot a
 * swap landed on (so a swapped-in match stays if it lands behind the
 * cursor; the loop bound is the running count).
 * The comparison uses the same low 32-bit word as the 32-bit corpus even on
 * the native host, where entity pointers can be wider. */
// FUNCTION: LEGORR 0x00451f60
void cmd_queue_cancel_match(int value)
{
    const int block = s_ping;
    int count = s_queued_count[block];
    unsigned i = 0;
    while ((int)i < count) {
        if ((uint32_t)(uintptr_t)s_queue[block][i].a1 == (uint32_t)value) {
            count = count - 1;
            s_queue[block][i] = s_queue[block][count];
            s_queued_count[block] = count;
        }
        i = i + 1;
    }
}
// FUNCTION: LEGORR 0x00452010
void cmd_bind_key(int key, int op, int a1, int a2, int a3, int a4)
{
    if (s_keybind_count >= KEYBIND_MAX) {
        return;
    }
    KeyBind* b = &s_keybinds[s_keybind_count++];
    b->key = key & 0xff;
    b->op = op; b->a1 = a1; b->a2 = a2; b->a3 = a3; b->a4 = a4;
}
// FUNCTION: LEGORR 0x00452060
void cmd_produce(int op, const void* a1, int a2, const int* pair)
{
    int block = s_ping;
    if ((unsigned)s_queued_count[block] >= CMD_QUEUE_CAP) {
        return; /* queue full: silent drop (FUN_00452060) */
    }
    QueueRec* q = &s_queue[block][s_queued_count[block]++];
    /* b63 note: an explicit zero-fill phase + single pair null-test was
     * tried (corpus 0x452092..0x4520ba shape) and measured LOWER — the
     * toolchain's store/register scheduling diverges; the fused writes
     * below are the measured-best shape. */
    q->opcode = op;
    q->a1 = a1;
    q->a2 = a2;
    q->a3 = (pair != NULL) ? pair[0] : 0;
    q->a4 = (pair != NULL) ? pair[1] : 0;
}

// FUNCTION: LEGORR 0x004520d0
void cmd_queue_tick(void)
{
    int i;
    /* 1. Producer block: the held-key pre-pass (SelectionRoot step 1 —
     *    cheat-keybinds; the demo binds keys via cmd_bind_key). */
    uint32_t* engine_state = engine_current_state_word();
    if ((engine_state == NULL ||
         (*engine_state & ENGINE_STATE_TIME_FROZEN) == 0u) &&
        s_ctx != NULL && s_ctx->input_keyheld != NULL) {
        cmd_keybind_tick(s_ctx->input_keyheld);
    }

    /* 2. Remap the first dword of every queued record while the old bank
     *    is still active. This is FUN_0045d870 over each 0x14-byte record. */
    int block = s_ping;
    int count = s_queued_count[block];
    for (i = 0; i < count; ++i) {
        QueueRec* q = &s_queue[block][i];
        command_entity_reference_remap((uint32_t*)q);

    }

    /* 3. Retire the old producer bank, flip the ping-pong selector, and
     *    clear the newly active bank before dispatching the saved count. */
    s_queued_count[block] = 0;
    s_ping = !s_ping;
    s_queued_count[s_ping] = 0;

    /* 4. The opcode switch (finding 06/10 §"The queue executor"). */
    for (i = 0; i < count; ++i) {
        QueueRec* q = &s_queue[block][i];
        EntitySlot* ent = (EntitySlot*)q->a1;
        switch (q->opcode) {
        case 5:
        case 6:
            cmd_create_move(ent, (const float*)&q->a3, q->a2);
            break;
        case 8:
            selection_drag_area_command_create(selection_active_records_get(),
                selection_active_count_get(), &q->a3, q->a2);
            break;
        case 0x17:
        case 0x1d:
            cmd_create_task(ent);
            break;
        case 0x19:
            cmd_create_gated(ent, (void*)(uintptr_t)q->a2);
            break;
        case 0x2b:
            cmd_create_target(ent, (void*)(uintptr_t)q->a2);
            break;
        case 1:
            selection_candidate_filter_default(ent, q->a2);
            break;
        case 3:
            selection_clear(s_ctx);
            break;
        case 4:
            selection_entity_remove(ent);
            break;
        case 9:
            selection_command_dispatch(q->a2);
            break;
        case 0xb:
            media_min_distance_update_mode(1, NULL, 0);
            break;
        case 0xc:
            sound_alias_play(q->a2, 0);
            break;
        case 10:
            /* GlobalRenderRecordSetFromInput also needs its active view and
             * renderer hooks; the queue entry supplies neither. */
            break;
        case 0x13:
            /* CmdQueueFlush calls TerrainStampImpactHandler(a3, a4,
             * low16(a2), high16(a2)); the helper now reads the active level
             * and terrain owners directly. */
            (void)terrain_stamp_impact_handle_current(
                (uint32_t)q->a3, (uint32_t)q->a4,
                (int)((uint32_t)q->a2 & 0xffffu),
                (int)((uint32_t)q->a2 >> 16));
            break;
        case 0x15:
        case 0x1b:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            (void)entity_build_placement_command_setup(
                cell_xy, 0, NULL, 1, q->opcode == 0x1b);
            break;
        }
        case 0x1f:
        {
            GameContext* context = pending_scan_context_get();
            LevelState* level = context != NULL ? context->level : NULL;
            int owner_index;
            /* The reference ignores the queue payload here: it takes the
             * type-3 entity index from level +0xb0, uses that same index as
             * the spawned entity's owner, and samples the active terrain's
             * width and height. */
            if (level != NULL) {
                owner_index = (int)active_engine_field_b0_get(level);
                if (owner_index >= 0 &&
                    entity_at(context, 3, owner_index) != NULL) {
                    (void)unported_ledger_stubs::
                        emergence_random_spawn_attempt(owner_index,
                            owner_index, level_width(level),
                            level_height(level));
                }
            }
            break;
        }
        case 0x21:
            (void)lego::entity_conditional_type_five_spawn(ent);
            break;
        case 0x23:
            entity_random_valid_cell_command_create(ent);
            break;
        case 0x25:
            (void)unported_ledger_stubs::
                ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
                    ent, (EntitySlot*)(uintptr_t)q->a2);
            break;
        case 0x2d: selection_state_tick(s_ctx); break;
        case 0x34:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            entity_build_placement_command_pair_create(cell_xy, 0);
            break;
        }
        case 0x36:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            entity_build_placement_command_batch_expand(cell_xy);
            break;
        }
        case 0x40:
            (void)selected_entity_state_monitor();
            break;
        default:
            /* SelectionRoot has no default dispatch branch. */
            break;
        }
    }
}

// FUNCTION: LEGORR 0x00452430
void entity_random_valid_cell_command_create(void* entity)
{
    const LevelState* lvl = (s_ctx != NULL) ? s_ctx->level : NULL;
    const int w = (lvl != NULL) ? level_width(lvl) : 0;
    const int h = (lvl != NULL) ? level_height(lvl) : 0;
    if (w <= 0 || h <= 0) {
        return; /* deviation: the corpus modulos zero on an absent level */
    }
    int x = 0, y = 0;
    int found = 0;
    for (unsigned tries = 0; tries < 1000u; ++tries) {
        x = cmd_rand15() % w;
        y = cmd_rand15() % h;
        if (level_cell_flag_8_bounded(lvl, x, y) != 0u) {
            found = 1;
            break;
        }
    }
    if (found != 0) {
        /* FUN_00452060 copies the two optional words from a stack pair. */
        int pair[2];
        pair[0] = x;
        pair[1] = y;
        cmd_produce(0x24, entity, 0, pair);
    }
}

/* SelectionObjectDetach (FUN_004524b0): cancel queued commands for the
 * object, remove it from the selected set, and clear the current selection
 * context when it refers to that object. */
// FUNCTION: LEGORR 0x004524b0
void selection_object_detach(void* entity)
{
    if (entity == NULL) {
        return;
    }
    cmd_queue_cancel_match((int)(uintptr_t)entity);
    (void)selection_entity_remove(entity);
    if (media_min_distance_context() == entity) {
        media_min_distance_update_mode(1, NULL, 0);
    }
}
// FUNCTION: LEGORR 0x00452520
int selection_snapshot_create(void** array_out, unsigned* count_out)
{
    const int count = selection_count(s_ctx);
    if (count == 0) {
        return 0;
    }
    if (count_out != NULL) {
        *count_out = (unsigned)count;
    }
    void* copy = engine_heap_alloc((long)count * (long)sizeof(void*));
    if (array_out != NULL) {
        *array_out = copy;
    }
    if (copy != NULL) {
        EntitySlot* const* src = selection_record_array(s_ctx);
        const unsigned n = (unsigned)count & 0x3fffffffu;
        for (unsigned i = 0; i < n; ++i) {
            ((void**)copy)[i] = src[i];
        }
    }
    return 1;
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

// FUNCTION: LEGORR 0x004525b0
int selection_active_has_any_get(void)
{
    return s_selection_active_count != 0u;
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

// FUNCTION: LEGORR 0x00452650
int cmd_array_find_value_index(const int* array, unsigned count, int value,
                               unsigned* index_out)
{
    if (count != 0) { /* deviation: the corpus tests the count first */
        if (array != NULL) { /* pinned anti-crash guard — the corpus
                              * dereferences blindly */
            for (unsigned i = 0; i != count; ++i) {
                if (array[i] == value) {
                    if (index_out != NULL) {
                        *index_out = i;
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
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

/* SelectionCandidateFilterDefault (FUN_004526c0) is the exact mode-1
 * forwarding wrapper around SelectionCandidateFilter. */
// FUNCTION: LEGORR 0x004526c0
void selection_candidate_filter_default(void* entity, int mode)
{
    (void)selection_candidate_filter(entity, mode, 1);
}

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

/* SelectionCandidateEligibilityCheck (FUN_00452870, findings 77/85/468):
 * corpus order kept — state gate, shared predicate, then the clear-bit
 * band; the byte-read of +0x3e4's low byte (& 10 decimal) maps to the
 * flag-word mask 0xA. */
// FUNCTION: LEGORR 0x00452870
int selection_candidate_eligibility_check(const EntitySlot* e)
{
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
#endif
    if ((e->flags_3e8 & 0x100u) == 0u) {
        return 0;
    }
    const int shared_eligible = entity_shared_eligibility_check(e, 1);
    if ((shared_eligible != 0)
        && ((uint8_t)e->flags_3e4 & 0x0Au) == 0u /* corpus: low byte & 10 */
        && (e->flags_3e0 & 0x24000000u) == 0u
        && ((e->type != 1)
            || (e->flags_3e0 & 0x00040000u) == 0u)) {
        return 1; /* type 1 has the additional 0x40000 rejection (finding 468). */
    }
    return 0;
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

// FUNCTION: LEGORR 0x00452cc0
int selected_entity_state_monitor(void)
{
    const int count = (s_ctx != NULL) ? s_ctx->selection_count : 0;
    /* Faithful corpus shape: the counter is not used inside the loop —
     * every pass inspects the first selection entry. */
    for (int i = 0; i < count; ++i) {
        EntitySlot* head = (count > 0)
            ? (EntitySlot*)selection_record_array(s_ctx)[0]
            : NULL;
        if (head == NULL) {
            continue;
        }
        if ((entity_stats_word_get(head) & 0x20000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)head->owner_36c;
            if (owner != NULL) {
                owner->flags_3e4 &= 0xfffffff7u;
                owner->owner_36c = NULL;
            }
        }
        if (head->type == 4) {
            /* FUN_00409ff0 replaces tracked object pointers before teardown. */
            entity_destroy_tracked_pointer_cleanup(head);
        }
        else {
            if (head->type == 0xb) {
                /* FUN_0040d0a0 removes type-11 objects from the cell chain. */
                terrain_entity_cell_record_remove(head);
            }
            (void)entity_teardown(head);
        }
    }
    return count;
}
// FUNCTION: LEGORR 0x00452d60
int selection_command_dispatch(int param)
{
    const void* sel = media_min_distance_context();
    if (sel != NULL) {
        const EntitySlot* s = (const EntitySlot*)sel;
        if ((s->flags_3e8 & 0x10u) != 0u
            && (s->type == 2 || cmd_entity_owner_at(s) != NULL)) {
            media_min_distance_update_mode(0, (void*)s, param);
            return 1;
        }
    }
    const int count = selection_count(s_ctx);
    for (int i = 0; i < count; ++i) {
        const EntitySlot* e = selection_record_array(s_ctx)[i];
        if (e == NULL) {
            continue; /* deviation: the corpus dereferences blindly */
        }
        if (e->type == 2 && cmd_entity_owner_at(e) != NULL) {
            e = (const EntitySlot*)cmd_entity_owner_at(e);
        } /* (the corpus checks the swapped-in entity's own owner word) */
        if ((e->flags_3e8 & 0x10u) != 0u
            && (e->type == 2 || cmd_entity_owner_at(e) != NULL)) {
            media_min_distance_update_mode(0, (void*)e, param);
            return 1;
        }
    }
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

/* --- selection snapshot / dispatch (see cmd.hpp) -----------------------------
 * The owner read takes the EntitySlot's +0x36c mirror (pointer-sized; the
 * corpus reads the flat 32-bit word), and the flag reads take the
 * flags_3e8 mirror's low byte as the corpus's byte read. */
static const void* cmd_entity_owner_at(const EntitySlot* e)
{
    return e->owner_36c;
}

static int cmd_rand15(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_cmd_rand_state = s_cmd_rand_state * 214013u + 2531011u;
    return (int)((s_cmd_rand_state >> 16) & 0x7fffu);
#endif
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the debug-key binder (00452010) takes five words; the port's entry takes six. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (cmd_bind_key)
extern "C" void lego_key_callback_set_adapter(int key, int op, int a1, int a2, int a3)
{
    cmd_bind_key(key, op, a1, a2, a3, 0);
}
