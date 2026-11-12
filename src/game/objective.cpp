// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/string.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "level.hpp"
#include "game.hpp"
#include "objective.hpp"
#include "entity.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include "savegame.hpp"
#include "mission.hpp"
#include "gods98/message.hpp"
#include "simulation_ui_ports.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "effects.hpp"
#include "util.hpp"
#include "crt/crt.hpp"
#include "objective_shared.hpp"
#include "objective_for_reward.hpp"

ObjectiveDeferredCallback s_objective_deferred_callback;
static AdvisorTimerActionFn s_advisor_timer_action;
static AdvisorTimerFinalizeFn s_advisor_timer_finalize;
static void* s_advisor_timer_user;
LevelState* s_objective_active_level;
char s_objective_voice_alias[256];
int s_objective_voice_alias_pending;
int s_objective_voice_stream_handle = -1;
int s_objective_voice_result_active;
int s_objective_tick_resolved;
static AdvisorAnimationPositionUpdateState s_advisor_position_update_state;
static AdvisorPositionUpdateValueResolveFn s_advisor_position_value_resolver;
static void* s_advisor_position_value_resolver_user;
static CoordinateListHeadGetHook s_coordinate_list_head_get;
static void* s_coordinate_list_head_context;
RewardTaskResourceRecord s_reward_task_resources[10];
void objective_advisor_clear_active_state(void);

void advisor_sound_alias_set(int slot)
{
    /* DAT_004a6768 receives the value from the configured advisor sound
     * table, whose entries are sound-bank slots (the 0x47b8e0 and 0x47ad70
     * callers consume it as a slot), not SoundAliasPlay IDs. */
    s_advisor_sound_slot = slot;
}

void coordinate_list_visit_hooks_set(CoordinateListHeadGetHook head_get,
                                     void* context)
{
    s_coordinate_list_head_get = head_get;
    s_coordinate_list_head_context = context;
}

/* CoordinateListCallbackVisit (FUN_00456cb0): resolve the current list,
 * visit each {x,y,next} record, and stop as soon as the visitor returns 0. */
void coordinate_list_callback_visit(void* list_key,
                                    CoordinateListVisitCallback callback,
                                    void* user)
{
    struct CoordinateListNode {
        int x;
        int y;
        CoordinateListNode* next;
    };
    void* record;
    if (callback == NULL) return;
    if (s_coordinate_list_head_get != NULL) {
        record = s_coordinate_list_head_get(
            list_key, s_coordinate_list_head_context);
    }
    else {
        const unsigned int list_index = (unsigned int)(uintptr_t)list_key;
        GameContext* const game = entity_pool_callback_context_get();
        LevelState* const level = game != NULL && game->level != NULL
            ? game->level : s_objective_active_level;
        if (level == NULL || list_index >= 0x37u)
            return;
        record = level->pool_heads[list_index];
    }
    while (record != NULL) {
        CoordinateListNode* const node = (CoordinateListNode*)record;
        if (!callback(record, node->x, node->y, user)) break;
        record = node->next;
    }
}

void reward_task_resource_slot_set(unsigned int slot, uint32_t flags,
                                   void* image_surface, void* font_record)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].flags = flags;
    s_reward_task_resources[slot].image_surface = image_surface;
    s_reward_task_resources[slot].font_record = font_record;
}

void reward_task_font_record_set(unsigned int slot, void* font_record)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].font_record = font_record;
    if (font_record != NULL) s_reward_task_resources[slot].flags |= 1u;
    else s_reward_task_resources[slot].flags &= ~1u;
}

void* reward_task_font_record_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].font_record : NULL;
}

void* reward_task_image_surface_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].image_surface : NULL;
}

void reward_task_box_image_surface_set(unsigned int slot, void* surface)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].box_image_surface = surface;
}

void* reward_task_box_image_surface_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].box_image_surface : NULL;
}

void objective_deferred_callback_set(ObjectiveDeferredCallback callback)
{
    s_objective_deferred_callback = callback;
}

void advisor_timer_hooks_set(AdvisorTimerActionFn action,
                             AdvisorTimerFinalizeFn finalize, void* user)
{
    s_advisor_timer_action = action;
    s_advisor_timer_finalize = finalize;
    s_advisor_timer_user = user;
}

