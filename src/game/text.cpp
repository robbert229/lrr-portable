// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-045 Text.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-045.csv. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "game/savegame.hpp"
#include "engine.hpp"
#include "heap.hpp"
#include "input.hpp"
#include "message.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "message_shared.hpp"
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "gods98/surface.hpp"
#include "gods98/block.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "frontend_shared.hpp"
#include "camera.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "tickers.hpp"
#include "scene_resources.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/scene_render.hpp"
#include "scene_pool.hpp"
#include "engine_shared.hpp"

// GLOBAL: LEGORR 0x00503190
const char* s_ui_message_labels[26] = {
    "Text_Walk", "Text_Drill", "Text_NoDrill", "Text_CantDrill",
    "Text_Reinforce", "Text_NoReinforce", "Text_CantReinforce",
    "Text_PickupCrystal", "Text_Encyclopedia", "Text_RadarHelp",
    "Text_BuildHelp", "Text_SelectionHelp", "Text_Drive",
    "Text_CantDrive", "Text_MakeTeleporter", "Text_CannotPlaceBuilding",
    "Text_CrystalOreDisabled", "Text_CrystalFound", "Text_CavernDiscovered",
    "Text_OreFound", "Text_AirSupplyLow", "Text_AirSupplyRunningOut",
    "Text_SpaceToContinue", "Text_GameCompleted", "Text_ManTrained",
    "Text_UnitUpgraded"
};

// GLOBAL: LEGORR 0x005031f8
char* s_ui_message_text_slots[26];

/* EngineModeSet (FUN_004565f0) clears the two advisor-text request globals;
 * their normal writers/readers are in the advisor text band. */
// GLOBAL: LEGORR 0x00503610
uint32_t s_overlay_word_03610; /* DAT_00503610 role */
// GLOBAL: LEGORR 0x00503614
uint32_t s_overlay_word_03614; /* DAT_00503614 role */

static float s_ui_message_line_delay = 100.0f;
static unsigned int s_ui_message_count = 26u;
static unsigned int s_ui_message_slot_count = 26u;
static unsigned int s_ui_message_current;
static uint32_t s_ui_message_flags;

static char s_ui_message_image_names[26][32];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void advisor_text_output_copy(void* output, const char* text, int mode);
static int advisor_text_output_draw(void* output, float delta);

// FUNCTION: LEGORR 0x0046a7f0
void ui_message_strings_initialize(void* primary_font, void* secondary_font,
                                   void* message_resource, void* subtitle_font,
                                   float line_delay)
{
    s_ui_message_primary_font = primary_font;
    s_ui_message_secondary_font = secondary_font;
    s_ui_message_resource = message_resource;
    s_ui_message_subtitle_font = subtitle_font;
    s_ui_message_count = 26u;
    s_ui_message_slot_count = 26u;
    s_ui_message_line_delay = (line_delay == 0.0f) ? 100.0f : line_delay;
}

// FUNCTION: LEGORR 0x0046aa90
void ui_message_overlay_state_initialize(void)
{
    s_ui_message_count = 26u;
    s_ui_message_current = 0u;
    s_ui_message_flags = 0u;
    text_buffer_reset(s_ui_message_primary_font);
    text_buffer_reset(s_ui_message_secondary_font);
}

// FUNCTION: LEGORR 0x0046aad0
int ui_message_label_index_find(const char* label, int* out_index)
{
    int i;
    if (label == NULL) return 0;
    for (i = 0; i < 26; ++i) {
        if (strcasecmp_lite(s_ui_message_labels[i], label) == 0) {
            if (out_index == NULL) return 0;
            *out_index = i;
            return 1;
        }
    }
    return 0;
}
// FUNCTION: LEGORR 0x0046ab10
void ui_message_text_slot_assign(int index, const char* text)
{
    size_t length;
    const char* source;
    char* target;
    if (index < 0 || index >= 26 || text == NULL) return;
    engine_heap_free_wrapper(s_ui_message_text_slots[index]);
    s_ui_message_text_slots[index] = NULL;
    length = strlen(text);
    s_ui_message_text_slots[index] = (char*)engine_heap_alloc((long)length + 1);
    if (s_ui_message_text_slots[index] != NULL) {
        source = text;
        target = s_ui_message_text_slots[index];
        while (*source != '\0') {
            if (*source == '_') {
                *target = ' ';
            } else if (*source == '\\' && source[1] == 'n') {
                *target = '\n';
                ++source;
            } else {
                *target = *source;
            }
            ++source;
            ++target;
        }
        *target = '\0';
    }
    texture_release(s_ui_message_image_slots[index]);
    s_ui_message_image_slots[index] = NULL;
}

