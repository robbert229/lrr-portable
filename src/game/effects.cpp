// SPDX-License-Identifier: LGPL-3.0-or-later
#include "effects.hpp"
#include "combat_ray_ports.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "crt/crt.hpp"
#include "engine.hpp"
#include "ai.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "gods98/texture.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "gods98/block.hpp"
#include "level.hpp"
#include "weapon.hpp"
#include "util.hpp"
#include "util.hpp"
#include "bezier.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t s_active_effect_occupied[10];
static uint32_t s_active_segment_visual_occupied[10];
static ActiveEffectRecord s_active_effect_records[10];
/* Each record's flight path. The original embeds the BezierCurve at +4 of
 * its 0x2b0-byte record (00503920); the port's record keeps its own layout,
 * so the paths sit beside it, one per slot. */
static BezierCurve s_active_effect_paths[10];
static ActiveEffectBuildHooks s_active_effect_build_hooks;
static EntityImpulseApplyHook s_entity_impulse_hook;
static EntityDirectionApplyHook s_entity_direction_hook;
static TimedSegmentVisualHooks s_timed_segment_visual_hooks;
static void* s_timed_segment_visuals[10][2];
static void* s_timed_segment_visual_owners[10];
static SimtickSfxTimedVisualRecord s_timed_segment_tick_records[10];
static SimtickSfxProjectileRecord s_active_effect_tick_records[10];
static int s_sfx_tick_hooks_installed;
static KindEffectDispatchHooks s_kind_effect_dispatch_hooks;
static EffectApplyHooks s_effect_apply_hooks;
static ScreenSpaceEffectDispatchHook s_screen_space_effect_dispatch_hook;
struct ScreenEffectTickRecord {
    void* entity;
    float remaining;
};
static struct ScreenEffectTickRecord s_screen_effect_tick_records[3][50];
static ScreenEffectTickHooks s_screen_effect_tick_hooks;
static float s_screen_effect_pulse_timer;
static void* s_screen_space_effect_dispatch_context;
static TransientEffectHooks s_transient_effect_hooks;
static TransientEffectQuadBasisGetHook s_transient_effect_quad_basis_hook;
static void* s_transient_effect_quad_basis_context;
// GLOBAL: LEGORR 0x00552100
static TextureSurface* s_transient_effect_textures[12];
// GLOBAL: LEGORR 0x00552130
static uint32_t s_transient_effect_texture_count;
static void* s_transient_effect_head;
static void* s_transient_effect_tail;
typedef struct TransientEffectInstanceLinks {
    void* instance;
    void* scene_object;
    void* render_record;
    struct TransientEffectInstanceLinks* next;
    struct TransientEffectInstanceLinks* previous;
    struct TransientEffectInstanceLinks* all_next;
} TransientEffectInstanceLinks;
static TransientEffectInstanceLinks* s_transient_effect_links;
static int s_transient_effect_enabled = 1;
static TimedEffectRuntimeHooks s_timed_effect_runtime_hooks;
static TimedEffectSlotRenderHooks s_timed_effect_slot_render_hooks;
/* The legacy UI owns these globals and renders synchronously. Reuse one
 * snapshot to keep the mapped slot renderer's frame-local record small. */
static TooltipOverlayState s_timed_effect_overlay_state;
// GLOBAL: LEGORR 0x004aa64c
static uint32_t s_timed_effect_cue_gate = 1u;
static SceneTransformState s_scene_transform_state;
static LightEffectParameterState s_light_effect_parameters;
static EntityNodeScaleApplyHook s_entity_node_scale_apply_hook;
static void* s_entity_node_scale_apply_context;
static EventTimerActionHook s_event_timer_action_hook;
static void* s_event_timer_action_context;
static EventTimerInteractionFlagsHook s_event_timer_interaction_flags_hook;
static void* s_event_timer_interaction_flags_context;

static TransientEffectInstanceLinks* transient_effect_links_find(
    const void* instance)
{
    TransientEffectInstanceLinks* links;
    for (links = s_transient_effect_links; links != NULL;
         links = links->all_next) {
        if (links->instance == instance) return links;
    }
    return NULL;
}

static TransientEffectInstanceLinks* transient_effect_links_add(void* instance)
{
    TransientEffectInstanceLinks* links;
    if (instance == NULL) return NULL;
    links = transient_effect_links_find(instance);
    if (links != NULL) return links;
    links = (TransientEffectInstanceLinks*)malloc(sizeof(*links));
    if (links == NULL) return NULL;
    memset(links, 0, sizeof(*links));
    links->instance = instance;
    if (sizeof(void*) == 4u) {
        unsigned char* bytes = (unsigned char*)instance;
        links->scene_object = *(void**)(bytes + 0x28c);
        links->render_record = *(void**)(bytes + 0x290);
    }
    links->all_next = s_transient_effect_links;
    s_transient_effect_links = links;
    return links;
}

static void* transient_effect_scene_object_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL && links->scene_object != NULL)
        return links->scene_object;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x28c);
    return NULL;
}

static void* transient_effect_render_record_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL && links->render_record != NULL)
        return links->render_record;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x290);
    return NULL;
}

static void* transient_effect_next_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL) return links->next != NULL ? links->next->instance : NULL;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x298);
    return NULL;
}

static void* transient_effect_previous_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL)
        return links->previous != NULL ? links->previous->instance : NULL;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x29c);
    return NULL;
}

static void transient_effect_link_pointer_set(void* instance, size_t offset,
                                               void* value)
{
    if (sizeof(void*) == 4u && instance != NULL)
        *(void**)((unsigned char*)instance + offset) = value;
}

void event_timer_action_hook_set(EventTimerActionHook hook, void* context)
{
    s_event_timer_action_hook = hook;
    s_event_timer_action_context = context;
}

void event_timer_interaction_flags_hook_set(
    EventTimerInteractionFlagsHook hook, void* context)
{
    s_event_timer_interaction_flags_hook = hook;
    s_event_timer_interaction_flags_context = context;
}

uint32_t event_timer_interaction_flags_get(void)
{
    return s_event_timer_interaction_flags_hook != NULL
        ? s_event_timer_interaction_flags_hook(
              s_event_timer_interaction_flags_context) : 0u;
}

// FUNCTION: LEGORR 0x0041EAC0
void event_timer_countdown_advance(float delta)
{
    EventTimerState state;
    frontend_event_timer_state_get(&state);
    if ((state.flags & 0x80u) != 0u) return;
    if ((state.flags & 0x100u) == 0u) {
        if ((state.flags & 4u) != 0u) {
            state.timer -= delta * state.rate;
            if (state.timer <= state.lower) {
                state.flags |= 0x80u;
                state.timer = state.lower;
            }
        }
        frontend_event_timer_state_set(&state);
        return;
    }
    state.timer += delta * state.rate;
    if (state.current_event == 0x16 &&
        (state.external_flags & 2u) != 0u) {
        if (s_event_timer_action_hook != NULL) {
            s_event_timer_action_hook(s_event_timer_action_context, 8, 0);
            s_event_timer_action_hook(s_event_timer_action_context, 6, 1);
        }
        state.timer = state.upper;
    }
    if (state.timer < state.upper) {
        frontend_event_timer_state_set(&state);
        return;
    }
    state.flags &= ~0x100u;
    state.timer = state.upper;
    sound_alias_play(0x15, 0);
    if (state.next_event == 0x16) {
        state.flags = (state.flags & ~4u) | 0x80u;
        if (s_event_timer_action_hook != NULL)
            s_event_timer_action_hook(s_event_timer_action_context, 8, 0);
        simtick_global_ui_flags_clear_bits12();
        simtick_global_ui_flags_set_bit2();
    }
    else if (state.next_event == 0x17) {
        state.flags = (state.flags & ~4u) | 0x80u;
        if (s_event_timer_action_hook != NULL)
            s_event_timer_action_hook(s_event_timer_action_context, 0x0b, 0);
        global_state_low_word_set(global_state_low_word_get() | 1u);
        global_flag_bit0_clear();
    }
    else {
        state.flags |= 4u;
    }
    state.current_event = state.next_event;
    state.current_x = state.next_x;
    state.current_y = state.next_y;
    frontend_event_timer_state_set(&state);
}