void advisor_timer_array_advance_and_advisor_clear(AdvisorTimerState* state,
                                                   float delta)
{
    float advance;
    int i;
    if (state == NULL) return;
    advance = delta * 1000.0f * 0.04f;
    for (i = 0; i < 4; ++i) state->timers[i] += advance;
    if (state->countdown == 0.0f) return;
    if (state->countdown >= 0.0f) {
        if (state->advisor_active == 0 || state->hold_countdown != 0) {
            state->countdown -= advance;
            if (state->countdown > 0.0f) goto finalize;
        }
        else {
            if (state->stored_toggle != 0 &&
                ((state->advisor_scroll > 0 && state->final_timer != 0.0f) ||
                 state->final_timer == 0.0f)) {
                if (s_advisor_timer_action != NULL)
                    s_advisor_timer_action(state->update_pending, 1,
                                           s_advisor_timer_user);
            }
            state->stored_toggle = state->update_pending;
            if (state->update_pending != 0) goto finalize;
        }
    }
    state->countdown = 0.0f;
finalize:
    if (state->finalize_pending != 0 && state->countdown < 1000.0f &&
        s_advisor_timer_finalize != NULL)
        s_advisor_timer_finalize(s_advisor_timer_user);
}

int s_objective_announce_target_gate = 1;

/* Explicit-level adapter used by port call sites that already own the level
 * pointer. It shares state transitions but does not claim the target ABI. */
void objective_announce(LevelState* s, int who)
{
    int previous_gate = s_objective_announce_target_gate;
    if (s == NULL) return;
    s_objective_active_level = s;
    s_objective_announce_target_gate = 0;
    objective_announce_current(who);
    s_objective_announce_target_gate = previous_gate;
}

void objective_active_level_bind(LevelState* s)
{
    s_objective_active_level = s;
}

void objective_active_level_clear(LevelState* s)
{
    if (s_objective_active_level == s) s_objective_active_level = NULL;
}

int objective_presentation_kind(const LevelState* s)
{
    if (s == NULL || s != s_objective_active_level)
        return OBJECTIVE_PRESENTATION_NONE;
    if ((s_objective_result_word & 1u) != 0u)
        return OBJECTIVE_PRESENTATION_BRIEFING;
    if ((s_objective_result_word & 6u) == 0u)
        return OBJECTIVE_PRESENTATION_NONE;
    if ((s_objective_result_word & 2u) != 0u)
        return OBJECTIVE_PRESENTATION_ACHIEVED;
    return (s_objective_result_word & 0x100u) != 0u
        ? OBJECTIVE_PRESENTATION_CRYSTAL_FAILED : OBJECTIVE_PRESENTATION_FAILED;
}

int objective_briefing_active(const LevelState* s)
{
    return objective_presentation_kind(s) == OBJECTIVE_PRESENTATION_BRIEFING;
}

static void objective_presentation_cleanup(LevelState* s)
{
    objective_advisor_clear_active_state();
    if (s_objective_voice_stream_handle != -1) {
        (void)sound_stream_stop_and_release(0);
        s_objective_voice_stream_handle = -1;
    }
    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
    sound_deferred_queue_drain(0);
    if (system_state_flags_get() == 0u)
        engine_debug_word_bit_set(0x1000u, 0);
    s_objective_voice_alias_pending = 0;
    s_objective_voice_alias[0] = '\0';
    s_objective_voice_result_active = 0;
    s_objective_tick_resolved = 1;
    s->obj_stream[0] = '\0';
}

int objective_briefing_acknowledge(LevelState* s)
{
    if (!objective_briefing_active(s)) return 0;
    (void)global_value_004ff9bc_store_and_return(&s->objective_pending);
    s_objective_result_word &= ~1u;
    s->objective &= ~LVL_OBJECTIVE_STILL_PENDING;
    s->objective_pending = 0;
    if ((engine_debug_word_peek() & 0x800u) != 0u) {
        media_cd_watchdog_update(1);
        engine_debug_word_bit_set(0x800u, 0);
    }
    objective_presentation_cleanup(s);
    return 1;
}

