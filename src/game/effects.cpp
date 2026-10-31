// SPDX-License-Identifier: LGPL-3.0-or-later
#include "effects.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "crt/crt.hpp"
#include "engine.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/view.hpp"
#include "level.hpp"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t s_active_effect_occupied[10];
static uint32_t s_active_segment_visual_occupied[10];
static ActiveEffectRecord s_active_effect_records[10];
static ActiveEffectBuildHooks s_active_effect_build_hooks;
static EntityImpulseApplyHook s_entity_impulse_hook;
static EntityDirectionApplyHook s_entity_direction_hook;
static TimedSegmentVisualHooks s_timed_segment_visual_hooks;
static void* s_timed_segment_visuals[10][2];
static void* s_timed_segment_visual_owners[10];
static MeshSegmentQuadEmitHook s_mesh_segment_quad_emit_hook;
static KindEffectDispatchHooks s_kind_effect_dispatch_hooks;
static EffectApplyHooks s_effect_apply_hooks;
static ScreenSpaceEffectDispatchHook s_screen_space_effect_dispatch_hook;
static struct ScreenEffectTickRecord {
    void* entity;
    float remaining;
} s_screen_effect_tick_records[3][50];
static ScreenEffectTickHooks s_screen_effect_tick_hooks;
static float s_screen_effect_pulse_timer;
static void* s_screen_space_effect_dispatch_context;
static TransientEffectHooks s_transient_effect_hooks;
static TextureSurface* s_transient_effect_textures[12];
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
// GLOBAL: LEGORR 0x004aa64c
static uint32_t s_timed_effect_cue_gate = 1u;
static SceneTransformState s_scene_transform_state;
static SceneTransformApplyHook s_scene_transform_apply_hook;
static void* s_scene_transform_apply_context;
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
    overlay.x = x;
    overlay.y = y;
    overlay.width = top;
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
                               overlay.x + (float)state->padding,
                               overlay.y + (float)state->padding,
                               state->special_item.width,
                               state->special_item.height);
        }
        return;
    }
    if (state->items == NULL) return;
    row_x = overlay.x;
    row_height = 0.0f;
    for (i = 0; i < state->item_count; ++i) {
        const TimedEffectRenderItem* item = &state->items[i];
        if (!item->enabled || item->handle == NULL) continue;
        if (hooks->render_item != NULL)
            hooks->render_item(hooks->context, item->handle, row_x, overlay.y,
                               item->width, item->height);
        row_x += item->width;
        if (row_height < item->height) row_height = item->height;
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
uint32_t light_effect_move_config_load(void* config, void* section)
{
    return s_light_effect_config_load_hook != NULL
        ? s_light_effect_config_load_hook(s_light_effect_config_load_context,
                                          2, config, section) : 0u;
}

// FUNCTION: LEGORR 0x0044c7c0
void light_effect_configuration_load(void* config, void* section)
{
    (void)light_effect_blink_config_load(config, section);
    (void)light_effect_fade_config_load(config, section);
    (void)light_effect_move_config_load(config, section);
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

// FUNCTION: LEGORR 0x0044cef0
void light_effect_move_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i)
{
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
}

const LightEffectParameterState* light_effect_parameter_state_peek(void)
{
    return &s_light_effect_parameters;
}

void scene_transform_apply_hook_set(SceneTransformApplyHook hook,
                                    void* context)
{
    s_scene_transform_apply_hook = hook;
    s_scene_transform_apply_context = context;
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
    if (s_scene_transform_apply_hook != NULL) {
        s_scene_transform_apply_hook(s_scene_transform_apply_context, target,
                                      x, y, z, 1.0f);
    }
}

// FUNCTION: LEGORR 0x0044c790
void scene_transform_enable_set(int enabled)
{
    if (enabled != 0) {
        s_scene_transform_state.flags |= 0x100u;
        if (s_scene_transform_apply_hook != NULL) {
            s_scene_transform_apply_hook(
                s_scene_transform_apply_context,
                s_scene_transform_state.target,
                s_scene_transform_state.base[0],
                s_scene_transform_state.base[1],
                s_scene_transform_state.base[2], 1.0f);
        }
    }
    else {
        s_scene_transform_state.flags &= ~0x100u;
    }
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
    if (s_scene_transform_apply_hook == NULL) return;
    for (i = 0; i < 3; ++i) {
        color[i] = s_scene_transform_state.current[i];
        if (color[i] < 0.0f) color[i] = 0.0f;
        if (color[i] > 1.0f) color[i] = 1.0f;
    }
    s_scene_transform_apply_hook(s_scene_transform_apply_context,
                                 s_scene_transform_state.target,
                                 color[0], color[1], color[2], 1.0f);
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
        if (s_active_effect_occupied[i] == 0) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x0046ff90
int active_segment_visual_record_slot_find(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        if (s_active_segment_visual_occupied[i] == 0) return i;
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
    }
}

void active_segment_visual_record_slots_clear(void)
{
    int i;
    for (i = 0; i < 10; ++i) s_active_segment_visual_occupied[i] = 0;
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
    if (s_entity_impulse_hook != NULL)
        s_entity_impulse_hook(entity, magnitude, 1, 1.0f);
    if (s_entity_direction_hook != NULL)
        s_entity_direction_hook(entity, 0, direction);
}

void timed_segment_visual_hooks_set(const TimedSegmentVisualHooks* hooks)
{
    if (hooks == NULL) {
        s_timed_segment_visual_hooks.scene_get = NULL;
        s_timed_segment_visual_hooks.visual_create = NULL;
        s_timed_segment_visual_hooks.segment_build = NULL;
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
    active_segment_visual_record_slot_set(slot, 1);
}

void mesh_segment_quad_emit_hook_set(MeshSegmentQuadEmitHook hook)
{
    s_mesh_segment_quad_emit_hook = hook;
}

// FUNCTION: LEGORR 0x00470760
void mesh_build_eight_sided_segment(void* mesh, float radius,
                                    const float start[3], const float end[3],
                                    float red, float green, float blue,
                                    float alpha)
{
    float axis[3];
    float basis_a[3];
    float basis_b[3];
    float length;
    float inverse_length;
    float vertices[16][3];
    int i;
    if (mesh == NULL || start == NULL || end == NULL ||
        s_mesh_segment_quad_emit_hook == NULL) return;
    axis[0] = end[0] - start[0];
    axis[1] = end[1] - start[1];
    axis[2] = end[2] - start[2];
    length = (float)sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    inverse_length = 1.0f / length;
    axis[0] *= inverse_length;
    axis[1] *= inverse_length;
    axis[2] *= inverse_length;
    if ((float)fabs(axis[1]) < 0.9f) {
        basis_a[0] = -axis[2]; basis_a[1] = 0.0f; basis_a[2] = axis[0];
    }
    else {
        basis_a[0] = 0.0f; basis_a[1] = axis[2]; basis_a[2] = -axis[1];
    }
    inverse_length = 1.0f / (float)sqrt(basis_a[0] * basis_a[0] +
                                        basis_a[1] * basis_a[1] +
                                        basis_a[2] * basis_a[2]);
    basis_a[0] *= inverse_length;
    basis_a[1] *= inverse_length;
    basis_a[2] *= inverse_length;
    basis_b[0] = axis[1] * basis_a[2] - axis[2] * basis_a[1];
    basis_b[1] = axis[2] * basis_a[0] - axis[0] * basis_a[2];
    basis_b[2] = axis[0] * basis_a[1] - axis[1] * basis_a[0];
    for (i = 0; i < 8; ++i) {
        float angle = (float)i * 0.78539816339f;
        float ca = (float)cos(angle) * radius;
        float cb = (float)sin(angle) * radius;
        int end_index = i + 8;
        vertices[i][0] = start[0] + basis_a[0] * ca + basis_b[0] * cb;
        vertices[i][1] = start[1] + basis_a[1] * ca + basis_b[1] * cb;
        vertices[i][2] = start[2] + basis_a[2] * ca + basis_b[2] * cb;
        vertices[end_index][0] = end[0] + basis_a[0] * ca + basis_b[0] * cb;
        vertices[end_index][1] = end[1] + basis_a[1] * ca + basis_b[1] * cb;
        vertices[end_index][2] = end[2] + basis_a[2] * ca + basis_b[2] * cb;
    }
    for (i = 0; i < 8; ++i) {
        int next = (i + 1) & 7;
        float quad[12];
        quad[0] = vertices[i][0]; quad[1] = vertices[i][1]; quad[2] = vertices[i][2];
        quad[3] = vertices[next][0]; quad[4] = vertices[next][1]; quad[5] = vertices[next][2];
        quad[6] = vertices[next + 8][0]; quad[7] = vertices[next + 8][1]; quad[8] = vertices[next + 8][2];
        quad[9] = vertices[i + 8][0]; quad[10] = vertices[i + 8][1]; quad[11] = vertices[i + 8][2];
        s_mesh_segment_quad_emit_hook(mesh, quad, red, green, blue, alpha);
    }
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
    float position[3];
    float direction[3];
    float duration;
    if (entity == NULL || s_kind_effect_dispatch_hooks.entity_position_get == NULL ||
        s_kind_effect_dispatch_hooks.facing_vector_get == NULL ||
        s_kind_effect_dispatch_hooks.terrain_height_get == NULL ||
        s_kind_effect_dispatch_hooks.entity_half_height_get == NULL ||
        s_kind_effect_dispatch_hooks.resource_duration_get == NULL) return;
    switch (kind) {
    case 1: resource_name = "Lazer"; break;
    case 2: resource_name = "Pusher"; break;
    case 3: resource_name = "Freezer"; break;
    case 4: resource_name = "Lazer"; break;
    default: return;
    }
    if (!s_kind_effect_dispatch_hooks.entity_position_get(
            s_kind_effect_dispatch_hooks.context, entity, position)) return;
    s_kind_effect_dispatch_hooks.facing_vector_get(
        s_kind_effect_dispatch_hooks.context, entity, direction);
    position[2] = s_kind_effect_dispatch_hooks.terrain_height_get(
        s_kind_effect_dispatch_hooks.context, position[0], position[1]) -
        s_kind_effect_dispatch_hooks.entity_half_height_get(
            s_kind_effect_dispatch_hooks.context, entity);
    duration = s_kind_effect_dispatch_hooks.resource_duration_get(
        s_kind_effect_dispatch_hooks.context, resource_name);
    if (s_kind_effect_dispatch_hooks.effect_spawn != NULL)
        s_kind_effect_dispatch_hooks.effect_spawn(
            s_kind_effect_dispatch_hooks.context, entity, kind, position,
            direction, duration);
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

// FUNCTION: LEGORR 0x0046faf0
int entity_spawn_record_slot_init(void* owner, const float position[3],
                                  const float direction[3],
                                  uint32_t auxiliary, uint32_t mode)
{
    uint32_t type, flags;
    int slot;
    unsigned int i;
    ActiveEffectRecord* record;
    if (position == NULL || direction == NULL ||
        !active_effect_type_resolve(mode, &type, &flags)) return 0;
    if (s_active_effect_build_hooks.surface_type_exists != NULL &&
        !s_active_effect_build_hooks.surface_type_exists(
            s_active_effect_build_hooks.context, type, flags)) return 0;
    if (mode == 1 && s_active_effect_build_hooks.pre_spawn_prepare != NULL)
        s_active_effect_build_hooks.pre_spawn_prepare(
            s_active_effect_build_hooks.context, owner, auxiliary);
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
    active_effect_record_slot_set(slot, 1);
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
        record->direction[i] = end[i] - start[i];
    }
    active_effect_direction_normalize(record->direction);
    distance = (float)sqrt((end[0] - start[0]) * (end[0] - start[0]) +
                           (end[1] - start[1]) * (end[1] - start[1]) +
                           (end[2] - start[2]) * (end[2] - start[2]));
    record->sample_count = 50;
    record->duration = s_active_effect_build_hooks.flight_curve_build != NULL
        ? s_active_effect_build_hooks.flight_curve_build(
            s_active_effect_build_hooks.context, record, end) : distance;
    if (s_active_effect_build_hooks.entity_spawn != NULL)
        record->spawned_entity = s_active_effect_build_hooks.entity_spawn(
            s_active_effect_build_hooks.context, type, flags, start);
    active_effect_record_slot_set(slot, 1);
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
                                 int command, uint32_t type, int mode)
{
    uint32_t flags = 0;
    float direction[2];
    if (entity == NULL || direction_xy == NULL) return;
    direction[0] = direction_xy[0];
    direction[1] = direction_xy[1];
    if (s_effect_apply_hooks.entity_motion_check != NULL &&
        s_effect_apply_hooks.entity_motion_check(s_effect_apply_hooks.context,
                                                 entity) != 0) {
        if (command != 0 && s_effect_apply_hooks.command_effect_apply != NULL)
            s_effect_apply_hooks.command_effect_apply(s_effect_apply_hooks.context,
                                                     -1, command);
    }
    if (s_effect_apply_hooks.entity_flags_get != NULL)
        flags = s_effect_apply_hooks.entity_flags_get(
            s_effect_apply_hooks.context, entity);
    if (mode == 1) {
        if ((flags & 0x02000000u) != 0 &&
            s_effect_apply_hooks.impulse_apply != NULL) {
            float amount = s_effect_apply_hooks.entity_value_get != NULL
                ? s_effect_apply_hooks.entity_value_get(
                      s_effect_apply_hooks.context, entity, 0x6a2d0u) : 0.0f;
            s_effect_apply_hooks.impulse_apply(s_effect_apply_hooks.context,
                                               entity, amount, 1);
        }
        if (command != 0 && s_effect_apply_hooks.command_effect_apply != NULL)
            s_effect_apply_hooks.command_effect_apply(s_effect_apply_hooks.context,
                                                     0, command);
    }
    else if (mode == 2) {
        if ((flags & 0x04000000u) != 0) {
            float amount = s_effect_apply_hooks.entity_value_get != NULL
                ? s_effect_apply_hooks.entity_value_get(
                      s_effect_apply_hooks.context, entity, 0x6a2b0u) : 0.0f;
            float duration = s_effect_apply_hooks.entity_value_get != NULL
                ? s_effect_apply_hooks.entity_value_get(
                      s_effect_apply_hooks.context, entity, 0x6a290u) : 0.0f;
            direction[0] *= 1.5f;
            direction[1] *= 1.5f;
            if (s_effect_apply_hooks.impulse_apply != NULL)
                s_effect_apply_hooks.impulse_apply(s_effect_apply_hooks.context,
                                                   entity, amount, 1);
            if (s_effect_apply_hooks.effect_apply != NULL)
                s_effect_apply_hooks.effect_apply(s_effect_apply_hooks.context,
                                                  entity, mode, direction,
                                                  duration, amount);
        }
        if (command != 0 && s_effect_apply_hooks.command_effect_apply != NULL)
            s_effect_apply_hooks.command_effect_apply(s_effect_apply_hooks.context,
                                                     1, command);
    }
    else if (mode == 3) {
        if ((flags & 0x01000000u) != 0) {
            float duration = s_effect_apply_hooks.entity_value_get != NULL
                ? s_effect_apply_hooks.entity_value_get(
                      s_effect_apply_hooks.context, entity, 0x6a310u) : 0.0f;
            float amount = s_effect_apply_hooks.entity_value_get != NULL
                ? s_effect_apply_hooks.entity_value_get(
                      s_effect_apply_hooks.context, entity, 0x6a2f0u) : 0.0f;
            if (duration <= 0.0f) duration = 10.0f;
            if (s_effect_apply_hooks.impulse_apply != NULL)
                s_effect_apply_hooks.impulse_apply(s_effect_apply_hooks.context,
                                                   entity, amount, 0);
            (void)entity_one_shot_attached_effect_start(
                (EntitySlot*)entity, duration);
            if (s_effect_apply_hooks.effect_apply != NULL)
                s_effect_apply_hooks.effect_apply(s_effect_apply_hooks.context,
                                                  entity, mode, direction,
                                                  duration, amount);
        }
        if (command != 0 && s_effect_apply_hooks.command_effect_apply != NULL)
            s_effect_apply_hooks.command_effect_apply(s_effect_apply_hooks.context,
                                                     2, command);
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
    int paused = s_screen_effect_tick_hooks.simulation_paused != NULL
        ? s_screen_effect_tick_hooks.simulation_paused(
              s_screen_effect_tick_hooks.context) : 0;
    int pool;
    int slot;
    if (paused == 0) {
        if (s_screen_effect_pulse_timer >= 12.5f) {
            if (s_screen_effect_pulse_timer >= 25.0f) {
                s_screen_effect_pulse_timer -= 25.0f;
            } else if (s_screen_effect_tick_hooks.pulse_tick != NULL) {
                for (slot = 0; slot < capacities[2]; ++slot) {
                    void* entity = s_screen_effect_tick_records[2][slot].entity;
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

void* transient_effect_emitter_create(int scene_context,
                                      uint32_t position_x,
                                      uint32_t position_y, int* direction,
                                      float red, float green, float blue,
                                      float rate, int sound_id, int mode,
                                      uint32_t seed, int auxiliary)
{
    void* instance;
    if (s_transient_effect_hooks.emitter_create == NULL) return NULL;
    instance = s_transient_effect_hooks.emitter_create(
        s_transient_effect_hooks.context, scene_context, position_x,
        position_y, direction, red, green, blue, rate, sound_id, mode,
        seed, auxiliary);
    if (instance == NULL) return NULL;
    {
        TransientEffectInstanceLinks* links =
            transient_effect_links_add(instance);
        TransientEffectInstanceLinks* tail =
            transient_effect_links_find(s_transient_effect_tail);
        if (links == NULL) {
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
    if (s_transient_effect_hooks.instance_release != NULL) {
        s_transient_effect_hooks.instance_release(
            s_transient_effect_hooks.context, instance, resource);
    }
    else {
        if ((bytes[0x294] & 2u) != 0u)
            ddiscene_object_destroy(resource);
        engine_heap_free(instance);
    }
    if (links != NULL) free(links);
}

// FUNCTION: LEGORR 0x00465970
void transient_effect_registry_release_all(void)
{
    while (s_transient_effect_head != NULL) {
        void* instance = s_transient_effect_head;
        void* resource = NULL;
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
    if (record != NULL && s_timed_effect_runtime_hooks.slot_render != NULL)
        s_timed_effect_runtime_hooks.slot_render(
            s_timed_effect_runtime_hooks.context, record,
            render_x, render_y);
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
        : global_state_low_word_get();
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
