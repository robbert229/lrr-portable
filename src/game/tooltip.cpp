// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-046 ToolTip.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-046.csv. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
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
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
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
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
/* C linkage for ToolTip_SetSFX, which GAME-021's lego_unit.c calls (issue
 * #544). The frozen header below redeclares it and inherits it. */
extern "C" void timed_effect_cue_request_set_corpus(int index, uint32_t value);
#include "frontend_for_tooltip.hpp"
#include "ai.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include "cmd_for_tooltip.hpp"
#include "combat_ray_ports.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "weapon.hpp"
#include "bezier.h"
#include <stddef.h>
#include "effects_for_tooltip.hpp"
#include "model.hpp"
#include "view_for_tooltip.hpp"

// GLOBAL: LEGORR 0x004aa64c
static uint32_t s_timed_effect_cue_gate = 1u;

static const char* s_tooltip_name_table[38];

/* The legacy UI owns these globals and renders synchronously. Reuse one
 * snapshot to keep the mapped slot renderer's frame-local record small. */
static TooltipOverlayState s_timed_effect_overlay_state;

/* TooltipOverlayStateInitialize @ 0x46b1d0: the target's ten stack-word entry
 * publishes its fixed labels and geometry/color globals, sets the timed
 * tooltip duration in 25-unit ticks, and clears 0x1839 dwords at the start
 * of the 0x27c-byte overlay-record table. This includes the first 12 bytes
 * of record 39 but leaves its +0x278 traversal-bound flag intact. */
// FUNCTION: LEGORR 0x0046b1d0
void tooltip_overlay_state_initialize(
    void* font, uint32_t value2, uint32_t value3, float scale,
    uint32_t value5, uint32_t value6, uint32_t value7,
    float color_r, float color_g, float color_b)
{
    float threshold = scale * 25.0f;
    int i;
    tooltip_overlay_state_fill(&s_tooltip_overlay_state, font,
                               value2, value3, scale,
                               value5, value6, value7,
                               color_r, color_g, color_b,
                               object_field_be4_dword_get(font));
    for (i = 0; i < 38; ++i) s_tooltip_name_table[i] = s_tooltip_names[i];
    s_timed_effect_tick = threshold;
    memset(s_timed_effect_records, 0,
           (size_t)0x1839u * sizeof(uint32_t));
}
/* CaseInsensitiveNameTableLookup (FUN_0046b4d0) searches the table published
 * by TooltipOverlayStateInitialize. Results are one-based. The fallback to
 * canonical labels keeps config-only callers usable before GameConfigInit's
 * renderer setup path has initialized the global table in this port. */
// FUNCTION: LEGORR 0x0046b4d0
int case_insensitive_name_table_lookup(const char* name, int* out_index)
{
    /* Corpus shape: the walk strides the name slots by four with a
     * one-based counter, feeds each entry to strcasecmp_lite untested,
     * and answers through the raw out pointer. Deviation: the name NULL
     * gate stays (the port's menu caller can pass a null optional name;
     * the corpus strides it blindly). */
    if (name == NULL) {
        return 0;
    }
    int index = 1;
    while (index <= 38) {
        const char* candidate = s_tooltip_name_table[index - 1];
        if (candidate == NULL) candidate = s_tooltip_names[index - 1];
        if (strcasecmp_lite(name, candidate) == 0) {
            *out_index = index;
            return 1;
        }
        ++index;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046b660
void command_sample_stats_accumulate(int kind, const void* sample)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS) {
        return;
    }
    TimedEffectRecord* record = timed_effect_record_get((unsigned int)kind);
    uint32_t* slot;
    if (record == NULL) return;
    slot = (uint32_t*)record;
    if (sample == NULL) {
        slot[CMD_STATS_HEAD_DWORD] = 0u;
        slot[CMD_STATS_TOTAL_DWORD] = 0u;
    } else {
        uint32_t v4 = 0u, v8 = 0u;           /* sample+4, sample+8 */
        __builtin_memcpy(&v4, (const char*)sample + 4, sizeof v4);
        __builtin_memcpy(&v8, (const char*)sample + 8, sizeof v8);
        if ((int)slot[CMD_STATS_HEAD_DWORD] < (int)v8) {
            slot[CMD_STATS_WINDOW_DWORD] = (uint32_t)(
                (int)slot[CMD_STATS_WINDOW_DWORD] -
                (int)slot[CMD_STATS_HEAD_DWORD]);
            slot[CMD_STATS_HEAD_DWORD] = v8;
            slot[CMD_STATS_WINDOW_DWORD] = (uint32_t)(
                (int)slot[CMD_STATS_WINDOW_DWORD] + (int)v8);
        }
        slot[CMD_STATS_TOTAL_DWORD] = (uint32_t)(
            (int)slot[CMD_STATS_TOTAL_DWORD] + (int)v4);
        if ((int)slot[CMD_STATS_HIGH_DWORD] <
            (int)slot[CMD_STATS_TOTAL_DWORD]) {
            slot[CMD_STATS_HIGH_DWORD] = slot[CMD_STATS_TOTAL_DWORD];
        }
    }
    const uint32_t idx = slot[CMD_STATS_COUNT_DWORD];
    if (idx < CMD_STATS_LOG_DWORDS) {
        slot[CMD_STATS_LOG_DWORD + idx] = (uint32_t)(uintptr_t)sample;
    }
    slot[CMD_STATS_COUNT_DWORD] = idx + 1u;
}