int objective_presentation_acknowledge(LevelState* s)
{
    int kind = objective_presentation_kind(s);
    if (kind == OBJECTIVE_PRESENTATION_NONE) return 0;
    if (kind == OBJECTIVE_PRESENTATION_BRIEFING)
        return objective_briefing_acknowledge(s);
    /* ObjectivePoll selects the terminal page independently, but rechecks
     * the entity gate immediately before releasing the final page. */
    if (!entity_flag_mask_callbacks_all_pass(0x40bu)) return 0;
    if (kind == OBJECTIVE_PRESENTATION_ACHIEVED) {
        s_objective_result_word &= ~2u;
        s->objective &= ~LVL_OBJECTIVE_ACHIEVED;
    } else {
        s_objective_result_word &= ~0x104u;
        s->objective &= ~(LVL_OBJECTIVE_FAILED | LVL_OBJECTIVE_CRYSTALS);
    }
    objective_presentation_cleanup(s);
    return 1;
}

const char* objective_voice_alias_get(void)
{
    return s_objective_voice_alias;
}

int objective_voice_alias_pending_take(void)
{
    const int pending = s_objective_voice_alias_pending;
    s_objective_voice_alias_pending = 0;
    return pending;
}

const char* objective_stream(const LevelState* s)
{
    return s != NULL ? s->obj_stream : "";
}

