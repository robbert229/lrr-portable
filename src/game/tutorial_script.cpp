// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-028 GAME-028: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-028.csv. */
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
#include "objective_for_tutorial_script.hpp"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "heap.hpp"
#include "crt/crt_for_tutorial_script.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "terrain_feedback.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_for_tutorial_script.hpp"
#include <stdint.h>
#include <limits.h>
#include "gods98/camera.hpp"
#include "type_config_ports.hpp"
#include "upgrades.hpp"
#include "scene_render.hpp"
#include "level_for_tutorial_script.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "tickers.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "scene_pool.hpp"
/* GAME-021 lego_unit.c reads this word (C linkage, ADR 0005 step 6). */
extern "C" uint32_t s_overlay_word_03610;
#include "engine_for_tutorial_script.hpp"
#include "message_for_tutorial_script.hpp"
#include "record_for_tutorial_script.hpp"
extern "C" int tutorial_script_file_load_target(const char* path);
#include "tutorial_hud_for_tutorial_script.hpp"

/* DeferredCallbackSet (FUN_00453900) — zero-init module static (the
 * corpus's DAT_004a673c presence word). */
// GLOBAL: LEGORR 0x004a673c
uint32_t s_deferred_callback_presence;

// GLOBAL: LEGORR 0x004a6768
int s_advisor_sound_slot = -1;

// GLOBAL: LEGORR 0x004ff958
static uint32_t s_tutorial_4ff958_state[11];

/* --- the remaining unowned DAT one-liners (see crt.hpp) ---------------------
 * Zero-initialized module statics per the s_system_state precedent — the
 * globals' meanings are unresolved (finding 383) and none has a ported
 * feeder. */
// GLOBAL: LEGORR 0x004ff978
uint32_t s_flag_004ff978 = 0u;

// GLOBAL: LEGORR 0x004ff980
uint32_t      s_system_state = 0u;   /* DAT_004ff980 */

/* --- unowned DAT-global counter/accumulator band (see crt.hpp) -----------
 * None of these globals has a ported owner or feeder, so each lands as a
 * zero-initialized module static (the s_system_state precedent). */
// GLOBAL: LEGORR 0x004ff990
uint32_t s_counter_004ff990 = 0u;

// GLOBAL: LEGORR 0x004ff9bc
uint32_t             s_objective_switch = 0u; /* DAT_004ff9bc */

extern uint32_t s_objective_switch;

/* NERP callback metadata is the third dword of each 12-byte callback-table
 * row at 0x004a5948. The retail table has 293 rows. Metadata determines
 * argument count and whether the result feeds a condition; keep the bytecode
 * cursor aligned even for callbacks whose effects have not been ported yet. */
static const uint8_t s_tutorial_command_kinds[293] = {
    3,0,0,0,0,0,0,0,3,3,3,3,0,0,0,0,
    5,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,0,0,0,0,4,4,4,0,4,4,4,4,0,4,
    4,0,4,0,0,0,0,4,4,4,4,0,0,0,0,0,
    0,0,0,0,4,5,4,4,4,0,4,4,5,0,4,5,
    0,0,0,0,0,4,4,0,0,4,4,0,6,1,5,1,
    5,0,1,4,4,0,4,4,0,0,1,0,4,4,0,5,
    0,0,4,4,0,4,4,0,5,5,4,3,4,4,4,1,
    0,0,0,0,0,4,4,0,4,4,0,4,0,4,4,4,
    0,0,4,0,4,0,4,4,4,0,0,4,4,4,5,4,
    4,0,1,0,4,0,4,0,4,0,4,3,0,0,5,4,
    0,0,0,0,0,1,0,1,3,4,3,3,3,3,4,0,
    0,0,4,4,4,0,4,4,0,4,4,0,4,4,0,4,
    0,0,0,4,4,4,4,4,4,4,4,4,0,0,0,0,
    0,0,0,0,0,4,0,4,0,4,4,4,3,4,4,0,
    4,4,0,4,4,1,4,0,4,4,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0
};

static uint32_t tutorial_command_kind_default(uint32_t command);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void objective_advisor_live_token_release(int kind, uintptr_t handle, void* user);
static uintptr_t objective_advisor_live_token_resolve(int kind, const char* value, void* user);
static int tutorial_command_invoke(uint32_t command, const uint32_t* args, uint32_t argc, uint32_t* result);
static uint32_t tutorial_compare_values(uint32_t op, uint32_t previous, uint32_t current);
static int tutorial_camera_focus_target_apply(GameContext* game, EngineContext* engine, uint32_t target_index);
static int advisor_inline_name_equal(const char* a, const char* b);
static void advisor_inline_output_char(char* output, uint32_t output_size, uint32_t* used, char value);

/* TutorialNerpScriptFileLoad @ 0x00452df0. The two global clear bands have
 * exact extents of 45 and 11 dwords; retain their unmapped fields as raw
 * sidecars while keeping the observed engine calls in their original order. */