void timed_effect_cue_update(TimedEffectCueState* state, float x, float y,
                             float delta,
                             const TimedEffectCueHooks* hooks)
{
    int render_phase_seen = 0;
    int time_phase_seen = 0;
    unsigned int i;
    if (state == NULL) return;
    for (i = 0; i < 45u; ++i) {
        TimedEffectCueSlot* slot = &state->slots[i];
        if ((slot->flags & 3u) == 0u) continue;
        if ((slot->flags & 4u) == 0u || time_phase_seen) {
            slot->timer = 0.0f;
        }
        else if (slot->timer < state->threshold || render_phase_seen) {
            state->cue_active = 1u;
            time_phase_seen = 1;
            slot->timer += delta;
        }
        else {
            if (slot->cue_id != 0u && state->cue_active != 0u &&
                state->cue_suppressed == 0u && hooks != NULL &&
                hooks->cue_done != NULL) {
                state->cue_active = hooks->cue_done(hooks->context,
                                                    slot->cue_id) == 0
                    ? 1u : 0u;
            }
            if (hooks != NULL && hooks->slot_render != NULL)
                hooks->slot_render(hooks->context, slot, x, y);
            render_phase_seen = 1;
            time_phase_seen = 1;
            slot->timer += delta;
        }
        slot->flags &= ~4u;
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

void timed_effect_slot_render(const TimedEffectSlotRenderState* state,
                              const TimedEffectSlotRenderHooks* hooks)
{
    RectangularGridOverlayState overlay;
    float top, left, inset, x, y, width, height, row_x, row_height;
    unsigned int i;
    if (state == NULL || hooks == NULL) return;
    inset = (float)((state->bands + state->padding) * 2u);
    if ((state->flags & 8u) != 0u) {
        top = state->alternate_top;
        left = state->alternate_left;
    }
    else {
        top = state->top_a > state->top_b ? state->top_a : state->top_b;
        left = state->column * state->column_width + state->left;
    }
    x = state->wanted_width;
    if ((state->flags & 0x10u) != 0u) {
        if (state->wanted_width + 32.0f <= state->view_width)
            x = state->wanted_width + 32.0f - (top + inset);
        else x = state->view_width - (top + inset);
    }
    else if (top + inset + state->wanted_width > state->view_width)
        x = state->view_width - (top + inset);
    height = inset + left;
    y = state->origin_y + state->wanted_height;
    if (state->view_height < y + height)
        y = state->wanted_height - height - 1.0f;
    memset(&overlay, 0, sizeof(overlay));
    overlay.x = top + inset;
    overlay.y = y;
    overlay.width = x;
    overlay.height = height;
    overlay.bands = state->bands;
    memcpy(overlay.fill_color, state->colors[0], sizeof(overlay.fill_color));
    memcpy(overlay.first_line_color, state->colors[1],
           sizeof(overlay.first_line_color));
    memcpy(overlay.second_line_color, state->colors[2],
           sizeof(overlay.second_line_color));
    overlay.overlay_mode = (state->flags & 8u) != 0u;
    rectangular_grid_overlay_geometry_draw(&overlay, &hooks->overlay_hooks);
    if (hooks->overlay_background != NULL)
        hooks->overlay_background(hooks->context, overlay.x, overlay.y,
                                  overlay.width, overlay.height,
                                  state->colors,
                                  (state->flags & 8u) != 0u ? 2 : 0);
    if ((state->flags & 8u) != 0u) {
        if (state->special_item.enabled && state->special_item.handle != NULL &&
            hooks->render_item != NULL) {
            hooks->render_item(hooks->context, state->special_item.handle,
                               overlay.x + (float)(state->bands + state->padding),
                               overlay.y + (float)(state->bands + state->padding),
                               state->special_item.width,
                               state->special_item.height);
        }
        return;
    }
    if (state->items == NULL) return;
    row_x = overlay.x + (float)(state->bands + state->padding);
    row_height = 0.0f;
    {
        float row_y = overlay.y + (float)(state->bands + state->padding) +
            state->column * state->column_width;
        for (i = 0; i < state->item_count; ++i) {
            const TimedEffectRenderItem* item = &state->items[i];
            if (!item->enabled || item->handle == NULL) {
                row_y += row_height;
                row_height = 0.0f;
                row_x = overlay.x + (float)(state->bands + state->padding);
                continue;
            }
            if (hooks->render_item != NULL)
                hooks->render_item(hooks->context, item->handle, row_x,
                                   row_y, item->width, item->height);
            row_x += item->width;
            if (row_height < item->height) row_height = item->height;
        }
    }
    (void)row_height;
}

void entity_node_scale_apply_hook_set(EntityNodeScaleApplyHook hook,
                                      void* context)
{
    s_entity_node_scale_apply_hook = hook;
    s_entity_node_scale_apply_context = context;
}

// EntityNodeScaleUpdate (FUN_0044bb30): the accessor call sites pass an extra
// value that the one-argument cdecl readers ignore; only the final scaling
// affects the selected record value.
// FUNCTION: LEGORR 0x0044bb30
void entity_node_scale_update(void* entity_ptr, int doubled)
{
    EntitySlot* entity = (EntitySlot*)entity_ptr;
    float first;
    float second;
    if (entity == NULL || s_entity_node_scale_apply_hook == NULL) return;
    first = entity_stats_float_at128(entity);
    second = entity_stats_float_at12c(entity);
    if (doubled != 0) {
        first = entity_stats_float_at124(entity);
        first = entity_stats_float_at120(entity);
        first += first;
        second = entity_stats_float_at128(entity);
        second = entity_stats_float_at124(entity);
        second = entity_stats_float_at120(entity);
        second *= 4.0f;
    }
    else {
        first = entity_stats_float_at120(entity);
        second = entity_stats_float_at120(entity);
    }
    s_entity_node_scale_apply_hook(s_entity_node_scale_apply_context,
                                   entity, first, second);
}
static LightEffectConfigLoadHook s_light_effect_config_load_hook;
static void* s_light_effect_config_load_context;

static int light_effect_config_range_pair_read(BlockFile* config,
                                               const char* path,
                                               float* minimum,
                                               float* maximum)
{
    const char* value = block_get_strptr(config, path);
    char copy[256];
    char* separator;
    char* end;
    double first;
    double second;
    size_t length = 0u;
    if (value == NULL || minimum == NULL || maximum == NULL) return 0;
    while (length + 1u < sizeof(copy) && value[length] != '\0') {
        copy[length] = value[length];
        ++length;
    }
    if (value[length] != '\0') return 0;
    copy[length] = '\0';
    separator = strchr(copy, ':');
    if (separator == NULL || strchr(separator + 1, ':') != NULL) return 0;
    *separator++ = '\0';
    first = strtod(copy, &end);
    if (end == copy) return 0;
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0') return 0;
    second = strtod(separator, &end);
    if (end == separator) return 0;
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0') return 0;
    *minimum = (float)first;
    *maximum = (float)second;
    return 1;
}

static int light_effect_config_path_build(char* path, size_t path_capacity,
                                          const char* config_root,
                                          const char* key)
{
    static const char middle[] = "::LightEffects::";
    size_t root_length;
    size_t middle_length = sizeof(middle) - 1u;
    size_t key_length;
    if (path == NULL || config_root == NULL || key == NULL ||
        path_capacity == 0u) return 0;
    root_length = strlen(config_root);
    key_length = strlen(key);
    if (root_length + middle_length + key_length + 1u > path_capacity)
        return 0;
    memcpy(path, config_root, root_length);
    memcpy(path + root_length, middle, middle_length);
    memcpy(path + root_length + middle_length, key, key_length + 1u);
    return 1;
}

static int light_effect_config_float_triplet_read(BlockFile* config,
                                                  const char* path,
                                                  float values[3])
{
    const char* value = block_get_strptr(config, path);
    char copy[256];
    char* cursor;
    unsigned int i;
    size_t length = 0u;
    if (value == NULL || values == NULL) return 0;
    while (length + 1u < sizeof(copy) && value[length] != '\0') {
        copy[length] = value[length];
        ++length;
    }
    if (value[length] != '\0') return 0;
    copy[length] = '\0';
    cursor = copy;
    for (i = 0u; i < 3u; ++i) {
        char* end;
        double parsed = strtod(cursor, &end);
        if (end == cursor) return 0;
        values[i] = (float)parsed;
        while (*end == ' ' || *end == '\t') ++end;
        if (i != 2u) {
            if (*end != ':') return 0;
            cursor = end + 1;
        }
        else if (*end != '\0') {
            return 0;
        }
    }
    return 1;
}

static uint32_t light_effect_float_word(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}

int light_effect_waypoint_config_load(void* config,
                                     const char* config_root)
{
    BlockFile* file = (BlockFile*)config;
    const char* root = config_root != NULL ? config_root : "Lego*";
    char path[256];
    float minimum[3];
    float maximum[3];
    float range_min[3];
    float range_max[3];
    float parameters[12];
    static const char* const keys[5] = {
        "FadeRGBMin", "FadeRGBMax", "RandomRangeForTimeBetweenFades",
        "RandomRangeForFadeTimeFade", "RandomRangeForHoldTimeOfFade"
    };
    unsigned int i;
    if (file == NULL) return 0;
    snprintf(path, sizeof(path), "%s::LightEffects::%s", root, keys[0]);
    if (!config_rgb_vector_read(file, path, &minimum[0], &minimum[1],
                                &minimum[2])) return 0;
    snprintf(path, sizeof(path), "%s::LightEffects::%s", root, keys[1]);
    if (!config_rgb_vector_read(file, path, &maximum[0], &maximum[1],
                                &maximum[2])) return 0;
    for (i = 0u; i < 3u; ++i) {
        snprintf(path, sizeof(path), "%s::LightEffects::%s", root,
                 keys[i + 2u]);
        if (!light_effect_config_range_pair_read(file, path,
                &range_min[i], &range_max[i])) return 0;
        range_min[i] *= 25.0f;
        range_max[i] *= 25.0f;
    }
    for (i = 0u; i < 3u; ++i) {
        parameters[i] = minimum[i];
        parameters[i + 3u] = maximum[i];
        parameters[i + 6u] = range_min[i];
        parameters[i + 9u] = range_max[i];
    }
    motion_waypoint_parameters_store(
        parameters[0], parameters[1], parameters[2],
        parameters[3], parameters[4], parameters[5],
        parameters[6], parameters[9], parameters[7], parameters[10],
        parameters[8], parameters[11]);
    light_effect_fade_parameters_store(
        light_effect_float_word(parameters[0]),
        light_effect_float_word(parameters[1]),
        light_effect_float_word(parameters[2]),
        light_effect_float_word(parameters[3]),
        light_effect_float_word(parameters[4]),
        light_effect_float_word(parameters[5]),
        light_effect_float_word(range_min[0]),
        light_effect_float_word(range_max[0]),
        light_effect_float_word(range_min[1]),
        light_effect_float_word(range_max[1]),
        light_effect_float_word(range_min[2]),
        light_effect_float_word(range_max[2]));
    return 1;
}

void light_effect_config_load_hook_set(LightEffectConfigLoadHook hook,
                                       void* context)
{
    s_light_effect_config_load_hook = hook;
    s_light_effect_config_load_context = context;
}

// FUNCTION: LEGORR 0x0044c7f0
uint32_t light_effect_blink_config_load(void* config, void* section)
{
    return s_light_effect_config_load_hook != NULL
        ? s_light_effect_config_load_hook(s_light_effect_config_load_context,
                                          0, config, section) : 0u;
}

// FUNCTION: LEGORR 0x0044c9c0
uint32_t light_effect_fade_config_load(void* config, void* section)
{
    return s_light_effect_config_load_hook != NULL
        ? s_light_effect_config_load_hook(s_light_effect_config_load_context,
                                          1, config, section) : 0u;
}

// FUNCTION: LEGORR 0x0044cca0
uint32_t light_effect_move_config_load(void* config,
                                      const char* config_root)
{
    BlockFile* file = (BlockFile*)config;
    const char* root = config_root != NULL ? config_root : "Lego*";
    static const char* const keys[4] = {
        "RandomRangeForTimeBetweenMoves",
        "RandomRangeForSpeedOfMove",
        "RandomRangeForDistOfMove",
        "MoveLimit"
    };
    char path[256];
    float time_min, time_max;
    float speed_min, speed_max;
    float distance_min, distance_max;
    float move_limit[3];
    if (file == NULL ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[0]) ||
        !light_effect_config_range_pair_read(file, path, &time_min,
                                             &time_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[1]) ||
        !light_effect_config_range_pair_read(file, path, &speed_min,
                                             &speed_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[2]) ||
        !light_effect_config_range_pair_read(file, path, &distance_min,
                                             &distance_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[3]) ||
        !light_effect_config_float_triplet_read(file, path, move_limit)) {
        return 0u;
    }
    light_effect_move_parameters_store(
        light_effect_float_word(time_min * 25.0f),
        light_effect_float_word(time_max * 25.0f),
        light_effect_float_word(speed_min * 25.0f),
        light_effect_float_word(speed_max * 25.0f),
        light_effect_float_word(distance_min),
        light_effect_float_word(distance_max),
        light_effect_float_word(move_limit[0]),
        light_effect_float_word(move_limit[1]),
        light_effect_float_word(move_limit[2]));
    return 1u;
}

// FUNCTION: LEGORR 0x0044c7c0
void light_effect_configuration_load(void* config,
                                    const char* config_root)
{
    (void)light_effect_blink_config_load(config, (void*)config_root);
    (void)light_effect_fade_config_load(config, (void*)config_root);
    (void)light_effect_move_config_load(config, config_root);
}

// FUNCTION: LEGORR 0x0044c970
void light_effect_blink_parameters_store(uint32_t a, uint32_t b,
                                         uint32_t c, uint32_t d,
                                         uint32_t e, uint32_t f)
{
    s_light_effect_parameters.blink[0] = a;
    s_light_effect_parameters.blink[1] = b;
    s_light_effect_parameters.blink[2] = c;
    s_light_effect_parameters.blink[3] = d;
    s_light_effect_parameters.blink[4] = e;
    s_light_effect_parameters.blink[5] = f;
    s_light_effect_parameters.flags |= 1u;
}

// FUNCTION: LEGORR 0x0044cc10
void light_effect_fade_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i,
                                        uint32_t j, uint32_t k, uint32_t l)
{
    s_light_effect_parameters.fade[0] = a;
    s_light_effect_parameters.fade[1] = b;
    s_light_effect_parameters.fade[2] = c;
    s_light_effect_parameters.fade[3] = d;
    s_light_effect_parameters.fade[4] = e;
    s_light_effect_parameters.fade[5] = f;
    s_light_effect_parameters.fade[6] = g;
    s_light_effect_parameters.fade[7] = h;
    s_light_effect_parameters.fade[8] = i;
    s_light_effect_parameters.fade[9] = j;
    s_light_effect_parameters.fade[10] = k;
    s_light_effect_parameters.fade[11] = l;
    s_light_effect_parameters.flags |= 4u;
}