// FUNCTION: LEGORR 0x0046b700
void timed_effect_cue_request_set_corpus(int index, uint32_t value)
{
    s_timed_effect_records[index].cue = value;
    s_timed_effect_records[index].flags |= 3u;
}

// FUNCTION: LEGORR 0x0046b730
void timed_effect_persistence_set_corpus(int index, int set)
{
    uint32_t* flags = &s_timed_effect_records[index].flags;
    *flags = (set != 0) ? (*flags | 0x10u) : (*flags & ~0x10u);
}
// FUNCTION: LEGORR 0x0046b770
void timed_effect_update_request_set_corpus(int index)
{
    s_timed_effect_records[index].flags |= 4u;
}

/* Keep the address-mapped leaf blind like the corpus; the public wrapper
 * retains the bounds check exercised by the portable API tests. */
// FUNCTION: LEGORR 0x0046b7a0
void timed_effect_timestamp_reset_corpus(int index)
{
    s_timed_effect_records[index].timestamp = s_timed_effect_tick;
}

// FUNCTION: LEGORR 0x0046b7c0
void timed_effect_runtime_update(uint32_t render_x, uint32_t render_y,
                                 float delta)
{
    float now;
    uint32_t tick_bits;
    uint32_t cue_gate;
    uint32_t global_flags;
    int rendered = 0;
    int advanced = 0;
    unsigned int slot;
    tick_bits = timed_effect_tick_word_peek();
    memcpy(&now, &tick_bits, sizeof(now));
    if (s_timed_effect_runtime_hooks.time_get != NULL)
        now = s_timed_effect_runtime_hooks.time_get(
            s_timed_effect_runtime_hooks.context);
    cue_gate = s_timed_effect_runtime_hooks.cue_gate_get != NULL
        ? s_timed_effect_runtime_hooks.cue_gate_get(
              s_timed_effect_runtime_hooks.context)
        : s_timed_effect_cue_gate;
    global_flags = s_timed_effect_runtime_hooks.global_flags_get != NULL
        ? s_timed_effect_runtime_hooks.global_flags_get(
              s_timed_effect_runtime_hooks.context)
        /* The reference tests DAT_00556ed0, the debug/cheat word. */
        : engine_debug_word_peek();
    for (slot = 1u; slot < 39u; ++slot) {
        TimedEffectRecord* record = timed_effect_record_get(slot);
        if (record == NULL) continue;
        if ((record->flags & 3u) == 0u) continue;
        if ((record->flags & 4u) == 0u || advanced != 0) {
            record->timestamp = 0.0f;
        }
        else if (record->timestamp < now || rendered != 0) {
            cue_gate = 1u;
            s_timed_effect_cue_gate = cue_gate;
            if (s_timed_effect_runtime_hooks.cue_gate_set != NULL)
                s_timed_effect_runtime_hooks.cue_gate_set(
                    s_timed_effect_runtime_hooks.context, cue_gate);
            advanced = 1;
            record->timestamp += delta;
        }
        else {
            if (record->cue != 0u && cue_gate != 0u &&
                (global_flags & 0x100000u) == 0u &&
                s_timed_effect_runtime_hooks.attached_effect_update != NULL) {
                cue_gate = s_timed_effect_runtime_hooks.attached_effect_update(
                    s_timed_effect_runtime_hooks.context, record->cue, 0) == 0
                    ? 1u : 0u;
                s_timed_effect_cue_gate = cue_gate;
                if (s_timed_effect_runtime_hooks.cue_gate_set != NULL)
                    s_timed_effect_runtime_hooks.cue_gate_set(
                        s_timed_effect_runtime_hooks.context, cue_gate);
            }
            else if (record->cue != 0u && cue_gate != 0u &&
                     (global_flags & 0x100000u) == 0u) {
                cue_gate = sound_submit_with_global_cooldown(
                    (int)record->cue, NULL) == 0 ? 1u : 0u;
                s_timed_effect_cue_gate = cue_gate;
            }
            timed_effect_slot_render_record(record, render_x, render_y);
            rendered = 1;
            advanced = 1;
            record->timestamp += delta;
        }
        record->flags &= ~4u;
    }
}
// FUNCTION: LEGORR 0x0046b8b0
void rectangular_grid_overlay_geometry_draw(
    const RectangularGridOverlayState* state,
    const RectangularGridOverlayHooks* hooks)
{
    float points[10];
    unsigned int band, bands;
    if (state == NULL || hooks == NULL) return;
    if (hooks->region_fill != NULL)
        hooks->region_fill(hooks->context, state,
                           state->overlay_mode != 0 ? 2 : 0);
    if (hooks->polyline_draw == NULL) return;
    bands = state->bands > 5u ? 5u : state->bands;
    for (band = 0; band < bands; ++band) {
        float inset = (float)band;
        float left = state->x + inset;
        float top = state->y + inset;
        float right = state->x + state->width - 1.0f - inset;
        float bottom = state->y + state->height - 1.0f - inset;
        points[0] = left; points[1] = top;
        points[2] = right; points[3] = top;
        points[4] = right; points[5] = bottom;
        points[6] = left; points[7] = bottom;
        points[8] = left; points[9] = top;
        hooks->polyline_draw(hooks->context, points, 5,
                             state->first_line_color, 0);
        points[0] = left + 1.0f; points[1] = top + 1.0f;
        points[2] = right - 1.0f; points[3] = top + 1.0f;
        points[4] = right - 1.0f; points[5] = bottom - 1.0f;
        points[6] = left + 1.0f; points[7] = bottom - 1.0f;
        points[8] = left + 1.0f; points[9] = top + 1.0f;
        hooks->polyline_draw(hooks->context, points, 5,
                             state->second_line_color, 0);
    }
}