// FUNCTION: LEGORR 0x00452df0
int tutorial_script_file_load_target(const char* path)
{
    uint32_t size = 0u;
    void* data = NULL;
    int mode = 1;
    uint32_t* state_word = s_tutorial_555d40_state;
    int state_count = 45;
    for (; state_count != 0; --state_count) *state_word++ = 0u;
    ui_message_overlay_state_initialize();
    state_word = s_tutorial_4ff958_state;
    state_count = 11;
    for (; state_count != 0; --state_count) *state_word++ = 0u;
    if (s_tutorial_script_ops.mode_set != NULL)
        s_tutorial_script_ops.mode_set(mode, s_tutorial_script_ops.context);
    else
        (void)engine_mode_set_target(&mode);
    if (path != NULL) {
        if (s_tutorial_script_ops.load != NULL)
            data = s_tutorial_script_ops.load(
                path, &size, s_tutorial_script_ops.context);
        else {
            int32_t byte_size = 0;
            data = read_whole_file(path, &byte_size);
            if (byte_size > 0) size = (uint32_t)byte_size;
        }
    }
    s_tutorial_script_data = data;
    s_tutorial_script_size = size;
    return data != NULL;
}

// FUNCTION: LEGORR 0x00452e70
int objective_advisor_token_file_load(const char* path)
{
    Resource* resource;
    int32_t size;
    int result;
    objective_advisor_tokens_release();
    if (path == NULL || path[0] == '\0') return 0;
    resource = resource_open(path, RESOURCE_AUTO);
    if (resource == NULL) return 0;
    size = resource_read(resource);
    if (size < 0) {
        resource_free(resource);
        return 0;
    }
    result = objective_advisor_tokens_parse(
        (const char*)resource->data, (uint32_t)size,
        objective_advisor_live_token_resolve,
        objective_advisor_live_token_release, NULL);
    resource_free(resource);
    return result;
}

// FUNCTION: LEGORR 0x00453200
int objective_advisor_token_lookup(uint32_t index)
{
    if (index < s_adv_token_count && s_adv_tokens != NULL) {
        return (int)s_adv_tokens[index];
    }
    if (index < s_adv_text_token_count && s_adv_text_tokens != NULL)
        return (int)(uint32_t)(uintptr_t)s_adv_text_tokens[index];
    return 0;
}

// FUNCTION: LEGORR 0x00453220
int tutorial_display_resources_release(void)
{
    tutorial_script_data_release(s_tutorial_script_data);
    s_tutorial_script_data = NULL;
    s_tutorial_script_size = 0;
    tutorial_script_program_clear();
    memset(s_tutorial_nerp_registers, 0, sizeof(s_tutorial_nerp_registers));
    memset(&s_nerp_runtime, 0, sizeof(s_nerp_runtime));
    tutorial_camera_focus_state_reset();
    tutorial_scroll_runtime_state_reset();
    if (s_tutorial_engine_context != NULL &&
        s_tutorial_engine_context->views[0] != NULL) {
        s_tutorial_engine_context->views[0]->target_record = NULL;
        s_tutorial_engine_context->views[0]->follow_id = 0;
    }
    objective_advisor_tokens_release();
    tutorial_tokens_clear();
    if (s_tutorial_script_ops.resource_state_release != NULL)
        s_tutorial_script_ops.resource_state_release(
            s_tutorial_script_ops.context);
    /* AdvisorDisplayStateResourcesRelease reinitializes the shared message
     * overlay after clearing the tutorial-owned resource tables. */
    ui_message_overlay_state_initialize();
    return 1;
}

// FUNCTION: LEGORR 0x004532e0
void tutorial_indirect_handle_resolve(uint32_t* value)
{
    uint32_t index;
    uint32_t kind;
    uint32_t resolved;
    if (value == NULL || (*value & 0xffff0000u) != 0x00020000u) return;
    index = *value & 0xffffu;
    kind = s_tutorial_script_ops.command_kind != NULL
        ? s_tutorial_script_ops.command_kind(
            index, s_tutorial_script_ops.context)
        : tutorial_command_kind_default(index);
    if (kind != 0u) return;
    if (s_tutorial_script_ops.indirect_resolve != NULL)
        *value = s_tutorial_script_ops.indirect_resolve(
            index, s_tutorial_script_ops.context);
    else if (s_tutorial_script_ops.command != NULL &&
             s_tutorial_script_ops.command !=
                 tutorial_builtin_command_adapter)
        *value = s_tutorial_script_ops.command(
            index, NULL, 0u, s_tutorial_script_ops.context);
    else if (tutorial_builtin_command(index, NULL, 0u, &resolved))
        *value = resolved;
}