int objective_advisor_shown(const LevelState* s)
{
    if (s == NULL || s->obj_stream[0] == '\0') {
        return 0;
    }
    /* The corpus honours the outcome's suppression bit (0x10 generic /
     * 0x20 achieved / 0x80 failed) before showing the hint. */
    uint32_t suppress = LVL_OBJECTIVE_NO_ADVISOR;
    if (s->objective & LVL_OBJECTIVE_ACHIEVED) {
        suppress |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (s->objective & LVL_OBJECTIVE_FAILED) {
        suppress |= LVL_OBJECTIVE_NO_FAIL_ADV;
    }
    return (s->objective & suppress) == 0 ? 1 : 0;
}

/* Explicit-level adapter used by the portable frame loop. */
int objective_tick(LevelState* s, float dt)
{
    objective_tick_target(NULL, s, dt, dt);
    return s_objective_tick_resolved;
}

/* --- the advisor action window band (see objective.hpp) ------------------- */

/* Both former copies of DAT_004b31bc now live in Advisor.c's globals. */
void objective_advisor_clear_active_state(void)
{
    advisor_clear_active_state();
}

void advisor_scene_ready_set(int ready)
{
    engine_context_mode_global_set(ready);
}

void advisor_animation_position_update_state_set(
    const AdvisorAnimationPositionUpdateState* state)
{
    if (state == NULL) {
        memset(&s_advisor_position_update_state, 0,
               sizeof(s_advisor_position_update_state));
        return;
    }
    if (state->action_row < 0 || state->action_row >= ADV_ACTION_ROWS ||
        state->point_count > ADVISOR_POSITION_UPDATE_MAX_POINTS) return;
    s_advisor_position_update_state = *state;
}

void advisor_animation_position_update_value_resolver_set(
    AdvisorPositionUpdateValueResolveFn resolver, void* user)
{
    s_advisor_position_value_resolver = resolver;
    s_advisor_position_value_resolver_user = user;
}

/* AdvisorActionIndicatorRequest (FUN_0041e890).  The reference jump table
 * has one category for action 0, a second for these action IDs, and the
 * default category for every other value, including out-of-range values. */
int advisor_action_indicator_request(int action, int request_mode)
{
    static const unsigned char category5[69] = {
        0,1,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,0,0,0,1,0,1,
        1,1,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,1,1,1,1,0,
        1,1,0,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1
    };
    EventTimerState frontend_state;
    frontend_event_timer_state_get(&frontend_state);
    frontend_state.flags |= 0x20u;
    if (request_mode != 0) frontend_state.flags |= 0x40u;
    frontend_event_timer_state_set(&frontend_state);
    s_advisor_position_update_state.action_row = action == 0 ? 4 :
        (action > 0 && action < 69 && category5[action] ? 5 : 3);
    s_advisor_position_update_state.trigger_selector = action;
    return 1;
}

/* AdvisorAnimationPositionAndRequestUpdate (FUN_0041c4f0). The live mode,
 * interaction word, scroll/lower offsets, and back-button extents come from
 * their existing owners. The point table and row/trigger tuple are the four
 * unowned DAT inputs represented by the explicit bounded state above. */
// FUNCTION: LEGORR 0x0041c4f0
static void advisor_animation_position_and_request_update_internal(
    int position_mode, int input_a, int input_b, const float* position,
    const TextureSurface* image, int typed)
{
    EventTimerState frontend_state;
    const AdvisorAnimationPositionUpdateState* state =
        &s_advisor_position_update_state;
    int matched = 0;
    int use_catalog_point = 0;
    int resolved_value = 0;
    float offset_x, offset_y;
    int width = 0, height = 0;
    float new_x, new_y;
    float old_x, old_y;

    frontend_event_timer_state_get(&frontend_state);
    if ((frontend_state.flags & 0x20u) == 0u ||
        global_flag_bit0_is_set() != 0u) return;

    if (state->trigger_selector == 0x49) {
        if (state->cached_input_a == input_a &&
            state->cached_input_b == input_b &&
            (typed || (s_advisor_position_value_resolver != NULL &&
            s_advisor_position_value_resolver(
                0x49, input_a, input_b, &resolved_value,
                s_advisor_position_value_resolver_user)))) {
            matched = 1;
        }
    }
    else if (position_mode == state->trigger_selector) {
        matched = 1;
        if (position_mode == 0) {
            use_catalog_point = 1;
        }
        else if (!typed && (s_advisor_position_value_resolver == NULL ||
            !s_advisor_position_value_resolver(
                position_mode, input_a, input_b, &resolved_value,
                s_advisor_position_value_resolver_user))) {
            matched = 0;
        }
    }
    if (!matched) return;

    if (use_catalog_point) {
        const int mode = (int)s_obj_sel_word;
        uint32_t point_index;
        if (mode < 0 || mode > 0x12) return;
        point_index = typed ? (uint32_t)entity_query_flag_count(mode) :
                              resource_handle_set_band_word_peek(mode);
        if (typed) {
            const FeSurroundImage* surround;
            float origin[2];
            /* Count zero aliases pointer storage in the executable. */
            if (point_index == 0u || point_index > 11u) return;
            surround = interface_surround_image_peek((int)point_index - 1);
            if (surround == NULL) return;
            interface_menu_action_layout_peek(origin);
            (void)interface_back_button_dimensions(&width, &height);
            offset_x = (frontend_state.flags & 0x80u) != 0u
                ? frontend_state.timer : frontend_state.lower;
            new_x = (float)surround->width + offset_x + (float)width * 0.5f;
            new_y = (float)surround->height + origin[1] + (float)height * 0.5f;
        }
        else {
            if (point_index >= state->point_count ||
                point_index >= ADVISOR_POSITION_UPDATE_MAX_POINTS) return;
            (void)interface_back_button_dimensions(&width, &height);
            offset_x = (frontend_state.flags & 0x80u) != 0u
                ? frontend_state.timer : frontend_state.lower;
            offset_y = state->vertical_origin;
            new_x = state->points[point_index].x + offset_x +
                    (float)width * 0.5f;
            new_y = state->points[point_index].y + offset_y +
                    (float)height * 0.5f;
        }
    }
    else {
        /* The decompile's zero mapped-value arm uses a fixed 40-pixel
         * height. A nonzero value is a pointer-like UI record whose +8
         * height has no live owner in the portable frontend. */
        if ((!typed && resolved_value != 0) || position == NULL) return;
        new_x = position[0];
        if ((frontend_state.flags & 0x80u) == 0u)
            new_x = frontend_state.lower;
        new_y = (typed && image != NULL ? (float)image->height * 0.5f
                                         : 20.0f) + position[1];
    }

    if (state->action_row < 0 || state->action_row >= ADV_ACTION_ROWS) return;
    advisor_action_position_get(state->action_row, &old_x, &old_y);
    old_x += new_x;
    old_y += new_y;
    advisor_action_position_and_mode_set(state->action_row, 0x0c,
                                          old_x, old_y);
    (void)advisor_request_admit(state->action_row,
                                (frontend_state.flags & 0x40u) != 0u);
    frontend_state.flags &= ~0x20u;
    frontend_event_timer_state_set(&frontend_state);
}

void advisor_animation_position_and_request_update_image(
    int position_mode, int input_a, int input_b, const float* position,
    const TextureSurface* image)
{
    advisor_animation_position_and_request_update_internal(
        position_mode, input_a, input_b, position, image, 1);
}

void advisor_animation_position_and_request_update(
    int position_mode, int input_a, int input_b, const float* position)
{
    advisor_animation_position_and_request_update_internal(
        position_mode, input_a, input_b, position, NULL, 0);
}


typedef struct ObjectiveAdvisorResourceToken {
    const char* name;
    uintptr_t handle;
} ObjectiveAdvisorResourceToken;

/* The reference's DAT_00555da0 buffer owns every text/name pointer below.
 * Keep one copied buffer and grow the three tables as their entries appear. */
static char* s_adv_token_source;
const char** s_adv_text_tokens;
uint32_t s_adv_text_token_count;
static ObjectiveAdvisorResourceToken* s_adv_image_tokens;
static uint32_t s_adv_image_token_count;
static ObjectiveAdvisorResourceToken* s_adv_sound_tokens;
static uint32_t s_adv_sound_token_count;
static ObjectiveAdvisorTokenReleaseFn s_adv_token_release;
static void* s_adv_token_release_user;
static uint32_t s_adv_token_generation;

/* Compatibility table for the old 32-bit pointer-return interface and its
 * test installer. Parsed callers should use objective_advisor_token_text_get
 * on 64-bit hosts, where a native pointer cannot fit in the target dword. */
// GLOBAL: LEGORR 0x00555da8
const uint32_t* s_adv_tokens;
// GLOBAL: LEGORR 0x00555da4
uint32_t s_adv_token_count;

void objective_advisor_tokens_release(void)
{
    uint32_t i;
    ++s_adv_token_generation;
    if (s_adv_token_generation == 0u) ++s_adv_token_generation;
    if (s_adv_token_release != NULL) {
        for (i = 0; i < s_adv_image_token_count; ++i) {
            if (s_adv_image_tokens[i].handle != (uintptr_t)0)
                s_adv_token_release(1, s_adv_image_tokens[i].handle,
                                    s_adv_token_release_user);
        }
        for (i = 0; i < s_adv_sound_token_count; ++i) {
            if (s_adv_sound_tokens[i].handle != (uintptr_t)0)
                s_adv_token_release(2, s_adv_sound_tokens[i].handle,
                                    s_adv_token_release_user);
        }
    }
    free(s_adv_token_source);
    free(s_adv_text_tokens);
    free(s_adv_image_tokens);
    free(s_adv_sound_tokens);
    s_adv_token_source = NULL;
    s_adv_text_tokens = NULL;
    s_adv_text_token_count = 0u;
    s_adv_image_tokens = NULL;
    s_adv_image_token_count = 0u;
    s_adv_sound_tokens = NULL;
    s_adv_sound_token_count = 0u;
    s_adv_token_release = NULL;
    s_adv_token_release_user = NULL;
    s_adv_tokens = NULL;
    s_adv_token_count = 0u;
}

static char* objective_advisor_next_line(char* source, uint32_t length,
                                         uint32_t* cursor)
{
    uint32_t start;
    while (*cursor < length && (unsigned char)source[*cursor] < 0x20u) {
        source[*cursor] = '\0';
        ++*cursor;
    }
    if (*cursor >= length) return NULL;
    start = *cursor;
    while (*cursor < length && (unsigned char)source[*cursor] >= 0x20u) {
        if (source[*cursor] == '_') source[*cursor] = ' ';
        ++*cursor;
    }
    if (*cursor < length) {
        source[*cursor] = '\0';
        ++*cursor;
    }
    return source + start;
}

static int objective_advisor_text_append(const char* text)
{
    const char** grown;
    if (s_adv_text_token_count == 0xffffffffu / sizeof(*grown)) return 0;
    grown = (const char**)realloc((void*)s_adv_text_tokens,
        (size_t)(s_adv_text_token_count + 1u) * sizeof(*grown));
    if (grown == NULL) return 0;
    s_adv_text_tokens = grown;
    s_adv_text_tokens[s_adv_text_token_count++] = text;
    return 1;
}

static int objective_advisor_resource_append(
    ObjectiveAdvisorResourceToken** table, uint32_t* count,
    const char* name, uintptr_t handle)
{
    ObjectiveAdvisorResourceToken* grown;
    if (*count == 0xffffffffu / sizeof(*grown)) return 0;
    grown = (ObjectiveAdvisorResourceToken*)realloc(*table,
        (size_t)(*count + 1u) * sizeof(*grown));
    if (grown == NULL) return 0;
    *table = grown;
    grown[*count].name = name;
    grown[*count].handle = handle;
    ++*count;
    return 1;
}

int objective_advisor_tokens_parse(const char* source, uint32_t source_length,
                                   ObjectiveAdvisorTokenResolveFn resolve,
                                   ObjectiveAdvisorTokenReleaseFn release,
                                   void* user)
{
    uint32_t cursor = 0u;
    if (source == NULL && source_length != 0u) return 0;
    objective_advisor_tokens_release();
    s_adv_token_release = release;
    s_adv_token_release_user = user;
    if (source_length == 0u) return 1;
    s_adv_token_source = (char*)malloc((size_t)source_length + 1u);
    if (s_adv_token_source == NULL) {
        objective_advisor_tokens_release();
        return 0;
    }
    memcpy(s_adv_token_source, source, source_length);
    s_adv_token_source[source_length] = '\0';

    while (cursor < source_length) {
        char* line = objective_advisor_next_line(s_adv_token_source,
                                                  source_length, &cursor);
        int kind = 0;
        if (line == NULL) break;
        if (*line == ':' || *line == '$') {
            const char* name = line + 1;
            char* value_line;
            uintptr_t handle;
            kind = (*line == ':') ? 1 : 2;
            value_line = objective_advisor_next_line(s_adv_token_source,
                                                      source_length, &cursor);
            if (value_line == NULL || *name == '\0' || *value_line == '\0' ||
                resolve == NULL) continue;
            handle = resolve(kind, value_line, user);
            if (handle == 0u) continue;
            if (kind == 1) {
                if (!objective_advisor_resource_append(&s_adv_image_tokens,
                        &s_adv_image_token_count, name, handle)) {
                    if (release != NULL) release(kind, handle, user);
                    continue;
                }
            } else {
                if (!objective_advisor_resource_append(&s_adv_sound_tokens,
                        &s_adv_sound_token_count, name, handle)) {
                    if (release != NULL) release(kind, handle, user);
                    continue;
                }
            }
        } else if (*line != '\0') {
            if (!objective_advisor_text_append(line)) {
                objective_advisor_tokens_release();
                return 0;
            }
        }
    }
    s_adv_token_count = s_adv_text_token_count;
    return 1;
}

void objective_advisor_tokens_install(const uint32_t* table, uint32_t count)
{
    objective_advisor_tokens_release();
    s_adv_tokens = table;
    s_adv_token_count = count;
}

const char* objective_advisor_token_text_get(uint32_t index)
{
    return (index < s_adv_text_token_count && s_adv_text_tokens != NULL)
        ? s_adv_text_tokens[index] : NULL;
}

uint32_t objective_advisor_image_token_count(void)
{
    return s_adv_image_token_count;
}

const char* objective_advisor_image_token_name_get(uint32_t index)
{
    return (index < s_adv_image_token_count && s_adv_image_tokens != NULL)
        ? s_adv_image_tokens[index].name : NULL;
}

uintptr_t objective_advisor_image_token_handle_get(uint32_t index)
{
    return (index < s_adv_image_token_count && s_adv_image_tokens != NULL)
        ? s_adv_image_tokens[index].handle : 0u;
}

void* objective_advisor_image_token_resource_get(uint32_t index)
{
    return (void*)objective_advisor_image_token_handle_get(index);
}

uint32_t objective_advisor_sound_token_count(void)
{
    return s_adv_sound_token_count;
}

const char* objective_advisor_sound_token_name_get(uint32_t index)
{
    return (index < s_adv_sound_token_count && s_adv_sound_tokens != NULL)
        ? s_adv_sound_tokens[index].name : NULL;
}

uintptr_t objective_advisor_sound_token_handle_get(uint32_t index)
{
    return (index < s_adv_sound_token_count && s_adv_sound_tokens != NULL)
        ? s_adv_sound_tokens[index].handle : 0u;
}

uint32_t objective_advisor_token_generation_get(void)
{
    return s_adv_token_generation;
}

/* ObjectiveSidecarAppendGate (FUN_0043bc40). The entity's stats accessors,
 * activity, owner, carried-object, selection, task, and OSF state are live in
 * the portable model. Scene-object creation/labels remain renderer-owned. */
static void objective_sidecar_activity_set(EntitySlot* entity, int activity)
{
    ai_set_activity(entity, activity, 0);
    ai_idle_chain_record_attach(entity);
    entity->activity_last = entity->activity;
    entity->activity_timer = 0.0f;
}

static void objective_sidecar_owner_detach(GameContext* ctx,
                                           EntitySlot* entity)
{
    EntitySlot* owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL) return;
    selection_target_command_update(ctx, entity);
    owner->flags_3e4 &= ~8u;
    owner->owner_36c = NULL;
    entity->owner_36c = NULL;
}