// FUNCTION: LEGORR 0x0046bc30
void timed_effect_slot_render_record(const TimedEffectRecord* record,
                                     uint32_t render_x, uint32_t render_y)
{
    TooltipOverlayState* config = &s_timed_effect_overlay_state;
    RectangularGridOverlayState overlay;
    uint32_t colors[4][3];
    const uint32_t* words;
    uint32_t flags, padding, bands, count, i;
    float inset, top, left, width, height, y, row_x, row_y, row_height;
    void* resource = NULL;
    if (record == NULL) return;
    words = (const uint32_t*)record;
    flags = words[0x278 / 4];
    tooltip_overlay_state_current(config);
    bands = config->values[0];
    padding = config->values[1];
    inset = (float)((padding + bands) * 2u);
    if ((flags & 8u) != 0u) {
        uint32_t handle = words[0x270 / 4];
        if (s_timed_effect_runtime_hooks.resource_handle_resolve != NULL)
            resource = s_timed_effect_runtime_hooks.resource_handle_resolve(
                s_timed_effect_runtime_hooks.context, handle);
        else if (sizeof(void*) == 4)
            resource = (void*)(uintptr_t)handle;
        top = 0.0f;
        left = 0.0f;
        if (resource != NULL &&
            s_timed_effect_runtime_hooks.resource_extent_get != NULL)
            s_timed_effect_runtime_hooks.resource_extent_get(
                s_timed_effect_runtime_hooks.context, resource, &top, &left);
        else if (resource != NULL && sizeof(void*) == 4) {
            top = (float)*(uint32_t*)((unsigned char*)resource + 4);
            left = (float)*(uint32_t*)((unsigned char*)resource + 8);
        }
    }
    else {
        uint32_t first = words[0x204 / 4];
        uint32_t second = words[0x26c / 4];
        top = (float)(first > second ? first : second);
        left = (float)(words[0x208 / 4] * config->font_metric +
                       words[0x264 / 4]);
    }
    top += inset;
    left += inset;
    if ((flags & 0x10u) == 0u) {
        if ((int)config->values[2] < (int)top + (int)render_x)
            width = (float)((int)config->values[2] - (int)top);
        else
            width = (float)render_x;
    }
    else if ((float)render_x + 32.0f <= (float)config->values[2])
        width = (float)render_x + 32.0f - (float)(uint32_t)top;
    else
        width = (float)((int)config->values[2] - (int)top);
    y = (float)(config->values[4] + render_y);
    height = left;
    if ((float)config->values[3] < y + height)
        y = (float)(render_y - (uint32_t)height) - 1.0f;
    memset(&overlay, 0, sizeof(overlay));
    overlay.x = width;
    overlay.y = y;
    overlay.width = top;
    overlay.height = height;
    overlay.bands = bands;
    for (i = 0; i < 3u; ++i) {
        colors[0][i] = (uint32_t)(config->color_base[i] * 255.0f);
        colors[1][i] = (uint32_t)(config->color_high[i] * 255.0f);
        colors[2][i] = (uint32_t)(config->color_low[i] * 255.0f);
        colors[3][i] = 0u;
    }
    memcpy(overlay.fill_color, colors[0], sizeof(overlay.fill_color));
    memcpy(overlay.first_line_color, colors[1],
           sizeof(overlay.first_line_color));
    memcpy(overlay.second_line_color, colors[2],
           sizeof(overlay.second_line_color));
    overlay.overlay_mode = (flags & 8u) != 0u;
    rectangular_grid_overlay_geometry_draw(&overlay,
        &s_timed_effect_slot_render_hooks.overlay_hooks);
    if (s_timed_effect_slot_render_hooks.overlay_background != NULL)
        s_timed_effect_slot_render_hooks.overlay_background(
            s_timed_effect_slot_render_hooks.context, width, y, top, height,
            colors, overlay.overlay_mode ? 2 : 0);
    if ((flags & 8u) != 0u) {
        if (resource != NULL &&
            s_timed_effect_slot_render_hooks.render_item != NULL)
            s_timed_effect_slot_render_hooks.render_item(
                s_timed_effect_slot_render_hooks.context, resource,
                width + (float)(padding + bands),
                y + (float)(padding + bands), top - inset, left - inset);
        return;
    }
    if (s_timed_effect_slot_render_hooks.render_text != NULL)
        s_timed_effect_slot_render_hooks.render_text(
            s_timed_effect_slot_render_hooks.context, config->font,
            (const char*)record + 4, width + (float)(padding + bands),
            y + (float)(padding + bands));
    count = words[0x25c / 4];
    if (count > 20u) count = 20u;
    row_x = width + (float)(padding + bands);
    row_y = y + (float)(padding + bands) +
        (float)words[0x208 / 4] * (float)config->font_metric;
    row_height = 0.0f;
    for (i = 0; i < count; ++i) {
        uint32_t handle = words[0x20c / 4 + i];
        float item_width = 0.0f;
        float item_height = 0.0f;
        resource = NULL;
        if (s_timed_effect_runtime_hooks.resource_handle_resolve != NULL)
            resource = s_timed_effect_runtime_hooks.resource_handle_resolve(
                s_timed_effect_runtime_hooks.context, handle);
        else if (sizeof(void*) == 4)
            resource = (void*)(uintptr_t)handle;
        if (resource == NULL) {
            row_y += row_height;
            row_height = 0.0f;
            row_x = width + (float)(padding + bands);
            continue;
        }
        if (s_timed_effect_runtime_hooks.resource_extent_get != NULL)
            s_timed_effect_runtime_hooks.resource_extent_get(
                s_timed_effect_runtime_hooks.context, resource,
                &item_width, &item_height);
        else if (sizeof(void*) == 4) {
            item_width = (float)*(uint32_t*)((unsigned char*)resource + 4);
            item_height = (float)*(uint32_t*)((unsigned char*)resource + 8);
        }
        if (s_timed_effect_slot_render_hooks.render_item != NULL)
            s_timed_effect_slot_render_hooks.render_item(
                s_timed_effect_slot_render_hooks.context, resource, row_x,
                row_y, item_width, item_height);
        row_x += item_width;
        if (row_height < item_height) row_height = item_height;
    }
}