void light_effect_move_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i)
{
    float parameters[9];
    memcpy(&parameters[0], &a, sizeof(a));
    memcpy(&parameters[1], &b, sizeof(b));
    memcpy(&parameters[2], &c, sizeof(c));
    memcpy(&parameters[3], &d, sizeof(d));
    memcpy(&parameters[4], &e, sizeof(e));
    memcpy(&parameters[5], &f, sizeof(f));
    memcpy(&parameters[6], &g, sizeof(g));
    memcpy(&parameters[7], &h, sizeof(h));
    memcpy(&parameters[8], &i, sizeof(i));
    s_light_effect_parameters.move[0] = a;
    s_light_effect_parameters.move[1] = b;
    s_light_effect_parameters.move[2] = c;
    s_light_effect_parameters.move[3] = d;
    s_light_effect_parameters.move[4] = e;
    s_light_effect_parameters.move[5] = f;
    s_light_effect_parameters.move[6] = g;
    s_light_effect_parameters.move[7] = h;
    s_light_effect_parameters.move[8] = i;
    s_light_effect_parameters.flags |= 0x40u;
    motion_jitter_parameters_store(
        parameters[0], parameters[1], parameters[2], parameters[3],
        parameters[4], parameters[5], parameters[6], parameters[7],
        parameters[8]);
}

const LightEffectParameterState* light_effect_parameter_state_peek(void)
{
    return &s_light_effect_parameters;
}

// FUNCTION: LEGORR 0x0044c710
void scene_transform_state_initialize(void* target, void* owner,
                                      float x, float y, float z)
{
    s_scene_transform_state.target = target;
    s_scene_transform_state.owner = owner;
    s_scene_transform_state.base[0] = x;
    s_scene_transform_state.base[1] = y;
    s_scene_transform_state.base[2] = z;
    s_scene_transform_state.current[0] = x;
    s_scene_transform_state.current[1] = y;
    s_scene_transform_state.current[2] = z;
    motion_gameplay_scene_bind(target, s_scene_transform_state.flags);
    motion_gameplay_scene_owner_bind(target, owner);
    motion_waypoint_position_bind(x, y, z);
    render_object_set_packed_rgba_from_float_channels(target, x, y, z,
                                                       1.0f);
}

// FUNCTION: LEGORR 0x0044c790
void scene_transform_enable_set(int enabled)
{
    if (enabled != 0) {
        s_scene_transform_state.flags |= 0x100u;
        render_object_set_packed_rgba_from_float_channels(
            s_scene_transform_state.target,
            s_scene_transform_state.base[0],
            s_scene_transform_state.base[1],
            s_scene_transform_state.base[2], 1.0f);
    }
    else {
        s_scene_transform_state.flags &= ~0x100u;
    }
    motion_gameplay_scene_bind(s_scene_transform_state.target,
                               s_scene_transform_state.flags);
}

void scene_transform_current_set(float x, float y, float z)
{
    s_scene_transform_state.current[0] = x;
    s_scene_transform_state.current[1] = y;
    s_scene_transform_state.current[2] = z;
}

// FUNCTION: LEGORR 0x0044cff0
void scene_transform_rgb_clamp_apply(void)
{
    float color[3];
    unsigned int i;
    for (i = 0; i < 3; ++i) {
        color[i] = s_scene_transform_state.current[i];
        if (color[i] < 0.0f) color[i] = 0.0f;
        if (color[i] > 1.0f) color[i] = 1.0f;
    }
    render_object_set_packed_rgba_from_float_channels(
        s_scene_transform_state.target, color[0], color[1], color[2], 1.0f);
}

const SceneTransformState* scene_transform_state_peek(void)
{
    return &s_scene_transform_state;
}

// FUNCTION: LEGORR 0x0046fef0
void effect_mode_surface_query_type_resolve(uint32_t mode,
                                            uint32_t* surface_type,
                                            uint32_t* flags)
{
    switch (mode) {
    case 1: *surface_type = 0x13; *flags = 0; break;
    case 2: *surface_type = 0x10; *flags = 0; break;
    case 3: *surface_type = 0x11; *flags = 0; break;
    case 4: *surface_type = 5;    *flags = 0; break;
    default: break;
    }
}

// FUNCTION: LEGORR 0x0046ff70
int active_effect_record_slot_find(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        if (s_active_effect_occupied[i] == 0 ||
            s_active_effect_records[i].spawned_entity == NULL) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x0046ff90
int active_segment_visual_record_slot_find(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        if (s_active_segment_visual_occupied[i] == 0 ||
            s_timed_segment_visuals[i][0] == NULL) return i;
    }
    return -1;
}

void active_effect_record_slot_set(int slot, int occupied)
{
    if (slot >= 0 && slot < 10)
        s_active_effect_occupied[slot] = occupied != 0 ? 1u : 0u;
}

void active_segment_visual_record_slot_set(int slot, int occupied)
{
    if (slot >= 0 && slot < 10)
        s_active_segment_visual_occupied[slot] = occupied != 0 ? 1u : 0u;
}

void active_effect_record_slots_clear(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        s_active_effect_occupied[i] = 0;
        memset(&s_active_effect_records[i], 0, sizeof(s_active_effect_records[i]));
        memset(&s_active_effect_tick_records[i], 0,
               sizeof(s_active_effect_tick_records[i]));
    }
}

void active_segment_visual_record_slots_clear(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        s_active_segment_visual_occupied[i] = 0;
        s_timed_segment_visuals[i][0] = NULL;
        s_timed_segment_visuals[i][1] = NULL;
        s_timed_segment_visual_owners[i] = NULL;
        memset(&s_timed_segment_tick_records[i], 0,
               sizeof(s_timed_segment_tick_records[i]));
    }
}

void entity_effect_motion_hooks_set(EntityImpulseApplyHook impulse_hook,
                                    EntityDirectionApplyHook direction_hook)
{
    s_entity_impulse_hook = impulse_hook;
    s_entity_direction_hook = direction_hook;
}

// FUNCTION: LEGORR 0x0046f380
void entity_apply_impulse_and_direction(void* entity, float magnitude,
                                        uint32_t direction)
{
    if (s_entity_impulse_hook != NULL) {
        s_entity_impulse_hook(entity, magnitude, 1, 1.0f);
    }
    else {
        entity_damage_apply((EntitySlot*)entity, magnitude, 1);
    }
    if (s_entity_direction_hook != NULL) {
        s_entity_direction_hook(entity, 0, direction);
    }
    else {
        entity_directional_activity_start((EntitySlot*)entity, NULL,
                                          (int)direction);
    }
}

void timed_segment_visual_hooks_set(const TimedSegmentVisualHooks* hooks)
{
    if (hooks == NULL) {
        s_timed_segment_visual_hooks.scene_get = NULL;
        s_timed_segment_visual_hooks.visual_create = NULL;
        s_timed_segment_visual_hooks.segment_build = NULL;
        s_timed_segment_visual_hooks.resource_release = NULL;
        s_timed_segment_visual_hooks.context = NULL;
        return;
    }
    s_timed_segment_visual_hooks = *hooks;
}

// FUNCTION: LEGORR 0x00470690
void timed_entity_segment_visual_create(void* entity, const float start[3],
                                       const float end[3])
{
    int slot = active_segment_visual_record_slot_find();
    void* scene;
    if (slot < 0 || s_timed_segment_visual_hooks.scene_get == NULL ||
        s_timed_segment_visual_hooks.visual_create == NULL ||
        s_timed_segment_visual_hooks.segment_build == NULL) return;
    scene = s_timed_segment_visual_hooks.scene_get(entity);
    s_timed_segment_visuals[slot][0] =
        s_timed_segment_visual_hooks.visual_create(scene, 0x4400u);
    s_timed_segment_visuals[slot][1] =
        s_timed_segment_visual_hooks.visual_create(scene, 0x4400u);
    s_timed_segment_visual_hooks.segment_build(s_timed_segment_visuals[slot][0],
                                               0.2f, start, end,
                                               0.6f, 0.6f, 0.6f, 1.0f);
    s_timed_segment_visual_hooks.segment_build(s_timed_segment_visuals[slot][1],
                                               1.0f, start, end,
                                               0.1f, 0.2f, 0.5f, 1.0f);
    s_timed_segment_visual_owners[slot] = entity;
    s_timed_segment_tick_records[slot].first_handle =
        s_timed_segment_visuals[slot][0];
    s_timed_segment_tick_records[slot].second_handle =
        s_timed_segment_visuals[slot][1];
    s_timed_segment_tick_records[slot].resource_owner = scene;
    s_timed_segment_tick_records[slot].remaining = 5.0f;
    active_segment_visual_record_slot_set(slot, 1);
}

// FUNCTION: LEGORR 0x00470760
void mesh_build_eight_sided_segment(void* mesh, float radius,
                                    const float start[3], const float end[3],
                                    float red, float green, float blue,
                                    float alpha)
{
    static const uint16_t triangle_indices[48] = {
        0, 1, 9, 9, 8, 0, 1, 2, 10, 10, 9, 1,
        2, 3, 11, 11, 10, 2, 3, 4, 12, 12, 11, 3,
        4, 5, 13, 13, 12, 4, 5, 6, 14, 14, 13, 5,
        6, 7, 15, 15, 14, 6, 7, 0, 8, 8, 15, 7
    };
    static const uint32_t texture_pairs[32] = {
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u
    };
    uint16_t source_indices[96];
    float positions[48];
    float normals[24];
    float dx;
    float dy;
    float dz;
    float inverse_length;
    float axis_y;
    float axis_z;
    float frame_x;
    float frame_y;
    float off_x[8];
    float off_y[8];
    float off_z[8];
    float f6;
    float f9;
    float f5;
    float f8;
    float fparam;
    float diagonal_x;
    float diagonal_y;
    float diagonal_z;
    float diagonal_r;
    float diagonal_f9;
    float normal_length;
    int record_index;
    int i;
    if (mesh == NULL || start == NULL || end == NULL) return;

    dx = end[0] - start[0];
    dy = end[1] - start[1];
    dz = end[2] - start[2];
    /* Keep the decompile's component evaluation order (z, y, x). */
    inverse_length = 1.0f / (float)sqrt(dz * dz + dy * dy + dx * dx);
    axis_y = inverse_length * dy;
    axis_z = inverse_length * dz;
    if (axis_y == 0.0f && axis_z == 0.0f) {
        frame_x = 1.0f;
        frame_y = 0.0f;
    }
    else if (axis_y == 0.0f) {
        frame_y = 1.0f;
        frame_x = 0.0f;
    }
    else {
        frame_x = (float)sqrt(1.0f / ((axis_z * axis_z) /
                                      (axis_y * axis_y) + 1.0f));
        frame_y = -(axis_z / axis_y) * frame_x;
    }

    f6 = frame_y * radius;
    f9 = frame_x * radius;
    f5 = (frame_x * axis_y - frame_y * axis_z) * radius;
    f8 = -(frame_x * inverse_length * dx) * radius;
    fparam = frame_y * inverse_length * dx * radius;
    diagonal_x = f6 * 0.41421354f;
    diagonal_y = f5 * 0.41421354f;
    diagonal_z = f8 * 0.41421354f;
    diagonal_r = fparam * 0.41421354f;
    diagonal_f9 = f9 * 0.41421354f;

    off_x[0] = diagonal_y; off_y[0] = f6 + diagonal_z; off_z[0] = f9 + diagonal_r;
    off_x[1] = f5; off_y[1] = f8 + diagonal_x; off_z[1] = fparam + diagonal_f9;
    off_x[2] = f5; off_y[2] = f8 - diagonal_x; off_z[2] = fparam - diagonal_f9;
    off_x[3] = diagonal_y; off_y[3] = diagonal_z - f6; off_z[3] = diagonal_r - f9;
    off_x[4] = -diagonal_y; off_y[4] = -f6 - diagonal_z; off_z[4] = -f9 - diagonal_r;
    off_x[5] = -f5; off_y[5] = -diagonal_x - f8; off_z[5] = -diagonal_f9 - fparam;
    off_x[6] = -f5; off_y[6] = diagonal_x - f8; off_z[6] = diagonal_f9 - fparam;
    off_x[7] = -diagonal_y; off_y[7] = f6 - diagonal_z; off_z[7] = f9 - diagonal_r;

    for (i = 0; i < 8; ++i) {
        float nx;
        float ny;
        float nz;
        int vertex = i * 3;
        positions[vertex] = start[0] + off_x[i];
        positions[vertex + 1] = start[1] + off_y[i];
        positions[vertex + 2] = start[2] + off_z[i];
        positions[24 + vertex] = end[0] + off_x[i];
        positions[24 + vertex + 1] = end[1] + off_y[i];
        positions[24 + vertex + 2] = end[2] + off_z[i];
        nx = off_x[i];
        ny = off_y[i];
        nz = off_z[i];
        normal_length = (float)sqrt(nx * nx + nz * nz + ny * ny);
        normals[vertex] = nx / normal_length;
        normals[vertex + 1] = ny / normal_length;
        normals[vertex + 2] = nz / normal_length;
    }
    for (i = 0; i < 48; ++i) {
        source_indices[i * 2] = triangle_indices[i];
        source_indices[i * 2 + 1] = 0;
    }

    record_index = mesh_record_allocate_or_append(mesh, 16u, 16u, 3u,
                                                   source_indices);
    if (record_index < 0) return;
    (void)render_object_color_vector_set(mesh, record_index, red, green, blue, 3);
    (void)screen_effect_item_opacity_set(mesh, record_index, alpha, 4);
    mesh_segment_vertex_fill(mesh, record_index, 0, 16, positions,
                             (const uint32_t*)normals, texture_pairs);
}