// FUNCTION: LEGORR 0x0043bc40
int objective_sidecar_append_gate(EntitySlot* entity)
{
    GameContext* ctx;
    LevelState* level;
    uint32_t stats_flags;
    int cell_x = 0;
    int cell_y = 0;
    int cell_xy[2];
    int cell_valid = 0;
    EntitySlot* owner;

    if (entity == NULL) return 0;
    ctx = entity->owner_ctx;
    if (ctx == NULL) ctx = entity_pool_callback_context_get();
    level = ctx != NULL ? ctx->level : NULL;

    if (entity->type != 4
        && ((entity->flags_3e4 & 0x08000000u) != 0u
            || (entity->flags_3e0 & 0x00200000u) != 0u)) return 0;
    if (entity->field_340 > 0.0f) return 0;

    stats_flags = entity_stats_word_get(entity);

    /* FUN_0043bc40 first completes a linked +0x3cc child when the source
     * stats word has one of the three documented state groups. EntitySlot's
     * spawn_child_3cc mirror owns that exact link. */
    if ((stats_flags & 0x80000070u) != 0u &&
        entity->spawn_child_3cc != NULL &&
        (entity->spawn_child_3cc->flags_3e0 & 0x00200000u) != 0u) {
        EntitySlot* child = entity->spawn_child_3cc;
        child->field_340 = -1.0f;
        child->flags_3e0 &= ~0x00200000u;
        entity_kind_record_update_dispatch(entity);
        ai_set_activity(child, 0, 1);
        activity_register_bookkeeping(child);
    }

    /* The first linked-activity path reads the pointer at +0x3cc. That field
     * is represented by spawn_child_3cc; it is distinct from carried-object
     * (+0x308) and reciprocal pair (+0x3c0) links. */

    if ((entity->flags_3e8 & 0x00800000u) != 0u) {
        objective_sidecar_owner_detach(ctx, entity);
        if (entity->type == 2 && (entity->flags_3ec & 8u) != 0u
            && ctx != NULL) {
            osf_add(ctx, (int32_t)entity->flags_3f0,
                    (int32_t)entity->indexed_record_slot_330,
                    entity->name);
        }
        /* FUN_0046cf90 is a resource/display callback with no game-side
         * equivalent. The remaining spawn/finalize/teardown operations are
         * represented by existing module APIs. */
        if (entity->type == 1) {
            /* Deliberately no guessed replacement for FUN_0046cf90. */
        }
        lego::entity_kind_one_random_spawn_trigger(entity);
        if (entity->type != 5) {
            if (entity->type == 4)
                entity_destroy_tracked_pointer_cleanup(entity);
            else
                (void)entity_teardown(entity);
            return 1;
        }
        entity_type5_finalize_if_allowed(entity);
        return 1;
    }

    if (entity->type == 3) {
        if ((entity_state_flags_get(entity) & 0x8000u) == 0u) {
            if ((entity_state_flags_get(entity) & 0x00100000u) == 0u)
                return 0;
            entity_state_reset(entity, 0, 1);
            entity->flags_3e0 |= 0x00020000u;
            objective_sidecar_activity_set(entity, 0x0d);
            return 1;
        }
        if ((entity->flags_3e0 & 0x00100000u) != 0u) return 0;
        entity_action_route_state_initialize(entity);
        return 1;
    }

    /* The type-4 path branch reads the dword at +0x390 and the byte at
     * +0x3ec. Retail applies the BuildingTypes cell-line effects only when
     * both veto bits are clear, then marks the one-shot bit before calling
     * 0x409d50. */
    if (entity->type == 4 && (entity->flags_390 & 4u) == 0u &&
        (entity->flags_3ec & 8u) == 0u) {
        if ((entity->flags_3e4 & 0x00100000u) != 0u) return 0;
        entity->flags_3e4 |= 0x00100000u;
        unported_ledger_stubs::entity_cell_path_effects_apply_current(entity);
        objective_sidecar_activity_set(entity, 0x2a);
        task_registry_remove_type10_entries_for_entity(entity);
        return 1;
    }

    if (entity->type != 1 && entity->type != 2 && entity->type != 0xb
        && entity->type != 4 && entity->type != 6) return 0;
    if (entity->type == 6) {
        entity->flags_3e8 |= 0x00800000u;
        return 1;
    }

    if ((entity->flags_3e8 & 0x00040000u) == 0u
        || (entity->flags_3e0 & 0x00400000u) != 0u) return 0;

    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    cell_valid = level != NULL;
    if ((entity->flags_3ec & 8u) != 0u)
        lego::entity_kind_one_random_spawn_trigger(entity);

    if ((entity->flags_3ec & 0x80u) != 0u) {
        owner = (EntitySlot*)entity->owner_36c;
        if (owner != NULL) owner->flags_3ec &= ~0x80u;
        entity->owner_36c = NULL;
    }

    if (entity->owner_36c != NULL && cell_valid) {
        EntitySlot* carried = (EntitySlot*)entity->carry_link_308;
        owner = (EntitySlot*)entity->owner_36c;
        if ((stats_flags & 1u) != 0u
            && (entity->flags_3e0 & 0x400u) != 0u
            && carried != NULL
            && terrain_cell_route_and_occupancy_check(
                   carried, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            carried->field_340 = -1.0f;
        }
        if ((stats_flags & 0x20000u) == 0u
            || terrain_cell_route_and_occupancy_check(
                   owner, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            objective_sidecar_owner_detach(ctx, entity);
        }
        else {
            owner->field_340 = -1.0f;
        }
    }

    if (entity->type == 0xb)
        terrain_entity_cell_record_remove(entity);

    if (cell_valid) {
        cell_xy[0] = cell_x;
        cell_xy[1] = cell_y;
        if (entity->type == 2 || entity->type == 1 || entity->type == 4)
            message_dispatch(0x26, 0, 0, cell_xy);
        if (entity->type == 2)
            message_dispatch(10, 0, 0, cell_xy);
        else if (entity->type == 1)
            message_dispatch(0xb, 0, 0, cell_xy);
        else if (entity->type == 4)
            message_dispatch(0xc, 0, 0, cell_xy);
    }

    if ((entity->flags_3e4 & 2u) == 0u) {
        entity_state_reset(entity, 0, 1);
        entity->activity_last = entity->activity;
    }
    else {
        entity_state_reset(entity, 1, 1);
    }
    entity->flags_3e0 |= 0x00400000u;
    /* Renderer sidecar allocation, transform copy, and labels are absent
     * from GameContext. The final selection detach is directly represented. */
    (void)selection_entity_remove(entity);
    selection_object_detach(entity);
    return 1;
}