// FUNCTION: LEGORR 0x0046abb0
void ui_message_text_slot_assign_with_image(int index, const char* text,
                                            const char* image_path,
                                            const char* display_path)
{
    if (index < 0 || index >= 26) return;
    ui_message_text_slot_assign(index, text);
    snprintf(s_ui_message_image_names[index],
             sizeof(s_ui_message_image_names[index]), "%s",
             display_path != NULL ? display_path : "");
    s_ui_message_image_slots[index] = image_path != NULL
        ? image_surface_load(image_path, 0u, 0u) : NULL;
    if (s_ui_message_image_slots[index] != NULL) {
        texture_source_color_key_set(s_ui_message_image_slots[index], 0u, 0u);
    }
}

/* AdvisorTextRequestSetup (FUN_0046ac20): normalize the request mask,
 * publish the two overlay globals, notify the actor, and set pending bit 0. */
// FUNCTION: LEGORR 0x0046ac20
void advisor_text_request_setup(uint32_t request, uint32_t flags)
{
    if (flags == 0u) flags = 0x8000u;
    flags |= 0x10000u;
    engine_overlay_words_set(flags, request);
    s_advisor_text_request_source = NULL;
    advisor_inline_token_render((char*)(uintptr_t)request, NULL, 1);
    s_advisor_text_state_flags |= 1u;
}

/* AdvisorTextAnimationUpdate (FUN_0046ac60): update the active text slot,
 * optionally restart its timeout, advance an available animation, and
 * maintain the slot-selected flag. */
// FUNCTION: LEGORR 0x0046ac60
void advisor_text_animation_update(int slot, int force, int selected)
{
    if (force == 0) {
        if (s_advisor_text_animation_timer <= 0.0f) {
            s_advisor_text_current_slot = slot;
        }
    }
    else {
        s_advisor_text_current_slot = slot;
        s_advisor_text_animation_timer = s_advisor_text_animation_reset_timer;
    }
    if (s_advisor_text_animation_ops.is_paused == NULL ||
        s_advisor_text_animation_ops.is_paused(
            s_advisor_text_animation_ops.context) == 0) {
        int request = 0;
        if (s_advisor_text_animation_ops.animation_find != NULL &&
            s_advisor_text_animation_ops.animation_find(
                s_advisor_text_animation_ops.context, slot, &request) != 0 &&
            s_advisor_text_animation_ops.animation_play != NULL) {
            s_advisor_text_animation_ops.animation_play(
                s_advisor_text_animation_ops.context, request, 0);
        }
    }
    if (s_advisor_text_current_slot == slot && selected != 0) {
        s_advisor_text_state_flags |= 4u;
    }
    else {
        s_advisor_text_state_flags &= ~4u;
    }
}

/* AdvisorTextDisplayWorker (FUN_0046ad00): update the active advisor slot
 * or message timer, render the selected text path, then close or finish the
 * interface overlay according to the global display state. */