void kind_effect_dispatch_hooks_set(const KindEffectDispatchHooks* hooks)
{
    if (hooks == NULL) {
        s_kind_effect_dispatch_hooks.entity_position_get = NULL;
        s_kind_effect_dispatch_hooks.facing_vector_get = NULL;
        s_kind_effect_dispatch_hooks.terrain_height_get = NULL;
        s_kind_effect_dispatch_hooks.entity_half_height_get = NULL;
        s_kind_effect_dispatch_hooks.resource_duration_get = NULL;
        s_kind_effect_dispatch_hooks.effect_spawn = NULL;
        s_kind_effect_dispatch_hooks.cooldown_set = NULL;
        s_kind_effect_dispatch_hooks.context = NULL;
        return;
    }
    s_kind_effect_dispatch_hooks = *hooks;
}

// FUNCTION: LEGORR 0x004420d0
void kind_effect_slot_init_dispatch(void* entity, int kind)
{
    const char* resource_name;
    uint32_t resource_index;
    float position[3];
    float direction[3];
    float duration;
    EntitySlot* owner = (EntitySlot*)entity;
    GameContext* game;
    LevelState* level;
    if (owner == NULL) return;
    switch (kind) {
    case 1: resource_name = "Lazer"; break;
    case 2: resource_name = "Pusher"; break;
    case 3: resource_name = "Freezer"; break;
    case 4: resource_name = "Lazer"; break;
    default: return;
    }
    game = owner->owner_ctx;
    if (game == NULL) game = entity_pool_callback_context_get();
    level = game != NULL ? game->level : NULL;
    if (s_kind_effect_dispatch_hooks.entity_position_get != NULL) {
        if (!s_kind_effect_dispatch_hooks.entity_position_get(
                s_kind_effect_dispatch_hooks.context, entity, position)) return;
    }
    else if (owner->attached_scene_object != NULL &&
             ddiscene_object_position_get(owner->attached_scene_object, NULL,
                                          position)) {
        /* Use the live scene pose when it has been installed. */
    }
    else {
        position[0] = owner->x;
        position[1] = owner->y;
        position[2] = owner->z;
    }
    if (s_kind_effect_dispatch_hooks.facing_vector_get != NULL) {
        s_kind_effect_dispatch_hooks.facing_vector_get(
            s_kind_effect_dispatch_hooks.context, entity, direction);
    }
    else if (owner->attached_scene_object != NULL &&
             ddiscene_object_facing_get(owner->attached_scene_object, NULL,
                                        direction)) {
        /* The renderer's facing vector is authoritative when available. */
    }
    else {
        direction[0] = (float)cos((double)owner->rot);
        direction[1] = (float)sin((double)owner->rot);
        direction[2] = 0.0f;
    }
    if (s_kind_effect_dispatch_hooks.terrain_height_get != NULL) {
        position[2] = s_kind_effect_dispatch_hooks.terrain_height_get(
            s_kind_effect_dispatch_hooks.context, position[0], position[1]);
    }
    else if (level != NULL && level->ptr_terrain != NULL) {
        position[2] = terrain_surface_height_interpolate(
            level->ptr_terrain, position[0], position[1]);
    }
    else {
        return;
    }
    position[2] -= s_kind_effect_dispatch_hooks.entity_half_height_get != NULL
        ? s_kind_effect_dispatch_hooks.entity_half_height_get(
              s_kind_effect_dispatch_hooks.context, entity)
        : entity_stats_float_at74(owner) * 0.5f;
    resource_index = lego::object_type_name_index_find(resource_name);
    duration = s_kind_effect_dispatch_hooks.resource_duration_get != NULL
        ? s_kind_effect_dispatch_hooks.resource_duration_get(
              s_kind_effect_dispatch_hooks.context, resource_name)
        : lego::weapon_type_recharge_time_get((int)resource_index);
    if (s_kind_effect_dispatch_hooks.effect_spawn != NULL) {
        s_kind_effect_dispatch_hooks.effect_spawn(
            s_kind_effect_dispatch_hooks.context, entity, kind, position,
            direction, duration);
    }
    else if (kind >= 1 && kind <= 3) {
        /* FUN_0046faf0 owns the three direct-emitter modes. Its auxiliary
         * argument is the resolved object-type index used by the spawn
         * record, and the mode is the action kind. Kind 4 uses the separate
         * flight-curve record and needs the endpoint at owner +0x3a8, which
         * EntitySlot does not currently expose. */
        (void)entity_spawn_record_slot_init(owner, position, direction,
                                            resource_index, (uint32_t)kind);
    }
    if (s_kind_effect_dispatch_hooks.cooldown_set != NULL)
        s_kind_effect_dispatch_hooks.cooldown_set(
            s_kind_effect_dispatch_hooks.context, entity, duration);
}

void active_effect_build_hooks_set(const ActiveEffectBuildHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_active_effect_build_hooks, 0, sizeof(s_active_effect_build_hooks));
    }
    else s_active_effect_build_hooks = *hooks;
}

const ActiveEffectRecord* active_effect_record_peek(int slot)
{
    return slot >= 0 && slot < 10 ? &s_active_effect_records[slot] : NULL;
}

static void* active_effect_component_get(ActiveEffectRecord* record)
{
    EntitySlot* entity;
    if (record == NULL || record->spawned_entity == NULL) return NULL;
    entity = (EntitySlot*)record->spawned_entity;
    return entity->type_component_handle;
}

static BezierCurve* active_effect_path(const ActiveEffectRecord* record)
{
    return &s_active_effect_paths[record - s_active_effect_records];
}

/* FUN_0046f920's walk: the point `distance` along the flight path, as
 * (horizontal distance, height offset). */
static int active_effect_curve_sample(const ActiveEffectRecord* record,
                                      float distance, float point[2])
{
    Point2F r;
    if (record == NULL || point == NULL) return 0;
    (void)path_arc_length_interpolate(active_effect_path(record), distance,
                                      &r);
    point[0] = r.x;
    point[1] = r.y;
    return 1;
}

static void active_effect_projectile_update(void* owner_record, float dt,
                                            void*)
{
    ActiveEffectRecord* record = (ActiveEffectRecord*)owner_record;
    void* component = active_effect_component_get(record);
    float component_position[3];
    float direction_x;
    float direction_y;
    float direction_z;
    if (record == NULL || component == NULL ||
        !ddiscene_object_position_get(component, NULL, component_position))
        return;
    record->previous_position[0] = component_position[0];
    record->previous_position[1] = component_position[1];
    record->previous_position[2] = component_position[2];
    if (record->initialized == 0u) {
        float step = record->speed * dt;
        record->position[0] += record->direction[0] * step;
        record->position[1] += record->direction[1] * step;
        record->position[2] += record->direction[2] * step;
        direction_x = record->direction[0];
        direction_y = record->direction[1];
        direction_z = record->direction[2];
    }
    else {
        float curve_point[2];
        record->curve_elapsed += record->speed * dt;
        (void)active_effect_curve_sample(record, record->curve_elapsed,
                                         curve_point);
        record->position[0] = record->direction[0] * curve_point[0] +
                              record->curve_origin[0];
        record->position[1] = record->curve_origin[1] +
                              record->direction[1] * curve_point[0];
        record->position[2] = record->curve_origin[2] + curve_point[1];
        direction_x = record->position[0] - component_position[0];
        direction_y = record->position[1] - component_position[1];
        direction_z = (record->position[2] - component_position[2]) * 0.25f;
    }
    (void)ddiscene_object_position_set(component, NULL, record->position[0],
                                       record->position[1], record->position[2]);
    (void)ddiscene_object_pose_method_dispatch(component, NULL, direction_x,
        direction_y, direction_z, 0.0f, 0.0f, -1.0f);
}

static void active_effect_ray_hit_scan(void* owner_record, void*)
{
    ActiveEffectRecord* record = (ActiveEffectRecord*)owner_record;
    float delta[3];
    float hit_point[3];
    float max_distance = 1.0f;
    float range;
    float distance;
    void* hit_entity = NULL;
    EntitySlot* projectile;
    int hit;
    if (record == NULL || record->spawned_entity == NULL) return;
    projectile = (EntitySlot*)record->spawned_entity;
    range = lego::weapon_type_range_get((int)record->auxiliary);
    delta[0] = record->target[0] - record->position[0];
    delta[1] = record->target[1] - record->position[1];
    delta[2] = record->target[2] - record->position[2];
    distance = (float)sqrt(delta[0] * delta[0] + delta[1] * delta[1] +
                           delta[2] * delta[2]);
    if (range != 0.0f && range <= distance) {
        (void)entity_teardown(projectile);
        record->spawned_entity = NULL;
        s_active_effect_occupied[record - s_active_effect_records] = 0u;
        return;
    }
    delta[0] = record->position[0] - record->previous_position[0];
    delta[1] = record->position[1] - record->previous_position[1];
    delta[2] = record->position[2] - record->previous_position[2];
    hit = scene_segment_trace_nearest_hit(record->previous_position, delta,
                                          &hit_entity, &max_distance,
                                          record->spawned_entity);
    if (!hit) return;
    hit_point[0] = record->previous_position[0] + delta[0] * max_distance;
    hit_point[1] = record->previous_position[1] + delta[1] * max_distance;
    hit_point[2] = record->previous_position[2] + delta[2] * max_distance;
    (void)entity_teardown(projectile);
    record->spawned_entity = NULL;
    s_active_effect_occupied[record - s_active_effect_records] = 0u;
    if (hit_entity != NULL) {
        effect_apply_by_kind_branch(hit_entity, record->direction, hit_point,
                                    record->auxiliary, (int)record->mode);
    }
}

static SimtickSfxProjectileRecord* active_effect_tick_record_get(int slot,
                                                                 void*)
{
    if (slot < 0 || slot >= 10) return NULL;
    s_active_effect_tick_records[slot].tracked_entity =
        s_active_effect_records[slot].spawned_entity;
    s_active_effect_tick_records[slot].owner_record =
        &s_active_effect_records[slot];
    return &s_active_effect_tick_records[slot];
}