// FUNCTION: LEGORR 0x00453320
void tutorial_script_command_dispatch(float delta)
{
    uint32_t pc = 0u;
    uint32_t previous = 0;
    uint32_t last_result = 0u;
    uint32_t compare_op = 0;
    int negate_next = 0;
    if (s_tutorial_script_data == NULL && s_tutorial_program_count == 0u)
        return;
    while (pc < s_tutorial_program_count) {
        uint32_t word = s_tutorial_program_words[pc];
        uint32_t command = word & 0xffffu;
        uint32_t kind = 0u;
        uint32_t argc = 0u;
        uint32_t args[3];
        uint32_t result = 0u;
        uint32_t i;
        uint32_t step = 1u;
        if ((word & 0x20000u) == 0u) {
            if ((word & 0x10000u) != 0u) {
                negate_next = 0;
                if (command == 0u) {
                    /* Preserve the accumulated predicate and select AND. */
                    compare_op = 1u;
                } else if (command == 1u) {
                    negate_next = 1;
                } else if (command == 2u) {
                    compare_op = 2u;
                } else if (command >= 5u && command <= 10u) {
                    compare_op = command - 2u;
                }
            } else if ((word & 0x40000u) != 0u) {
                previous = 0u;
                compare_op = 0u;
                negate_next = 0;
            } else if ((word & 0x80000u) != 0u) {
                if (previous != 0u) {
                    pc = command < s_tutorial_program_count
                        ? command : s_tutorial_program_count;
                }
                previous = 0u;
                compare_op = 0u;
                negate_next = 0;
            } else {
                uint32_t current = (uint32_t)(int32_t)(int16_t)command;
                if (negate_next) current = current == 0u;
                last_result = current;
                previous = compare_op != 0u
                    ? tutorial_compare_values(compare_op, previous, current)
                    : current;
                negate_next = 0;
                compare_op = 1u;
            }
            ++pc;
            continue;
        }
        if (command == 0u && (previous != 0u || compare_op == 0u)) break;
        kind = s_tutorial_script_ops.command_kind != NULL
            ? s_tutorial_script_ops.command_kind(command,
                                                  s_tutorial_script_ops.context)
            : tutorial_command_kind_default(command);
        if (kind <= 2u) {
            if (kind == 1u) argc = 1u;
            else if (kind == 2u) argc = 2u;
            for (i = 0u; i < argc; ++i) {
                args[i] = pc + 1u + i < s_tutorial_program_count
                    ? s_tutorial_program_words[pc + 1u + i] : 0u;
                tutorial_indirect_handle_resolve(&args[i]);
            }
            step += argc;
            (void)tutorial_command_invoke(command,
                argc == 0u ? NULL : args, argc, &result);
            if (negate_next) result = result == 0u;
            last_result = result;
            previous = compare_op != 0u
                ? tutorial_compare_values(compare_op, previous, result)
                : result;
            negate_next = 0;
            compare_op = 1u;
        } else if (kind >= 3u && kind <= 6u) {
            argc = kind - 3u;
            for (i = 0u; i < argc; ++i) {
                args[i] = pc + 1u + i < s_tutorial_program_count
                    ? s_tutorial_program_words[pc + 1u + i] : 0u;
                if (previous != 0u)
                    tutorial_indirect_handle_resolve(&args[i]);
            }
            step += argc;

            if (previous != 0u) {
                (void)tutorial_command_invoke(command,
                    argc == 0u ? NULL : args, argc, &result);
            }
            previous = 0u;
            compare_op = 0u;
            negate_next = 0;
        } else {
            ++pc;
            continue;
        }
        pc += step;
    }
    /* This dispatcher receives simulation seconds. Only the camera's
     * reference-shaped tween consumes the original 25-Hz frame units. */
    tutorial_camera_focus_tween_update(view_camera_delta_from_seconds(delta));
}

/* CameraFocusScrollScaleCommit (FUN_004538a0) — see engine.hpp. The corpus
 * reads the level-state root global and strides both windows blindly. */
// FUNCTION: LEGORR 0x004538a0
int camera_focus_scroll_scale_commit(const EngineContext* ctx,
                                     const void* record)
{
    if (record == NULL) {
        return 0; /* deviation: the corpus strides the record blindly */
    }
    uint32_t focus = 0, guard = 0;
    __builtin_memcpy(&focus, (const char*)record + 0x00, sizeof focus);
    __builtin_memcpy(&guard, (const char*)record + 0x04, sizeof guard);
    if (focus == 0) {
        const EngineState* st = ctx != NULL ? ctx->state_block : NULL;
        if (st != NULL && st->focus_gate_27c != 0) {
            return 0;
        }
    }
    global_scroll_guard_set((int)guard);
    scroll_scale_set_and_clamp((float)focus * 0.01f);
    global_scroll_guard_set((int)guard);
    return 0;
}

// FUNCTION: LEGORR 0x00453900
void deferred_callback_set(uint32_t value)
{
    s_deferred_callback_presence = value;
}

// FUNCTION: LEGORR 0x00453910
void objective_deferred_callback_invoke_if_set(void)
{
    if (s_objective_deferred_callback != NULL)
        s_objective_deferred_callback();
}