// FUNCTION: LEGORR 0x0046c030
void render_resource_binding_initialize(RenderResourceBinding* binding,
                                        void* key, void* config_a,
                                        void* config_b)
{
    unsigned int selector;
    if (binding == NULL) return;
    binding->key = NULL;
    binding->resource = NULL;
    binding->state = 0;
    binding->flags = 0;
    if (s_render_resource_binding_hooks.resource_find != NULL) {
        for (selector = 0; selector < 3; ++selector) {
            binding->resource =
                s_render_resource_binding_hooks.resource_find(
                    s_render_resource_binding_hooks.context,
                    config_a, config_b, selector);
            if (binding->resource != NULL) break;
        }
    }
    binding->key = key;
}

// FUNCTION: LEGORR 0x0046c0b0
void render_resource_binding_copy_and_convert(
    const RenderResourceBinding* source,
    RenderResourceBinding* destination)
{
    if (source == NULL || destination == NULL) return;
    *destination = *source;
    if (s_render_resource_binding_hooks.resource_load != NULL) {
        destination->resource =
            s_render_resource_binding_hooks.resource_load(
                s_render_resource_binding_hooks.context, source->resource);
    }
}

// FUNCTION: LEGORR 0x0046c0f0
void render_resource_binding_release(RenderResourceBinding* binding)
{
    if (binding == NULL) return;
    if (s_render_resource_binding_hooks.resource_release != NULL) {
        s_render_resource_binding_hooks.resource_release(
            s_render_resource_binding_hooks.context, binding->resource);
    }
}

