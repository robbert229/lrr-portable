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
#include "effects_shared.hpp"
#include "effects_for_weapons.hpp"
#include "effects_for_interface.hpp"
#include "effects_for_legoobject.hpp"

uint32_t s_active_effect_occupied[10];
uint32_t s_active_segment_visual_occupied[10];
ActiveEffectRecord s_active_effect_records[10];
/* Each record's flight path. The original embeds the BezierCurve at +4 of
 * its 0x2b0-byte record (00503920); the port's record keeps its own layout,
 * so the paths sit beside it, one per slot. */
static BezierCurve s_active_effect_paths[10];
ActiveEffectBuildHooks s_active_effect_build_hooks;
EntityImpulseApplyHook s_entity_impulse_hook;
EntityDirectionApplyHook s_entity_direction_hook;
TimedSegmentVisualHooks s_timed_segment_visual_hooks;
void* s_timed_segment_visuals[10][2];
void* s_timed_segment_visual_owners[10];
SimtickSfxTimedVisualRecord s_timed_segment_tick_records[10];
static SimtickSfxProjectileRecord s_active_effect_tick_records[10];
static int s_sfx_tick_hooks_installed;
KindEffectDispatchHooks s_kind_effect_dispatch_hooks;
EffectApplyHooks s_effect_apply_hooks;
static ScreenSpaceEffectDispatchHook s_screen_space_effect_dispatch_hook;
struct ScreenEffectTickRecord s_screen_effect_tick_records[3][50];
ScreenEffectTickHooks s_screen_effect_tick_hooks;
static void* s_screen_space_effect_dispatch_context;
TransientEffectHooks s_transient_effect_hooks;
TransientEffectQuadBasisGetHook s_transient_effect_quad_basis_hook;
void* s_transient_effect_quad_basis_context;
// GLOBAL: LEGORR 0x00552100
TextureSurface* s_transient_effect_textures[12];
// GLOBAL: LEGORR 0x00552130
uint32_t s_transient_effect_texture_count;
void* s_transient_effect_head;
void* s_transient_effect_tail;
TransientEffectInstanceLinks* s_transient_effect_links;
int s_transient_effect_enabled = 1;
TimedEffectRuntimeHooks s_timed_effect_runtime_hooks;
TimedEffectSlotRenderHooks s_timed_effect_slot_render_hooks;
SceneTransformState s_scene_transform_state;
LightEffectParameterState s_light_effect_parameters;
EntityNodeScaleApplyHook s_entity_node_scale_apply_hook;
void* s_entity_node_scale_apply_context;
EventTimerActionHook s_event_timer_action_hook;
void* s_event_timer_action_context;
static EventTimerInteractionFlagsHook s_event_timer_interaction_flags_hook;
static void* s_event_timer_interaction_flags_context;

TransientEffectInstanceLinks* transient_effect_links_find(
    const void* instance)
{
    TransientEffectInstanceLinks* links;
    for (links = s_transient_effect_links; links != NULL;
         links = links->all_next) {
        if (links->instance == instance) return links;
    }
    return NULL;
}

TransientEffectInstanceLinks* transient_effect_links_add(void* instance)
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

void transient_effect_link_pointer_set(void* instance, size_t offset,
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

LightEffectConfigLoadHook s_light_effect_config_load_hook;
void* s_light_effect_config_load_context;

int light_effect_config_range_pair_read(BlockFile* config,
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

uint32_t light_effect_float_word(float value)
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

void scene_transform_current_set(float x, float y, float z)
{
    s_scene_transform_state.current[0] = x;
    s_scene_transform_state.current[1] = y;
    s_scene_transform_state.current[2] = z;
}

const SceneTransformState* scene_transform_state_peek(void)
{
    return &s_scene_transform_state;
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

BezierCurve* active_effect_path(const ActiveEffectRecord* record)
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

void effect_apply_hooks_set(const EffectApplyHooks* hooks)
{
    if (hooks == NULL) memset(&s_effect_apply_hooks, 0, sizeof(s_effect_apply_hooks));
    else s_effect_apply_hooks = *hooks;
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

void transient_effect_quad_basis_hook_set(
    TransientEffectQuadBasisGetHook hook, void* context)
{
    s_transient_effect_quad_basis_hook = hook;
    s_transient_effect_quad_basis_context = context;
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