// FUNCTION: LEGORR 0x00453920
void tutorial_advisor_scroll_step_advance(void)
{
    static const uint32_t scroll_scale_record[2] = { 100u, 0u };
    tutorial_message_generation_sync();
    media_action_feedback_sound_dispatch(0);
    if (s_nerp_runtime.message_offset != 0u) {
        --s_nerp_runtime.message_offset;
        if ((int32_t)s_nerp_runtime.message_offset > 3)
            s_nerp_runtime.message_offset = 3u;
        if ((int32_t)s_nerp_runtime.message_offset >
            (int32_t)s_nerp_runtime.message_maximum)
            s_nerp_runtime.message_offset = s_nerp_runtime.message_maximum;
        {
            uint32_t index = s_nerp_runtime.message_maximum -
                             s_nerp_runtime.message_offset;
            const char* token = tutorial_objective_token_lookup(
                tutorial_message_history_get(index));
            advisor_text_request_setup_text(token, 0u);
        }
        if (s_nerp_runtime.message_offset == 0u) {
            engine_global_state_flags_bits6_and_9_set_target(0, 1);
            if (system_state_flags_get() != 0u)
                (void)camera_focus_scroll_scale_commit(
                    s_tutorial_engine_context, scroll_scale_record);
        }
        engine_global_state_flags_bit7_set_target(1);
    } else {
        if (s_nerp_runtime.message_pending_ack != 0u)
            ++s_nerp_runtime.message_ack_count;
        s_nerp_runtime.message_pending_ack = 0u;
    }
}
// FUNCTION: LEGORR 0x00453bb0
void tutorial_frame_timer_array_advance(float delta)
{
    const float timer_delta = delta * 1000.0f * 0.04f;
    uint32_t i;
    tutorial_message_generation_sync();
    for (i = 0u; i < 4u; ++i)
        s_tutorial_nerp_timers[i] += timer_delta;
    if (s_nerp_runtime.message_timer == 0.0f) return;
    if (s_nerp_runtime.message_timer >= 0.0f) {
        if (deferred_callback_peek() == 0u ||
            s_nerp_runtime.message_flags != 0u) {
            s_nerp_runtime.message_timer -= timer_delta;
            if (s_nerp_runtime.message_timer > 0.0f) goto message_tail;
        } else {
            if (s_nerp_runtime.message_previous_ack != 0u &&
                (((int32_t)s_nerp_runtime.message_offset > 0 &&
                  s_nerp_runtime.message_suppress_arrow != 0u) ||
                 s_nerp_runtime.message_suppress_arrow == 0u)) {
                engine_global_state_flags_bits6_and_9_set_target(
                    s_nerp_runtime.message_pending_ack != 0u, 1);
            }
            s_nerp_runtime.message_previous_ack =
                s_nerp_runtime.message_pending_ack;
            if (s_nerp_runtime.message_pending_ack != 0u) goto message_tail;
        }
    }
    s_nerp_runtime.message_timer = 0.0f;
message_tail:
    if (s_nerp_runtime.message_sound_active != 0u &&
        s_nerp_runtime.message_timer < 1000.0f)
        advisor_clear_active_state();
}

// FUNCTION: LEGORR 0x00453da0
void tutorial_camera_focus_tween_update(float delta)
{
    float scale = 1.0f;
    float step;
    if (delta <= 0.0f) return;
    if (s_tutorial_script_ops.delta_scale != NULL)
        scale = s_tutorial_script_ops.delta_scale(
            s_tutorial_script_ops.context);
    if (s_tutorial_focus_valid != 0) {
        if (!tutorial_camera_focus_target_apply(s_tutorial_game_context,
                                                s_tutorial_engine_context,
                                                s_tutorial_focus_id)) {
            s_tutorial_focus_valid = 0u;
        }
        if (s_tutorial_script_ops.camera_focus != NULL)
            s_tutorial_script_ops.camera_focus(
                s_tutorial_focus_id, 0.0f, 0.0f,
                s_tutorial_script_ops.context);
    }
    if (s_tutorial_focus_offset_active != 0) {
        step = s_tutorial_focus_offset_target / (delta * 25.0f);
        s_tutorial_focus_offset += step;
        if (s_tutorial_focus_offset_target - 5.0f < s_tutorial_focus_offset &&
            s_tutorial_focus_offset < s_tutorial_focus_offset_target + 5.0f) {
            s_tutorial_focus_offset_active = 0;
            s_tutorial_focus_offset = 0.0f;
        }
        if (s_tutorial_script_ops.camera_offset != NULL)
            s_tutorial_script_ops.camera_offset(
                step, s_tutorial_script_ops.context);
        else if (s_tutorial_engine_context != NULL)
            camera_zoom_update(
                s_tutorial_engine_context->camera_controller, step);
    }
    if (s_tutorial_focus_angle_active != 0) {
        float remaining = s_tutorial_focus_angle <= 0.0f
                        ? s_tutorial_focus_angle + s_tutorial_focus_angle_target
                        : s_tutorial_focus_angle - s_tutorial_focus_angle_target;
        float scaled = remaining * 0.05555556f * 0.33333334f * scale;
        s_tutorial_focus_angle_velocity += scaled;
        if ((s_tutorial_focus_angle_target > 0.0f &&
             s_tutorial_focus_angle_target - 1.0f <
                 s_tutorial_focus_angle_velocity) ||
            (s_tutorial_focus_angle_target < 0.0f &&
             s_tutorial_focus_angle_velocity <
                 s_tutorial_focus_angle_target + 1.0f)) {
            s_tutorial_focus_angle_active = 0;
            s_tutorial_focus_angle_velocity = 0.0f;
        }
        s_tutorial_focus_angle += scaled;
        if (s_tutorial_script_ops.camera_rotation != NULL)
            s_tutorial_script_ops.camera_rotation(
                scaled * 0.017453292f, s_tutorial_script_ops.context);
        else if (s_tutorial_engine_context != NULL) {
            float current = 0.0f;
            float next;
            memcpy(&current,
                   s_tutorial_engine_context->camera_controller + 0x84,
                   sizeof(current));
            next = current + scaled * 0.017453292f;
            camera_rotation_set(
                s_tutorial_engine_context->camera_controller, next);
        }
    }
    if (s_tutorial_script_ops.camera_state != NULL &&
        s_tutorial_script_ops.camera_state(s_tutorial_script_ops.context) != 0 &&
        s_tutorial_script_ops.camera_scroll != NULL)
        s_tutorial_script_ops.camera_scroll(100, 0.0f,
                                            s_tutorial_script_ops.context);
}