static SimtickSfxTimedVisualRecord* timed_visual_tick_record_get(int slot,
                                                                 void*)
{
    return slot >= 0 && slot < 10
        ? &s_timed_segment_tick_records[slot] : NULL;
}

static void timed_visual_resource_release(void* handle, void* resource_owner,
                                          void*)
{
    int i;
    if (s_timed_segment_visual_hooks.resource_release != NULL) {
        s_timed_segment_visual_hooks.resource_release(
            handle, resource_owner, s_timed_segment_visual_hooks.context);
    }
    for (i = 0; i < 10; ++i) {
        if (s_timed_segment_visuals[i][0] == handle) {
            s_timed_segment_visuals[i][0] = NULL;
            s_active_segment_visual_occupied[i] = 0u;
        }
        if (s_timed_segment_visuals[i][1] == handle)
            s_timed_segment_visuals[i][1] = NULL;
    }
}

void effects_simtick_sfx_hooks_install(void)
{
    SimtickSfxBankHooks hooks;
    if (s_sfx_tick_hooks_installed) return;
    memset(&hooks, 0, sizeof(hooks));
    hooks.projectile_record_get = active_effect_tick_record_get;
    hooks.projectile_update = active_effect_projectile_update;
    hooks.ray_hit_scan = active_effect_ray_hit_scan;
    hooks.timed_visual_record_get = timed_visual_tick_record_get;
    hooks.resource_release = timed_visual_resource_release;
    simtick_sfx_bank_hooks_set(&hooks);
    s_sfx_tick_hooks_installed = 1;
}

static int active_effect_type_resolve(uint32_t mode, uint32_t* type,
                                      uint32_t* flags)
{
    *type = 0;
    *flags = 0;
    switch (mode) {
    case 1: *type = 0x13; break;
    case 2: *type = 0x10; break;
    case 3: *type = 0x11; break;
    case 4: *type = 5; break;
    default: return 0;
    }
    return 1;
}

static void active_effect_direction_normalize(float direction[3])
{
    float length = (float)sqrt(direction[0] * direction[0] +
                               direction[1] * direction[1] +
                               direction[2] * direction[2]);
    float inverse = 1.0f / length;
    direction[0] *= inverse;
    direction[1] *= inverse;
    direction[2] *= inverse;
}

/* FUN_0046fc70's path: a cubic in the vertical plane from the start to the
 * end, (horizontal distance, height). It leaves at (1, -1.3) scaled to a
 * fifth of the distance and arrives along (0.3, 1) scaled to a third. The
 * total length becomes the flight duration. record->duration holds the
 * straight-line distance on entry. */
static void active_effect_curve_samples_build(ActiveEffectRecord* record,
                                              const float endpoint[3])
{
    BezierCurve* path = active_effect_path(record);
    float distance = record->duration;
    Point2F p0, p1, p2, p3;
    p0.x = 0.0f;
    p0.y = 0.0f;
    p1.x = 1.0f;
    p1.y = -1.3f;
    p2.x = distance;
    p2.y = endpoint[2] - record->curve_origin[2];
    p3.x = 0.3f;
    p3.y = 1.0f;
    vector2_set_length(&p1, distance * 0.2f);
    vector2_set_length(&p3, distance * 0.33333334f);
    bezier_curve_samples_build(path, &p0, &p1, &p2, &p3, BEZIER_MAXPOINTS);
    record->curve_total_length = polyline_segment_lengths_total(path);
}

// FUNCTION: LEGORR 0x0046faf0
int entity_spawn_record_slot_init(void* owner, const float position[3],
                                  const float direction[3],
                                  uint32_t auxiliary, uint32_t mode)
{
    uint32_t type, flags;
    int slot;
    unsigned int i;
    ActiveEffectRecord* record;
    /* Retail performs this owner/mode preparation before resolving the
     * surface pair or testing its table pointer. Use the injected callback
     * when present; otherwise run the live queued-grid animation path that
     * implements FUN_00437300(owner, auxiliary, 1.0). */
    if (mode == 1) {
        if (s_active_effect_build_hooks.pre_spawn_prepare != NULL) {
            s_active_effect_build_hooks.pre_spawn_prepare(
                s_active_effect_build_hooks.context, owner, auxiliary);
        }
        else {
            process_queued_grid_animations(owner, (int)auxiliary, 1.0f);
        }
    }
    if (position == NULL || direction == NULL ||
        !active_effect_type_resolve(mode, &type, &flags)) return 0;
    if (s_active_effect_build_hooks.surface_type_exists != NULL &&
        !s_active_effect_build_hooks.surface_type_exists(
            s_active_effect_build_hooks.context, type, flags)) return 0;
    slot = active_effect_record_slot_find();
    if (slot < 0) return 0;
    record = &s_active_effect_records[slot];
    memset(record, 0, sizeof(*record));
    record->auxiliary = auxiliary;
    record->mode = mode;
    record->owner = owner;
    record->speed = 5.0f;
    for (i = 0; i < 3; ++i) {
        record->position[i] = position[i];
        record->previous_position[i] = position[i];
        record->direction[i] = direction[i];
    }
    active_effect_direction_normalize(record->direction);
    if (s_active_effect_build_hooks.entity_spawn != NULL)
        record->spawned_entity = s_active_effect_build_hooks.entity_spawn(
            s_active_effect_build_hooks.context, type, flags, position);
    record->initialized = 0;
    active_effect_record_slot_set(slot,
                                  record->spawned_entity != NULL);
    if (s_active_effect_build_hooks.record_activate != NULL)
        s_active_effect_build_hooks.record_activate(
            s_active_effect_build_hooks.context, record, slot);
    return 1;
}

// FUNCTION: LEGORR 0x0046fc70
int object_flight_curve_record_init(void* owner, const float start[3],
                                    const float end[3], uint32_t auxiliary,
                                    uint32_t mode)
{
    uint32_t type, flags;
    int slot;
    unsigned int i;
    float distance;
    ActiveEffectRecord* record;
    if (start == NULL || end == NULL ||
        !active_effect_type_resolve(mode, &type, &flags)) return 0;
    if (s_active_effect_build_hooks.surface_type_exists != NULL &&
        !s_active_effect_build_hooks.surface_type_exists(
            s_active_effect_build_hooks.context, type, flags)) return 0;
    slot = active_effect_record_slot_find();
    if (slot < 0) return 0;
    record = &s_active_effect_records[slot];
    memset(record, 0, sizeof(*record));
    record->mode = mode;
    record->auxiliary = auxiliary;
    record->owner = owner;
    record->initialized = 1;
    record->speed = 4.0f;
    record->curve[0] = 0.3f;
    record->curve[1] = 1.0f;
    record->curve[2] = 1.0f;
    record->curve[3] = -1.3f;
    for (i = 0; i < 3; ++i) {
        record->position[i] = start[i];
        record->previous_position[i] = start[i];
        record->curve_origin[i] = start[i];
        record->direction[i] = end[i] - start[i];
    }
    active_effect_direction_normalize(record->direction);
    distance = (float)sqrt((end[0] - start[0]) * (end[0] - start[0]) +
                           (end[1] - start[1]) * (end[1] - start[1]) +
                           (end[2] - start[2]) * (end[2] - start[2]));
    record->duration = distance;
    active_effect_curve_samples_build(record, end);
    if (s_active_effect_build_hooks.flight_curve_build != NULL) {
        record->duration = s_active_effect_build_hooks.flight_curve_build(
            s_active_effect_build_hooks.context, record, end);
    }
    else {
        record->duration = record->curve_total_length;
    }
    if (s_active_effect_build_hooks.entity_spawn != NULL)
        record->spawned_entity = s_active_effect_build_hooks.entity_spawn(
            s_active_effect_build_hooks.context, type, flags, start);
    active_effect_record_slot_set(slot,
                                  record->spawned_entity != NULL);
    if (s_active_effect_build_hooks.record_activate != NULL)
        s_active_effect_build_hooks.record_activate(
            s_active_effect_build_hooks.context, record, slot);
    return 1;
}