// FUNCTION: LEGORR 0x0046ad00
void advisor_text_display_worker(float delta)
{
    AdvisorTextDisplayOps* ops = &s_advisor_text_display_ops;
    AdvisorTextDisplayState state;
    int old_slot;
    int alternate;
    int slot_available = 0;
    int message_pending;
    int update_enabled;
    int image_path;
    const char* request_text = NULL;
    char rendered_text[512];
    if (ops->state_get == NULL) return;
    memset(&state, 0, sizeof(state));
    if (!ops->state_get(ops->context, &state)) return;
    old_slot = state.current_slot;
    message_pending = state.message_flags != 0u;
    update_enabled = !state.advisor_mode_valid || state.advisor_mode != 0;
    image_path = !message_pending && update_enabled && old_slot != 0x1a &&
                 state.slot_image != NULL;
    if (state.message_flags == 0u) {
        if (update_enabled) {
            slot_available = ops->slot_is_available != NULL ?
                ops->slot_is_available(ops->context, state.current_slot) : 0;
            if (slot_available && state.current_slot != 0x1a) {
                alternate = state.hidden_slot != 0;
                if (ops->slot_text_draw == NULL) {
                    const char* text = ui_message_text_slot_get(
                        (unsigned int)state.current_slot);
                    void* output = alternate != 0 ? state.alternate_text_output :
                                                    state.primary_text_output;
                    advisor_text_output_copy(output, text, 1);
                }
                if (state.message_timer <= 0.0f) {
                    state.current_slot = 0x1a;
                    if (ops->current_slot_set != NULL)
                        ops->current_slot_set(ops->context, state.current_slot);
                }
                state.message_timer -= delta;
                if (ops->message_timer_set != NULL)
                    ops->message_timer_set(ops->context, state.message_timer);
            }
        }
    }
    else if (state.message_id == 0) {
        state.message_flags = 0u;
        message_pending = 0;
        if (ops->message_flags_set != NULL)
            ops->message_flags_set(ops->context, 0u);
    }
    else {
        request_text = state.message_text;
        if (request_text == NULL && state.message_token_index_valid != 0u)
            request_text = objective_advisor_token_text_get(
                state.message_token_index);
        if (request_text != NULL) {
            advisor_inline_token_render((char*)request_text, rendered_text,
                                        0);
            advisor_text_output_copy(state.primary_text_output,
                                     rendered_text, 0);
        }
        if ((state.message_flags & 0x8000u) == 0u) {
            state.message_flags--;
            if (ops->message_flags_set != NULL)
                ops->message_flags_set(ops->context, state.message_flags);
        }
    }
    if (((state.interface_flags & 0x100000u) != 0u ||
         (ops->overlay_active != NULL &&
          ops->overlay_active(ops->context))) && old_slot != 0x16) {
        if (ops->interface_finish != NULL) {
            AdvisorTextDisplayState presentation = state;
            presentation.current_slot = old_slot;
            presentation.hidden_slot = image_path;
            ops->interface_finish(ops->context, &presentation, delta);
        }
        if (ops->overlay_close != NULL) ops->overlay_close(ops->context);
        return;
    }
    if (ops->interface_finish != NULL) {
        AdvisorTextDisplayState presentation = state;
        presentation.current_slot = old_slot;
        presentation.hidden_slot = image_path;
        ops->interface_finish(ops->context, &presentation, delta);
    }
    if (message_pending) {
        if (request_text != NULL) {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
        else if (ops->message_text_draw != NULL) {
            ops->message_text_draw(ops->context, state.message_id,
                                   state.x, state.y);
        }
        else {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
    }
    else {
        if (slot_available && old_slot != 0x1a) {
            alternate = state.hidden_slot != 0;
            if (ops->slot_text_draw != NULL) {
                ops->slot_text_draw(ops->context, old_slot, alternate,
                                    state.x, state.y);
            }
            else {
                void* output = alternate != 0 ? state.alternate_text_output :
                                                state.primary_text_output;
                (void)advisor_text_output_draw(output, delta);
            }
            if (state.slot_image != NULL) {
                DisplaySurface* target = surface_active_render_target_get();
                void* pixels = NULL;
                int pitch = 0;
                if (target != NULL && surface_lock(target, &pixels, &pitch) == 0) {
                    surface_blit_texture(pixels, pitch, target->w, target->h,
                        (TextureSurface*)state.slot_image,
                        (int)state.x, (int)state.y);
                    surface_unlock(target, pixels);
                }
            }
        }
        else {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
    }
}
static void advisor_message_layout_result(void* output, int overflow,
                                          int bottom)
{
    int line;
    if (output == NULL || output != s_message_text_buffers[0] ||
        s_message_font == NULL) return;
    line = font_atlas_record_line_height_get(s_message_font);
    if (line <= 0) return;
    if (overflow) {
        s_message_panel_y -= (float)line;
        s_message_text_y -= (float)line;
        s_message_text_height += (float)line;
        s_advisor_text_state_flags &= ~1u;
        advisor_message_layout_commit();
    } else if ((s_advisor_text_state_flags & 1u) != 0u) {
        /* The reference compares the last drawn baseline with the original
         * text bottom minus one font line before contracting a new message. */
        if (435.0f + 42.0f - (float)line <= (float)bottom) {
            s_advisor_text_state_flags &= ~1u;
        } else {
            s_message_panel_y += (float)line;
            s_message_text_y += (float)line;
            s_message_text_height -= (float)line;
            if (s_message_panel_y >= 409.0f) {
                advisor_message_layout_reset();
                s_advisor_text_state_flags &= ~1u;
            } else advisor_message_layout_commit();
        }
    }
}

/* The engine text-output record already owns its font, position, wrapping,
 * and scroll state. Use the shared record writer and wrapped renderer when a
 * display-state provider supplies that record but no backend-specific draw
 * callback. */
static void advisor_text_output_copy(void* output, const char* text, int mode)
{
    if (output == NULL || text == NULL) return;
    objective_advisor_text_copy(output, mode, "%s", text);
}

static int advisor_text_output_draw(void* output, float delta)
{
    int bottom = 0;
    int overflow;
    if (output == NULL) return 0;
    overflow = text_buffer_draw_wrapped(output, 0u, delta, &bottom);
    advisor_message_layout_result(output, overflow, bottom);
    return overflow;
}