// FUNCTION: LEGORR 0x00454880
void global_counter_004ff990_increment(void)
{
    s_counter_004ff990 = s_counter_004ff990 + 1u;
}

// FUNCTION: LEGORR 0x00454890
void global_counter_004ff990_reset(void)
{
    s_counter_004ff990 = 0u;
}

// FUNCTION: LEGORR 0x00454b70
int objective_voice_alias_raise_commit(LevelState* s)
{
    if (s != NULL) s_objective_active_level = s;
    objective_announce_current(1);
    return 1;
}

/* ObjectiveVoiceAliasRaiseGateBit27 (FUN_00454b80) — see objective.hpp. */
// FUNCTION: LEGORR 0x00454b80
int objective_voice_alias_raise_gate_bit27(LevelState* s)
{
    if ((media_cd_flags_get() & 0x8000000u) == 0u) {
        if (s != NULL) s_objective_active_level = s;
        objective_announce_current(2);
    }
    return 1;
}

// FUNCTION: LEGORR 0x00456210
uint32_t flh_magic_uu_noop(void)
{
    return 0u;
}

// FUNCTION: LEGORR 0x00456240
uint32_t system_state_flags_get(void)
{
    return s_system_state;
}

// FUNCTION: LEGORR 0x00456550
uint32_t system_state_flags_set_target(const uint32_t* value)
{
    s_system_state = *value;
    return *value;
}

/* The mapped body takes only the mode pointer; keep the portable context
 * adapter separate to preserve that x86 argument layout. */
// FUNCTION: LEGORR 0x004565f0
int engine_mode_set_target(const int* mode)
{
    global_flag_004ff978_install((uint32_t)*mode);
    if (*mode == 1) {
        engine_global_state_flags_bits6_and_9_set_target(0, 1);
        engine_global_state_flags_bit7_set_target(0);
    }
    s_overlay_word_03610 = 0u;
    s_overlay_word_03614 = 0u;
    return *mode;
}

// FUNCTION: LEGORR 0x00456640
uint32_t global_flag_004ff978_get(void)
{
    return s_flag_004ff978;
}
/* Mapped target: stores the input dword globally and returns the same value;
 * the retail body dereferences the pointer without a null check. */
// FUNCTION: LEGORR 0x004567c0
uint32_t global_value_004ff9bc_store_and_return_target(const void* value)
{
    s_objective_switch = *(const uint32_t*)value;
    return *(const uint32_t*)value;
}

// FUNCTION: LEGORR 0x004567d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t objective_switch_state_get(void)
{
    __asm {
        mov eax, dword ptr [s_objective_switch]
        test eax, eax
        je objective_switch_state_get_false
        mov dword ptr [s_objective_switch], 0
        mov eax, 1
        ret
    objective_switch_state_get_false:
        xor eax, eax
        ret
    }
}
#else
uint32_t objective_switch_state_get(void)
{
    /* FUN_004567d0 consumes the pending state and returns a boolean. */
    if (s_objective_switch == 0u) return 0u;
    s_objective_switch = 0u;
    return 1u;
}
#endif

/* SoundAliasPlayGlobal (FUN_00456810) forwards the configured bank slot
 * directly to SoundBufferPlayWithPosition in global mode 2, with loop 0.
 * It does not run alias lookup or variant selection. */