void effect_apply_hooks_set(const EffectApplyHooks* hooks)
{
    if (hooks == NULL) memset(&s_effect_apply_hooks, 0, sizeof(s_effect_apply_hooks));
    else s_effect_apply_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0046f770
void effect_apply_by_kind_branch(void* entity, const float direction_xy[2],
                                 const float effect_position[3],
                                 uint32_t type, int mode)
{
    EntitySlot* target = (EntitySlot*)entity;
    uint32_t flags;
    float direction[2];
    if (entity == NULL || direction_xy == NULL) return;
    direction[0] = direction_xy[0];
    direction[1] = direction_xy[1];
    if (entity_type3_flag04000000_test(target) != 0 &&
        effect_position != NULL) {
        scene_light_create_at_position(effect_position);
    }
    flags = entity_state_flags_get(target);
    if (mode == 1) {
        if ((flags & 0x02000000u) != 0) {
            entity_apply_impulse_and_direction(
                entity, entity_stats_float_at11c(target), 1u);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(0, effect_position, NULL);
    }
    else if (mode == 2) {
        if ((flags & 0x04000000u) != 0) {
            float amount = entity_stats_float_at118(target);
            float scalar = entity_stats_float_at114(target);
            int32_t pair_words[2];
            int32_t scalar_word;
            direction[0] *= 1.5f;
            direction[1] *= 1.5f;
            entity_apply_impulse_and_direction(entity, amount, 1u);
            memcpy(pair_words, direction, sizeof(pair_words));
            memcpy(&scalar_word, &scalar, sizeof(scalar_word));
            (void)entity_pair_scalar_fields_set(target, pair_words,
                                                scalar_word);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(1, effect_position, NULL);
    }
    else if (mode == 3) {
        if ((flags & 0x01000000u) != 0) {
            float duration = entity_stats_float_at10c(target);
            float amount = entity_stats_float_at110(target);
            if (duration <= 0.0f) duration = 10.0f;
            entity_apply_impulse_and_direction(entity, amount, 0u);
            (void)entity_one_shot_attached_effect_start(target, duration);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(2, effect_position, NULL);
    }
    else if (s_effect_apply_hooks.generic_motion_apply != NULL) {
        s_effect_apply_hooks.generic_motion_apply(s_effect_apply_hooks.context,
                                                 entity, type, direction);
    }
}

void screen_space_effect_dispatch_hook_set(ScreenSpaceEffectDispatchHook hook,
                                           void* context)
{
    s_screen_space_effect_dispatch_hook = hook;
    s_screen_space_effect_dispatch_context = context;
}

void screen_effect_tick_hooks_set(const ScreenEffectTickHooks* hooks)
{
    if (hooks == NULL) memset(&s_screen_effect_tick_hooks, 0,
                              sizeof(s_screen_effect_tick_hooks));
    else s_screen_effect_tick_hooks = *hooks;
}

void screen_effect_slot_set(int pool, int slot, void* entity, float remaining)
{
    static const int capacities[3] = { 20, 20, 50 };
    if (pool < 0 || pool >= 3 || slot < 0 || slot >= capacities[pool]) return;
    s_screen_effect_tick_records[pool][slot].entity = entity;
    s_screen_effect_tick_records[pool][slot].remaining = remaining;
}

void* screen_effect_slot_peek(int pool, int slot, float* remaining)
{
    static const int capacities[3] = { 20, 20, 50 };
    if (remaining != NULL) *remaining = 0.0f;
    if (pool < 0 || pool >= 3 || slot < 0 || slot >= capacities[pool])
        return NULL;
    if (remaining != NULL)
        *remaining = s_screen_effect_tick_records[pool][slot].remaining;
    return s_screen_effect_tick_records[pool][slot].entity;
}

// FUNCTION: LEGORR 0x004075a0
void screen_effects_tick(float delta)
{
    static const int capacities[3] = { 20, 20, 50 };
    const int mode_zero = current_engine_mode_is_zero() ? 1 : 0;
    int paused = s_screen_effect_tick_hooks.simulation_paused != NULL
        ? s_screen_effect_tick_hooks.simulation_paused(
              s_screen_effect_tick_hooks.context) : mode_zero;
    int pool;
    int slot;
    /* The flagged-object ring window is owned by the AI registry module
     * (FUN_00407540); TickEntityScreenEffects advances its float timer by
     * reinterpreting the retained seed dword, clearing the pointer on expiry. */
    for (slot = 0; slot < 20; ++slot) {
        void* entity = NULL;
        uint32_t timer_bits = 0u;
        float timer;
        ai_object_registry_slot_peek(0, slot, &entity, &timer_bits);
        if (entity == NULL) continue;
        memcpy(&timer, &timer_bits, sizeof(timer));
        timer -= delta;
        memcpy(&timer_bits, &timer, sizeof(timer_bits));
        if (timer < 0.0f) entity = NULL;
        ai_object_registry_slot_set(0, slot, entity, timer_bits);
    }
    if (paused == 0) {
        if (s_screen_effect_pulse_timer >= 12.5f) {
            if (s_screen_effect_pulse_timer >= 25.0f) {
                s_screen_effect_pulse_timer -= 25.0f;
            } else if (s_screen_effect_tick_hooks.pulse_tick != NULL) {
                /* TickEntityScreenEffects' 20-entry pulse window is the
                 * type-4 registry (DAT_00557b24), not the independent
                 * 50-entry effect sidecar. Use the registry populated by
                 * Type4ObjectRegistryInsert (0x407410). */
                const int pulse_count = ai_object_registry_window_count(1);
                for (slot = 0; slot < pulse_count; ++slot) {
                    void* entity = NULL;
                    ai_object_registry_slot_peek(1, slot, &entity, NULL);
                    if (entity != NULL)
                        s_screen_effect_tick_hooks.pulse_tick(entity, delta,
                            s_screen_effect_tick_hooks.context);
                }
            }
        }
        s_screen_effect_pulse_timer += delta;
    }
    for (pool = 0; pool < 3; ++pool) {
        for (slot = 0; slot < capacities[pool]; ++slot) {
            struct ScreenEffectTickRecord* record =
                &s_screen_effect_tick_records[pool][slot];
            if (record->entity == NULL) continue;
            if (paused == 0 &&
                (s_screen_effect_tick_hooks.entity_eligible == NULL ||
                 s_screen_effect_tick_hooks.entity_eligible(record->entity,
                    pool, s_screen_effect_tick_hooks.context)) &&
                s_screen_effect_tick_hooks.entity_tick != NULL) {
                s_screen_effect_tick_hooks.entity_tick(record->entity, pool,
                    delta, s_screen_effect_tick_hooks.context);
            }
            if (pool < 2) {
                record->remaining -= delta;
                if (record->remaining < 0.0f) record->entity = NULL;
            } else if (record->remaining <= 0.0f) {
                record->entity = NULL;
            }
        }
    }
}

void transient_effect_hooks_set(const TransientEffectHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_transient_effect_hooks, 0, sizeof(s_transient_effect_hooks));
    } else {
        s_transient_effect_hooks = *hooks;
    }
}

void transient_effect_instance_handles_set(void* instance,
                                           void* scene_object,
                                           void* render_record)
{
    TransientEffectInstanceLinks* links = transient_effect_links_add(instance);
    if (links == NULL) return;
    links->scene_object = scene_object;
    links->render_record = render_record;
    transient_effect_link_pointer_set(instance, 0x28cu, scene_object);
    transient_effect_link_pointer_set(instance, 0x290u, render_record);
}

/* Keep the actual host-width registry in step with SceneEffectRegistryReset's
 * three observed words. The CRT mirror preserves the target-word inspector;
 * this owner holds the native pointers used by emitter traversal. */
int transient_effect_registry_reset(void)
{
    s_transient_effect_head = NULL;
    s_transient_effect_tail = NULL;
    s_transient_effect_texture_count = 0u;
    return scene_effect_registry_reset();
}

// FUNCTION: LEGORR 0x004653a0
void transient_effect_textures_preload(const char* filename_prefix,
                                      const char* filename_format,
                                      uint32_t texture_count)
{
    uint32_t i;
    if (s_transient_effect_hooks.textures_preload != NULL) {
        s_transient_effect_hooks.textures_preload(
            s_transient_effect_hooks.context, filename_prefix,
            filename_format, texture_count);
        return;
    }

    /* FUN_004653a0 keeps one cache reference per numbered bitmap.  The
     * executable's table is a DAT global; this port owns the same references
     * here until the next preload replaces the table. */
    for (i = 0; i < s_transient_effect_texture_count; ++i) {
        if (s_transient_effect_textures[i] != NULL)
            texture_release(s_transient_effect_textures[i]);
        s_transient_effect_textures[i] = NULL;
    }
    s_transient_effect_texture_count = texture_count > 12u
        ? 12u : texture_count;
    for (i = 0; i < s_transient_effect_texture_count; ++i) {
        char filename[260];
        if (filename_prefix == NULL || filename_format == NULL) break;
        sprintf(filename, filename_format, filename_prefix, (int)i);
        s_transient_effect_textures[i] = texture_get_or_load(NULL, filename);
    }
    if (i < s_transient_effect_texture_count)
        s_transient_effect_texture_count = i;
}

/* Renderer-owned resource creation stays behind the narrow record and mesh
 * hooks; the emitter record setup, simulation warm-up, and registry do not. */
// FUNCTION: LEGORR 0x00465430
void* transient_effect_emitter_create(void* scene_context,
                                      uint32_t position_x,
                                      uint32_t position_y, int* direction,
                                      float red, float green, float blue,
                                      float rate, int parameter_9, int mode,
                                      uint32_t seed, int auxiliary)
{
    static const uint16_t mesh_indices[6] = { 0u, 3u, 2u, 2u, 1u, 0u };
    unsigned char* bytes;
    uint32_t* words;
    const void* terrain = NULL;
    void* scene_object = NULL;
    void* render_record = NULL;
    uint32_t slot_count;
    unsigned int slot;
    float center[2];
    float position_x_world = 0.0f;
    float position_y_world = 0.0f;
    float position_z_world = 0.0f;
    float dx, dy, dz, inverse_length;
    void* instance;
    int owns_scene_object = 0;
    int render_failed = 0;

    instance = engine_heap_alloc(0x2a8L);
    if (instance == NULL || direction == NULL) {
        if (instance != NULL) engine_heap_free(instance);
        return NULL;
    }
    memset(instance, 0, 0x2a8u);
    bytes = (unsigned char*)instance;
    words = (uint32_t*)instance;

    if (mode == 0 || mode == 2) {
        GameContext* game;
        float block_size;
        int block_size_integer;
        int cell_x, cell_y;
        int sample_x, sample_y;
        uint32_t jitter_x, jitter_y;
        if (s_transient_effect_hooks.current_terrain_get != NULL)
            terrain = s_transient_effect_hooks.current_terrain_get(
                s_transient_effect_hooks.context);
        else {
            /* FUN_00465430 reads the active level's terrain handle. When no
             * renderer override is installed, GameContext owns that handle. */
            game = entity_pool_callback_context_get();
            if (game != NULL && game->level != NULL)
                terrain = game->level->ptr_terrain;
        }
        if (terrain == NULL) {
            engine_heap_free(instance);
            return NULL;
        }
        block_size = terrain_blocksize_get(terrain);
        block_size_integer = crt_float_to_long(block_size);
        if (block_size_integer <= 0 ||
            !terrain_cell_world_center_xy_get(
                terrain, (int)position_x, (int)position_y, center)) {
            engine_heap_free(instance);
            return NULL;
        }
        jitter_x = ((seed % 0x109eu) % (uint32_t)block_size_integer);
        jitter_y = ((seed % 0x5ebu) % (uint32_t)block_size_integer);
        position_x_world = (float)jitter_x - block_size * 0.5f + center[0];
        position_y_world = (float)jitter_y - block_size * 0.5f + center[1];
        if (terrain_world_xy_to_cell(terrain, position_x_world,
                                     position_y_world, &sample_x, &sample_y)) {
            position_z_world = (float)terrain_surface_height_get(
                terrain, sample_x, sample_y);
        }
        else {
            sample_x = (int)position_x;
            sample_y = (int)position_y;
            position_z_world = (float)terrain_surface_height_get(
                terrain, sample_x, sample_y);
        }
        scene_object = lego::ddiscene_object_allocate(
            (const lego::DDiSceneObjectDescriptor*)scene_root_object_get());
        if (scene_object == NULL) {
            engine_heap_free(instance);
            return NULL;
        }
        owns_scene_object = 1;
        (void)ddiscene_object_position_set(scene_object, NULL,
                                           position_x_world,
                                           position_y_world,
                                           position_z_world);
        words[0x97] = (uint32_t)crt_float_to_long(position_x_world);
        words[0x98] = (uint32_t)crt_float_to_long(position_y_world);
        words[0x99] = (uint32_t)crt_float_to_long(position_z_world);
        words[0xa5] = 2u;
        if ((seed & 3u) == 0u) words[0xa8] = (uint32_t)auxiliary;
    }
    else if (mode == 1) {
        scene_object = scene_context;
        words[0xa5] = 4u;
    }
    else {
        engine_heap_free(instance);
        return NULL;
    }

    dx = (float)direction[0];
    dy = (float)direction[1];
    dz = (float)direction[2];
    inverse_length = 1.0f / (float)sqrt(dx * dx + dy * dy + dz * dz);
    words[0x9a] = (uint32_t)(int)(dx * inverse_length);
    words[0x9b] = (uint32_t)(int)(dy * inverse_length);
    words[0x9c] = (uint32_t)(int)(dz * inverse_length);
    words[0x9d] = (uint32_t)(int)red;
    words[0x9e] = (uint32_t)(int)green;
    words[0x9f] = (uint32_t)(int)blue;

    if (mode == 2) {
        words[0xa0] = 4u;
        words[0xa1] = 0u;
        words[0xa5] |= 8u;
        words[0xa2] = 0x40000000u;
    }
    else {
        if ((0.0f < rate) || (rate < 1.0f))
            slot_count = (uint32_t)crt_float_to_long(rate);
        else
            slot_count = 10u;
        words[0xa0] = slot_count;
        words[0xa2] = (uint32_t)parameter_9;
    }
    if (scene_object == NULL ||
        s_transient_effect_hooks.render_record_create == NULL) {
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }
    render_record = s_transient_effect_hooks.render_record_create(
        s_transient_effect_hooks.context,
        *(void**)((unsigned char*)scene_object + 4), 0x00465ee0u,
        0x4800u, instance);
    if (render_record == NULL) {
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }
    transient_effect_instance_handles_set(instance, scene_object,
                                           render_record);

    if (words[0xa0] != 0u) {
        if (s_transient_effect_texture_count == 0u) {
            render_failed = 1;
        }
        for (slot = 0; !render_failed && slot < words[0xa0]; ++slot) {
            int render_slot = mesh_record_allocate_or_append(
                render_record, 4u, 2u, 3u, mesh_indices);
            if (render_slot < 0) {
                render_failed = 1;
                break;
            }
            lego::animated_texture_frame_surface_set(
                render_record, render_slot,
                s_transient_effect_textures[
                    slot % s_transient_effect_texture_count]);
            (void)render_object_color_vector_set(
                render_record, render_slot, red, green, blue, 0);
            (void)render_object_color_vector_set(
                render_record, render_slot, red * 0.2f, green * 0.2f,
                blue * 0.2f, 3);
            indexed_record_flag6c_bit0_set(render_record, render_slot, 1);
            {
                uint32_t random_value = (uint32_t)level_rand_u16();
                int32_t* particle = (int32_t*)(bytes +
                    (size_t)render_slot * 0x3cu);
                float first = (float)(random_value % 4u) + 6.0f;
                random_value = (uint32_t)level_rand_u16();
                particle[10] = (int32_t)first;
                particle[11] = (int32_t)((float)(random_value % 4u) +
                                           first + 2.0f);
                if ((words[0xa5] & 8u) == 0u)
                    particle[10] = (int32_t)((float)particle[10] * 0.5f);
                else
                    particle[11] = (int32_t)((float)particle[11] * 2.0f);
            }
        }
    }
    if (render_failed) {
        if (s_transient_effect_hooks.render_record_release != NULL)
            s_transient_effect_hooks.render_record_release(
                s_transient_effect_hooks.context, render_record);
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }

    *(float*)(bytes + 0x258) = 0.0f;
    if (mode != 2) {
        int frame;
        for (frame = 0; frame < 10; ++frame) {
            if (*(float*)(bytes + 0x258) <= 0.0f) {
                for (slot = 0; slot < words[0xa0]; ++slot) {
                    if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) == 0u) {
                        (void)transient_effect_particle_slot_initialize(
                            instance, slot);
                        break;
                    }
                }
            }
            *(float*)(bytes + 0x258) -= 5.0f;
            for (slot = 0; slot < words[0xa0]; ++slot) {
                if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) != 0u)
                    transient_effect_segment_advance(instance, slot, 5.0f);
            }
        }
    }
    {
        TransientEffectInstanceLinks* links =
            transient_effect_links_add(instance);
        TransientEffectInstanceLinks* tail =
            transient_effect_links_find(s_transient_effect_tail);
        if (links == NULL) {
            if (s_transient_effect_hooks.render_record_release != NULL)
                s_transient_effect_hooks.render_record_release(
                    s_transient_effect_hooks.context, render_record);
            if (owns_scene_object) ddiscene_object_destroy(scene_object);
            if (s_transient_effect_hooks.instance_release != NULL)
                s_transient_effect_hooks.instance_release(
                    s_transient_effect_hooks.context, instance, NULL);
            else
                engine_heap_free(instance);
            return NULL;
        }
        links->next = NULL;
        links->previous = tail;
        transient_effect_link_pointer_set(instance, 0x298u, NULL);
        transient_effect_link_pointer_set(instance, 0x29cu,
                                          s_transient_effect_tail);
        if (tail != NULL) {
            tail->next = links;
            transient_effect_link_pointer_set(tail->instance, 0x298u,
                                              instance);
        }
        else
            s_transient_effect_head = instance;
    }
    if (s_transient_effect_tail == NULL)
        s_transient_effect_head = instance;
    s_transient_effect_tail = instance;
    return instance;
}