// FUNCTION: LEGORR 0x0046c110
float animation_resource_advance_time(RenderResourceBinding* binding,
                                     void* argument)
{
    if (binding != NULL &&
        s_render_resource_binding_hooks.animation_time_get != NULL) {
        (void)s_render_resource_binding_hooks.animation_time_get(
            s_render_resource_binding_hooks.context, binding->resource,
            argument);
    }
    return 0.0f;
}

// FUNCTION: LEGORR 0x0046c130
void scene_object_set_animation_with_fallback(
    RenderResourceBinding* binding, void* name)
{
    if (scene_action_name_set(binding->resource, (const char*)name) == 0u)
        (void)scene_action_name_set(binding->resource,
                                    ai_activity_name(AI_ACT_STAND));
}

/* ToolTip_DrawBox (0046b8b0) in the original ABI, for Reward.c (GAME-037,
 * finding 560). A thin unannotated adapter (ADR 0005 step 6): the original
 * takes the box and its three colours by value; the port's entry takes its
 * state record and the renderer's hooks, which the original ABI does not
 * carry, so the box is described but not drawn. */
#include "effects.h"

// PORT-ONLY: adapter or forwarder for the port contract
void tooltip_draw_box(Area2F rect, ColourRGBF fill, ColourRGBF highlight,
                      ColourRGBF shadow, int dark)
{
    RectangularGridOverlayState state;
    memset(&state, 0, sizeof(state));
    state.x = rect.x;
    state.y = rect.y;
    state.width = rect.width;
    state.height = rect.height;
    memcpy(state.fill_color, &fill, sizeof(state.fill_color));
    memcpy(state.first_line_color, &highlight, sizeof(state.first_line_color));
    memcpy(state.second_line_color, &shadow, sizeof(state.second_line_color));
    state.overlay_mode = dark;
    rectangular_grid_overlay_geometry_draw(&state, NULL);
}