// FUNCTION: LEGORR 0x00456810
void advisor_sound_alias_play_global(void)
{
    if (s_advisor_sound_slot != -1 && sound_bank_sound_on())
        (void)sound_bank_entry_play_mode(s_advisor_sound_slot, 0, 2);
}

// FUNCTION: LEGORR 0x00456830
void advisor_inline_token_render(char* source, char* output, int draw)
{
    const uint32_t output_size = 512u;
    uint32_t used = 0u;
    void* image_handles[20];
    uint32_t image_token_count = 0u;
    uint32_t i = 0u;
    uint32_t image_table_count = objective_advisor_image_token_count();
    uint32_t sound_table_count = objective_advisor_sound_token_count();
    uint32_t token_generation = objective_advisor_token_generation_get();
    float timer_values[3];
    float final_duration;
    tutorial_message_timer_values_get(timer_values);
    final_duration = timer_values[2];
    memset(image_handles, 0, sizeof(image_handles));
    if (output != NULL && output_size != 0u) output[0] = '\0';
    if (source == NULL) return;
    if (token_generation != s_advisor_inline_token_generation) {
        s_advisor_inline_token_generation = token_generation;
        s_advisor_inline_last_sound = sound_table_count;
    }

    /* With neither resource table initialized the retail helper takes its
     * memcpy path and leaves marker characters untouched. */
    if (image_table_count == 0u && sound_table_count == 0u) {
        while (source[i] != '\0')
            advisor_inline_output_char(output, output_size, &used,
                                       source[i++]);
        if (draw != 0) {
            s_advisor_inline_duration = final_duration;
            tutorial_message_duration_set(final_duration);
        }
        return;
    }

    while (source[i] != '\0') {
        char marker = source[i];
        char close = marker == '<' ? '>' : (marker == '#' ? '#' : '\0');
        if (close != '\0' && source[i + 1u] != '\0') {
            char name[256];
            uint32_t start = i + 1u;
            uint32_t end = start;
            uint32_t name_length;
            uint32_t entry;
            int matched = 0;
            while (source[end] != '\0' && source[end] != close &&
                   end - start < sizeof(name) - 1u) ++end;
            if (source[end] == close) {
                name_length = end - start;
                memcpy(name, source + start, name_length);
                name[name_length] = '\0';
                if (marker == '<') {
                    for (entry = 0u; entry < image_table_count; ++entry) {
                        const char* token_name =
                            objective_advisor_image_token_name_get(entry);
                        if (advisor_inline_name_equal(name, token_name)) {
                            void* handle =
                                objective_advisor_image_token_resource_get(entry);
                            if (image_token_count <
                                sizeof(image_handles) / sizeof(image_handles[0]))
                                image_handles[image_token_count] = handle;
                            if (image_token_count < 255u) ++image_token_count;
                            matched = 1;
                            break;
                        }
                    }
                } else {
                    for (entry = 0u; entry < sound_table_count; ++entry) {
                        const char* token_name =
                            objective_advisor_sound_token_name_get(entry);
                        if (advisor_inline_name_equal(name, token_name)) {
                            uintptr_t encoded_handle =
                                objective_advisor_sound_token_handle_get(entry);
                            if (entry != s_advisor_inline_last_sound) {
                                float duration = 0.0f;
                                int sound_slot = encoded_handle != 0u
                                    ? (int)(uint32_t)encoded_handle - 1 : -1;
                                s_advisor_inline_last_sound = entry;
                                if (s_advisor_inline_token_ops.sound_duration != NULL)
                                    duration =
                                        s_advisor_inline_token_ops.sound_duration(
                                            s_advisor_inline_token_ops.context,
                                            (uint32_t)sound_slot);
                                else if (sound_slot >= 0)
                                    duration = (float)
                                        sound_streamed_wave_buffer_step_rate(
                                            sound_slot);
                                final_duration = duration * timer_values[0] + timer_values[1];
                                if (sound_slot >= 0 && sound_bank_sound_on()) {
                                    sound_deferred_play_expire();
                                    (void)sound_deferred_timer_set_if_idle(
                                        duration * 25.0f);
                                    advisor_sound_alias_set(sound_slot);
                                    advisor_sound_alias_play_global();
                                    if (system_state_flags_get() == 0u)
                                        (void)advisor_request_admit(0x14, 1);
                                    tutorial_message_sound_active_set();
                                    if (s_advisor_inline_token_ops.sound_transition != NULL)
                                        s_advisor_inline_token_ops.sound_transition(
                                            s_advisor_inline_token_ops.context,
                                            (uint32_t)sound_slot,
                                            final_duration);
                                }
                            }
                            matched = 1;
                            break;
                        }
                    }
                }
                /* Both recognized and unrecognized markup spans are consumed
                 * by the retail scanner; only ordinary text is copied. */
                (void)matched;
                i = end + 1u;
                continue;
            }
            /* An unterminated span consumes the rest of the input. */
            break;
        }
        advisor_inline_output_char(output, output_size, &used, marker);
        ++i;
    }
    if (s_advisor_inline_token_ops.image_draw != NULL) {
        uint32_t draw_count = image_token_count;
        uint32_t draw_index;
        float center = s_advisor_inline_token_ops.image_x != 0.0f
            ? s_advisor_inline_token_ops.image_x : 260.0f;
        float y = s_advisor_inline_token_ops.image_y != 0.0f
            ? s_advisor_inline_token_ops.image_y : 386.0f;
        float image_step = s_advisor_inline_token_ops.image_step_x != 0.0f
            ? s_advisor_inline_token_ops.image_step_x : 40.0f;
        if (draw_count > sizeof(image_handles) / sizeof(image_handles[0]))
            draw_count = sizeof(image_handles) / sizeof(image_handles[0]);
        for (draw_index = 0u; draw_index < draw_count; ++draw_index) {
            if (image_handles[draw_index] != NULL)
                s_advisor_inline_token_ops.image_draw(
                    s_advisor_inline_token_ops.context,
                    image_handles[draw_index], draw_index,
                    center - (float)image_token_count * 20.0f +
                        (float)draw_index * image_step,
                    y);
        }
    }
    if (draw != 0) {
        s_advisor_inline_duration = final_duration;
        tutorial_message_duration_set(final_duration);
    }
}