// FUNCTION: LEGORR 0x00465c50
int transient_effect_particle_slot_initialize(void* emitter,
                                              uint32_t slot)
{
    unsigned char* bytes = (unsigned char*)emitter;
    uint32_t* slot_words;
    uint32_t random_value;
    float random_vector[3];
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float z;
    float scale;
    float speed;
    int result;
    if (emitter == NULL) return 0;
    slot_words = (uint32_t*)(bytes + (size_t)slot * 0x3cu);
    if ((*(uint32_t*)(bytes + 0x294) & 8u) != 0u) {
        uint32_t count = *(uint32_t*)(bytes + 0x284);
        if (count == 3u) *(uint32_t*)(bytes + 0x294) |= 0x10u;
        *(uint32_t*)(bytes + 0x284) = count + 1u;
    }
    slot_words[0] = 1u;
    indexed_record_flag6c_bit0_set(transient_effect_render_record_get(emitter),
                                   (int)slot, 0);
    d3drm_vector_random(random_vector);
    z = -fabsf(random_vector[2]) * 0.17f - 0.83f;
    ((float*)slot_words)[1] = random_vector[0] * 0.17f;
    ((float*)slot_words)[2] = random_vector[1] * 0.17f;
    ((float*)slot_words)[3] = z;
    z += *(float*)(bytes + 0x270);
    speed = *(float*)(bytes + 0x288);
    scale = -speed / z;
    z *= scale;
    ((float*)slot_words)[4] = scale *
        (*(float*)(bytes + 0x268) + random_vector[0] * 0.17f);
    ((float*)slot_words)[5] = scale *
        (*(float*)(bytes + 0x26c) + random_vector[1] * 0.17f);
    ((float*)slot_words)[6] = z;
    *(uint32_t*)(bytes + 0x294) = 4u;
    (void)ddiscene_object_position_get(
        transient_effect_scene_object_get(emitter), NULL, position);
    ((float*)slot_words)[7] = position[0];
    ((float*)slot_words)[8] = position[1];
    ((float*)slot_words)[9] = position[2];
    slot_words[12] = 0u;
    slot_words[14] = 0u;
    random_value = (uint32_t)level_rand_u16();
    slot_words[13] = random_value % 0x1eu + 0x28u;
    *(float*)(bytes + 600) = (0.5f / speed) *
                              ((float*)slot_words)[11];
    result = (int)(random_value / 0x1eu);
    return result;
}

// FUNCTION: LEGORR 0x00465e00
void transient_effect_segment_advance(void* emitter, uint32_t slot,
                                      float delta)
{
    unsigned char* bytes = (unsigned char*)emitter;
    float* particle;
    float distance;
    void* resource;
    if (emitter == NULL) return;
    particle = (float*)(bytes + (size_t)slot * 0x3cu);
    resource = transient_effect_render_record_get(emitter);
    particle[7] += particle[4] * delta;
    particle[8] += particle[5] * delta;
    particle[9] += particle[6] * delta;
    distance = fabsf(particle[6] * delta) + particle[14];
    particle[14] = distance;
    if ((float)(int)particle[13] <= distance) {
        *(uint32_t*)particle = 0u;
        indexed_record_flag6c_bit0_set(resource, (int)slot, 1);
        return;
    }
    particle[12] += delta;
    (void)screen_effect_item_opacity_set(resource, (int)slot,
        0.7f - (distance / (float)(int)particle[13]) * 0.7f, 4);
}

void transient_effect_quad_basis_hook_set(
    TransientEffectQuadBasisGetHook hook, void* context)
{
    s_transient_effect_quad_basis_hook = hook;
    s_transient_effect_quad_basis_context = context;
}

/* InstanceQuadVerticesBuild (FUN_00465f40): use the particle's current
 * center/size/distance and scene orientation to build its four corners.
 * Hosts may override the orientation lookup, while the normal scene object
 * path uses the same pose interface as the retail +0x7c call. */
// FUNCTION: LEGORR 0x00465f40
void transient_effect_instance_quad_vertices_build(
    void* instance, uint32_t slot, void* transform_source,
    const float triple[3])
{
    static const uint32_t uv_pairs[8] = {
        0u, 0x3f800000u, 0u, 0u,
        0x3f800000u, 0u, 0x3f800000u, 0x3f800000u
    };
    unsigned char* bytes = (unsigned char*)instance;
    float* particle;
    float orientation[3];
    float side[3];
    float up[3] = { 0.0f, 0.0f, -1.0f };
    float scale;
    float side_scale;
    float up_scale;
    float positions[12];
    void* record;
    int axis;

    if (instance == NULL || triple == NULL) return;
    if (slot >= *(uint32_t*)(bytes + 0x280u)) return;
    particle = (float*)(bytes + (size_t)slot * 0x3cu);
    record = transient_effect_render_record_get(instance);
    if (record == NULL) return;
    if (s_transient_effect_quad_basis_hook != NULL) {
        if (!s_transient_effect_quad_basis_hook(
                s_transient_effect_quad_basis_context, transform_source,
                orientation)) return;
    }
    else if (!ddiscene_object_facing_get(transform_source, NULL,
                                         orientation)) return;

    side[0] = orientation[1];
    side[1] = -orientation[0];
    side[2] = 0.0f;
    if (orientation[2] > 0.3f)
        d3drm_vector_rotate(up, up, side, 0.7f);

    scale = particle[14] / (float)(int)particle[13] + 1.0f;
    side_scale = particle[10] * scale;
    up_scale = particle[11] * scale;
    for (axis = 0; axis < 3; ++axis) {
        float center = particle[7 + axis];
        side[axis] *= side_scale;
        up[axis] *= up_scale;
        positions[axis] = center + side[axis];
        positions[3 + axis] = center - side[axis];
        positions[6 + axis] = positions[3 + axis] - up[axis];
        positions[9 + axis] = positions[axis] - up[axis];
    }
    instance_quad_vertex_data_fill(record, (int)slot, 0, 4, positions,
                                   triple, uv_pairs);
}

// FUNCTION: LEGORR 0x00465b00
void transient_effect_instances_tick(float delta)
{
    void* instance = s_transient_effect_head;
    while (instance != NULL) {
        unsigned char* bytes = (unsigned char*)instance;
        void* next = transient_effect_next_get(instance);
        uint32_t flags;
        int all_inactive = 1;
        uint32_t slot_count;
        uint32_t slot;
        if ((*(uint32_t*)(bytes + 0x294) & 0x20u) == 0u &&
            *(uint32_t*)(bytes + 0x2a4) == 0u) {
            if (*(uint32_t*)(bytes + 0x2a0) != 0u) {
                int sound = sound_positional_sound_submit(0,
                    (int)*(uint32_t*)(bytes + 0x2a0), 1, 0,
                    (const float*)(bytes + 0x25c));
                *(uint32_t*)(bytes + 0x2a4) = (uint32_t)sound;
            }
            if (*(uint32_t*)(bytes + 0x2a4) == 0u)
                *(uint32_t*)(bytes + 0x294) |= 0x20u;
        }
        flags = *(uint32_t*)(bytes + 0x294);
        if ((flags & 1u) == 0u) {
            if ((flags & 8u) == 0u || (flags & 0x10u) == 0u) {
                if (*(float*)(bytes + 0x258) <= 0.0f) {
                    slot_count = *(uint32_t*)(bytes + 0x280);
                    for (slot = 0u; slot < slot_count; ++slot) {
                        if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) == 0u) {
                            transient_effect_particle_slot_initialize(
                                instance, slot);
                            break;
                        }
                    }
                }
                *(float*)(bytes + 0x258) -= delta;
            }
            slot_count = *(uint32_t*)(bytes + 0x280);
            for (slot = 0u; slot < slot_count; ++slot) {
                if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) != 0u) {
                    transient_effect_segment_advance(instance, slot, delta);
                    all_inactive = 0;
                }
            }
            flags = *(uint32_t*)(bytes + 0x294);
            if ((flags & 0x18u) == 0x18u && all_inactive)
                transient_effect_instance_release(instance,
                    transient_effect_scene_object_get(instance));
        }
        instance = next;
    }
}