/* TutorialPathCoordinateIndex (FUN_00456b80) — see level.hpp. The corpus
 * sweeps x-outer/y-inner over the grid dims and head-pushes each cell's
 * coordinate byte onto the chain table, clearing the byte as consumed. */
// FUNCTION: LEGORR 0x00456b80
void tutorial_path_coordinate_index(LevelState* s)
{
    if (s == NULL) {
        return; /* deviation: the corpus strides the block blindly */
    }
    for (int i = 0; i != 0x37; ++i) {
        s->pool_heads[i] = NULL;
    }
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if (level_load_ptr(s, LEVEL_OFF_VOXELS) == NULL) {
        return; /* deviation: the corpus strides the voxel global */
    }
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            uint8_t* cell = voxel_at(s, x, y) + 0x04;
            const int coord = *cell;
            if (coord != 0) {
                const uint32_t ux = (uint32_t)x;
                const uint32_t uy = (uint32_t)y;
                /* the corpus reads the superseded head and threads the
                 * node's next link after the allocation */
                void* old = (coord < 0x37) ? s->pool_heads[coord] : NULL;
                void* node = engine_heap_alloc(8u + sizeof(void*));
                if (node == NULL) {
                    continue; /* deviation: the corpus strides the alloc */
                }
                __builtin_memcpy((char*)node + 0x0, &ux, 4);
                __builtin_memcpy((char*)node + 0x4, &uy, 4);
                __builtin_memcpy((char*)node + 0x8, &old, sizeof(old));
                if (coord < 0x37) {
                    s->pool_heads[coord] = node;
                }
                *cell = 0;
            }
        }
    }
}

/* ObjectPoolListHeadGet (FUN_00456c30, finding 374): the corpus's exact
 * in-range bounds (-1 < index < 0x37) over the +0x114 slot table. */
// FUNCTION: LEGORR 0x00456c30
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int object_pool_list_head_get(const void* pools, int index)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        test eax, eax
        jl short object_pool_list_head_get_zero
        cmp eax, 37h
        jge short object_pool_list_head_get_zero
        mov ecx, dword ptr [esp + 4]
        mov edx, dword ptr [ecx + eax*4 + 114h]
        test edx, edx
        je short object_pool_list_head_get_zero
        mov eax, edx
        ret
    object_pool_list_head_get_zero:
        xor eax, eax
        ret
    }
}
#else
int object_pool_list_head_get(const void* pools, int index)
{
    if (-1 < index && index < 0x37) {
        const int32_t v = (int32_t)rec_dword(
            pools, (size_t)(0x114 + index * 4));
        if (v != 0) {
            return v;
        }
    }
    return 0;
}
#endif

/* ObjectPoolListsFreeAll (FUN_00456c60) — see record.hpp. The corpus reads
 * each head through the bounds-checked getter and strides the dword slot
 * blind for the clear; the port reads/clears the native link slot. */
// FUNCTION: LEGORR 0x00456c60
void object_pool_lists_free_all(void* pools)
{
    if (pools == NULL) {
        return; /* deviation: the corpus strides the global blindly */
    }
    for (int i = 0; i < 0x37; ++i) {
        const size_t slot = 0x114 + (size_t)i * sizeof(void*);
        /* The corpus calls the bounds-checked head getter for this read;
         * that carrier keeps the corpus-layout dword view (stride 4, a
         * 32-bit answer) and this table is the widened-link native image,
         * so the native slot is read in place. */
        void* node = NULL;
        __builtin_memcpy(&node, (char*)pools + slot, sizeof node);
        while (node != NULL) {
            void* next = *(void* const*)((const char*)node + 8);
            engine_heap_free(node);
            node = next;
        }
        *(void**)((char*)pools + slot) = NULL; /* blind clear */
    }
}

namespace unported_ledger_stubs {
// CoordinateListCallbackVisit
// FUNCTION: LEGORR 0x00456cb0
void ledger_stub_00456cb0_CoordinateListCallbackVisit(
    void* list_key, CoordinateListVisitCallback callback, void* user)
{
    coordinate_list_callback_visit(list_key, callback, user);
}
} // namespace unported_ledger_stubs

static uintptr_t objective_advisor_live_token_resolve(int kind,
                                                       const char* value,
                                                       void* user)
{
    (void)user;
    if (value == NULL || value[0] == '\0') return (uintptr_t)0;
    if (kind == 1) {
        TextureSurface* image = image_surface_load(value, 0u, 0u);
        return (uintptr_t)image;
    }
    if (kind == 2) {
        int slot = sound_bank_entry_load(value, 0, 0, 0);
        /* Zero is a valid bank slot; reserve encoded zero for a load miss. */
        return slot >= 0 ? (uintptr_t)(uint32_t)(slot + 1) : (uintptr_t)0;
    }
    return (uintptr_t)0;
}

static void objective_advisor_live_token_release(int kind, uintptr_t handle,
                                                  void* user)
{
    (void)user;
    if (handle == (uintptr_t)0) return;
    if (kind == 1) {
        texture_release((TextureSurface*)handle);
    }
    else if (kind == 2) {
        int slot = (int)(uint32_t)handle - 1;
        if (slot >= 0) (void)sound_bank_entry_release(slot);
    }
}

static int advisor_inline_name_equal(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return a == b;
    while (*a != '\0' && *b != '\0') {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
    }
    return *a == *b;
}

static void advisor_inline_output_char(char* output, uint32_t output_size,
                                       uint32_t* used, char value)
{
    if (output == NULL || output_size == 0u || used == NULL ||
        *used + 1u >= output_size) return;
    output[(*used)++] = value;
    output[*used] = '\0';
}

static uint32_t tutorial_command_kind_default(uint32_t command)
{
    return command < 293u ? s_tutorial_command_kinds[command] : 0u;
}
static uint32_t tutorial_compare_values(uint32_t op, uint32_t previous,
                                        uint32_t current)
{
    switch (op) {
    case 1: return previous != 0u && current != 0u;
    case 2: return previous != 0u || current != 0u;
    case 3: return current < previous;
    case 4: return previous < current;
    case 5: return previous == current;
    case 6: return current <= previous;
    case 7: return previous <= current;
    case 8: return previous != current;
    default: return current;
    }
}

static int tutorial_command_invoke(uint32_t command, const uint32_t* args,
                                   uint32_t argc, uint32_t* result)
{
    if (result == NULL) return 0;
    if (s_tutorial_script_ops.command != NULL) {
        *result = s_tutorial_script_ops.command(
            command, argc == 0u ? NULL : args, argc,
            s_tutorial_script_ops.context);
        return 1;
    }
    return tutorial_builtin_command(command,
        argc == 0u ? NULL : args, argc, result);
}

static int tutorial_camera_focus_target_apply(GameContext* game,
                                              EngineContext* engine,
                                              uint32_t target_index)
{
    EntitySlot* entity;
    int cell_x;
    int cell_y;
    int cell_xy[2];
    ViewportTargetClampState clamp_state;
    float world[3];
    if (game == NULL || engine == NULL || game->level == NULL ||
        target_index >= object_pointer_registry_count_get()) return 0;
    entity = object_pointer_registry_entity_get(target_index);
    if (entity == NULL) return 0;
    entity_position_to_terrain_cell(entity, game->level, &cell_x, &cell_y);
    cell_xy[0] = cell_x;
    cell_xy[1] = cell_y;
    memset(&clamp_state, 0, sizeof(clamp_state));
    clamp_state.map_width = level_width(game->level) - 1;
    clamp_state.map_height = level_height(game->level) - 1;
    clamp_state.edge_extent = view_grid_edge_get();
    clamp_state.global_flags = engine->state;
    /* 0x42de40 preserves an object's exact world point unless the map
     * inset actually changes its cell. Do not quantize scripted follow. */
    entity_transform_position_get(entity, world);
    if (viewport_target_position_clamp(&clamp_state, cell_xy) &&
        !terrain_cell_world_center_xy_get(game->level->ptr_terrain,
            clamp_state.target[0], clamp_state.target[1], world)) return 0;
    engine->state = clamp_state.global_flags;
    view_primary_target_request(engine, world[0], world[1], 1);
    return 1;
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00456240 (NERPFunc__GetTutorialFlags) takes the NERPs
 * parameter stack; the port's entry takes none. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (NERPFunc__GetTutorialFlags 00456240)
extern "C" uint32_t system_state_flags_get_original(const int* parameters)
{
    (void)parameters;
    return system_state_flags_get();
}