// FUNCTION: LEGORR 0x004659b0
void transient_effect_instance_release(void* instance, void* resource)
{
    unsigned char* bytes;
    void* render_record;
    uint32_t sound_handle;
    void* previous;
    void* next;
    TransientEffectInstanceLinks* links;
    TransientEffectInstanceLinks* all_previous;
    if (instance == NULL) return;
    bytes = (unsigned char*)instance;
    /* The reference selects the retained resource only for instances whose
     * ownership bit is set; otherwise its second argument is the resource
     * whose render instance must be detached. */
    if ((bytes[0x294] & 2u) != 0u)
        resource = transient_effect_scene_object_get(instance);
    render_record = transient_effect_render_record_get(instance);
    memcpy(&sound_handle, bytes + 0x2a4u, sizeof(sound_handle));
    links = transient_effect_links_find(instance);
    next = transient_effect_next_get(instance);
    previous = transient_effect_previous_get(instance);
    if (next != NULL) {
        TransientEffectInstanceLinks* next_links =
            transient_effect_links_find(next);
        if (next_links != NULL) next_links->previous = links != NULL
            ? links->previous : NULL;
        transient_effect_link_pointer_set(next, 0x29cu, previous);
    }
    else s_transient_effect_tail = previous;
    if (previous != NULL) {
        TransientEffectInstanceLinks* previous_links =
            transient_effect_links_find(previous);
        if (previous_links != NULL) previous_links->next = links != NULL
            ? links->next : NULL;
        transient_effect_link_pointer_set(previous, 0x298u, next);
    }
    else s_transient_effect_head = next;
    if (links != NULL) {
        all_previous = NULL;
        if (s_transient_effect_links == links)
            s_transient_effect_links = links->all_next;
        else {
            all_previous = s_transient_effect_links;
            while (all_previous != NULL && all_previous->all_next != links)
                all_previous = all_previous->all_next;
            if (all_previous != NULL) all_previous->all_next = links->all_next;
        }
    }
    /* FUN_004659b0 detaches the emitter render instance before releasing the
     * owned scene object, then stops its latched sound handle before freeing
     * the emitter record. */
    if (render_record != NULL &&
        s_transient_effect_hooks.render_record_release != NULL) {
        s_transient_effect_hooks.render_record_release(
            s_transient_effect_hooks.context, render_record);
    }
    if ((bytes[0x294] & 2u) != 0u && resource != NULL)
        ddiscene_object_destroy(resource);
    if (sound_handle != 0u)
        sound_handle_stop_forward((int)sound_handle);
    if (s_transient_effect_hooks.instance_release != NULL) {
        s_transient_effect_hooks.instance_release(
            s_transient_effect_hooks.context, instance, resource);
    }
    else {
        engine_heap_free(instance);
    }
    if (links != NULL) free(links);
}

// FUNCTION: LEGORR 0x00465970
void transient_effect_registry_release_all(void)
{
    while (s_transient_effect_head != NULL) {
        void* instance = s_transient_effect_head;
        void* resource = transient_effect_scene_object_get(instance);
        transient_effect_instance_release(instance, resource);
    }
    s_transient_effect_head = NULL;
    s_transient_effect_tail = NULL;
}

// FUNCTION: LEGORR 0x00465a90
void transient_effect_registry_enable_set(int enabled)
{
    void* instance;
    s_transient_effect_enabled = enabled != 0;
    for (instance = s_transient_effect_head; instance != NULL;
         instance = transient_effect_next_get(instance)) {
        transient_effect_enable_set(instance, s_transient_effect_enabled);
    }
}

// FUNCTION: LEGORR 0x00465ac0
void transient_effect_enable_set(void* instance, int enabled)
{
    unsigned char* bytes;
    uint32_t* flags;
    if (instance == NULL) return;
    bytes = (unsigned char*)instance;
    flags = (uint32_t*)(bytes + 0x294);
    record_flag2c_bit0_set(transient_effect_render_record_get(instance),
                            enabled);
    if (enabled != 0) *flags |= 1u;
    else *flags &= ~1u;
}

int transient_effect_registry_enabled_get(void)
{
    return s_transient_effect_enabled;
}

void timed_effect_runtime_hooks_set(const TimedEffectRuntimeHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_timed_effect_runtime_hooks, 0,
               sizeof(s_timed_effect_runtime_hooks));
    else
        s_timed_effect_runtime_hooks = *hooks;
}

void timed_effect_slot_render_hooks_set(
    const TimedEffectSlotRenderHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_timed_effect_slot_render_hooks, 0,
               sizeof(s_timed_effect_slot_render_hooks));
    else
        s_timed_effect_slot_render_hooks = *hooks;
}

TimedEffectRecord* timed_effect_record_get(unsigned int slot)
{
    return (TimedEffectRecord*)timed_effect_record_storage_get(slot);
}

void timed_effect_cue_request_set(unsigned int slot, uint32_t cue)
{
    TimedEffectRecord* record = timed_effect_record_get(slot);
    if (record == NULL) return;
    record->cue = cue;
    record->flags |= 3u;
}

void timed_effect_persistence_set(unsigned int slot, int enabled)
{
    TimedEffectRecord* record = timed_effect_record_get(slot);
    if (record == NULL) return;
    if (enabled != 0) record->flags |= 0x10u;
    else record->flags &= ~0x10u;
}

void timed_effect_update_request_set(unsigned int slot)
{
    TimedEffectRecord* record = timed_effect_record_get(slot);
    if (record != NULL) record->flags |= 4u;
}

void timed_effect_timestamp_reset(unsigned int slot)
{
    TimedEffectRecord* record = timed_effect_record_get(slot);
    if (record != NULL) {
        uint32_t tick = timed_effect_tick_word_peek();
        memcpy(&record->timestamp, &tick, sizeof(record->timestamp));
        if (s_timed_effect_runtime_hooks.time_get != NULL)
            record->timestamp = s_timed_effect_runtime_hooks.time_get(
                s_timed_effect_runtime_hooks.context);
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

void timed_effect_hover_reset(void)
{
    unsigned int slot;
    for (slot = 1u; slot < 39u; ++slot) {
        TimedEffectRecord* record = timed_effect_record_get(slot);
        if (record != NULL) {
            record->timestamp = 0.0f;
            record->flags &= ~4u;
        }
    }
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

/* FUNCTION: LEGORR 0x0040e8d0
 * The reference gates a ground-marker draw on bit 0 of its effect mode,
 * bit 1 of the render flags, and a non-null effect object. The callback
 * seam represents the renderer call at 0x004248b0 while keeping this
 * helper callable without importing Direct3DRM into game logic. */
void screen_space_effect_dispatch_if_enabled(uint8_t effect_flags,
                                             uint32_t render_flags,
                                             void* effect, void* argument)
{
    if ((effect_flags & 1u) == 0u || (render_flags & 2u) == 0u ||
        effect == NULL || s_screen_space_effect_dispatch_hook == NULL)
        return;
    s_screen_space_effect_dispatch_hook(s_screen_space_effect_dispatch_context,
                                        effect, argument, 0, 1.0f, 0);
}

namespace unported_ledger_stubs {

void ledger_stub_004653a0_TransientEffectTexturesPreload(
    const char* filename_prefix, const char* filename_format,
    uint32_t texture_count)
{
    transient_effect_textures_preload(filename_prefix, filename_format,
                                      texture_count);
}

void* ledger_stub_00465430_TransientEffectEmitterCreate(
    void* scene_context, uint32_t position_x, uint32_t position_y,
    int* direction, float red, float green, float blue, float rate,
    int sound_id, int mode, uint32_t seed, int auxiliary)
{
    return transient_effect_emitter_create(scene_context, position_x,
        position_y, direction, red, green, blue, rate, sound_id, mode,
        seed, auxiliary);
}

void ledger_stub_00465970_TransientEffectRegistryReleaseAll(void)
{
    transient_effect_registry_release_all();
}

void ledger_stub_004659b0_TransientEffectInstanceRelease(void* instance,
                                                        void* alternate)
{
    transient_effect_instance_release(instance, alternate);
}

void ledger_stub_00465a90_TransientEffectRegistryEnableSet(int enabled)
{
    transient_effect_registry_enable_set(enabled);
}

void ledger_stub_00465ac0_TransientEffectEnableSet(void* instance,
                                                  int enabled)
{
    transient_effect_enable_set(instance, enabled);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void ledger_stub_00465f40_InstanceQuadVerticesBuild(
    void* instance, uint32_t slot, void* transform_source,
    const float triple[3])
{
    transient_effect_instance_quad_vertices_build(instance, slot,
                                                  transform_source, triple);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void ledger_stub_0046b7c0_TimedEffectAndCueUpdate(
    uint32_t render_x, uint32_t render_y, float delta)
{
    timed_effect_runtime_update(render_x, render_y, delta);
}

void ledger_stub_0046b700_TimedEffectCueRequestSet(unsigned int slot,
                                                  uint32_t cue)
{
    timed_effect_cue_request_set(slot, cue);
}

void ledger_stub_0046b730_TimedEffectPersistenceSet(unsigned int slot,
                                                    int enabled)
{
    timed_effect_persistence_set(slot, enabled);
}

void ledger_stub_0046b770_TimedEffectUpdateRequestSet(unsigned int slot)
{
    timed_effect_update_request_set(slot);
}

void ledger_stub_0046b7a0_TimedEffectTimestampReset(unsigned int slot)
{
    timed_effect_timestamp_reset(slot);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

static float combat_ray_configured_range_get(void*, int kind)
{
    return lego::weapon_type_range_get(kind);
}

static int combat_ray_scene_trace(void*, const float origin[3],
                                  const float direction[3],
                                  float* hit_fraction, float hit_point[3],
                                  void** hit_object_out)
{
    float distance;
    float nearest_distance;
    void* hit_object = NULL;
    int hit;
    if (hit_fraction == NULL || hit_point == NULL || origin == NULL ||
        direction == NULL) return 0;
    distance = (float)sqrt((double)(direction[0] * direction[0] +
                                    direction[1] * direction[1] +
                                    direction[2] * direction[2]));
    if (distance <= 0.0f) return 0;
    /* SceneSegmentTraceNearestHit bounds and returns distance in units of
     * the supplied direction vector (0 at origin, 1 at its endpoint). */
    nearest_distance = 1.0f;
    hit = scene_segment_trace_nearest_hit(origin, direction, &hit_object,
                                          &nearest_distance, NULL);
    if (hit == 0) return 0;
    *hit_fraction = nearest_distance;
    hit_point[0] = origin[0] + direction[0] * *hit_fraction;
    hit_point[1] = origin[1] + direction[1] * *hit_fraction;
    hit_point[2] = origin[2] + direction[2] * *hit_fraction;
    if (hit_object_out != NULL) *hit_object_out = hit_object;
    return 1;
}

static void combat_ray_entity_release(void*, void* entity)
{
    (void)entity_teardown((EntitySlot*)entity);
}

// RayHitScanAndEffectApply
// FUNCTION: LEGORR 0x0046f3b0
void ledger_stub_0046f3b0_RayHitScanAndEffectApply(
    CombatRayState* state, const CombatRayHooks* hooks)
{
    CombatRayHooks live_hooks;
    if (hooks != NULL || state == NULL) {
        combat_ray_hit_scan_and_effect_apply(state, hooks);
        return;
    }
    live_hooks.configured_range_get = combat_ray_configured_range_get;
    live_hooks.raycast = combat_ray_scene_trace;
    live_hooks.entity_release = combat_ray_entity_release;
    /* The retail call passes the effect direction stored at record +0x290.
     * CombatRayState has no corresponding field, so leave this callback
     * unset instead of substituting the ray's origin-to-target delta. */
    live_hooks.impact_apply = NULL;
    live_hooks.context = state;
    combat_ray_hit_scan_and_effect_apply(state, &live_hooks);
}

// LaserSpatialSoundAndImpact
// FUNCTION: LEGORR 0x0046ffb0
void* ledger_stub_0046ffb0_LaserSpatialSoundAndImpact(
    void* request, int kind, float scale, const float origin[3],
    const float direction[3], const LaserImpactHooks* hooks)
{
    return laser_spatial_sound_and_impact(request, kind, scale, origin,
                                           direction, hooks);
}


} // namespace unported_ledger_stubs
