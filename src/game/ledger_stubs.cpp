// SPDX-License-Identifier: LGPL-3.0-or-later
// Generated corpus entries whose reconstructed APIs remain grouped here.
// Entries marked STUB are inert placeholders; FUNCTION entries are callable.

#include <stddef.h>
#include <math.h>

#include "terrain_grid.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "cmd.hpp"
#include "ai.hpp"
#include "spawn.hpp"
#include "effects.hpp"
#include "weapon.hpp"
#include "savegame.hpp"
#include "mission.hpp"
#include "objective.hpp"
#include "ui_list.hpp"
#include "seh_runtime.hpp"
#include "port_loop_ports.hpp"
#include "screen_pick.hpp"
#include "entity_quad_collision.hpp"
#include "type_motion_scale.hpp"
#include "camera_drag.hpp"
#include "entity_motion_probe.hpp"
#include "pointer_cursor.hpp"
#include "priority_config_ports.hpp"
#include "activity_timeout.hpp"
#include "type_config_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "entity_pose_delta.hpp"
#include "combat_ray_ports.hpp"
#include "entity_motion_ports.hpp"
#include "terrain_feedback.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "util.hpp"
#include "../crt/crt.hpp"
#include "../gods98/message.hpp"
#include "../gods98/surface.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/scene_resources.hpp"
#include "../gods98/scene_pool.hpp"
#include "../gods98/heap.hpp"
#include "../gods98/record.hpp"
#include "../gods98/camera.hpp"
#include "../gods98/sound.hpp"
#include "../gods98/audio.hpp"
#include "../gods98/media.hpp"
#include "../gods98/movie.hpp"
#include "../gods98/model.hpp"
#include "../gods98/block.hpp"
#include "../gods98/flic.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/input.hpp"
#include "frontend.hpp"
#include "select.hpp"
#include "../gods98/view.hpp"
#include "../gods98/engine.hpp"
#include "../gods98/frameloop.hpp"
#include "../gods98/block.hpp"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static char s_directx_error_message[1024];
extern float random_float_range(float minimum, float maximum);
// GLOBAL: LEGORR 0x005343c8
static InputActionToggleState s_input_action_toggle_state;
static uint32_t* s_ui_transition_feature_word;
static unported_ledger_stubs::CellGridPointHitTestState*
    s_cell_grid_point_hit_test_state;
static unported_ledger_stubs::GlobalSubsystemResetState*
    s_global_subsystem_reset_state;
static unported_ledger_stubs::DependencyGridState* s_dependency_grid_state;
static unported_ledger_stubs::UiEntryHitTestState* s_ui_entry_hit_test_state;
static const unported_ledger_stubs::ObjectTaskTargetPool*
    s_object_task_target_pool;
static unported_ledger_stubs::ObjectTaskEligibilityOps*
    s_object_task_eligibility_ops;

void ui_transition_control_feature_word_bind(uint32_t* feature_word)
{
    s_ui_transition_feature_word = feature_word;
}

void unported_ledger_stubs::cell_grid_point_hit_test_state_bind(
    unported_ledger_stubs::CellGridPointHitTestState* state)
{
    s_cell_grid_point_hit_test_state = state;
}

void unported_ledger_stubs::global_subsystem_reset_state_bind(
    unported_ledger_stubs::GlobalSubsystemResetState* state)
{
    s_global_subsystem_reset_state = state;
}

void unported_ledger_stubs::dependency_grid_state_bind(
    unported_ledger_stubs::DependencyGridState* state)
{
    s_dependency_grid_state = state;
}

void unported_ledger_stubs::ui_entry_hit_test_state_bind(
    unported_ledger_stubs::UiEntryHitTestState* state)
{
    s_ui_entry_hit_test_state = state;
}

void unported_ledger_stubs::object_task_target_pool_bind(
    const unported_ledger_stubs::ObjectTaskTargetPool* pool)
{
    s_object_task_target_pool = pool;
}

void unported_ledger_stubs::object_task_eligibility_ops_bind(
    unported_ledger_stubs::ObjectTaskEligibilityOps* ops)
{
    s_object_task_eligibility_ops = ops;
}

const unported_ledger_stubs::ObjectTaskTargetFilterRecord*
// FUNCTION: LEGORR 0x00404f10
unported_ledger_stubs::object_task_target_filter_find(
    uint32_t type, uint32_t selector, uint32_t companion)
{
    const unported_ledger_stubs::ObjectTaskTargetPool* pool =
        s_object_task_target_pool;
    uint32_t index;
    if (pool == NULL || pool->records == NULL) return NULL;
    for (index = 0u; index < pool->count; ++index) {
        const unported_ledger_stubs::ObjectTaskTargetFilterRecord* record =
            &pool->records[index];
        if (record->type != 5u && record->type != 7u &&
            record->type != 9u && record->type != 0xfu) continue;
        if (record->type != 5u && record->entity_handle != NULL &&
            !entity_shared_eligibility_check(
                (const EntitySlot*)record->entity_handle, 0)) continue;
        if (record->target_name != 0u &&
            (type != record->target_name ||
             (record->target_selector != -1 &&
              selector != (uint32_t)record->target_selector) ||
             companion != record->target_companion)) continue;
        return record;
    }
    return NULL;
}

namespace unported_ledger_stubs {

int ledger_stub_0044a3d0_PlaceCellNearestResolve(const EntitySlot* target,
                                                 int cell_xy[2]);

void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const EntitySlot* entity);
void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int cell_xy[2], uint32_t filter_a, uint32_t filter_b,
    uint32_t filter_c);
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b);
void* ledger_stub_00438df0_EntityLookupAtWorldPosition(
    const float center[2]);

// GLOBAL: LEGORR 0x004e84ec
static int s_type11_cell_coordinates[10][2];
// GLOBAL: LEGORR 0x004e8540
static unsigned int s_type11_cell_coordinate_count;

// FUNCTION: LEGORR 0x004372a0
void spawn_coordinate_pool_clear_impl(void)
{
    unsigned int i;
    GameContext* context = entity_pool_callback_context_get();
    /* LAB_0043b350 is not recovered; preserve the guard's enter/leave path. */
    entity_iteration_callback_guard(context, NULL, NULL);
    grid_record_count_tables_clear();
    grid_object_records_release();
    level_predug_coordinate_pool_clear();
    camera_predug_spawn_state_clear();
    camera_spawn_coordinate_hook_set(NULL, NULL);
    for (i = 0; i < 10u; ++i) {
        s_type11_cell_coordinates[i][0] = 0;
        s_type11_cell_coordinates[i][1] = 0;
    }
    s_type11_cell_coordinate_count = 0u;
    queued_grid_animation_accumulator_clear();
    ai_idle_table_reset();
    entity_global_animation_state_set(0u);
}

static CellStepPathResolveHook s_cell_step_path_hook;
static CellStepRandomRangeHook s_cell_step_random_hook;
static void* s_cell_step_hook_user;
static EmergenceSpawnHooks s_emergence_spawn_hooks;
static MiscObjectResourceHooks s_misc_resource_hooks;
static void* s_misc_resource_handles[22];
static SehRuntimeHooks s_seh_runtime_hooks;
// GLOBAL: LEGORR 0x00575ce0
static GameFileHandle* s_startup_streams[4];
static uint32_t s_startup_window_mode;
static BufferedCharReadHook s_buffered_char_read_hook;
static void* s_buffered_char_read_context;
static RegistryCreateKeyHook s_registry_create_key_hook;
static RegistryQueryValueHook s_registry_query_value_hook;
static RegistryCloseKeyHook s_registry_close_key_hook;
static void* s_registry_hook_context;
static DependencyConfigEntryHook s_dependency_config_hook;
static void* s_dependency_config_context;
static CellVisualChannelReleaseHook s_cell_visual_release_hook;
static void* s_cell_visual_release_context;
static InputDeviceReleaseHooks s_input_device_release_hooks;
static uint32_t s_input_device_release_flags;
static uint32_t s_input_device_system_flags;
static int s_input_device_active_track = -1;

void input_device_release_state_set(uint32_t input_flags,
                                    uint32_t system_flags,
                                    int active_track)
{
    s_input_device_release_flags = input_flags;
    s_input_device_system_flags = system_flags;
    s_input_device_active_track = active_track;
}

void input_device_release_hooks_set(const InputDeviceReleaseHooks* hooks)
{
    if (hooks == NULL) memset(&s_input_device_release_hooks, 0,
                              sizeof(s_input_device_release_hooks));
    else s_input_device_release_hooks = *hooks;
}

// FUNCTION: LEGORR 0x004588e0
void input_device_release(void)
{
    InputDeviceReleaseHooks* hooks = &s_input_device_release_hooks;
    uint32_t flags = s_input_device_release_flags;
    if ((flags & 1u) != 0u) {
        if (hooks->primary_release != NULL) {
            hooks->primary_release(hooks->context);
        }
        s_input_device_release_flags &= ~1u;
        if ((s_input_device_system_flags & 0x800u) != 0u) {
            if (hooks->secondary_reenable != NULL) {
                hooks->secondary_reenable(hooks->context, 1);
            }
            s_input_device_system_flags &= ~0x800u;
        }
    }
    else if ((flags & 2u) != 0u) {
        s_input_device_release_flags &= ~2u;
    }
    else if ((flags & 4u) != 0u) {
        s_input_device_release_flags &= ~0x104u;
    }

    if (hooks->advisor_clear != NULL) hooks->advisor_clear(hooks->context);
    if (s_input_device_active_track != -1) {
        if (hooks->stream_stop != NULL) {
            hooks->stream_stop(hooks->context, 0);
        }
        s_input_device_active_track = -1;
    }
    if (hooks->transition_control != NULL) {
        hooks->transition_control(hooks->context, 0, 0);
    }
    if (hooks->sound_cleanup != NULL) {
        hooks->sound_cleanup(hooks->context, 0);
    }
    if (hooks->system_state_get != NULL &&
        hooks->system_state_get(hooks->context, 0) == 0) {
        s_input_device_system_flags &= ~0x1000u;
    }
}

uint32_t input_device_release_flags_get(void)
{
    return s_input_device_release_flags;
}

uint32_t input_device_release_system_flags_get(void)
{
    return s_input_device_system_flags;
}

int input_device_release_active_track_get(void)
{
    return s_input_device_active_track;
}

void startup_stream_handle_set(int slot, void* handle)
{
    if (slot < 0 || slot >= 4) return;
    s_startup_streams[slot] = (GameFileHandle*)handle;
}

void* startup_stream_handle_get(int slot)
{
    if (slot < 0 || slot >= 4) return NULL;
    return s_startup_streams[slot];
}

void startup_stream_window_mode_set(uint32_t value)
{
    s_startup_window_mode = value;
}

uint32_t startup_stream_window_mode_get(void)
{
    return s_startup_window_mode;
}

void startup_stream_state_reset(void)
{
    s_startup_streams[0] = NULL;
    s_startup_streams[1] = NULL;
    s_startup_streams[2] = NULL;
    s_startup_streams[3] = NULL;
    s_startup_window_mode = 0;
}

void startup_streams_close(void)
{
    int index;
    for (index = 0; index < 3; index++) {
        if (s_startup_streams[index] != NULL) {
            game_file_close(s_startup_streams[index]);
            s_startup_streams[index] = NULL;
        }
    }
}

void buffered_char_stream_read_hook_set(BufferedCharReadHook read_char,
                                        void* context)
{
    s_buffered_char_read_hook = read_char;
    s_buffered_char_read_context = context;
}

// FUNCTION: LEGORR 0x0048b4a0
int buffered_char_stream_read_to_string(void* stream, char* output)
{
    char* cursor = output;
    int value;
    if (output == NULL || s_buffered_char_read_hook == NULL) return 0;
    value = s_buffered_char_read_hook(s_buffered_char_read_context, stream);
    while (value != 0) {
        *cursor++ = (char)value;
        value = s_buffered_char_read_hook(s_buffered_char_read_context,
                                         stream);
    }
    *cursor = '\0';
    return 1;
}

void registry_path_hooks_set(RegistryCreateKeyHook create_key,
                             RegistryQueryValueHook query_value,
                             RegistryCloseKeyHook close_key, void* context)
{
    s_registry_create_key_hook = create_key;
    s_registry_query_value_hook = query_value;
    s_registry_close_key_hook = close_key;
    s_registry_hook_context = context;
}

int registry_path_create_keys(void* root, const char* subkey_path,
                              const char* value_name, void* output,
                              uint32_t* size, uint32_t* value_type)
{
    void* opened[32];
    uint32_t opened_count = 0u;
    const char* cursor = subkey_path;
    void* key = root;
    int result = 0;
    if (root == NULL || subkey_path == NULL
        || s_registry_create_key_hook == NULL
        || s_registry_query_value_hook == NULL) return 0;
    while (*cursor != '\0') {
        char component[100];
        size_t length = 0u;
        void* child;
        while (*cursor == '\\') ++cursor;
        while (cursor[length] != '\0' && cursor[length] != '\\') {
            if (length + 1u >= sizeof(component)) goto cleanup;
            component[length] = cursor[length];
            ++length;
        }
        if (length == 0u) break;
        component[length] = '\0';
        if (opened_count >= 32u) goto cleanup;
        child = s_registry_create_key_hook(key, component,
                                            s_registry_hook_context);
        if (child == NULL) goto cleanup;
        opened[opened_count++] = child;
        key = child;
        cursor += length;
    }
    result = s_registry_query_value_hook(key, value_name, output, size,
                                          value_type,
                                          s_registry_hook_context);
cleanup:
    while (opened_count != 0u) {
        void* child = opened[--opened_count];
        if (s_registry_close_key_hook != NULL)
            s_registry_close_key_hook(child, s_registry_hook_context);
    }
    return result;
}

void dependency_config_entry_hook_set(DependencyConfigEntryHook hook,
                                      void* context)
{
    s_dependency_config_hook = hook;
    s_dependency_config_context = context;
}

int dependency_grid_config_load(void* config, int level)
{
    BlockFile* file = (BlockFile*)config;
    BlockRec* group;
    BlockRec* entry;
    int processed = 0;
    if (file == NULL) return 0;
    group = block_subgroup(file, "Dependencies");
    if (group == NULL) return 0;
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        if (name == NULL || value == NULL) continue;
        if (s_dependency_config_hook != NULL
            && !s_dependency_config_hook(s_dependency_config_context, level,
                                         name, value)) continue;
        ++processed;
    }
    return processed;
}

void cell_visual_channel_release_hook_set(CellVisualChannelReleaseHook hook,
                                           void* context)
{
    s_cell_visual_release_hook = hook;
    s_cell_visual_release_context = context;
}

CellVisualChannelTable* cell_visual_channel_table_create(void)
{
    CellVisualChannelTable* table = (CellVisualChannelTable*)
        engine_heap_alloc((long)sizeof(CellVisualChannelTable));
    if (table != NULL) memset(table, 0, sizeof(*table));
    return table;
}

int cell_visual_channel_table_resource_set(CellVisualChannelTable* table,
                                            uint32_t channel,
                                            uint32_t index, void* resource)
{
    if (table == NULL || channel >= 2u || index >= 100u) return 0;
    table->resources[channel][index] = resource;
    if (resource != NULL && table->resource_count <= index)
        table->resource_count = index + 1u;
    return 1;
}

// FUNCTION: LEGORR 0x0040b810
void cell_visual_channel_table_resources_clear(CellVisualChannelTable* table)
{
    uint32_t channel;
    uint32_t index;
    if (table == NULL) return;
    table->resource_count = 0;
    for (channel = 0; channel < 2u; channel++) {
        for (index = 0; index < 100u; index++) {
            void* resource = table->resources[channel][index];
            if (resource != NULL && s_cell_visual_release_hook != NULL)
                s_cell_visual_release_hook(s_cell_visual_release_context,
                                           resource);
            table->resources[channel][index] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0040b7d0
void cell_visual_channel_table_dispose(CellVisualChannelTable* table)
{
    int index;
    if (table == NULL) return;
    cell_visual_channel_table_resources_clear(table);
    if (s_cell_visual_release_hook != NULL) {
        for (index = 0; index < 2; index++) {
            if (table->roots[index] != NULL)
                s_cell_visual_release_hook(s_cell_visual_release_context,
                                           table->roots[index]);
        }
    }
    engine_heap_free(table);
}

void seh_runtime_hooks_set(const SehRuntimeHooks* hooks)
{
    if (hooks == NULL) memset(&s_seh_runtime_hooks, 0,
                              sizeof(s_seh_runtime_hooks));
    else s_seh_runtime_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0048ec00
void seh_frame_unwind(void* target_frame,
                      SehExceptionRecord* exception_record)
{
    void* current_frame = NULL;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    if (head_slot != NULL) current_frame = *head_slot;
    if (s_seh_runtime_hooks.rtl_unwind != NULL)
        s_seh_runtime_hooks.rtl_unwind(target_frame,
            (void*)(uintptr_t)0x0048ec2cu, exception_record, NULL,
            s_seh_runtime_hooks.user);
    if (exception_record != NULL) exception_record->flags &= ~2u;
    if (head_slot != NULL && current_frame != NULL) {
        *(void**)current_frame = *head_slot;
        *head_slot = current_frame;
    }
}

// FUNCTION: LEGORR 0x0048eca0
uint32_t call_setting_frame_12(uintptr_t argument_1, uintptr_t argument_2,
    uintptr_t callback, int frame_index, uintptr_t argument_5)
{
    SehCrtExceptionFrame frame;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    uint32_t result = 0u;
    frame.previous_frame = head_slot != NULL ? *head_slot : NULL;
    frame.argument_8 = (uintptr_t)(frame_index + 1);
    frame.argument_c = argument_1;
    frame.argument_10 = argument_2;
    frame.argument_14 = argument_5;
    frame.continuation = callback;
    frame.unwind_in_progress = 0u;
    if (head_slot != NULL) *head_slot = &frame;
    if (s_seh_runtime_hooks.call_setting_frame != NULL)
        result = s_seh_runtime_hooks.call_setting_frame(
            (void*)crt_exception_frame_filter_invoke, argument_1,
            argument_5, &frame, s_seh_runtime_hooks.user);
    if (head_slot != NULL) *head_slot = frame.previous_frame;
    return result;
}

// FUNCTION: LEGORR 0x0048ed00
void crt_exception_frame_filter_invoke(uintptr_t exception,
    SehCrtExceptionFrame* frame, uintptr_t argument_3)
{
    if (frame != NULL &&
        s_seh_runtime_hooks.exception_dispatch != NULL) {
        (void)s_seh_runtime_hooks.exception_dispatch(exception,
            frame->argument_c, argument_3, 0u, frame->argument_8,
            frame->argument_10, (uintptr_t)frame, 0u,
            s_seh_runtime_hooks.user);
    }
}

// FUNCTION: LEGORR 0x0048ee00
uint32_t crt_exception_scope_filter_and_unwind(
    SehExceptionRecord* exception, SehCrtExceptionFrame* frame,
    uintptr_t argument_3)
{
    uint32_t result = 0u;
    if (exception == NULL || frame == NULL) return 0u;
    if ((exception->flags & 0x66u) != 0u) {
        frame->unwind_in_progress = 1u;
        return 1u;
    }
    if (s_seh_runtime_hooks.exception_dispatch != NULL) {
        result = s_seh_runtime_hooks.exception_dispatch(
            (uintptr_t)exception, frame->argument_c, argument_3, 0u,
            frame->argument_8, frame->argument_10, frame->argument_14,
            1u, s_seh_runtime_hooks.user);
    }
    if (frame->unwind_in_progress == 0u)
        seh_frame_unwind(frame, exception);
    if (s_seh_runtime_hooks.continuation_invoke != NULL)
        return s_seh_runtime_hooks.continuation_invoke(frame->continuation,
            s_seh_runtime_hooks.user);
    return result;
}

// FUNCTION: LEGORR 0x0048ed30
uint32_t crt_exception_frame_install_and_dispatch(
    const void* type_info, uintptr_t argument_2, uintptr_t argument_3,
    uintptr_t argument_4, uintptr_t argument_5, uintptr_t argument_6,
    uintptr_t argument_7)
{
    SehCrtExceptionFrame frame;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    frame.previous_frame = head_slot != NULL ? *head_slot : NULL;
    frame.argument_8 = argument_2;
    frame.argument_c = (uintptr_t)type_info;
    frame.argument_10 = argument_3;
    frame.argument_14 = argument_4;
    frame.continuation = 0x0048edcau;
    frame.unwind_in_progress = 0u;
    if (head_slot != NULL) *head_slot = &frame;
    if (s_seh_runtime_hooks.exception_install_dispatch != NULL)
        s_seh_runtime_hooks.exception_install_dispatch(
            type_info != NULL ? *(void* const*)type_info : NULL,
            &frame, argument_2, argument_3, argument_4, argument_5,
            argument_6, argument_7, s_seh_runtime_hooks.user);
    if (head_slot != NULL) {
        if (frame.unwind_in_progress == 0u)
            *head_slot = frame.previous_frame;
        else if (frame.previous_frame != NULL) {
            *(void**)frame.previous_frame = *head_slot;
            *head_slot = frame.previous_frame;
        } else *head_slot = NULL;
    }
    return 0u;
}

void misc_object_resource_hooks_set(const MiscObjectResourceHooks* hooks)
{
    if (hooks == NULL) memset(&s_misc_resource_hooks, 0,
                              sizeof(s_misc_resource_hooks));
    else s_misc_resource_hooks = *hooks;
}

void* misc_object_resource_peek(unsigned int slot)
{
    return slot < 22u ? s_misc_resource_handles[slot] : NULL;
}

// FUNCTION: LEGORR 0x00433770
int scene_lights_build_from_config(const SceneLightBuildHooks* hooks)
{
    static const struct {
        const char* name;
        int scene_slot;
        int kind;
        float rgb[3];
        float position[3];
        float direction[3];
        float intensity;
        float falloff;
        float radius;
        int optional;
    } specs[7] = {
        { "TopSpotRGB", 0, 2, { 1.0f, 1.0f, 1.0f },
          { 200.0f, 140.0f, -130.0f },
          { -1.0f, -0.8f, 0.75f }, 0.85f, 0.4f, 2000.0f, 0 },
        { "TrackSpotRGB", 1, 2, { 1.0f, 1.0f, 1.0f },
          { 0.0f, 0.0f, -150.0f },
          { 0.0f, 0.0f, 1.0f }, 1.0f, 1.0f, 2000.0f, 0 },
        { "AmbientRGB", 2, 0, { 1.0f, 1.0f, 1.0f },
          { 0.0f, 0.0f, 0.0f },
          { 0.0f, 0.0f, 0.0f }, 1.0f, 1.0f, 0.0f, 0 },
        { "FPLightRGB", 3, 1, { 1.0f, 1.0f, 1.0f },
          { 0.0f, 0.0f, 0.0f },
          { 0.0f, 0.0f, 1.0f }, 1.0f, 1.0f, 0.0f, 0 },
        { "FPRotLightRGB", 3, 3, { 1.0f, 1.0f, 1.0f },
          { 0.0f, 0.0f, 0.0f },
          { 0.0f, 1.0f, 0.0f }, 1.0f, 1.0f, 0.0f, 1 },
        { NULL, 3, 3, { 1.0f, 0.0f, 0.0f },
          { 0.0f, 0.0f, 0.0f },
          { 0.0f, 0.0f, 1.0f }, 1.0f, 1.0f, 0.0f, 0 },
        { NULL, 4, 2, { 0.8f, 0.8f, 0.8f },
          { 0.0f, 0.0f, 0.0f },
          { 0.0f, 0.0f, 1.0f }, 0.85f, 0.0f, 2000.0f, 0 }
    };
    unsigned int i;
    if (hooks == NULL || hooks->create == NULL) return 0;
    for (i = 0u; i < 7u; ++i) {
        SceneLightDefinition definition;
        uint32_t handle;
        definition.config_name = specs[i].name;
        definition.scene_slot = specs[i].scene_slot;
        definition.light_kind = specs[i].kind;
        memcpy(definition.rgb, specs[i].rgb, sizeof(definition.rgb));
        memcpy(definition.position, specs[i].position,
               sizeof(definition.position));
        memcpy(definition.direction, specs[i].direction,
               sizeof(definition.direction));
        definition.intensity = specs[i].intensity;
        definition.falloff = specs[i].falloff;
        definition.radius = specs[i].radius;
        if (definition.config_name != NULL) {
            if (hooks->rgb_read == NULL ||
                !hooks->rgb_read(definition.config_name, definition.rgb,
                                 hooks->user)) {
                if (specs[i].optional) continue;
                return 0;
            }
        }
        handle = hooks->create(&definition, hooks->user);
        if (handle == 0u) {
            if (specs[i].optional) continue;
            return 0;
        }
        if (hooks->enable != NULL) hooks->enable(handle, hooks->user);
    }
    return 1;
}

typedef struct MiscResourceSpec {
    const char* name;
    int kind;
    int fallback_kind;
    int load_flag;
} MiscResourceSpec;

// FUNCTION: LEGORR 0x00422850
void misc_object_resources_load(void* config)
{
    static const MiscResourceSpec specs[22] = {
        { "Boulder", 1, 2, 1 },
        { "BoulderExplodeIce", 3, -1, 1 },
        { "BoulderExplode", 3, -1, 1 },
        { "SmashPath", 3, -1, 1 },
        { "Explosion", 5, -1, 1 },
        { "Crystal", 1, 2, 1 },
        { "Dynamite", 4, -1, 1 },
        { "OohScary", 4, -1, 1 },
        { "Barrier", 4, -1, 1 },
        { "Ore", 1, 2, 1 },
        { "ProcessedOre", 1, 2, 1 },
        { "ElectricFence", 1, -1, 1 },
        { "ElectricFenceStud", 3, -1, 1 },
        { "ShortElectricFenceBeam", 6, -1, 0 },
        { "LongElectricFenceBeam", 7, -1, 1 },
        { "SpiderWeb", 4, -1, 1 },
        { "RechargeSparkle", 3, -1, 1 },
        { "MiniTeleportUp", 3, -1, 0 },
        { "IceCube", 4, -1, 1 },
        { "Pusher", 3, -1, 1 },
        { "Freezer", 3, -1, 1 },
        { "LaserShot", 3, -1, 1 }
    };
    unsigned int i;
    memset(s_misc_resource_handles, 0, sizeof(s_misc_resource_handles));
    if (s_misc_resource_hooks.load != NULL) {
        for (i = 0u; i < 22u; ++i) {
            void* handle = s_misc_resource_hooks.load(config, "MiscObjects",
                specs[i].name, specs[i].kind, specs[i].load_flag,
                s_misc_resource_hooks.user);
            if (handle == NULL && specs[i].fallback_kind >= 0) {
                handle = s_misc_resource_hooks.load(config, "MiscObjects",
                    specs[i].name, specs[i].fallback_kind,
                    specs[i].load_flag, s_misc_resource_hooks.user);
            }
            s_misc_resource_handles[i] = handle;
            if (handle != NULL && s_misc_resource_hooks.enable != NULL)
                s_misc_resource_hooks.enable(handle,
                                             s_misc_resource_hooks.user);
        }
    }
    if (s_misc_resource_hooks.post_load != NULL)
        s_misc_resource_hooks.post_load(config, s_misc_resource_hooks.user);
}

void emergence_spawn_hooks_set(const EmergenceSpawnHooks* hooks)
{
    if (hooks == NULL) memset(&s_emergence_spawn_hooks, 0,
                              sizeof(s_emergence_spawn_hooks));
    else s_emergence_spawn_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0043aea0
EntitySlot* emergence_random_spawn_attempt(int entity_index, int owner_index,
                                           int grid_width, int grid_height)
{
    unsigned int attempt;
    if (grid_width <= 0 || grid_height <= 0 || grid_width > 65535 ||
        grid_height > 65535 ||
        s_emergence_spawn_hooks.spawn_try == NULL) return NULL;
    for (attempt = 0; attempt < 1000u; ++attempt) {
        int16_t rx = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        int16_t ry = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        EntitySlot* entity = s_emergence_spawn_hooks.spawn_try(entity_index,
            owner_index, (int)rx % grid_width, (int)ry % grid_height,
            s_emergence_spawn_hooks.user);
        if (entity != NULL) return entity;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0043af30
EntitySlot* creature_spawn_from_emergence(void* base_entity, int mode_token,
                                          int owner_index, int cell_x,
                                          int cell_y)
{
    static const int offsets[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    static const float headings[4] = { 0.0f, 1.570796327f,
                                       3.141592654f, 4.712388980f };
    int direction;
    int neighbor_x = 0;
    int neighbor_y = 0;
    float origin[2];
    float neighbor[2];
    EntitySlot* created;
    if (mode_token != 3 ||
        (s_emergence_spawn_hooks.level == NULL &&
         s_emergence_spawn_hooks.cell_eligible == NULL) ||
        s_emergence_spawn_hooks.cell_world_position == NULL ||
        s_emergence_spawn_hooks.create == NULL) {
        return NULL;
    }
    if (s_emergence_spawn_hooks.level != NULL) {
        LevelState* level = s_emergence_spawn_hooks.level;
        if (!level_cell_walkable_flag(level, cell_x, cell_y) ||
            level_cell_flag_20_bounded(level, cell_x, cell_y) != 0u ||
            level_cell_mask_c0(level, cell_x, cell_y) != 0u ||
            level_cell_state_is_1(level, cell_x, cell_y) ||
            level_cell_flag_reserved(level, cell_x, cell_y) != 0u) {
            return NULL;
        }
    }
    else if (!s_emergence_spawn_hooks.cell_eligible(
                 cell_x, cell_y, s_emergence_spawn_hooks.user)) {
        return NULL;
    }
    for (direction = 0; direction < 4; ++direction) {
        neighbor_x = cell_x + offsets[direction][0];
        neighbor_y = cell_y + offsets[direction][1];
        if (s_emergence_spawn_hooks.level != NULL) {
            LevelState* level = s_emergence_spawn_hooks.level;
            if (level_cell_flag_8_bounded(level, neighbor_x, neighbor_y) == 0u ||
                level_cell_mask_180000(level, neighbor_x, neighbor_y) != 0 ||
                !terrain_cell_route_and_occupancy_check(NULL, level,
                    neighbor_x, neighbor_y, neighbor_x, neighbor_y, 0)) {
                continue;
            }
        }
        else if (!s_emergence_spawn_hooks.cell_eligible(
                     neighbor_x, neighbor_y,
                     s_emergence_spawn_hooks.user)) {
            continue;
        }
        break;
    }
    if (direction >= 4) return NULL;
    if (!s_emergence_spawn_hooks.cell_world_position(cell_x, cell_y, origin,
            s_emergence_spawn_hooks.user) ||
        !s_emergence_spawn_hooks.cell_world_position(neighbor_x, neighbor_y,
            neighbor, s_emergence_spawn_hooks.user)) return NULL;
    origin[0] = (origin[0] * 9.0f + neighbor[0] * 10.0f) / 19.0f;
    origin[1] = (origin[1] * 9.0f + neighbor[1] * 10.0f) / 19.0f;
    created = s_emergence_spawn_hooks.create(base_entity, 3, owner_index,
        origin[0], origin[1], headings[direction],
        s_emergence_spawn_hooks.user);
    if (created == NULL) return NULL;
    created->flags_3e0 |= 0x100u;
    entity_dispatch_event31_for_pending_flag(created);
    ai_set_activity(created, 0x0c, 0);
    ai_idle_chain_record_attach(created);
    message_dispatch(0x1f, 0u, (uint32_t)(uintptr_t)created, NULL);
    if (s_emergence_spawn_hooks.scene_action_attach != NULL) {
        s_emergence_spawn_hooks.scene_action_attach(created, 10, 0, 1, 0,
            s_emergence_spawn_hooks.user);
    }
    if (s_emergence_spawn_hooks.level != NULL)
        level_tile_mark(s_emergence_spawn_hooks.level, cell_x, cell_y, 1);
    created->pos_pair_2f4 = (float)cell_x;
    created->pos_pair_2f8 = (float)cell_y;
    {
        static const char* type_names[3] = {
            "RockMonster", "Lavamonster", "Icemonster"
        };
        unsigned int i;
        int type, row;
        for (i = 0u; i < 3u; ++i) {
            if (entity_type_row_for_name(type_names[i], &type, &row) &&
                created->type == type && created->etype == row) {
                message_dispatch((int)i + 2, 0u,
                                (uint32_t)(uintptr_t)created, NULL);
                break;
            }
        }
    }
    message_dispatch(0x27, 0u, (uint32_t)(uintptr_t)created, NULL);
    global_counter_at_00555324_increment();
    if (s_emergence_spawn_hooks.post_create != NULL)
        s_emergence_spawn_hooks.post_create(created, cell_x, cell_y,
                                            s_emergence_spawn_hooks.user);
    return created;
}

void entity_cell_step_hooks_set(CellStepPathResolveHook path_hook,
                                CellStepRandomRangeHook random_hook,
                                void* user)
{
    s_cell_step_path_hook = path_hook;
    s_cell_step_random_hook = random_hook;
    s_cell_step_hook_user = user;
}

int entity_cell_step_action_peek(const EntitySlot* entity, int* type_out,
                                 float out_xy[2], uint32_t* flags_out)
{
    if (type_out != NULL) *type_out = 0;
    if (out_xy != NULL) out_xy[0] = out_xy[1] = 0.0f;
    if (flags_out != NULL) *flags_out = 0u;
    if (entity == NULL || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    const AiWaypoint* step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    if (step->step_kind != 8 && step->step_kind != 9) return 0;
    if (type_out != NULL) *type_out = step->step_kind;
    if (out_xy != NULL) {
        out_xy[0] = step->weight_y;
        out_xy[1] = step->weight_x;
    }
    if (flags_out != NULL) *flags_out = step->pending;
    return 1;
}

// FUNCTION: LEGORR 0x0043a120
int entity_cell_step_command_issue_with_jitter(EntitySlot* entity,
    const int target_xy[2], const float input_xy[2], int action_mode,
    int mark_pending)
{
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0u;
    float route_x[AI_WAYPOINTS_MAX];
    float route_y[AI_WAYPOINTS_MAX];
    float jitter_x = 0.5f;
    float jitter_y = 0.5f;
    int mark_step = mark_pending;
    int route_result;
    uint32_t i;
    if (entity == NULL || target_xy == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u) return 0;
    start_x = level_cell_of(entity->x);
    start_y = level_cell_of(entity->y);
    if (start_x == target_xy[0] && start_y == target_xy[1]) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_xy[0];
        path_y[0] = target_xy[1];
        path_count = 1u;
    }
    else if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL) {
        if (!terrain_neighbor_path_check(entity->owner_ctx->level,
                (uint32_t)start_x, (uint32_t)start_y,
                (uint32_t)target_xy[0], (uint32_t)target_xy[1],
                &path_x, &path_y, &path_count, NULL, NULL)) return 0;
    }
    else if (s_cell_step_path_hook != NULL) {
        int fallback_xy[2] = { target_xy[0], target_xy[1] };
        if (!s_cell_step_path_hook(start_x, start_y, target_xy[0],
                                   target_xy[1], fallback_xy,
                                   s_cell_step_hook_user)) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = fallback_xy[0];
        path_y[0] = fallback_xy[1];
        path_count = 1u;
    }
    else return 0;
    if (path_count == 0u || path_count > AI_WAYPOINTS_MAX) {
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0u; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    route_result = entity_route_setup_and_marker_associate(
        entity, path_count, route_x, route_y, 0);
    free(path_x);
    free(path_y);
    if (!route_result || entity->wp_count <= 0) return 0;

    if (entity->type == 1) {
        float value = s_cell_step_random_hook != NULL
            ? s_cell_step_random_hook(0.55f, 0.75f, s_cell_step_hook_user)
            : (float)(int16_t)rng_thunk_draw15() * 3.051851e-05f * 0.20f
                + 0.55f;
        int direction;
        if (s_cell_step_random_hook != NULL) {
            direction = (int)s_cell_step_random_hook(
                0.0f, 3.999f, s_cell_step_hook_user);
        }
        else {
            int16_t draw = (int16_t)rng_thunk_draw15();
            direction = draw < 0 ? -(int)draw : (int)draw;
        }
        mark_step = 1;
        switch (direction & 3) {
        case 0: jitter_x = 0.5f; jitter_y = value; break;
        case 1: jitter_x = value; jitter_y = 0.5f; break;
        case 2: jitter_x = 0.5f; jitter_y = 1.0f - value; break;
        default: jitter_x = 1.0f - value; jitter_y = 0.5f; break;
        }
    } else if (input_xy != NULL) {
        jitter_x = input_xy[0];
        jitter_y = input_xy[1];
    }
    AiWaypoint* step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = (uint8_t)(action_mode == 0 ? 8 : 9);
    if (input_xy != NULL) {
        step->weight_y = jitter_x;
        step->weight_x = jitter_y;
    }
    if (mark_step != 0) step->pending |= 8u;
    return 1;
}

// FUNCTION: LEGORR 0x00403320
void ledger_stub_00403320_ObjectCommandTaskCreate(EntitySlot* entity,
                                                  int parameter)
{
    void* target = NULL;
    if (entity == NULL) return;
    if (entity->type == 2 && (entity->flags_3e0 & 0x400u) == 0u) {
        float position[2];
        entity_transform_position_get(entity, position);
        target = ledger_stub_00438df0_EntityLookupAtWorldPosition(position);
    }
    else if (entity->type == 1) {
        target = ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(entity);
    }
    if (target != NULL) {
        /* The original sweeps flagged records out of the entity chain, then
         * appends this task after the surviving tail.  Purging completed
         * commands uses a different policy and can drop records the corpus
         * keeps. */
        command_queue_sweep_relink(entity);
        cmd_entity_task_record_create_and_append(entity, target, parameter);
    }
}

void ledger_stub_00403750_CommandKindRecordIssueAndLink(
    uint32_t a, uint32_t b, uint32_t c, const int cell_xy[2], uint32_t zone,
    const float* random_pair, int set_flag)
{
    cmd_command_kind_record_issue(a, b, c, cell_xy, zone, random_pair,
                                  set_flag);
}

// FUNCTION: LEGORR 0x00403810
void ledger_stub_00403810_TerrainCellRecordCountUpdate(
    const int cell_xy[2], int a, int b, int c)
{
    void* record;
    EntitySlot* entity;
    record = ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
        cell_xy, (uint32_t)a, (uint32_t)b, (uint32_t)c);
    if (record != NULL) {
        if (entity_action_kind_entity_create_and_command_issue(
                (EntitySlot*)record, a, b, c) != NULL) return;
    }
    else if (c != 0 && b == 7) {
        record = ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
            cell_xy, 7u, 0u, (uint32_t)c);
        if (record != NULL) {
            void* count_record = ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
                NULL, cell_xy, 7u, 0u);
            uint32_t count = stats_instance_word_at_ec(count_record);
            if (count == 0u) return;
            while (count-- != 0u) {
                entity = (EntitySlot*)record;
                (void)entity_action_kind_entity_create_and_command_issue(
                    entity, 7, 0, c);
            }
            return;
        }
    }
    ++cmd_pending_command_counts[b * 0x10 + a * 0xf0 + c];
}

/* Object placement operations depend on game-specific policy, so keep that
 * policy in typed hooks while this module owns the search and dispatch flow. */
typedef struct ObjectPlacementHooks {
    LevelState* level;
    int (*candidate_allowed)(void* context, const int cell_xy[2],
                             void* object, uint32_t flags);
    int (*record_matches)(void* context, void* record, void* query);
    void* (*record_at)(void* context, const int cell_xy[2]);
    void* context;
} ObjectPlacementHooks;

// FUNCTION: LEGORR 0x00404e00
int ledger_stub_00404e00_ObjectPathSubmitToGridTarget(
    const ObjectPlacementHooks* hooks, void* object,
    const int target_xy[2], uint32_t flags)
{
    EntitySlot* entity = (EntitySlot*)object;
    int start_xy[2] = { -1, -1 };
    int* route_x = NULL;
    int* route_y = NULL;
    uint32_t route_count = 0u;
    float local_x;
    float local_y;
    float* points_x = NULL;
    float* points_y = NULL;
    uint32_t index;
    int found;

    if (hooks == NULL || hooks->level == NULL || entity == NULL ||
        target_xy == NULL) return 0;
    entity_position_to_terrain_cell(entity, hooks->level,
                                    &start_xy[0], &start_xy[1]);
    if (start_xy[0] < 0 || start_xy[1] < 0) return 0;

    if (start_xy[0] == target_xy[0] && start_xy[1] == target_xy[1]) {
        local_x = (float)start_xy[0];
        local_y = (float)start_xy[1];
        (void)entity_route_setup_and_marker_associate(
            entity, 1u, &local_x, &local_y, (int)flags);
        return 1;
    }

    found = terrain_neighbor_path_check(
        hooks->level, (uint32_t)start_xy[0], (uint32_t)start_xy[1],
        (uint32_t)target_xy[0], (uint32_t)target_xy[1], &route_x, &route_y,
        &route_count, NULL, NULL);
    if (!found) return 0;
    if (route_count == 0u || route_count > AI_WAYPOINTS_MAX) {
        free(route_x);
        free(route_y);
        return 0;
    }

    points_x = (float*)malloc((size_t)route_count * sizeof(float));
    points_y = (float*)malloc((size_t)route_count * sizeof(float));
    if (points_x == NULL || points_y == NULL) {
        free(points_x);
        free(points_y);
        free(route_x);
        free(route_y);
        return 0;
    }
    for (index = 0u; index < route_count; ++index) {
        points_x[index] = (float)route_x[index];
        points_y[index] = (float)route_y[index];
    }
    (void)entity_route_setup_and_marker_associate(
        entity, route_count, points_x, points_y, (int)flags);
    free(points_x);
    free(points_y);
    free(route_x);
    free(route_y);
    return 1;
}

// FUNCTION: LEGORR 0x00404ed0
int ledger_stub_00404ed0_CheckObjectCandidate(
    const EntitySlot* candidate, uint32_t field_cc)
{
    if (candidate->type == 6 &&
        (candidate->flags_3e8 & 0x80000000u) != 0u) {
        (void)ledger_stub_0044a3d0_PlaceCellNearestResolve(candidate, NULL);
        return 1;
    }
    return object_task_target_filter_find(
        (uint32_t)candidate->type, (uint32_t)candidate->etype,
        field_cc) != NULL;
}

void* ledger_stub_00404f10_FindMatchingObjectRecord(
    uint32_t type, uint32_t selector, uint32_t companion)
{
    return (void*)object_task_target_filter_find(type, selector, companion);
}

// FUNCTION: LEGORR 0x00404fc0
int ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
    const ObjectTaskEligibilityRecord* command, EntitySlot* entity,
    int screen_xy[2], int world_xy[2], uint32_t* out_flag,
    int require_command_gate, int relaxed_entity_gate)
{
    ObjectTaskEligibilityOps* ops = s_object_task_eligibility_ops;
    ObjectTaskEligibilityEntityView entity_view;
    ObjectTaskEligibilityEntityView target_view;
    EntitySlot* target;
    int kind, x = 0, y = 0, value;
    float simulation_time;
    int have_xy = 0;
    int transform_screen = 0;
    int world_from_entity = 0;
    int write_world_xy = 1;
    if (out_flag != NULL) *out_flag = 0u;
    if (command == NULL || entity == NULL || ops == NULL ||
        ops->global_feature_word == NULL ||
        ops->entity_view_get == NULL) return 0;
    target = command->target;
    if (target == NULL ||
        !ops->entity_view_get(ops->context, entity, &entity_view) ||
        !ops->entity_view_get(ops->context, target, &target_view)) return 0;
    kind = command->kind;

    /* Shared command and entity gates. */
    if ((kind != 3 || target_view.type != 8) &&
        command->task_scalar_18 > 0.0f) return 0;
    if ((entity_view.activity_flags & 0x20000u) != 0u &&
        entity_view.field_db == 0) return 0;
    if ((entity_view.flags_f8 & 0x800000u) != 0u ||
        (entity_view.flags_fb & 0x400u) != 0u) return 0;
    if ((*ops->global_feature_word & 1u) != 0u && kind != 0x1a) {
        if (ops->entity_special_gate == NULL ||
            ops->entity_special_gate(ops->context, entity)) return 0;
    }

    if (kind == 0x1a) {
        if (ops->entity_target_gate == NULL ||
            !ops->entity_target_gate(ops->context, entity, target) ||
            ops->entity_radius_gate == NULL ||
            !ops->entity_radius_gate(ops->context, entity, target) ||
            ops->entity_ray_gate == NULL ||
            ops->entity_ray_gate(ops->context, entity, target) ||
            ops->entity_eligible == NULL ||
            !ops->entity_eligible(ops->context, target, 1) ||
            ((*ops->global_feature_word & 1u) == 0u)) return 0;
        if (screen_xy != NULL && (ops->target_position_get == NULL ||
            !ops->target_position_get(ops->context, target,
                                      &screen_xy[0], &screen_xy[1]))) return 0;
        if (world_xy != NULL && (ops->target_world_position_get == NULL ||
            !ops->target_world_position_get(ops->context, target,
                                            &world_xy[0], &world_xy[1]))) return 0;
        return 1;
    }
    if (kind == 0x1e) {
        if (ops->target_kind_resolve == NULL ||
            ops->target_kind_resolve(ops->context, target) == 0 ||
            ops->target_activity_flags == NULL ||
            (ops->target_activity_flags(ops->context, entity) & 2) == 0 ||
            ops->target_position_get == NULL ||
            !ops->target_position_get(ops->context, target, &x, &y)) return 0;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 3) {
        if ((entity_view.flags_3e8 & 0x40u) == 0u ||
            ops->kind3_target_gate == NULL ||
            !ops->kind3_target_gate(ops->context, entity, target, 1)) return 0;
        if (entity_view.type == 1 &&
            (ops->kind1_activity_gate == NULL ||
             !ops->kind1_activity_gate(ops->context, entity))) return 0;
        if (target_view.type == 8 &&
            (entity_view.flags_fc & 8u) == 0u && relaxed_entity_gate) return 0;
        if (entity_view.type == 3 && target_view.type != 6) return 0;
        if (entity_view.type == 1) {
            if (target_view.type == 9 ||
                (target_view.type == 6 &&
                 (target_view.flags_3e8 & 0x80000000u) != 0u) ||
                ((entity_view.flags_f8 & 0x400u) != 0u &&
                 target_view.nested_type_2fc == 0xb)) return 0;
        }
        if (!ledger_stub_00404ed0_CheckObjectCandidate(
                target, target_view.task_owner_cc) ||
            ops->entity_target_relation == NULL ||
            !ops->entity_target_relation(ops->context, entity, 0)) return 0;
        if (ops->target_position_get == NULL ||
            !ops->target_position_get(ops->context, target, &x, &y)) return 0;
        have_xy = 1;
        world_from_entity = 1;
    }
    else if (kind == 8 || kind == 0 || kind == 0xb || kind == 0xc) {
        int walkable = 1;
        if (kind == 8) {
            walkable = ops->terrain_walkable != NULL &&
                ops->terrain_walkable(ops->context, entity, command->x,
                                      command->y,
                                      (command->field_17 & 8u) != 0u);
        }
        if (require_command_gate && kind != 0 && kind != 9 &&
            (ops->command_kind_gate == NULL ||
             !ops->command_kind_gate(ops->context, entity, kind))) return 0;
        if ((kind == 0xb && entity_view.type != 2) ||
            (kind == 0xc && (entity_view.activity_flags & 0x10000u) == 0u))
            return 0;
        if (kind == 8 && ((entity_view.flags_3e8 & 2u) == 0u ||
            !walkable || ((command->field_17 & 8u) != 0u &&
            (entity_view.activity_flags & 1u) == 0u))) return 0;
        x = command->x;
        y = command->y;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 10) {
        if ((require_command_gate &&
             (ops->command_kind_gate == NULL ||
              !ops->command_kind_gate(ops->context, entity, 10))) ||
            (relaxed_entity_gate && (entity_view.flags_fc & 0x10u) == 0u &&
             (command->field_17 & 0x200000u) == 0u) ||
            entity_view.type != 2) return 0;
        if (screen_xy != NULL) {
            if (ops->target_position_get == NULL ||
                !ops->target_position_get(ops->context, target, &x, &y))
                return 0;
            have_xy = 1;
            world_from_entity = 1;
        }
        else return 1;
    }
    else if (kind == 0x11) {
        if (ops->entity_eligible == NULL ||
            !ops->entity_eligible(ops->context, target, 0) ||
            entity_view.type != 2 || (entity_view.flags_f9 & 0x800u) != 0u ||
            entity_view.field_d1 > entity_view.field_d0 * 0.01f *
                                   ops->activity_scale ||
            (entity_view.flags_f8 & 0x400u) != 0u) return 0;
        return 1;
    }
    else if (kind == 0x12) {
        if (entity_view.type != 2 || entity_view.field_bc != 0 ||
            entity_view.field_db != 0 ||
            ((command->field_f & entity_view.flags_fc) == 0u &&
             relaxed_entity_gate) || ops->target_activity_flags == NULL) return 0;
        value = ops->target_activity_flags(ops->context, target);
        if ((value & 0x2000000) == 0) {
            if (ops->target_activity_position_get == NULL ||
                !ops->target_activity_position_get(ops->context, target,
                                                   &x, &y)) return 0;
            have_xy = 1;
            transform_screen = 1;
        }
        else if (ops->target_position_get != NULL) {
            if (!ops->target_position_get(ops->context, target, &x, &y))
                return 0;
            have_xy = 1;
            world_from_entity = 1;
            if (screen_xy == NULL) write_world_xy = 0;
        }
        else return 0;
    }
    else if (kind == 0x15) {
        if (entity_view.type != 1 || entity_view.field_db == 0 ||
            ops->target_kind_resolve == NULL ||
            ops->target_route_position_get == NULL ||
            !ops->target_route_position_get(ops->context, target, &x, &y))
            return 0;
        value = ops->target_kind_resolve(ops->context, target);
        (void)value;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 0x16) {
        if (entity_view.type != 2 || ops->task_coordinate_check == NULL ||
            !ops->task_coordinate_check(ops->context, command->x, command->y))
            return 0;
        x = command->x;
        y = command->y;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 0x13) {
        EntitySlot* resolved_target;
        if (entity_view.type != 2 || ops->target_activity_gate == NULL ||
            !ops->target_activity_gate(ops->context, entity) ||
            ops->target_visibility_check == NULL ||
            ops->target_visibility_check(ops->context, entity,
                                         (int)command->field_e) ||
            ops->kind13_target_resolve == NULL ||
            ops->target_position_get == NULL) return 0;
        resolved_target = ops->kind13_target_resolve(
            ops->context, entity, command->field_e);
        if (resolved_target == NULL) return 0;
        if (screen_xy == NULL) return 1;
        if (!ops->target_position_get(ops->context, resolved_target, &x, &y))
            return 0;
        have_xy = 1;
        world_from_entity = 1;
    }
    else if (kind == 0x17) {
        if (entity_view.type != 2 ||
            ((entity_view.flags_fc & 1u) != 0u && command->field_f == 1u) ||
            ((entity_view.flags_fc & 2u) != 0u && command->field_f == 2u) ||
            ((entity_view.flags_fc & 4u) != 0u && command->field_f == 4u) ||
            ((entity_view.flags_fc & 8u) != 0u && command->field_f == 8u) ||
            ((entity_view.flags_fc & 0x10u) != 0u && command->field_f == 0x10u) ||
            ((entity_view.flags_fc & 0x20u) != 0u && command->field_f == 0x20u) ||
            ops->simulation_time_get == NULL) return 0;
        simulation_time = ops->simulation_time_get(ops->context);
        if (entity_view.field_f5 < simulation_time) return 0;
        if (screen_xy != NULL || world_xy != NULL) {
            if (ops->target_position_get == NULL ||
                !ops->target_position_get(ops->context, target, &x, &y)) return 0;
            have_xy = 1;
            world_from_entity = 1;
        }
    }
    else if (kind != 0x1a && kind != 0x1e) {
        if (out_flag == NULL) return 1;
        *out_flag = 1u;
        return 1;
    }

    if (have_xy) {
        if (screen_xy != NULL && transform_screen) {
            if (ops->position_to_world == NULL ||
                !ops->position_to_world(ops->context, x, y,
                                        &screen_xy[0], &screen_xy[1])) return 0;
        }
        else if (screen_xy != NULL) {
            screen_xy[0] = x;
            screen_xy[1] = y;
        }
        if (world_xy != NULL && kind != 0x13 && write_world_xy) {
            if (world_from_entity) {
                if (ops->target_world_position_get == NULL) return 0;
                if (!ops->target_world_position_get(ops->context, target,
                                                    &world_xy[0], &world_xy[1]))
                    return 0;
            }
            else { world_xy[0] = x; world_xy[1] = y; }
        }
    }
    return 1;
}

// PendingCommandTimerFireScan
void ledger_stub_00405950_PendingCommandTimerFireScan()
{
    pending_command_timer_fire_scan();
}

// PendingCommandEntityAssignmentTick
void ledger_stub_00405c10_PendingCommandEntityAssignmentTick()
{
    pending_command_entity_assignment_tick();
}

// TypeTableResourceRowLoad
// FUNCTION: LEGORR 0x00406990
int ledger_stub_00406990_TypeTableResourceRowLoad(
    TypeConfigTable* table, void* section, uint32_t row_kind,
    const TypeConfigHooks* hooks)
{
    return type_config_table_load(table, section, row_kind, hooks);
}

// RendererRecordClone
// FUNCTION: LEGORR 0x004078c0
void ledger_stub_004078c0_EntityCursorTargetStepResolveAndTimerAdvance(
    EntitySlot* entity, float delta, uint32_t* value_out, float out_xy[2])
{
    if (entity == NULL) return;
    if (out_xy != NULL)
        (void)entity_cursor_target_screen_position_get(entity, out_xy);

    entity->auxiliary_state_388 -= delta;
    if (entity->auxiliary_state_388 <= 0.0f) {
        if (value_out != NULL)
            pending_type_value_slot_select(entity, value_out);
    }
    else if (value_out != NULL) {
        *value_out = entity->auxiliary_slot_38c;
    }
    if (value_out != NULL && *value_out == 0u)
        *value_out = pending_type_value_get(0x00557868u);
}

// BuildingTypeConfigRecordParse
// FUNCTION: LEGORR 0x00407d60
int ledger_stub_00407d60_BuildingTypeConfigRecordParse(
    TypeConfigTable* table, void* section, const TypeConfigHooks* hooks)
{
    return type_config_table_load(table, section, 2u, hooks);
}

void ledger_stub_00408620_SceneObjectRecordCloneRetainHandles(
    const SceneObjectRecord* source, SceneObjectRecord* destination)
{
    scene_object_record_clone_retain_handles(source, destination);
}



// FUNCTION: LEGORR 0x00408d70
int ledger_stub_00408d70_TerrainDescriptorCoordinateCountLimitCheck(
    uint32_t descriptor_key, int type, int row)
{
    uint8_t* record = (uint8_t*)path_record_last_peek();
    uint32_t descriptor_flags;
    uint32_t descriptor_value;
    uint32_t coordinate_count;
    uint32_t matching_count = 0u;
    uint32_t limit;
    uint32_t index;
    int selector;
    int result;

    /* TerrainDescriptorFindExact(0, &key) walks the path-record chain and
     * compares the key against record +8.  The port stores that same chain
     * through the widened +0x24 link. */
    while (record != NULL) {
        uint32_t key;
        memcpy(&key, record + 0x08, sizeof(key));
        if (key == descriptor_key) break;
        record = (uint8_t*)*(void**)(record + 0x24);
    }
    if (record == NULL) return 0;

    memcpy(&descriptor_value, record, sizeof(descriptor_value));
    memcpy(&coordinate_count, record + 0x20, sizeof(coordinate_count));
    if (coordinate_count != 0u) {
        uint32_t** coordinates = (uint32_t**)(record + 0x2c);
        for (index = 0u; index < coordinate_count; ++index) {
            const uint32_t* pair = coordinates[index];
            if (pair != NULL && pair[0] == (uint32_t)type &&
                pair[1] == (uint32_t)row) {
                ++matching_count;
            }
        }
    }

    memcpy(&descriptor_flags, record + 0xd0, sizeof(descriptor_flags));
    selector = (descriptor_flags & 2u) != 0u ? 15 : 4;
    if (type == 6) {
        limit = stats_table_word_at_b4(selector, (int)descriptor_value, 0);
    }
    else if (type == 7) {
        if (row == 0) {
            limit = entity_indexed_record_word_at_b0_get(
                selector, (int)descriptor_value, 0);
        }
        else {
            limit = stats_table_word_at_d0(selector,
                                           (int)descriptor_value, 0);
        }
    }
    else {
        return 1;
    }
    result = matching_count < limit;
    return result;
}

// FUNCTION: LEGORR 0x00408ec0
uint32_t ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    static const int neighbor_xy[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    GameContext* context = entity_pool_callback_context_get();
    uint8_t* descriptor = (uint8_t*)path_record_last_peek();
    uint32_t emitted = 0u;
    float scale;
    float edge;
    float command_offsets[4][2];
    uint32_t segment;

    if (cell_xy == NULL || footprint_xy == NULL || footprint_count == 0u ||
        context == NULL || context->level == NULL ||
        context->level->ptr_terrain == NULL) return 0u;

    /* TerrainDescriptorFindExact(cell_xy, 0): records are head-linked at
     * +0x24 and key the requested cell at +0x0c/+0x10. */
    while (descriptor != NULL) {
        int descriptor_x, descriptor_y;
        memcpy(&descriptor_x, descriptor + 0x0c, sizeof(descriptor_x));
        memcpy(&descriptor_y, descriptor + 0x10, sizeof(descriptor_y));
        if (descriptor_x == cell_xy[0] && descriptor_y == cell_xy[1]) break;
        descriptor = (uint8_t*)*(void**)(descriptor + 0x24);
    }
    if (descriptor == NULL) return 0u;

    /* The original obtains the terrain handle from the current level and
     * uses its +0x10 block size to place the four edge command anchors. */
    scale = terrain_blocksize_get(context->level->ptr_terrain);
    if (scale == 0.0f) return 0u;
    edge = 8.8f / scale;
    command_offsets[0][0] = 0.5f;
    command_offsets[0][1] = 1.0f - edge;
    command_offsets[1][0] = 1.0f - edge;
    command_offsets[1][1] = 0.5f;
    command_offsets[2][0] = 0.5f;
    command_offsets[2][1] = edge;
    command_offsets[3][0] = edge;
    command_offsets[3][1] = 0.5f;

    for (segment = 0u; segment < footprint_count; ++segment) {
        const int* point = footprint_xy + segment * 2u;
        int is_duplicate = segment + 1u < footprint_count &&
            point[0] == point[2] && point[1] == point[3];
        int direction;
        if (is_duplicate) {
            /* Duplicate consecutive coordinates delimit the same interior
             * pair in the corpus loop; it advances over both entries. */
            ++segment;
            continue;
        }

        for (direction = 0; direction < 4; ++direction) {
            int neighbor_x = point[0] + neighbor_xy[direction][0];
            int neighbor_y = point[1] + neighbor_xy[direction][1];
            uint32_t candidate;
            int inside = 0;
            for (candidate = 0u; candidate < footprint_count;) {
                const int* candidate_xy = footprint_xy + candidate * 2u;
                if (candidate + 1u < footprint_count &&
                    candidate_xy[0] == candidate_xy[2] &&
                    candidate_xy[1] == candidate_xy[3]) {
                    /* The corpus's paired-coordinate walk advances past a
                     * duplicate pair without considering either entry. */
                    candidate += 2u;
                    continue;
                }
                if (candidate_xy[0] == neighbor_x &&
                    candidate_xy[1] == neighbor_y) {
                    inside = 1;
                    break;
                }
                ++candidate;
            }
            if (!inside) {
                uint32_t descriptor_key;
                memcpy(&descriptor_key, descriptor + 0x08,
                       sizeof(descriptor_key));
                cmd_command_kind_record_issue(
                    9u, 0u, 0u, point, descriptor_key,
                    command_offsets[direction], 1);
                ++(*(uint32_t*)(descriptor + 0x1c));
                ++emitted;
            }
        }
    }
    return emitted;
}

// FUNCTION: LEGORR 0x004093b0
static void placement_cell_search_expand(const int cell_xy[2])
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    int direction;
    if (level == NULL || cell_xy == NULL) return;
    for (direction = 0; direction < 4; ++direction) {
        int next_xy[2];
        int nx = cell_xy[0] + offset[direction][0];
        int ny = cell_xy[1] + offset[direction][1];
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_secondary_flag_100(level, nx, ny) != 0u) continue;
        terrain_cell_flag100_set_and_track(level, nx, ny);
        next_xy[0] = nx;
        next_xy[1] = ny;
        placement_cell_search_expand(next_xy);
    }
}
void ledger_stub_004093b0_PassableAdjacentCellSearchExpand(
    const int cell_xy[2])
{
    placement_cell_search_expand(cell_xy);
}

// FindNearbyPlacementCandidate
// FUNCTION: LEGORR 0x00409470
static int nearby_placement_entity_search(LevelState* level, int x, int y,
                                          int resource_amount)
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    int direction;
    if (level == NULL) return 0;
    for (direction = 0; direction < 4; ++direction) {
        int cell[2];
        EntitySlot* found;
        cell[0] = x + offset[direction][0];
        cell[1] = y + offset[direction][1];
        if (level_cell_secondary_bit0(level, cell[0], cell[1]) != 0u) {
            found = entity_at_grid_position_find(cell);
            if (found != NULL && entity_available_resource_spend(
                    level, resource_amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, cell[0], cell[1]) &&
            !level_cell_occupied_or_flag(level, cell[0], cell[1], 1)) continue;
        if (level_cell_visited_flag(level, cell[0], cell[1]) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, cell[0], cell[1]);
        if (nearby_placement_entity_search(level, cell[0], cell[1],
                                           resource_amount)) return 1;
    }
    return 0;
}
// FUNCTION: LEGORR 0x00409450
int ledger_stub_00409450_FindNearbyPlacementCandidate(
    const int start_xy[2], int resource_amount)
{
    GameContext* context = entity_pool_callback_context_get();
    int found;
    if (context == NULL || context->level == NULL || start_xy == NULL) return 0;
    found = nearby_placement_entity_search(context->level, start_xy[0],
                                           start_xy[1], resource_amount);
    terrain_cell_visited_flags_clear_all(context->level);
    return found;
}

// NeighborCellEntitySearchRecurse
int ledger_stub_00409470_NeighborCellEntitySearchRecurse(
    const int cell_xy[2], int resource_amount)
{
    GameContext* context = entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL || cell_xy == NULL) return 0;
    return nearby_placement_entity_search(context->level, cell_xy[0],
                                          cell_xy[1], resource_amount);
}

// FUNCTION: LEGORR 0x00409600
void ledger_stub_00409600_TerrainDescriptorTilesResetAndFree(
    const int cell_xy[2])
{
    GameContext* context = entity_pool_callback_context_get();
    uint8_t* descriptor = (uint8_t*)path_record_last_peek();
    int* coordinates;
    uint32_t count;
    uint32_t entity_count;
    uint32_t entity_index;
    uint32_t progress;
    uint32_t descriptor_key;
    if (cell_xy == NULL || context == NULL || context->level == NULL)
        return;

    /* TerrainDescriptorFindExact(cell_xy, 0). */
    while (descriptor != NULL) {
        int x, y;
        memcpy(&x, descriptor + 0x0c, sizeof(x));
        memcpy(&y, descriptor + 0x10, sizeof(y));
        if (x == cell_xy[0] && y == cell_xy[1]) break;
        descriptor = (uint8_t*)*(void**)(descriptor + 0x24);
    }
    if (descriptor == NULL) return;
    memcpy(&progress, descriptor + 0x1c, sizeof(progress));
    if (progress == *(uint32_t*)(descriptor + 0x20)) return;
    coordinates = (int*)path_record_payload_get(descriptor);
    count = path_record_payload_count_get(descriptor);
    memcpy(&descriptor_key, descriptor + 0x08, sizeof(descriptor_key));
    if (count != 0u && coordinates == NULL) return;

    progress = 0u;
    while (progress < count) {
        int x = coordinates[progress * 2u];
        int y = coordinates[progress * 2u + 1u];
        int duplicate = progress + 1u < count &&
            x == coordinates[(progress + 1u) * 2u] &&
            y == coordinates[(progress + 1u) * 2u + 1u];
        uint8_t* cell = level_voxel(context->level, x, y);
        if (cell != NULL) {
            uint32_t* cell_flags = (uint32_t*)(cell + 0x08);
            if (duplicate) *cell_flags &= ~0x8000u;
            *cell_flags &= ~0x100000u;
            (void)level_cell_path_record_set(context->level, x, y, NULL);
            terrain_cell_refresh_dispatch(context, x, y);
            cancel_action_for_matching_coordinates(
                (const uint32_t*)coordinates + progress * 2u);
        }
        if (duplicate) ++progress;
        ++progress;
    }

    entity_count = path_record_entity_count_get(descriptor);
    for (entity_index = 0u; entity_index < entity_count; ++entity_index) {
        EntitySlot* entity = path_record_entity_get(descriptor, entity_index);
        if (entity == NULL) continue;
        if (entity->type == 9) {
            entity->flags_3e8 |= 0x800000u;
            entity->field_340 = -1.0f;
        }
        else {
            entity->flags_3ec &= ~0x100u;
            cmd_create_task(entity);
        }
    }

    cmd_cancel_by_coords((int)descriptor_key, 0, 0, 0);
    path_record_release(descriptor);
}

// CellPathRecordRegisterApply
// FUNCTION: LEGORR 0x00409790
uint32_t ledger_stub_00409790_CellPathRecordRegisterApply(
    uint32_t record_key, const int cell_xy[2], uint32_t direction,
    const int* footprint_xy, uint32_t footprint_count)
{
    GameContext* context = entity_pool_callback_context_get();
    void* record;
    int* coordinate_copy = NULL;
    uint32_t index = 0u;
    if (cell_xy == NULL ||
        (footprint_count != 0u && footprint_xy == NULL) ||
        (size_t)footprint_count > ((size_t)-1) / (2u * sizeof(int)))
        return 0u;

    record = path_record_allocate_and_link((const uint32_t*)cell_xy);
    if (record == NULL) return 0u;
    memcpy((char*)record + 0x00, &record_key, sizeof(record_key));
    memcpy((char*)record + 0x04, &direction, sizeof(direction));
    if (footprint_count != 0u) {
        coordinate_copy = (int*)engine_heap_alloc((long)(
            (size_t)footprint_count * 2u * sizeof(int)));
        if (coordinate_copy == NULL) {
            path_record_release(record);
            return 0u;
        }
        memcpy(coordinate_copy, footprint_xy,
               (size_t)footprint_count * 2u * sizeof(int));
    }
    if (!path_record_payload_set(record, coordinate_copy, footprint_count)) {
        engine_heap_free(coordinate_copy);
        path_record_release(record);
        return 0u;
    }
    if (footprint_count != 0u && path_record_surface_grid_get() != NULL) {
        /* The corpus passes 4.0f to TerrainCellListSurfaceHeightClampMutate;
         * that helper works with half the value on either side of its mean. */
        terrain_cell_surface_height_clamp(path_record_surface_grid_get(),
                                          coordinate_copy,
                                          footprint_count, 2.0f);
    }

    while (index < footprint_count) {
        int x = coordinate_copy[index * 2u];
        int y = coordinate_copy[index * 2u + 1u];
        uint8_t* cell = context != NULL && context->level != NULL
            ? level_voxel(context->level, x, y) : NULL;
        int duplicate = index + 1u < footprint_count &&
            x == coordinate_copy[(index + 1u) * 2u] &&
            y == coordinate_copy[(index + 1u) * 2u + 1u];
        if (cell != NULL) {
            uint32_t* cell_flags = (uint32_t*)(cell + 0x08);
            static const int neighbor_xy[8][2] = {
                { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
                { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }
            };
            int neighbor;
            if (duplicate) *cell_flags |= 0x8000u;
            *cell_flags |= 0x100000u;
            (void)level_cell_path_record_set(context->level, x, y, record);
            terrain_cell_refresh_dispatch(context, x, y);
            for (neighbor = 0; neighbor < 8; ++neighbor) {
                terrain_cell_refresh_dispatch(
                    context, x + neighbor_xy[neighbor][0],
                    y + neighbor_xy[neighbor][1]);
            }
            cancel_action_for_matching_coordinates(
                (const uint32_t*)coordinate_copy + index * 2u);
            if (duplicate) ++index;
        }
        ++index;
    }
    return *(uint32_t*)((char*)record + 0x08);
}

// RefreshEightNeighborCells
typedef void (*CellRefreshHook)(void* context, int x, int y);
// FUNCTION: LEGORR 0x00409940
uint32_t ledger_stub_00409940_RefreshEightNeighborCells(
    const int cell_xy[2], CellRefreshHook refresh, void* context)
{
    int dx, dy;
    uint32_t count = 0u;
    if (cell_xy == NULL || refresh == NULL) return 0u;
    for (dy = -1; dy <= 1; ++dy)
        for (dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            refresh(context, cell_xy[0] + dx, cell_xy[1] + dy);
            ++count;
        }
    return count;
}

// EntityCellPathEffectsApply
// FUNCTION: LEGORR 0x00409d50
void ledger_stub_00409d50_EntityCellPathEffectsApply(
    EntitySlot* entity, LevelState* level,
    const EntityCellPathEffectHooks* hooks)
{
    const int* source_cells = NULL;
    int source_cell[2];
    int rotation;
    uint32_t count = 0u;
    uint32_t i;
    const int* rotated;
    if (entity == NULL || level == NULL || hooks == NULL ||
        hooks->placement_cells_get == NULL || hooks->rotation_get == NULL ||
        !hooks->placement_cells_get(hooks->context, entity, &source_cells,
                                    &count) ||
        (count != 0u && source_cells == NULL)) return;
    entity_position_to_terrain_cell(entity, level, &source_cell[0],
                                    &source_cell[1]);
    rotation = hooks->rotation_get(hooks->context, entity);
    rotated = (const int*)rotated_pairs_scratch_fill(source_cell,
        source_cells, (int)count, rotation);
    for (i = 0; i < count; ++i) {
        int x = rotated[i * 2u];
        int y = rotated[i * 2u + 1u];
        int repeated = i + 1u < count &&
            x == rotated[(i + 1u) * 2u] &&
            y == rotated[(i + 1u) * 2u + 1u];
        if (hooks->cell_prepare != NULL)
            hooks->cell_prepare(hooks->context, x, y);
        if ((entity->flags_3ec & 8u) == 0u &&
            hooks->cell_effect_apply != NULL)
            hooks->cell_effect_apply(hooks->context, entity, x, y, repeated);
        if (hooks->cell_refresh != NULL)
            hooks->cell_refresh(hooks->context, x, y);
    }
    if (hooks->placement_commands_build != NULL)
        hooks->placement_commands_build(hooks->context, entity);
}

// EntityPlacementCommandsBuildOnce
typedef struct EntityPlacementCommandBuildState {
    void* entity;
    void* context;
    int built;
    int (*build)(void* context, void* entity);
} EntityPlacementCommandBuildState;
// FUNCTION: LEGORR 0x00409f20
int ledger_stub_00409f20_EntityPlacementCommandsBuildOnce(
    EntityPlacementCommandBuildState* state)
{
    if (state == NULL || state->entity == NULL || state->build == NULL)
        return 0;
    if (state->built) return 1;
    if (!state->build(state->context, state->entity)) return 0;
    state->built = 1;
    return 1;
}

// FUNCTION: LEGORR 0x0040ab70
int ledger_stub_0040ab70_DependencyGridConfigLoad(BlockFile* config,
                                                  int level)
{
    return unported_ledger_stubs::dependency_grid_config_load(config, level);
}

void ledger_stub_0040b000_TileMatchingCellsCollect(
    uint32_t first, uint32_t second, uint32_t selector,
    DependencyGridMatches* output)
{
    (void)dependency_grid_matches_collect(first, second, selector, output);
}

// FUNCTION: LEGORR 0x0040b000
int dependency_grid_matches_collect(uint32_t first, uint32_t second,
                                    uint32_t selector,
                                    DependencyGridMatches* output)
{
    DependencyGridState* state = s_dependency_grid_state;
    uint32_t row, column, variant;
    if (output == NULL) return 0;
    output->count = 0u;
    if (state == NULL) return 0;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            DependencyGridCell* cell = &state->cells[row][column];
            const uint32_t variant_limit =
                cell->variant_records_present != 0u
                    ? DEPENDENCY_GRID_VARIANTS : 1u;
            for (variant = 0u; variant < variant_limit; ++variant) {
                const DependencyGridRecordList* list =
                    cell->variant_records_present != 0u
                        ? &cell->variant_records[variant]
                        : &cell->base_records;
                uint32_t entry;
                if (list->records == NULL) continue;
                for (entry = 0u; entry < list->count; ++entry) {
                    const DependencyGridTuple* tuple = &list->records[entry];
                    uint32_t out_index;
                    if (tuple->first != first || tuple->second != second ||
                        (tuple->wildcard != 0u &&
                         tuple->selector != selector)) continue;
                    out_index = output->count;
                    if (out_index < DEPENDENCY_GRID_MATCH_CAPACITY) {
                        output->x[out_index] = row;
                        output->y[out_index] = column;
                        output->variant[out_index] = variant;
                        output->variant_present[out_index] =
                            cell->variant_records_present != 0u ? 1u : 0u;
                    }
                    ++output->count;
                }
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0040b1b0
uint32_t dependency_grid_occupancy_pass(void)
{
    DependencyGridState* state = s_dependency_grid_state;
    uint32_t row, column, variant, changed = 0u;
    if (state == NULL || state->occupancy_get == NULL) return 0u;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            DependencyGridCell* cell = &state->cells[row][column];
            for (variant = 0u; variant < DEPENDENCY_GRID_VARIANTS; ++variant) {
                uint32_t flags;
                int occupied;
                if (variant == 1u && cell->variant_records_present != 0u)
                    break;
                flags = cell->flags[variant];
                if ((flags & 8u) != 0u) continue;
                occupied = state->occupancy_get(state->context, row,
                                                column, variant);
                if (occupied == 0) {
                    if ((flags & 4u) == 0u) continue;
                    flags = (flags & ~4u) | 8u;
                }
                else if ((flags & 4u) == 0u) {
                    flags |= 4u;
                }
                else {
                    flags = (flags & ~4u) | 8u;
                }
                cell->flags[variant] = flags;
                ++changed;
            }
        }
    }
    return changed;
}

void ledger_stub_0040b1b0_DependencyGridOccupancyPass(void)
{
    (void)dependency_grid_occupancy_pass();
}

/* Retain the earlier port-facing mesh-grid adapter for internal callers. */
int ledger_stub_0040b310_LoadTileVariants(TerrainTileMeshGrid* grid,
                                          int editor_mode)
{
    return terrain_tile_mesh_grid_load(grid, editor_mode);
}

TerrainCellResourceGrid* ledger_stub_0040b310_LoadTileVariants(
    const char* bitmap_prefix, uint32_t width, uint32_t height)
{
    return terrain_cell_resource_grid_load(bitmap_prefix, width, height);
}

TileModelEntityRegistration* ledger_stub_0040b5f0_TileModelEntityRegister(
    void* manager, const char* first_entry, const char* second_entry,
    float scale_percent, int parameter,
    const TileModelEntityRegistrationHooks* hooks)
{
    return tile_model_entity_register(manager, first_entry, second_entry,
                                      scale_percent, parameter, hooks);
}

void ledger_stub_0040b880_CellVisualChannelRelay(
    SceneCellVisualRelayRecord* record, const float corner_points[12],
    uint32_t lookup_selector, uint32_t corner_index, float scale,
    float uniform_scale)
{
    scene_cell_visual_channels_apply(record, corner_points, lookup_selector,
                                     corner_index, scale, uniform_scale);
}

void ledger_stub_0040ba00_SceneRotationFlagsApply(
    void* object, float projection_scale, const float first[3],
    const float second[3],
    const float third[3], float scale, float uniform_scale,
    float child_value, uint8_t flags)
{
    scene_rotation_flags_apply(object, projection_scale, first, second,
                               third, scale, uniform_scale, child_value,
                               flags);
}

void ledger_stub_0040bb90_SceneFrameFromTripletApply(
    void* object, float projection_scale, const float first[3],
    const float second[3], const float third[3])
{
    scene_frame_from_triplet_apply(object, projection_scale, first, second,
                                   third);
}

// SetTimeoutTo10000
// FUNCTION: LEGORR 0x0040bdc0
void ledger_stub_0040bdc0_SetTimeoutTo10000(void)
{
    uint32_t* ignored_output = NULL;
    (void)scene_resource_availability_sweep(10000.0f, &ignored_output,
                                             &ignored_output);
}

// CellCornerOccupancyScan examines the center and ordered cardinal neighbors
// in the map-record sidecar and the level's special-state predicate.
// FUNCTION: LEGORR 0x0040e1e0
uint32_t ledger_stub_0040e1e0_CellCornerOccupancyScan(
    LevelState* level, int cell_x, int cell_y)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { 1, 0, -1, 0 };
    uint32_t i;
    if (level == NULL ||
        terrain_cell_object_record_present(cell_x, cell_y) ||
        terrain_cell_special_state_predicate(level, cell_x, cell_y, 0))
        return 0u;
    for (i = 0u; i < 4u; ++i) {
        int nx = cell_x + dx[i];
        int ny = cell_y + dy[i];
        int opposite = (int)((i - 2u) & 3u);
        int ox = cell_x + dx[opposite];
        int oy = cell_y + dy[opposite];
        int neighbor_record = terrain_cell_object_record_present(nx, ny);
        int neighbor_special = 0;
        if (!neighbor_record) {
            neighbor_special = terrain_cell_special_state_predicate(
                level, nx, ny, 0);
            if (!neighbor_special) continue;
        }
        if (terrain_cell_object_record_present(ox, oy)) return 1u;
        if (terrain_cell_special_state_predicate(level, ox, oy, 0) &&
            !neighbor_special &&
            (!neighbor_record || !terrain_cell_special_state_predicate(
                level, nx, ny, 0))) return 1u;
    }
    return 0u;
}

// LavaErosionNeighborMutate
void ledger_stub_0040f330_LavaErosionNeighborMutate(
    const int xy[2], unsigned int direction, int propagate)
{
    lava_erosion_neighbor_mutate(xy, direction, propagate);
}

// ErosionEightNeighborPropagate
void ledger_stub_0040f5f0_ErosionEightNeighborPropagate(
    const int xy[2], unsigned int direction)
{
    lava_erosion_eight_neighbor_propagate(xy, direction);
}

// LavaErosionCellEligibleAndMutate
int ledger_stub_0040f190_LavaErosionCellEligibleAndMutate(
    const int xy[2], int hint)
{
    return lava_erosion_cell_eligible_and_mutate(xy, hint);
}


// GlobalVectorSetAndNotifyListeners

// FUNCTION: LEGORR 0x00418040
int ledger_stub_00418040_CampaignChainSnapshotCreate(
    const GameContext* ctx, void* snapshot, size_t capacity)
{
    unsigned char* bytes = (unsigned char*)snapshot;
    size_t records_size;
    if (ctx == NULL || bytes == NULL || ctx->record_count < 0) return 0;
    records_size = (size_t)ctx->record_count * SAVE_RECORD_SIZE;
    if ((size_t)ctx->record_count != 0u
        && records_size / (size_t)ctx->record_count != SAVE_RECORD_SIZE)
        return 0;
    if (capacity < 0xb8u + records_size
        || (records_size != 0u && ctx->records == NULL)) return 0;
    memset(bytes, 0, 0xb8u);
    memcpy(bytes + 0x00, &ctx->save_slot, sizeof(ctx->save_slot));
    memcpy(bytes + 0x04, &ctx->active_slot, sizeof(ctx->active_slot));
    memcpy(bytes + 0x08, &ctx->record_count, sizeof(ctx->record_count));
    memcpy(bytes + 0x0c, &ctx->osf_count, sizeof(ctx->osf_count));
    memcpy(bytes + 0x10, ctx->saves_dir, sizeof(ctx->saves_dir));
    if (records_size != 0u)
        memcpy(bytes + 0xb8u, ctx->records, records_size);
    return 1;
}

// CategoryCellCandidateListBuild
typedef int (*CategoryCellEligibleHook)(void* context, int category,
                                        int x, int y);
// FUNCTION: LEGORR 0x00418920
uint32_t ledger_stub_00418920_CategoryCellCandidateListBuild(
    int category, const int* cells_xy, uint32_t cell_count,
    CategoryCellEligibleHook eligible, void* context, int* output_xy,
    uint32_t output_capacity)
{
    uint32_t i, count = 0u;
    if (cells_xy == NULL || eligible == NULL) return 0u;
    for (i = 0u; i < cell_count; ++i) {
        int x = cells_xy[i * 2u];
        int y = cells_xy[i * 2u + 1u];
        if (!eligible(context, category, x, y)) continue;
        if (output_xy != NULL && count < output_capacity) {
            output_xy[count * 2u] = x;
            output_xy[count * 2u + 1u] = y;
        }
        ++count;
    }
    return count;
}

// OverlayTextBufferRefresh
int ledger_stub_00418a00_OverlayTextBufferRefresh(void)
{
    return overlay_text_buffer_refresh();
}

// FUNCTION: LEGORR 0x0041a1a0
int ledger_stub_0041a1a0_UIEntryHitTestAndDispatch(
    int x, int y, int click)
{
    UiEntryHitTestState* state = s_ui_entry_hit_test_state;
    uint32_t index, entry_count;
    if (state == NULL || state->entries == NULL || state->count == 0u ||
        state->object_bounds_get == NULL) return 0;
    entry_count = state->count;
    if (entry_count > UI_HIT_ENTRY_CAPACITY)
        entry_count = UI_HIT_ENTRY_CAPACITY;

    for (index = 0u; index < entry_count; ++index) {
        UiEntryHitRecord* entry = &state->entries[index];
        int width = 0;
        int height = 0;
        int top = crt_float_to_long(entry->vertical_offset);
        if (!state->object_bounds_get(state->context, entry->fields[2],
                                      &width, &height)) continue;
        if (x < 0 || x > width || y < top || y > top + height) continue;
        if (click != 0) {
            if ((state->flags & 1u) != 0u && index == 0u) {
                panel_state_flags_toggle(7);
                state->flags &= ~1u;
                return 1;
            }
            if (index != 0u) {
                UiEntryHitRecord selected = *entry;
                uint32_t move;
                for (move = index; move != 0u; --move)
                    state->entries[move] = state->entries[move - 1u];
                state->entries[0] = selected;
            }
            /* RecordListEntryMoveToFront sets both state bits even when the
             * clicked row is already at index zero. */
            state->flags |= 3u;
        }
        return 1;
    }
    return 0;
}

int ledger_stub_0041baa0_SimulationPointerActionDispatch(
    int action, int kind, int entry, int active, int suppress_initial)
{
    return pointer_action_state_dispatch(action, kind, entry, active,
                                         suppress_initial);
}

// FUNCTION: LEGORR 0x0041c4f0
int ledger_stub_0041c4f0_AdvisorAnimationPositionAndRequestUpdate(
    int row, int immediate)
{
    advisor_apply_slot_transform(row);
    return advisor_request_admit(row, immediate);
}

int ledger_stub_0041c7b0_InputActionStateUpdate(const EngineContext* context,
                                                int action, int alternate)
{
    (void)context;
    (void)alternate;
    input_action_state_update(action);
    return 0;
}

// CatalogSlotFlagUpdate
void ledger_stub_0041c800_CatalogSlotFlagUpdate(int category, int slot)
{
    catalog_slot_flag_update(category, slot);
}

// FUNCTION: LEGORR 0x0041cd30
void ledger_stub_0041cd30_PositionTrackerBoxDraw(
    SurfaceRasterContext* raster, int x, int y, int width, int height,
    uint32_t flags, uint32_t pixel)
{
    SurfacePoint points[5];
    if (raster == NULL || (flags & 0x100000u) != 0u) return;
    points[0].x = x - 1; points[0].y = y - 1;
    points[1].x = x + width + 1; points[1].y = y - 1;
    points[2].x = x + width + 1; points[2].y = y + height + 1;
    points[3].x = x - 1; points[3].y = y + height + 1;
    points[4] = points[0];
    surface_draw_polyline(raster, points, 5, pixel);
    points[0].x = x + 2; points[0].y = y + 2;
    points[1].x = x + width - 2; points[1].y = y + 2;
    points[2].x = x + width - 2; points[2].y = y + height - 2;
    points[3].x = x + 2; points[3].y = y + height - 2;
    points[4] = points[0];
    surface_draw_polyline(raster, points, 5, pixel);
}


static UiActionDispatchState* s_ui_action_dispatch_state;

void ui_action_dispatch_state_install(UiActionDispatchState* state)
{
    s_ui_action_dispatch_state = state;
}

static void ui_action_dispatch_mode_request(UiActionDispatchState* state,
                                            int mode, int argument)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->ui_mode_request != NULL)
        ops->ui_mode_request(ops->context, mode, argument);
}

static void ui_action_dispatch_selection_clear(UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->selection_clear_request_and_hud_reset != NULL)
        ops->selection_clear_request_and_hud_reset(ops->context);
}

static void ui_action_dispatch_global_low_byte_clear(
    UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->global_state_low_byte_clear_and_refresh != NULL)
        ops->global_state_low_byte_clear_and_refresh(ops->context);
    state->action_flags &= ~0xffu;
}

// FUNCTION: LEGORR 0x0041dca0
int ui_action_dispatch(int action)
{
    UiActionDispatchState* state = s_ui_action_dispatch_state;
    const UiActionDispatchOps* ops;
    void* entity = NULL;
    uint32_t flags;
    int type;
    if (state == NULL) return 0;
    ops = state->ops;
    if (ops != NULL && ops->current_selection_get != NULL)
        entity = ops->current_selection_get(ops->context);
    if (ops != NULL && ops->action_already_held != NULL &&
        ops->action_already_held(ops->context, action, 1)) return 0;

    switch (action) {
    case 0:
        if (state->current_mode > 0xf && state->current_mode < 0x12) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (state->current_mode == 0x12) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        break;
    case 1:
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context, 2, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 2: ui_action_dispatch_mode_request(state, 0x13, 0); return 1;
    case 3: ui_action_dispatch_mode_request(state, 0x14, 0); return 1;
    case 4: ui_action_dispatch_mode_request(state, 0x15, 0); return 1;
    case 5:
    case 7:
        if (ops != NULL && ops->selected_map_coordinate_validate != NULL)
            ops->selected_map_coordinate_validate(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 6:
        if (ops != NULL && ops->selected_cell_reveal_mask_clear != NULL)
            ops->selected_cell_reveal_mask_clear(
                ops->context, state->selected_cell_x, state->selected_cell_y,
                0x20000000u);
        if (ops != NULL && ops->terrain_cell_update != NULL)
            ops->terrain_cell_update(ops->context, state->selected_cell_x,
                                     state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 9:
        if (ops != NULL && ops->selected_map_action_enqueue != NULL)
            ops->selected_map_action_enqueue(ops->context, 0xc, 0,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xb:
        if (ops != NULL && ops->terrain_cell_command_02510 != NULL)
            ops->terrain_cell_command_02510(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xc:
        if (ops != NULL && ops->terrain_cell_command_03030 != NULL)
            ops->terrain_cell_command_03030(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xd:
        if (ops != NULL && ops->terrain_cell_command_02d50 != NULL)
            ops->terrain_cell_command_02d50(ops->context,
                state->selected_cell_x, state->selected_cell_y);
cell_action_finalize:
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xe:
        if (ops != NULL && ops->terrain_cell_command_02fb0 != NULL)
            ops->terrain_cell_command_02fb0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xf:
        if (ops != NULL && ops->terrain_cell_command_02ae0 != NULL)
            ops->terrain_cell_command_02ae0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_command_complete != NULL)
            ops->terrain_cell_command_complete(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x10:
        flags = ops != NULL && ops->selected_cell_flags_get != NULL
            ? ops->selected_cell_flags_get(ops->context,
                state->selected_cell_x, state->selected_cell_y) : 0u;
        if ((flags & 0x40000000u) == 0u) {
            if (ops != NULL &&
                ops->terrain_descriptor_tiles_reset_and_free != NULL)
                ops->terrain_descriptor_tiles_reset_and_free(ops->context,
                    state->selected_cell_x, state->selected_cell_y);
        }
        else if (ops != NULL && ops->selection_group_clear != NULL)
            ops->selection_group_clear(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x11:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 9, 0);
        return 1;
    case 0x12:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        flags = ops != NULL && ops->entity_flags_get != NULL
            ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
        if ((flags & 0x02000000u) == 0u &&
            (ops == NULL || ops->entity_byte_flags_get == NULL ||
             (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
              & 0x40u) == 0u))
            ui_action_dispatch_mode_request(state, 0xb, 0);
        else
            ui_action_dispatch_mode_request(state, 10, 0);
        return 1;
    case 0x13:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 0xd, 0);
        return 1;
    case 0x14:
        flags = ops != NULL && ops->entity_flags_get != NULL
            ? ops->entity_flags_get(ops->context, entity, 0x3ec) : 0u;
        if ((flags & 1u) != 0u) {
            if (ops != NULL && ops->selected_entity_action_02ec0 != NULL)
                ops->selected_entity_action_02ec0(ops->context, entity);
            return 1;
        }
        if ((state->action_flags & 0x10u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x15:
        if ((state->action_flags & 8u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x16:
    case 0x17:
        if (ops != NULL && ops->selected_entities_detach_or_pose != NULL)
            ops->selected_entities_detach_or_pose(ops->context);
        return 1;
    case 0x18:
        if (ops != NULL && ops->ui_action_helper_00452be0 != NULL)
            ops->ui_action_helper_00452be0(ops->context);
        if (ops != NULL && ops->entity_kind_command_issue != NULL)
            ops->entity_kind_command_issue(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x19:
    case 0x1a:
        if (ops != NULL && ops->terrain_cell_to_world_position != NULL)
            ops->terrain_cell_to_world_position(ops->context, entity,
                                               action == 0x19 ? 0 : 1);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x1b:
        if ((state->action_flags & 2u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1c:
        if ((state->action_flags & 4u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1d:
        flags = 0x40u;
        goto action_flag_toggle;
    case 0x1e:
        flags = 0x20u;
        goto action_flag_toggle;
    case 0x1f:
        if (ops != NULL && ops->selected_entity_action_1f != NULL)
            ops->selected_entity_action_1f(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x20:
    case 0x28:
    case 0x29:
    case 0x2a:
        if (ops != NULL && ops->selected_entity_action_20_group != NULL)
            ops->selected_entity_action_20_group(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x21:
        if (ops != NULL && ops->selected_entity_action_21 != NULL)
            ops->selected_entity_action_21(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x22:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 1);
        break;
    case 0x23:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 0);
        break;
    case 0x24:
        if (ops != NULL && ops->selected_entity_action_24 != NULL)
            ops->selected_entity_action_24(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x25:
        if (ops != NULL && ops->selected_entity_predicate != NULL) {
            if (ops->selected_entity_predicate(ops->context, entity, 0x39cf0)) {
                state->selected_entity_slots[0] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 0, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d10)) {
                state->selected_entity_slots[1] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 1, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d30)) {
                state->selected_entity_slots[2] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 2, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
        }
        break;
    case 0x26:
    case 0x27:
        if (ops != NULL && ops->entity_action_kind_create != NULL)
            ops->entity_action_kind_create(ops->context, entity,
                                           action == 0x26 ? 6 : 7);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x2c:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 0xb, NULL, 0, 0);
        type = ops != NULL && ops->entity_type_get != NULL
            ? ops->entity_type_get(ops->context, entity) : 0;
        if (type == 1) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        if (type == 2) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (type == 4) {
            ui_action_dispatch_mode_request(state, 0xd, 0);
            return 1;
        }
        break;
    case 0x2d:
    case 0x2e:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 9, NULL,
                                       action == 0x2d ? 0 : 1, 0);
        ui_action_dispatch_mode_request(state, 0xf, 0);
        return 1;
    case 0x2f:
        if (ops != NULL && ops->selected_entity_action_2f != NULL)
            ops->selected_entity_action_2f(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x30:
        ui_action_dispatch_mode_request(state, 0x10, 0);
        return 1;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
        if (ops != NULL && ops->selected_entity_action_31_group != NULL) {
            int callback_argument = action == 0x31 ? 0 :
                action == 0x32 ? 1 : action == 0x33 ? 2 :
                action == 0x34 ? 3 : action == 0x35 ? 4 :
                action == 0x36 ? 5 : action == 0x38 ? 6 : 7;
            ops->selected_entity_action_31_group(ops->context,
                                                 callback_argument);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x39:
        ui_action_dispatch_mode_request(state, 0x11, 0);
        return 1;
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
        flags = action == 0x3a ? 4u : action == 0x3b ? 0x10u :
                action == 0x3c ? 0x20u : action == 0x3d ? 1u :
                action == 0x3e ? 2u : 8u;
        if (ops == NULL || ops->entity_action_record_find == NULL)
            return 0;
        {
            void* action_record = ops->entity_action_record_find(
                ops->context, entity, 0, (int)flags);
            if (action_record == NULL) return 0;
            if (ops->entity_action_record_apply != NULL)
                ops->entity_action_record_apply(ops->context, action_record,
                                                (int)flags, 1);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x40:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x41:
        if (ops != NULL && ops->building_upgrade_commit != NULL)
            ops->building_upgrade_commit(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x42:
        ui_action_dispatch_mode_request(state, 0x12, 0);
        return 1;
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity,
                action == 0x43 ? 1 : action == 0x44 ? 0 :
                action == 0x45 ? 2 : 3);
        if (ops != NULL && ops->selected_entity_field_f6_clear != NULL)
            ops->selected_entity_field_f6_clear(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x47:
        if ((state->action_flags & 1u) == 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            state->action_flags |= 1u;
            state->cursor_flags &= ~1u;
            ui_action_dispatch_selection_clear(state);
            return 1;
        }
        ui_action_dispatch_global_low_byte_clear(state);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x48:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x49:
        if (ops != NULL && ops->cursor_action_rejected != NULL &&
            ops->cursor_action_rejected(ops->context, state->cursor_action_x,
                state->cursor_action_y, 1)) return 0;
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context,
                state->cursor_action_x, state->cursor_action_y);
        if (state->cursor_action_x == 4) return 1;
        break;
    default:
        break;
    }
    ui_action_dispatch_selection_clear(state);
    return 1;

action_flag_set:
    if (action == 0x14) flags = 0x10u;
    else if (action == 0x15) flags = 8u;
    else if (action == 0x1b) flags = 2u;
    else flags = 4u;
action_flag_toggle:
    ui_action_dispatch_global_low_byte_clear(state);
    state->action_flags |= flags;
    return 1;
}

int ledger_stub_0041dca0_UiActionDispatch(int action)
{
    return ui_action_dispatch(action);
}

void ledger_stub_0041eac0_EventTimerCountdownAdvance(float delta)
{
    event_timer_countdown_advance(delta);
}

// FUNCTION: LEGORR 0x0041ee80
int ledger_stub_0041ee80_CellGridPointHitTest(
    uint32_t x, uint32_t y, float region_x, float region_y,
    uint32_t* action_out, uint32_t* cell_x_out, uint32_t* cell_y_out)
{
    CellGridPointHitTestState* state = s_cell_grid_point_hit_test_state;
    float point_x = (float)x;
    const float point_y = (float)y;
    uint32_t index;

    if (state == NULL || action_out == NULL ||
        (state->flags & 0x800u) == 0u || state->record_count == 0u ||
        state->records == NULL) return 0;

    region_x -= 15.0f;
    if (region_x <= point_x && point_x <= region_x + 15.0f &&
        region_y <= point_y && point_y <= region_y + 40.0f) {
        *action_out = 0x14u;
        return 1;
    }

    for (index = 0u; index < state->record_count; ++index) {
        float width;
        float height;
        float left;
        const int rectangle = index == 0u ? 0 : 1;
        const CellGridPointHitRecord* record = &state->records[index];

        if (state->rectangles_present[rectangle]) {
            width = state->rectangle_width[rectangle];
            height = state->rectangle_height[rectangle];
        }
        else {
            width = 20.0f;
            height = 40.0f;
        }
        left = region_x - width;
        if (left <= point_x && point_x <= left + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = 0x14u;
            return 1;
        }

        width = 40.0f;
        height = 40.0f;
        if (state->record_size_get != NULL &&
            !state->record_size_get(state->context, record->x,
                                    record->y, &width, &height)) {
            width = 40.0f;
            height = 40.0f;
        }
        region_x = left - width;
        if (region_x <= point_x && point_x <= region_x + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = record->action;
            if (cell_x_out != NULL) *cell_x_out = record->x;
            if (cell_y_out != NULL) *cell_y_out = record->y;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041fa80
uint32_t ledger_stub_0041fa80_StartLevelPilots(
    GameContext* ctx, EntitySlot** type_four_entities, uint32_t capacity,
    int* pilot_type, int* pilot_row)
{
    int resolved_type, resolved_row;
    int i, total;
    uint32_t count = 0;
    if (ctx == NULL || (type_four_entities == NULL && capacity != 0u)) return 0;
    total = entity_count(ctx, 4);
    for (i = 0; i < total && count < capacity; ++i) {
        const EntitySlot* entity = entity_at(ctx, 4, i);
        if (entity != NULL) type_four_entities[count++] = (EntitySlot*)entity;
    }
    if (!entity_type_row_for_name("Pilot", &resolved_type,
                                                  &resolved_row)) return 0;
    if (pilot_type != NULL) *pilot_type = resolved_type;
    if (pilot_row != NULL) *pilot_row = resolved_row;
    selected_level_start_object_update(ctx);
    return count;
}

// SelectedEntityGroundMarkerRender
int ledger_stub_004248b0_SelectedEntityGroundMarkerRender(
    void* entity, void* camera, float red, float green, float blue)
{
    return selected_entity_ground_marker_render(entity, camera, red, green,
                                                blue);
}

// FUNCTION: LEGORR 0x00425c30
void ledger_stub_00425c30_TransitionControl(int enabled, uint32_t parameter)
{
    GameContext* context = entity_pool_callback_context_get();
    uint32_t* feature_word = context != NULL ? &context->cheat
                                             : s_ui_transition_feature_word;
    uint32_t engine_flags;

    if (feature_word == NULL || (*feature_word & 2u) != 0u) return;
    engine_flags = global_state_low_word_get();
    if (enabled != 0)
        parameter = (~engine_flags >> 20) & 1u;

    if (parameter != 0u) {
        if ((engine_flags & 0x100000u) == 0u) {
            sound_buffer_association_stop_all();
            sound_stream_stop_and_release(0);
            sound_stream_stop_and_release(1);
        }
        global_state_low_word_set(engine_flags | 0x100000u);
        *feature_word |= 0x1000u;
        return;
    }

    periodic_entity_callback_dispatch(NULL);
    sound_alias_play(7, 1);
    global_state_low_word_set(engine_flags & ~0x100000u);
    *feature_word &= ~0x1000u;
}

/* Adapter only: GlobalRenderRecordSetFromInput (FUN_00425d50) takes one
 * input entity and operates on DAT_005560e8; this port seam takes explicit
 * render-state dependencies and is not the corpus ABI. */
void ledger_stub_00425d50_GlobalRenderRecordSetFromInput(
    void* record, void* view, const GlobalRenderRecordHooks* hooks)
{
    global_render_record_set_from_input(record, view, hooks);
}

/* Adapter only: UpdateRenderViewportState (FUN_00426250) is a zero-argument
 * global-state routine. The explicit state/hooks API is not its ABI. */
void ledger_stub_00426250_UpdateRenderViewportState(
    RenderViewportState* state, const RenderViewportHooks* hooks)
{
    render_viewport_state_update(state, hooks);
}

// FUNCTION: LEGORR 0x004263c0
void ledger_stub_004263c0_GlobalSubsystemStateReset(void)
{
    static const int mode = 1;
    GlobalSubsystemResetState* state = s_global_subsystem_reset_state;
    if (state == NULL) return;
    (void)engine_mode_set(state->engine, &mode);
    if (state->word_00503618 != NULL) *state->word_00503618 = 0u;
    text_buffer_reset(state->text_buffer);
}

/* CursorRayToCameraRoute (FUN_00426420) performs camera ray and pan updates;
 * this call only resets the view record's +0x9c cursor-ray accumulator
 * (FUN_00435f16), so it is deliberately left unmapped here. */
void ledger_stub_00426420_CursorRayToCameraRoute(ViewContext* view)
{
    cursor_screen_ray_context_reset(view);
}

// EntityCommandStatusFeedbackDispatch
void ledger_stub_00428020_EntityCommandStatusFeedbackDispatch(EntitySlot* entity)
{
    entity_command_status_feedback_dispatch(entity);
}

// TerrainCommandFeedbackDispatch
// FUNCTION: LEGORR 0x00428330
int ledger_stub_00428330_TerrainCommandFeedbackDispatch(
    TerrainCommandFeedbackState* state, int details_requested,
    int unused_parameter, int parameter_four,
    const TerrainCommandFeedbackHooks* hooks)
{
    (void)unused_parameter;
    return terrain_command_feedback_dispatch(state, details_requested,
                                             parameter_four, hooks);
}

/* Adapter only: LevelDebugInputCellMutatorDispatch (FUN_004288e0) consumes
 * global key transitions and performs several commands; this material setter
 * is not an ABI-compatible implementation. */
void ledger_stub_004288e0_LevelDebugInputCellMutatorDispatch(
    TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, uint8_t material,
    uint8_t rotation)
{
    terrain_cell_material_set(grid, x, y, material, rotation);
}

/* Adapter only: SelectionRectangleRender (FUN_004293b0) is zero-argument
 * global input/view state and draws through the active render interface.
 * This explicit state/hooks API is not the retail ABI. */
void ledger_stub_004293b0_SelectionRectangleRender(
    const SelectionRectangleRenderState* state,
    const SelectionRectangleRenderHooks* hooks)
{
    selection_rectangle_render(state, hooks);
}

/* Adapter only: CameraPointerRayRoute (FUN_00429470) takes screen x/y and
 * two output pointers, using the active global camera. The explicit hooks
 * argument below has no retail counterpart or production installer. */
int ledger_stub_00429470_CameraPointerRayRoute(
    const CameraPointerRayHooks* hooks, float x, float y,
    float* out_x, float* out_y)
{
    return camera_pointer_ray_route(hooks, x, y, out_x, out_y);
}

// LoadTileMeshes
// FUNCTION: LEGORR 0x0042b330
int ledger_stub_0042b330_LoadTileMeshes(TerrainTileMeshGrid* grid,
                                        const char* bitmap_prefix)
{
    if (grid == NULL) return 0;
    grid->bitmap_prefix = bitmap_prefix;
    return terrain_tile_mesh_grid_load(grid, grid->editor_mode);
}

// TextureSetConfigLoad
static const char* texture_set_config_value(BlockRec* group,
                                            const char* key)
{
    BlockRec* entry;
    if (group == NULL || key == NULL) return NULL;
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        if (name != NULL && strcmp(name, key) == 0)
            return block_value(entry);
    }
    return NULL;
}
// FUNCTION: LEGORR 0x0042bb60
int ledger_stub_0042bb60_TextureSetConfigLoad(TerrainTileMeshGrid* grid,
                                              BlockFile* config,
                                              const char* group_name)
{
    BlockRec* group;
    const char* width_text;
    const char* height_text;
    const char* mesh_name;
    int width;
    int height;
    if (grid == NULL || config == NULL || group_name == NULL) return 0;
    group = block_subgroup(config, group_name);
    if (group == NULL) return 0;
    width_text = texture_set_config_value(group, "surftextwidth");
    height_text = texture_set_config_value(group, "surftextheight");
    mesh_name = texture_set_config_value(group, "meshbasename");
    if (width_text == NULL || height_text == NULL || mesh_name == NULL)
        return 0;
    width = atoi(width_text);
    height = atoi(height_text);
    if (width <= 0 || height <= 0) return 0;
    grid->width = (uint32_t)width;
    grid->height = (uint32_t)height;
    grid->bitmap_prefix = mesh_name;
    if (!terrain_tile_mesh_grid_load(grid, grid->editor_mode)) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x0042c330
int ledger_stub_0042c330_EmergeMapTriggerSpawn(
    LevelState* level, const int cell_xy[2], int output_xy[2])
{
    unsigned char* records;
    uint32_t record_count;
    uint32_t owner_index;
    GameContext* context;
    const EntitySlot* base_entity;
    uint32_t record_index;
    if (level == NULL || cell_xy == NULL || level->ptr_layerl == NULL)
        return 0;
    records = (unsigned char*)level->ptr_layerl;
    record_count = level->word[LEVEL_OFF_LAYERL_COUNT / 4];
    owner_index = level->word[0xb0 / 4];
    context = entity_pool_callback_context_get();
    if (context == NULL || context->level != level) return 0;
    base_entity = entity_at(context, 3, (int)owner_index);
    if (base_entity == NULL) return 0;
    for (record_index = 0u; record_index < record_count; ++record_index) {
        unsigned char* record = records + record_index * 0x4cu;
        uint32_t slot_index;
        int record_x, record_y;
        float timer;
        memcpy(&record_x, record + 4, sizeof(record_x));
        memcpy(&record_y, record + 8, sizeof(record_y));
        memcpy(&timer, record + 0x0c, sizeof(timer));
        if (record_x != cell_xy[0] || record_y != cell_xy[1] ||
            timer > 0.0f) continue;
        for (slot_index = 0u; slot_index < 5u; ++slot_index) {
            unsigned char* entry = record + 0x18u + slot_index * 0x0cu;
            int spawn_x, spawn_y;
            uint32_t populated;
            EntitySlot* spawned;
            memcpy(&populated, entry, sizeof(populated));
            if (populated == 0u) continue;
            memcpy(&spawn_x, entry - 8, sizeof(spawn_x));
            memcpy(&spawn_y, entry - 4, sizeof(spawn_y));
            spawned = creature_spawn_from_emergence(
                (void*)base_entity, 3, (int)owner_index, spawn_x, spawn_y);
            if (spawned == NULL) continue;
            memcpy(record + 0x0c, &level->word[0x264 / 4], sizeof(float));
            if (output_xy != NULL) {
                output_xy[0] = spawn_x;
                output_xy[1] = spawn_y;
            }
            return 1;
        }
    }
    return 0;
}

// Port-facing adapter retained for callers that provide a table and hooks.
int ledger_stub_0042cf50_MiniFigureTypeTableLoad(
    TypeConfigTable* table, void* section, const TypeConfigHooks* hooks)
{
    /* FUN_0042cf50 uses 0x74-byte rows and its shared type-row parser. */
    return type_config_record_table_load(table, section, 1u, 0x74u, hooks);
}

// Port-facing adapter retained for callers that provide a table and hooks.
int ledger_stub_0042d2b0_BuildingTypesLoad(
    TypeConfigTable* table, void* section, const TypeConfigHooks* hooks)
{
    /* FUN_0042d2b0 uses 0x14c-byte building records. */
    return type_config_record_table_load(table, section, 2u, 0x14cu, hooks);
}

// Port-facing adapter retained for callers that provide table and hooks.
void ledger_stub_0042d600_ObjectNamesLoad(
    ObjectNameTableSet* tables, void* section,
    const ObjectNameLoadHooks* hooks)
{
    /* The five input name tables and counts are the vehicle, mini-figure,
     * monster, building, and upgrade tables consumed by the decompile. */
    object_names_load(tables, section, hooks);
}

// Port-facing adapter retained for callers that provide table and hooks.
void ledger_stub_0042da20_ObjectTheNamesLoad(
    ObjectNameTableSet* tables, BlockFile* config,
    const ObjectNameLoadHooks* hooks)
{
    BlockRec* section;
    if (tables == NULL || config == NULL || hooks == NULL) return;
    /* Unlike ObjectNamesLoad, this loader first opens the named config
     * subgroup and uses its own parallel table and handle globals. */
    section = block_subgroup(config, "ObjectTheNames");
    if (section == NULL) return;
    object_names_load(tables, section, hooks);
}

/* Adapter only: ViewportTargetPositionClamp (FUN_0042de40) takes a manager,
 * coordinate pair, and target selector, then updates global viewport state.
 * The state/position clamp seam below does not implement that route. */
int ledger_stub_0042de40_ViewportTargetPositionClamp(
    ViewportTargetClampState* state, const int position[2])
{
    return viewport_target_position_clamp(state, position);
}


/* Port adapter for TerrainStampImpactHandler (FUN_00431780). Its hook-bundle
 * argument is a reconstruction seam, not part of the corpus ABI; production
 * terrain/effect bindings are not installed yet. */
int ledger_stub_00431780_TerrainStampImpactHandler(
    uint32_t x, uint32_t y, int direction, int mode,
    const TerrainStampImpactHooks* hooks)
{
    return terrain_stamp_impact_handle(x, y, direction, mode, hooks);
}

// PlacementBlockedAdjacentCellScan
typedef int (*PlacementCellBlockedHook)(void* context, int x, int y);
// FUNCTION: LEGORR 0x00431b20
uint32_t ledger_stub_00431b20_PlacementBlockedAdjacentCellScan(
    const int cell_xy[2], PlacementCellBlockedHook blocked, void* context,
    int* blocked_xy, uint32_t capacity)
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    uint32_t count = 0u, i;
    if (cell_xy == NULL || blocked == NULL) return 0u;
    for (i = 0u; i < 4u; ++i) {
        int x = cell_xy[0] + offset[i][0];
        int y = cell_xy[1] + offset[i][1];
        if (!blocked(context, x, y)) continue;
        if (blocked_xy != NULL && count < capacity) {
            blocked_xy[count * 2u] = x;
            blocked_xy[count * 2u + 1u] = y;
        }
        ++count;
    }
    return count;
}

/* Port adapter for TerrainCellMutationProgress (FUN_00433380). The copied
 * grid and hook bundle model the observed cell/height updates, but do not
 * match the corpus's global-state ABI and have no production installer. */
int ledger_stub_00433380_TerrainCellMutationProgress(
    const TerrainCellMutationGrid* grid, uint32_t x, uint32_t y,
    const TerrainCellMutationHooks* hooks)
{
    return terrain_cell_mutation_progress(grid, x, y, hooks);
}

/* Adapter only: RenderQualityConfigApply (FUN_004334f0) has no explicit
 * arguments; it reads the global config and render device state. This port
 * seam accepts parsed settings and does not provide those global owners. */
int ledger_stub_004334f0_RenderQualityConfigApply(
    const RenderQualitySettings* settings)
{
    return render_quality_config_apply(settings);
}

// EntityNeighborBlockedCellStepProbe
typedef int (*EntityNeighborStepProbeHook)(void* context, EntitySlot* entity,
                                           int x, int y, int direction);
// FUNCTION: LEGORR 0x00433c10
int ledger_stub_00433c10_EntityNeighborBlockedCellStepProbe(
    EntitySlot* entity, const int cell_xy[2],
    EntityNeighborStepProbeHook probe, void* context, int output_xy[2])
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    int i;
    if (entity == NULL || cell_xy == NULL || probe == NULL) return -1;
    for (i = 0; i < 4; ++i) {
        int x = cell_xy[0] + offset[i][0];
        int y = cell_xy[1] + offset[i][1];
        if (probe(context, entity, x, y, i)) {
            if (output_xy != NULL) { output_xy[0] = x; output_xy[1] = y; }
            return i;
        }
    }
    return -1;
}

/* Adapter only: TerrainProjectedCellStateRefresh (FUN_00433e80) is a
 * zero-argument routine over global view and terrain state. The explicit
 * state/hooks model below has a different ABI and incomplete render owners. */
uint32_t ledger_stub_00433e80_TerrainProjectedCellStateRefresh(
    const TerrainProjectedCellRefreshState* state,
    const TerrainProjectedCellRefreshHooks* hooks)
{
    return terrain_projected_cell_state_refresh(state, hooks);
}

/* Adapter only: CellSurfaceUpdateDispatch (FUN_00434480) takes the live
 * terrain grid, cell coordinates, and two render parameters. The extra hook
 * bundle and cell-record pointer here are not the retail ABI. */
int ledger_stub_00434480_CellSurfaceUpdateDispatch(
    const uint8_t* cell, int x, int y, void* param_a, void* param_b,
    const TerrainCellSurfaceUpdateHooks* hooks)
{
    return terrain_cell_surface_update_dispatch(cell, x, y, param_a,
                                                param_b, hooks);
}

// PanelsConfigEntryApply
typedef int (*PanelsConfigApplyHook)(void* context, const char* key,
                                     const char* value);
// FUNCTION: LEGORR 0x004345f0
uint32_t ledger_stub_004345f0_PanelsConfigEntryApply(
    BlockRec* section, PanelsConfigApplyHook apply, void* context)
{
    BlockRec* entry;
    uint32_t applied = 0u;
    if (section == NULL || apply == NULL) return 0u;
    for (entry = block_first_child(section); entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        if (name != NULL && value != NULL && apply(context, name, value))
            ++applied;
    }
    return applied;
}

// PanelButtonLayoutConfigLoad
typedef int (*PanelButtonLayoutApplyHook)(void* context, const char* name,
                                          const char* value);
// FUNCTION: LEGORR 0x00434710
uint32_t ledger_stub_00434710_PanelButtonLayoutConfigLoad(
    BlockFile* config, const char* group_name,
    PanelButtonLayoutApplyHook apply, void* context)
{
    BlockRec* group;
    if (config == NULL || group_name == NULL || apply == NULL) return 0u;
    group = block_subgroup(config, group_name);
    if (group == NULL) return 0u;
    return ledger_stub_004345f0_PanelsConfigEntryApply(group, apply, context);
}

// SoundSamplesConfigLoad
typedef void* (*SoundSampleConfigResolveHook)(const char* name,
                                              void* context);
typedef void (*SoundSampleConfigRegisterHook)(const char* alias,
                                               void* sample,
                                               void* context);
typedef void (*SoundSampleConfigModeHook)(int enabled, void* context);
typedef struct SoundSampleConfigHooks {
    SoundSampleConfigResolveHook resolve;
    SoundSampleConfigRegisterHook register_sample;
    SoundSampleConfigModeHook mode_set;
    void* context;
} SoundSampleConfigHooks;
static SoundSampleConfigHooks s_sound_sample_config_hooks;
void sound_sample_config_hooks_set(const SoundSampleConfigHooks* hooks)
{
    if (hooks == NULL) memset(&s_sound_sample_config_hooks, 0,
                              sizeof(s_sound_sample_config_hooks));
    else s_sound_sample_config_hooks = *hooks;
}
// FUNCTION: LEGORR 0x00434a50
void ledger_stub_00434a50_SoundSamplesConfigLoad(BlockFile* config,
                                                  int allow_marked)
{
    BlockRec* group;
    BlockRec* entry;
    if (config == NULL) return;
    group = block_subgroup(config, "Samples");
    if (group == NULL) return;
    if (s_sound_sample_config_hooks.mode_set != NULL)
        s_sound_sample_config_hooks.mode_set(1,
            s_sound_sample_config_hooks.context);
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* value = block_value(entry);
        const char* sample_name = value;
        void* sample;
        if (sample_name == NULL) continue;
        if (sample_name[0] == '!' && allow_marked) ++sample_name;
        else if (sample_name[0] == '!' && !allow_marked) continue;
        if (s_sound_sample_config_hooks.resolve == NULL ||
            s_sound_sample_config_hooks.register_sample == NULL) continue;
        sample = s_sound_sample_config_hooks.resolve(sample_name,
            s_sound_sample_config_hooks.context);
        if (sample != NULL)
            s_sound_sample_config_hooks.register_sample(block_name(entry),
                sample, s_sound_sample_config_hooks.context);
    }
    if (s_sound_sample_config_hooks.mode_set != NULL)
        s_sound_sample_config_hooks.mode_set(0,
            s_sound_sample_config_hooks.context);
}

// TextMessagesConfigLoad
// FUNCTION: LEGORR 0x00434af0
void ledger_stub_00434af0_TextMessagesConfigLoad(BlockFile* config)
{
    BlockRec* group;
    BlockRec* entry;
    if (config == NULL) return;
    group = block_subgroup(config, "TextMessages");
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* text = block_value(entry);
        if (ui_message_label_index_find(label, &slot))
            ui_message_text_slot_assign(slot, text != NULL ? text : "");
    }
    group = block_subgroup(config, "TextMessagesWithImages");
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* value = block_value(entry);
        char copy[512];
        char* fields[3];
        unsigned int count = 1u;
        unsigned int i;
        if (!ui_message_label_index_find(label, &slot) || value == NULL)
            continue;
        strncpy(copy, value, sizeof(copy) - 1u);
        copy[sizeof(copy) - 1u] = '\0';
        fields[0] = copy;
        fields[1] = NULL;
        fields[2] = NULL;
        for (i = 0; copy[i] != '\0' && count < 3u; ++i) {
            if (copy[i] == ',') {
                copy[i] = '\0';
                fields[count++] = copy + i + 1;
            }
        }
        ui_message_text_slot_assign_with_image(slot, fields[0], fields[1],
                                                fields[2]);
    }
}

void ledger_stub_00434da0_AdvisorTooltipAndTooltipSoundConfigLoad(
    BlockFile* config)
{
    advisor_tooltip_and_sound_config_load(config);
}

/* Adapter only: SurfaceTypeDescriptionsConfigLoad (FUN_004350a0) takes the
 * config and section owners, then writes the original global tables. The
 * caller-supplied entries/hooks seam does not reproduce those owners. */
void ledger_stub_004350a0_SurfaceTypeDescriptionsConfigLoad(
    SurfaceTypeDescriptionEntry entries[18],
    const SurfaceTypeDescriptionHooks* hooks)
{
    surface_type_descriptions_load(entries, hooks);
}

// FUNCTION: LEGORR 0x00435550
int ledger_stub_00435550_GlobalStateZeroTransitionDetect(void)
{
    if (global_state_low_word_get() != 0u) return 0;
    global_state_low_byte_clear_and_refresh();
    return 1;
}

// ToolTipIconsConfigLoad
void ledger_stub_00437150_ToolTipIconsConfigLoad(void* config)
{
    tooltip_icons_config_load(config);
}

// ProcessQueuedGridAnimations
// FUNCTION: LEGORR 0x00437300
void ledger_stub_00437300_ProcessQueuedGridAnimations(
    void* anchor, int mode, float delta)
{
    process_queued_grid_animations(anchor, mode, delta);
}

// EntityCallbackDispatchUnlessTypeFive
// FUNCTION: LEGORR 0x004374f0
void ledger_stub_004374f0_EntityCallbackDispatchUnlessTypeFive(
    EntitySlot* entity)
{
    if (entity->type != 5)
        entity_callback_dispatch_unless_type_five(entity);
}

// EntityTeardown
int ledger_stub_00437540_EntityTeardown(EntitySlot* entity)
{
    return entity_teardown(entity);
}

// GridObjectRecordsRelease
void ledger_stub_004378e0_GridObjectRecordsRelease()
{
    grid_object_records_release();
}

// DeferredOListEntityCreateAndLink
void ledger_stub_00437940_DeferredOListEntityCreateAndLink(
    const int cell_xy[2])
{
    deferred_olist_entity_create_and_link(cell_xy);
}

// RoverWorkQueueUpdate
void ledger_stub_004383b0_RoverWorkQueueUpdate(EntitySlot* entity, int mode)
{
    rover_work_queue_update(entity, mode);
}

// EntityKindRecordUpdateDispatch
void ledger_stub_00438460_EntityKindRecordUpdateDispatch(EntitySlot* entity)
{
    entity_kind_record_update_dispatch(entity);
}

// EntityCreateWithPosition
// FUNCTION: LEGORR 0x00438610
EntitySlot* ledger_stub_00438610_EntityCreateWithPosition(
    GameContext* context, int type, const char* name, int value,
    float x, float y, float z)
{
    return entity_create_with_position(context, type, name, value, x, y, z);
}

// BuildingUpgradeResourceAdmissionCheck
int ledger_stub_00438770_BuildingUpgradeResourceAdmissionCheck(
    EntitySlot* building, uint32_t* out_secondary_cost)
{
    return building_upgrade_resource_admission_check(
        building, out_secondary_cost);
}

// BuildingUpgradeCommit
void ledger_stub_004387f0_BuildingUpgradeCommit(EntitySlot* building)
{
    building_upgrade_commit(building);
}

// EntityActionKindPrerequisiteCheck
int ledger_stub_004388b0_EntityActionKindPrerequisiteCheck(
    EntitySlot* entity, int action)
{
    return entity_action_kind_prerequisite_check(entity, action);
}

// EntityRouteMarkerCandidateFind
EntitySlot* ledger_stub_004389e0_EntityRouteMarkerCandidateFind(
    EntitySlot* source, int include_secondary)
{
    return entity_route_marker_candidate_find(source, include_secondary);
}

static void spatial_query_request_init(SpatialObjectQueryRequest* request,
                                       uint32_t mask)
{
    memset(request, 0, sizeof(*request));
    request->radius = 10000.0f;
    request->mask = mask;
}

static void spatial_query_center_from_entity(
    SpatialObjectQueryRequest* request, const EntitySlot* entity)
{
    if (entity != NULL) {
        entity_transform_position_get(entity, request->center);
        request->center_is_cell = 0;
    }
}

static void spatial_query_center_from_cell(
    SpatialObjectQueryRequest* request, const int cell_xy[2])
{
    float world_position[3];
    if (request == NULL || cell_xy == NULL) return;
    /* The reference converts the cell to its world-space center. A failed
     * conversion leaves the initialized center at (0, 0), but still runs the
     * entity-pool query. */
    if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                        world_position)) {
        request->center[0] = world_position[0];
        request->center[1] = world_position[1];
    }
    request->center_is_cell = 0;
}

// FUNCTION: LEGORR 0x00438a60
void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int cell_xy[2], uint32_t filter_a, uint32_t filter_b,
    uint32_t filter_c)
{
    SpatialObjectQueryRequest request;
    if (cell_xy == NULL) return NULL;
    spatial_query_request_init(&request, 1u);
    spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    request.filters[1] = filter_b;
    request.filters[2] = filter_c;
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438ae0
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 2u);
    if (entity != NULL) spatial_query_center_from_entity(&request, entity);
    else spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    request.filters[1] = filter_b;
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438b80
void* ledger_stub_00438b80_SpatialObjectQueryCenterMaskFromArgument(
    const EntitySlot* entity, uint32_t mask)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, mask);
    spatial_query_center_from_entity(&request, entity);
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438bf0
void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x80u);
    spatial_query_center_from_entity(&request, entity);
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438c60
void* ledger_stub_00438c60_EntityCandidateSearch(const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 4u);
    spatial_query_center_from_entity(&request, entity);
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438cd0
void* ledger_stub_00438cd0_SpatialObjectQueryCenterMask20(
    const float center[2])
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x20u);
    if (center == NULL) return NULL;
    request.center[0] = center[0];
    request.center[1] = center[1];
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438d30
void* ledger_stub_00438d30_SpatialObjectQueryCenterMask10(
    const float center[2])
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x10u);
    if (center == NULL) return NULL;
    request.center[0] = center[0];
    request.center[1] = center[1];
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438d90
void* ledger_stub_00438d90_SpatialObjectQueryCenterMask200(
    const float center[2])
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x200u);
    if (center == NULL) return NULL;
    request.center[0] = center[0];
    request.center[1] = center[1];
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438df0
void* ledger_stub_00438df0_EntityLookupAtWorldPosition(
    const float center[2])
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 8u);
    if (center == NULL) return NULL;
    request.center[0] = center[0];
    request.center[1] = center[1];
    return spatial_object_query_execute(&request);
}

// FUNCTION: LEGORR 0x00438e50
void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x100u);
    if (center != NULL) {
        request.center[0] = center[0];
        request.center[1] = center[1];
    }
    spatial_query_center_from_entity(&request, entity);
    request.filters[0] = extra;
    return spatial_object_query_execute(&request);
}

// EntityBuildPlacementCommandBatchExpand
void ledger_stub_00439280_EntityBuildPlacementCommandBatchExpand(
    const int cell_xy[2])
{
    entity_build_placement_command_batch_expand(cell_xy);
}

// EntityBuildPlacementCommandPairCreate
void ledger_stub_00439340_EntityBuildPlacementCommandPairCreate(
    const int cell_xy[2], int entity_id)
{
    entity_build_placement_command_pair_create(cell_xy, entity_id);
}

// EntityBuildPlacementCommandSetup
// FUNCTION: LEGORR 0x00439370
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    (void)entity_build_placement_command_setup(cell_xy, entity_id, world_xy,
                                               update_state, 0);
}

// EntityBuildPlacementAlternateCommandSetup
// FUNCTION: LEGORR 0x004394b0
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    (void)entity_build_placement_command_setup(cell_xy, entity_id, world_xy,
                                               update_state, 1);
}

// ToolNameStringResourcesLoad
void ledger_stub_00439950_ToolNameStringResourcesLoad(void* string_loader,
                                                     void* config)
{
    entity_tool_name_strings_load(string_loader, config);
}

// GatedStepCommandIssueWithTargetRecord
int ledger_stub_00439a20_GatedStepCommandIssueWithTargetRecord(
    EntitySlot* entity, EntitySlot* target)
{
    return entity_gated_step_command_issue(entity, target);
}

// TaskTargetCellEligibilityCheck
int ledger_stub_00439b80_TaskTargetCellEligibilityCheck(
    const EntitySlot* entity, const uint32_t* task_record)
{
    return entity_task_target_cell_eligibility_check(entity, task_record);
}

// EntityAcceptsTargetForTaskCheck
int ledger_stub_00439bd0_EntityAcceptsTargetForTaskCheck(
    const EntitySlot* entity, const EntitySlot* target, int apply_extra_gate)
{
    return entity_accepts_target_for_task_check(entity, target,
                                                apply_extra_gate);
}

// EntityIndexedFlagSetAndRefresh
void ledger_stub_00439c80_EntityIndexedFlagSetAndRefresh(EntitySlot* entity)
{
    entity_indexed_flag_set_and_refresh(entity);
}

// VehicleUnloadAdjacentCellFind
int ledger_stub_00439d50_VehicleUnloadAdjacentCellFind(
    const EntitySlot* entity, int out_xy[2])
{
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

// VehicleUnloadCellFindIfEligible
int ledger_stub_00439e10_VehicleUnloadCellFindIfEligible(
    const EntitySlot* entity, int out_xy[2])
{
    return entity_vehicle_unload_cell_find_if_eligible(entity, out_xy);
}

/* EntityAttachedObjectListDetach (FUN_00439e70): detach the carried-object
 * slot and clear its owner links. The port has one explicit carried object
 * instead of the corpus's variable-length attachment array. */
// FUNCTION: LEGORR 0x00439e70
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode)
{
    EntitySlot* carried;
    uint32_t state_flags;
    if (entity == NULL) return;
    state_flags = entity->flags_3e0;
    if ((state_flags & 0x400u) == 0u ||
        ((state_flags & 0x2000u) != 0u && reset_mode == 0)) return;
    carried = (EntitySlot*)entity->carry_link_308;
    if (reset_mode != 0 &&
        (entity_stats_word_get(entity) & 8u) == 0u) {
        (void)cmd_chain_drop(entity, 6);
        (void)cmd_chain_drop(entity, 9);
        (void)cmd_chain_drop(entity, 15);
        (void)cmd_chain_drop(entity, 20);
    }
    if (carried != NULL) {
        entity->carry_link_308 = NULL;
        entity->carry_state_324 = 0;
        carried->owner_36c = NULL;
        carried->link_2fc = 0u;
        if (reset_mode == 0) {
            if ((state_flags & 0x200u) == 0u) {
                (void)cmd_chain_drop(entity, 5);
                (void)cmd_chain_drop(entity, 6);
                (void)cmd_chain_drop(entity, 9);
                (void)cmd_chain_drop(entity, 15);
                (void)cmd_chain_drop(entity, 7);
                (void)cmd_chain_drop(entity, 4);
                (void)cmd_chain_drop(entity, 20);
            } else {
                (void)cmd_chain_drop(entity, 3);
            }
        } else {
            (void)cmd_chain_drop(entity, 5);
            if (carried->type != 7 && carried->type != 6)
                carried->flags_3ec |= 0x80000u;
        }
    }
    entity->carry_state_324 = 0;
    entity->flags_3e0 &= ~0x400u;
}

// EntityDepositActivityGate
int ledger_stub_0043a300_EntityDepositActivityGate(EntitySlot* entity,
                                                   EntitySlot* target)
{
    return entity_deposit_activity_gate(entity, target);
}

// EntityVisualResourceGetOrCreate
void ledger_stub_0043a5f0_EntityVisualResourceGetOrCreate(EntitySlot* entity)
{
    entity_visual_resource_get_or_create(entity);
}

// ActionKindEntityCreateAndCommandIssue
EntitySlot* ledger_stub_0043a650_ActionKindEntityCreateAndCommandIssue(
    EntitySlot* source, int kind, int selector, int argument)
{
    return entity_action_kind_entity_create_and_command_issue(
        source, kind, selector, argument);
}

// EntityActionRecordConsumeAndCleanup
void ledger_stub_0043a850_EntityActionRecordConsumeAndCleanup(
    EntitySlot* entity, EntityActionConsumeRecord* record)
{
    entity_action_record_consume_and_cleanup(entity, record);
}

// EntityPairInteractionGate
void ledger_stub_0043a9f0_EntityPairInteractionGate(
    EntitySlot* entity, const EntitySlot* partner)
{
    entity_pair_interaction_gate(entity, partner);
}

// ProximityActivityRaise
void ledger_stub_0043aab0_ProximityActivityRaise(EntitySlot* entity)
{
    entity_proximity_activity_raise(entity);
}

// TerrainCellVariantUpdateQueue
void ledger_stub_0043abf0_TerrainCellVariantUpdateQueue(void* entity)
{
    terrain_cell_variant_update_queue(entity);
}

// TaskCountAtGridCoordinateQuery
void ledger_stub_0043b360_TaskCountAtGridCoordinateQuery(
    uint32_t x, uint32_t y, uint32_t* out_count)
{
    entity_task_count_at_grid_coordinate_query(x, y, out_count);
}

// ClippedRectangleCallbackDispatch
void ledger_stub_0043b6c0_ClippedRectangleCallbackDispatch(
    void* payload, const float rect_a[2], const float rect_b[2])
{
    entity_clipped_rectangle_callback_dispatch(payload, rect_a, rect_b);
}

// TypeTwoEntityEventDispatch
void ledger_stub_0043b820_TypeTwoEntityEventDispatch(
    const EntitySlot* entity, uint32_t argument_a, uint32_t argument_b)
{
    entity_type_two_event_dispatch(entity, argument_a, argument_b);
}

// TypeTwoEntityPositionedCallbackDispatch
void ledger_stub_0043b850_TypeTwoEntityPositionedCallbackDispatch(
    const EntitySlot* entity, void* scene_object, uint32_t argument_a,
    uint32_t argument_b, int mode)
{
    entity_type_two_positioned_callback_dispatch(
        entity, scene_object, argument_a, argument_b, mode);
}

// BuildingDepositStateMonitor
void ledger_stub_0043bb20_BuildingDepositStateMonitor(EntitySlot* entity)
{
    entity_building_deposit_state_monitor(entity);
}

// ObjectiveSidecarAppendGate
int ledger_stub_0043bc40_ObjectiveSidecarAppendGate(
    ObjectiveSidecarSnapshot* state, void* context,
    ObjectiveSidecarActionHook action)
{
    return objective_sidecar_append_gate(state, context, action);
}

// EntityType26CommandCompletionGate
int ledger_stub_0043c3e0_EntityType26CommandCompletionGate(
    const EntitySlot* entity)
{
    return entity_type26_command_completion_gate(entity);
}

// KindFourPostCreateResolve
void ledger_stub_0043c570_KindFourPostCreateResolve(EntitySlot* entity)
{
    entity_kind_four_post_create_resolve(entity);
}

/* EntityCapacityOrStatePredicate (FUN_0043c650): gate on stats bit 0x80000,
 * compare the indexed stats-table +0xe0 value against the referenced count,
 * then reject an entity carrying the +0x3e8 high-bit state. */
// FUNCTION: LEGORR 0x0043c650
int entity_capacity_or_state_predicate(const EntitySlot* entity)
{
    uint32_t stats_flags;
    uint32_t limit;
    uint32_t current = 0u;
    if (entity == NULL) return 0;
    stats_flags = entity_stats_word_get(entity);
    if ((stats_flags & 0x80000u) == 0u) return 0;
    limit = stats_instance_word_at_e0(entity);
    if (entity->carry_ref_348 != NULL)
        memcpy(&current, entity->carry_ref_348, sizeof(current));
    if (limit != 0u && entity->carry_ref_348 != NULL && limit <= current)
        return 0;
    return (entity->flags_3e8 & 0x80000000u) == 0u ? 1 : 0;
}

// EntityEnergyLowTargetRouteCommandTick
// FUNCTION: LEGORR 0x0043f7d0
void ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick(
    EntitySlot* entity, float delta)
{
    float damage = 0.0f;
    float terrain_damage = 0.0f;
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->type == 10 || entity->type == 5) return;
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    if (entity->field_340 <= 0.0f) {
        entity->field_340 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        damage = entity_energy_decay_rate_get(entity) * 0.04f * delta;
        if (damage > 0.0f) entity_damage_apply(entity, damage, 0);
    }
    if (entity->progress_344 <= 0.0f) {
        entity->progress_344 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        const float decay = entity_stats_float_atdc(entity) * 0.04f * delta;
        entity->progress_344 -= decay;
        if (entity->progress_344 < 0.0f) entity->progress_344 = 0.0f;
    }

    if (level != NULL) {
        uint8_t* cell;
        entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
        cell = level_voxel(level, cell_x, cell_y);
        if (entity->attach_304 == NULL && cell != NULL && cell[2] == 6 &&
            (entity->type == 6 || entity->type == 7)) {
            level_state_field9c_add(level, 1);
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x00800000u;
        }
        if (entity_cell_type_speed_multiplier_resolve(
                entity, level, cell_x, cell_y, delta, &terrain_damage)) {
            entity_damage_apply(entity, terrain_damage, 1);
        }
    }

    if ((entity_state_flags_get(entity) & 0x00010000u) != 0u &&
        (entity->flags_3e0 & 0x00100100u) == 0u &&
        (entity->flags_3e4 & 0x08000000u) == 0u) {
        (void)entity_fence_spark_terrain_interaction(entity);
    }
    if (entity->field_340 < entity->progress_344)
        entity->progress_344 = entity->field_340;
    (void)entity_damage_hurt_sound_submit(entity, delta, terrain_damage);
    if (entity->activity_param != 0 && entity->activity_timer >= 0.0f)
        entity->activity_timer += delta;
}

// RecordPairPoseCopyAndActivityRaise
static EntityPairPoseHooks s_entity_pair_pose_hooks;

void entity_pair_pose_hooks_set(const EntityPairPoseHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_pair_pose_hooks, 0,
                              sizeof(s_entity_pair_pose_hooks));
    else s_entity_pair_pose_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0043fc20
int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity)
{
    EntitySlot* attached;
    EntityScenePose source_pose;
    float position[3];
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->owner_ctx == NULL ||
        s_entity_pair_pose_hooks.pose_read == NULL ||
        s_entity_pair_pose_hooks.pose_apply == NULL) return 0;
    attached = (EntitySlot*)entity->attach_304;
    if (attached == NULL ||
        !entity_world_center_position_read(attached, position)) return 0;
    if (s_entity_pair_pose_hooks.scene_present != NULL &&
        (!s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, entity) ||
         !s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, attached))) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    cell_x = level_cell_of(position[0]);
    cell_y = level_cell_of(position[1]);
    if (!entity_route_cell_candidate_accept(entity, level, cell_x, cell_y, 0)
        || !s_entity_pair_pose_hooks.pose_read(
            s_entity_pair_pose_hooks.context, attached, &source_pose)) return 0;

    entity->attach_304 = NULL;
    attached->carry_link_308 = NULL;
    attached->flags_3e0 &= ~0x400u;
    attached->carry_state_324 = 0;
    s_entity_pair_pose_hooks.pose_apply(s_entity_pair_pose_hooks.context,
                                        entity, &source_pose);
    entity_state_reset(attached, 0, 1);
    entity_activity_records_refresh(attached);
    entity_action_sequence_reset(attached, 0);
    entity_action_sequence_reset(entity, 0);
    entity->link_2fc = (uintptr_t)attached;
    entity->flags_3ec |= 0x8000u;
    if (s_entity_pair_pose_hooks.activity_raise != NULL)
        s_entity_pair_pose_hooks.activity_raise(
            s_entity_pair_pose_hooks.context, attached, 1);
    return 1;
}

// TerrainCellToWorldPosition
// FUNCTION: LEGORR 0x004401b0
int ledger_stub_004401b0_TerrainCellToWorldPosition(EntitySlot* entity,
                                                     int force)
{
    EntitySlot* owner;
    EntitySlot* linked;
    LevelState* level;
    int cell_x, cell_y;
    int should_transfer = 0;
    int target_pose_valid = 0;
    int linked_pose_valid = 0;
    float target_position[3];
    EntityScenePose target_pose;
    EntityScenePose linked_pose;
    EntityScenePose entity_pose;
    if (entity == NULL || entity->owner_ctx == NULL ||
        (entity_state_flags_get(entity) & 0x20000u) == 0u) return 0;
    owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL || (owner->flags_3e0 & 1u) != 0u) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;

    /* FUN_004401b0 asks the scene object for its current position before
     * resolving the terrain cell. EntitySlot coordinates are only the
     * fallback when the renderer has no pose provider. */
    if (s_entity_pair_pose_hooks.pose_read != NULL &&
        s_entity_pair_pose_hooks.pose_read(s_entity_pair_pose_hooks.context,
                                           entity, &entity_pose)) {
        target_position[0] = entity_pose.position[0];
        target_position[1] = entity_pose.position[1];
        target_position[2] = entity_pose.position[2];
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    else {
        target_position[0] = entity->x;
        target_position[1] = entity->y;
        target_position[2] = entity->z;
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    if ((entity_state_flags_get(entity) & 0x02000000u) != 0u &&
        entity_route_cell_candidate_accept(owner, level, cell_x, cell_y, 1))
        should_transfer = 1;
    if (!should_transfer && (entity->flags_3ec & 0x40u) == 0u) {
        int unload_cell[2];
        if (force == 0 &&
            entity_vehicle_unload_cell_find_if_eligible(entity, unload_cell)) {
            if (!terrain_cell_world_position_get(unload_cell[0], unload_cell[1],
                                                 target_position)) return 0;
            cell_x = unload_cell[0];
            cell_y = unload_cell[1];
            should_transfer = 1;
        }
        else if ((entity_state_flags_get(entity) & 0x02000000u) == 0u) {
            object_attached_record_allocate(entity);
        }
    }
    else if (!should_transfer && (entity->flags_3ec & 0x40u) != 0u) {
        linked = (EntitySlot*)entity->link_2fc;
        if (linked != NULL && linked->type == 4) {
            if (s_entity_pair_pose_hooks.scene_present != NULL &&
                !s_entity_pair_pose_hooks.scene_present(
                    s_entity_pair_pose_hooks.context, linked)) {
                linked = NULL;
            }
            if (linked != NULL &&
                s_entity_pair_pose_hooks.pose_read != NULL &&
                s_entity_pair_pose_hooks.pose_read(
                    s_entity_pair_pose_hooks.context, linked, &linked_pose)) {
                target_position[0] = linked_pose.position[0];
                target_position[1] = linked_pose.position[1];
                target_position[2] = linked_pose.position[2];
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                linked_pose_valid = 1;
                should_transfer = 1;
            }
            else if (linked != NULL &&
                     entity_world_center_position_read(linked,
                                                       target_position)) {
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                should_transfer = 1;
            }
        }
    }
    if (!should_transfer) return 0;

    target_position[2] = (float)terrain_surface_height_get(level->ptr_terrain,
                                                            cell_x, cell_y);
    entity_world_position_set(owner, target_position[0], target_position[1],
                              owner->rot, 0);
    owner->z = target_position[2];

    /* The linked-object branch explicitly copies its orientation. Other
     * branches only set the owner's position, preserving its current scene
     * orientation when the renderer can provide it. */
    if (s_entity_pair_pose_hooks.pose_apply != NULL) {
        if (linked_pose_valid) {
            target_pose = linked_pose;
            target_pose_valid = 1;
        }
        else if (s_entity_pair_pose_hooks.pose_read != NULL &&
                 s_entity_pair_pose_hooks.pose_read(
                     s_entity_pair_pose_hooks.context, owner, &target_pose)) {
            target_pose_valid = 1;
        }
        if (target_pose_valid) {
            target_pose.position[0] = target_position[0];
            target_pose.position[1] = target_position[1];
            target_pose.position[2] = target_position[2];
            s_entity_pair_pose_hooks.pose_apply(
                s_entity_pair_pose_hooks.context, owner, &target_pose);
        }
    }

    entity->owner_36c = NULL;
    owner->owner_36c = NULL;
    owner->flags_3e4 &= ~8u;
    ai_set_activity(owner, 0, 0);
    (void)selection_entity_remove(entity);
    return 1;
}

// LevelDebugCellRetargetDispatch
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict);
// FUNCTION: LEGORR 0x004403d0
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    EntitySlot* entity, EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks)
{
    LevelState* level;
    uint32_t target_state;
    int target_cell[2];
    unsigned int candidate;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    target_state = hooks != NULL && hooks->target_state_flags != NULL
        ? hooks->target_state_flags(hooks->context, target)
        : target->flags_3e8;
    if ((entity->flags_3f0 & 2u) != 0u &&
        (target_state & 0x00020000u) != 0u &&
        (target->flags_3ec & 0x40u) == 0u &&
        hooks != NULL && hooks->target_flag20_query != NULL &&
        hooks->target_flag20_query(hooks->context, target, 0u)) {
        entity_position_to_terrain_cell(target, level, &target_cell[0],
                                        &target_cell[1]);
        if (target_cell[0] >= 0 && target_cell[1] >= 0 &&
            ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
                entity, (unsigned int)target_cell[0],
                (unsigned int)target_cell[1], 0, &candidate, 1) &&
            entity->wp_count > 0) {
            step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
            step->weight_x = 0.5f;
            step->weight_y = 0.5f;
            switch (candidate & 3u) {
            case 0u: step->weight_x = 0.1f; break;
            case 1u: step->weight_y = 0.1f; break;
            case 2u: step->weight_x = 0.9f; break;
            case 3u: step->weight_y = 0.9f; break;
            }
            step->pending = (uint8_t)(step->pending | 8u);
            step->step_kind = 12u;
            entity->link_2fc = (uintptr_t)target;
            return 1;
        }
    }
    return hooks != NULL && hooks->fallback_retarget != NULL
        ? hooks->fallback_retarget(hooks->context, entity, target, level) : 0;
}

// SpatialEntityCallbackQuery
typedef struct SpatialEntityQueryRequest {
    EntitySlot* subject;
    float position[2];
    float values[3];
} SpatialEntityQueryRequest;
static EntitySpatialQueryHook s_spatial_entity_query_hook;
static void* s_spatial_entity_query_context;

void entity_spatial_query_hook_set(EntitySpatialQueryHook hook, void* context)
{
    s_spatial_entity_query_hook = hook;
    s_spatial_entity_query_context = context;
}

static int spatial_entity_query_adapter(EntitySlot* candidate, void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    if (request == NULL || s_spatial_entity_query_hook == NULL) return 0;
    return s_spatial_entity_query_hook(s_spatial_entity_query_context,
        request->subject, candidate, request->position, request->values);
}

int entity_spatial_query_dispatch(EntitySlot* subject,
                                  const float position[2],
                                  const float values[3])
{
    SpatialEntityQueryRequest request;
    GameContext* context;
    if (subject == NULL || position == NULL || values == NULL ||
        s_spatial_entity_query_hook == NULL) return 0;
    request.subject = subject;
    request.position[0] = position[0];
    request.position[1] = position[1];
    request.values[0] = values[0];
    request.values[1] = values[1];
    request.values[2] = values[2];
    context = entity_pool_callback_context_get();
    return entity_pool_find_by_callback_filtered(context,
        spatial_entity_query_adapter, &request);
}

// FUNCTION: LEGORR 0x004407b0
int ledger_stub_004407b0_SpatialEntityCallbackQuery(
    EntitySlot* subject, float value_a, float value_b, float value_c)
{
    float position[2];
    float values[3];
    if (subject == NULL) return 0;
    entity_transform_position_get(subject, position);
    values[0] = value_a;
    values[1] = value_b;
    values[2] = value_c;
    return entity_spatial_query_dispatch(subject, position, values);
}

// SpatialEntityCallbackQueryAtOptionalCoordinate
// FUNCTION: LEGORR 0x004408c0
int ledger_stub_004408c0_SpatialEntityCallbackQueryAtOptionalCoordinate(
    EntitySlot* subject, const float* optional_position, float value)
{
    float position[2];
    float values[3];
    if (subject == NULL) return 0;
    if (optional_position == NULL)
        entity_transform_position_get(subject, position);
    else {
        position[0] = optional_position[0];
        position[1] = optional_position[1];
    }
    values[0] = value;
    values[1] = 0.0f;
    values[2] = 0.0f;
    return entity_spatial_query_dispatch(subject, position, values);
}

// AttachedEntityDirectionVectorUpdate
// FUNCTION: LEGORR 0x00440a10
void ledger_stub_00440a10_AttachedEntityDirectionVectorUpdate(
    EntitySlot* entity)
{
    uint32_t i;
    if (entity == NULL || entity->attached_children == NULL) return;
    for (i = 0u; i < entity->attached_child_count; ++i) {
        EntitySlot* child = entity->attached_children[i];
        void* child_scene;
        void* record = NULL;
        void* parent_scene = NULL;
        int resolver_index = 0;
        int resolver_kind = 0;
        float old_position[3];
        float new_position[3];
        float dx, dy, dz, length, inverse_length;
        if (child == NULL || (child->flags_3e8 & 0x2000u) != 0u) continue;
        child_scene = child->attached_scene_object;
        if (child_scene == NULL ||
            !ddiscene_object_position_get(child_scene, NULL, old_position))
            continue;
        switch (entity->type) {
        case 1:
            record = entity->attached_direction_record_type1;
            resolver_kind = 2;
            resolver_index = (int)entity_indexed_record_word_at_f0_get(
                1, entity->etype, entity->indexed_record_slot_330) +
                child->attached_scene_slot_index;
            break;
        case 2:
            if (entity->owner_36c != NULL &&
                ((EntitySlot*)entity->owner_36c)->type == 1) {
                EntitySlot* linked = (EntitySlot*)entity->owner_36c;
                record = linked->attached_direction_record_type1;
                resolver_kind = 2;
                resolver_index = (int)entity_indexed_record_word_at_f0_get(
                    1, linked->etype, linked->indexed_record_slot_330) +
                    child->attached_scene_slot_index;
            }
            else {
                record = entity->attached_direction_record_type2;
                resolver_kind = 0;
            }
            break;
        case 3:
            record = entity->attached_direction_record_type3;
            resolver_kind = 0;
            break;
        case 4:
            record = entity->attached_direction_record_type4;
            resolver_kind = 1;
            resolver_index = child->attached_scene_slot_index;
            break;
        default:
            continue;
        }
        if (record == NULL) continue;
        switch (resolver_kind) {
        case 1:
            parent_scene = scene_object_indexed_handle_get_or_create(
                record, resolver_index);
            break;
        case 2:
            parent_scene = attached_entity_slot_object_get(
                (SceneResourceAnimationRecord*)record, resolver_index);
            break;
        default:
            parent_scene = renderer_cached_handle_48_get_or_create(record);
            break;
        }
        if (parent_scene == NULL) continue;
        (void)ddiscene_object_position_set(child_scene, parent_scene,
                                           0.0f, 0.0f, 0.0f);
        (void)ddiscene_object_pose_method_dispatch(
            child_scene, parent_scene, 0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f);
        if (!ddiscene_object_position_get(child_scene, NULL, new_position))
            continue;
        dx = new_position[0] - old_position[0];
        dy = new_position[1] - old_position[1];
        dz = new_position[2] - old_position[2];
        length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        inverse_length = 1.0f / length;
        child->attached_direction_length = length;
        child->attached_direction[0] = inverse_length * dx;
        child->attached_direction[1] = inverse_length * dy;
        child->attached_direction[2] = inverse_length * dz;
    }
}

// GridRouteSearchAdapter
// FUNCTION: LEGORR 0x00440c30
void ledger_stub_00440c30_GridRouteSearchAdapter(
    void* entity, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    uint32_t* out_count)
{
    /* FUN_00440c30 is a void eight-argument forwarder. The reference passes
     * its final count pointer unchanged and supplies null callback/context. */
    (void)terrain_neighbor_path_check(entity, start_x, start_y, goal_x,
                                      goal_y, out_x, out_y, out_count,
                                      NULL, NULL);
}

// TerrainGridPathSearch
int ledger_stub_00440c70_TerrainGridPathSearch(
    void* level, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    uint32_t* out_count, TerrainPathStopHook stop, void* user)
{
    return terrain_grid_path_search(level, start_x, start_y, goal_x, goal_y,
                                    out_x, out_y, out_count, stop, user);
}

// TerrainNeighborPathCheck
int ledger_stub_004410f0_TerrainNeighborPathCheck(
    void* entity, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    float* out_count, TerrainPathStopHook stop, void* user)
{
    uint32_t route_count = 0;
    int found = terrain_neighbor_path_check(
        entity, start_x, start_y, goal_x, goal_y, out_x, out_y,
        &route_count, stop, user);
    if (found && out_count != NULL) *out_count = (float)route_count;
    return found;
}

// SubobjectPoseDeltaNormalizeAndDispatch
// FUNCTION: LEGORR 0x004425f0
int ledger_stub_004425f0_SubobjectPoseDeltaNormalizeAndDispatch(
    EntityPoseDeltaState* state, const EntityPoseDeltaHooks* hooks,
    float target_x, float target_y)
{
    return entity_subobject_pose_delta_normalize_and_dispatch(
        state, hooks, target_x, target_y);
}

// EntityMovementAccumulatorApply
// FUNCTION: LEGORR 0x00442b10
int ledger_stub_00442b10_EntityMovementAccumulatorApply(
    EntityMovementAccumulatorState* state, void* entity, float delta,
    float* output_scale, const EntityMovementAccumulatorHooks* hooks)
{
    return entity_movement_accumulator_apply(state, entity, delta,
                                             output_scale, hooks);
}

// EntityPoseOrientationDispatch
// FUNCTION: LEGORR 0x00442f80
int ledger_stub_00442f80_EntityPoseOrientationDispatch(
    int kind, void* entity, const EntityPoseOrientationHooks* hooks)
{
    return entity_pose_orientation_dispatch(kind, entity, hooks);
}

// AdjacentPositionCandidateFind
// FUNCTION: LEGORR 0x00443710
int ledger_stub_00443710_AdjacentPositionCandidateFind(
    void* entity, const int candidate[2], int output[2],
    const EntityAdjacentRouteHooks* hooks)
{
    return entity_adjacent_position_candidate_find(entity, candidate, output,
                                                   hooks);
}

// PathStepAdvanceAndArrivalCheck
// FUNCTION: LEGORR 0x004438b0
float ledger_stub_004438b0_PathStepAdvanceAndArrivalCheck(
    EntityPathAdvanceState* state, void* entity, float delta,
    const EntityPathAdvanceHooks* hooks)
{
    return entity_path_step_advance_and_arrival_check(state, entity, delta,
                                                      hooks);
}

// ActivityTimeoutAccumulateAndRaise
// FUNCTION: LEGORR 0x004440f0
void ledger_stub_004440f0_ActivityTimeoutAccumulateAndRaise(
    void* entity, ActivityTimeoutState* state,
    const ActivityTimeoutHooks* hooks, float delta)
{
    activity_timeout_accumulate_and_raise(entity, state, hooks, delta);
}

// EntityRandomCellStepRouteIssue
// FUNCTION: LEGORR 0x00444260
int ledger_stub_00444260_EntityRandomCellStepRouteIssue(
    void* entity, const int cell[2], const EntityRandomCellStepHooks* hooks)
{
    return entity_random_cell_step_route_issue(entity, cell, hooks);
}

// FlaggedVectorStepCommandIssue
// FUNCTION: LEGORR 0x00444460
int ledger_stub_00444460_FlaggedVectorStepCommandIssue(
    FlaggedVectorStepState* state, void* entity,
    const FlaggedVectorStepHooks* hooks)
{
    return flagged_vector_step_command_issue(state, entity, hooks);
}

// StepMovementVectorResolve
// FUNCTION: LEGORR 0x00444680
int ledger_stub_00444680_StepMovementVectorResolve(
    StepMovementVectorState* state, void* entity, int advance,
    float output_vector[2], float* output_speed,
    const StepMovementVectorHooks* hooks)
{
    return step_movement_vector_resolve(state, entity, advance, output_vector,
                                        output_speed, hooks);
}

// EntityTurnToVectorActivityIssue
// FUNCTION: LEGORR 0x00444fb0
int ledger_stub_00444fb0_EntityTurnToVectorActivityIssue(
    EntityTurnToVectorState* state, void* entity, const float target[2],
    const EntityTurnToVectorHooks* hooks)
{
    return entity_turn_to_vector_activity_issue(state, entity, target, hooks);
}

// EntityPathStepVectorNormalize
// FUNCTION: LEGORR 0x004451e0
void ledger_stub_004451e0_EntityPathStepVectorNormalize(
    EntityPathStepState* state, void* entity,
    const EntityPathStepHooks* hooks)
{
    entity_path_step_vector_normalize(state, entity, hooks);
}

// EntityMoveVectorSurfaceProbe
// FUNCTION: LEGORR 0x004455a0
int ledger_stub_004455a0_EntityMoveVectorSurfaceProbe(
    EntitySlot* entity, const EntityMotionSurfaceProbeHooks* hooks)
{
    return entity_move_vector_surface_probe(entity, hooks);
}

// EntityBehaviorCallbackTick
// FUNCTION: LEGORR 0x004456e0
void ledger_stub_004456e0_EntityBehaviorCallbackTick(
    EntityBehaviorCallbackState* state,
    const EntityBehaviorCallbackHooks* hooks, void* entity)
{
    entity_behavior_callback_tick(state, hooks, entity);
}

// EntityMovementTickScaleAndDeltaGet
// FUNCTION: LEGORR 0x00446610
void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
    EntityMovementTickScaleState* state, void* entity, float delta,
    float* movement_delta, float* movement_scale,
    const EntityMovementTickScaleHooks* hooks)
{
    entity_movement_tick_scale_and_delta_get(state, entity, delta,
                                              movement_delta, movement_scale,
                                              hooks);
}

// EntityNearestConnectedTerrainCellRouteSelect
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker);
// FUNCTION: LEGORR 0x004469c0
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict)
{
    static const int offsets[8][2] = {
        { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 },
        { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
    };
    LevelState* level;
    int start_x, start_y;
    int* best_x = NULL;
    int* best_y = NULL;
    uint32_t best_count = 0xffffffffu;
    unsigned int best_index = 0xffffffffu;
    unsigned int candidate_count;
    unsigned int i;
    if (entity == NULL) return 0;
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(entity, level, &start_x, &start_y);
    candidate_count = include_diagonals ? 8u : 4u;
    for (i = 0; i < candidate_count; ++i) {
        const int x = (int)goal_x + offsets[i][0];
        const int y = (int)goal_y + offsets[i][1];
        int* path_x = NULL;
        int* path_y = NULL;
        uint32_t path_count = 0;
        if (!entity_route_cell_candidate_accept(entity, level, x, y, strict))
            continue;
        if (x == start_x && y == start_y) {
            path_x = (int*)malloc(sizeof(int));
            path_y = (int*)malloc(sizeof(int));
            if (path_x == NULL || path_y == NULL) {
                free(path_x);
                free(path_y);
                continue;
            }
            path_x[0] = x;
            path_y[0] = y;
            path_count = 1;
        }
        else if (!terrain_neighbor_path_check(level, (uint32_t)start_x,
                                               (uint32_t)start_y,
                                               (uint32_t)x, (uint32_t)y,
                                               &path_x, &path_y,
                                               &path_count, NULL, NULL)) {
            /* The route search may be unavailable before the level installs
             * its grid callback. Keep the one-cell neighbor behavior useful. */
            if (abs(x - start_x) + abs(y - start_y) == 1) {
                path_x = (int*)malloc(sizeof(int));
                path_y = (int*)malloc(sizeof(int));
                if (path_x != NULL && path_y != NULL) {
                    path_x[0] = x;
                    path_y[0] = y;
                    path_count = 1;
                }
                else {
                    free(path_x);
                    free(path_y);
                    path_x = NULL;
                    path_y = NULL;
                }
            }
        }
        if (path_count == 0 || path_count > AI_WAYPOINTS_MAX) {
            free(path_x);
            free(path_y);
            continue;
        }
        if (path_count < best_count) {
            free(best_x);
            free(best_y);
            best_x = path_x;
            best_y = path_y;
            best_count = path_count;
            best_index = i;
        }
        else {
            free(path_x);
            free(path_y);
        }
    }
    if (best_index == 0xffffffffu) return 0;
    {
        float route_x[AI_WAYPOINTS_MAX];
        float route_y[AI_WAYPOINTS_MAX];
        unsigned int j;
        for (j = 0; j < best_count; ++j) {
            route_x[j] = (float)best_x[j];
            route_y[j] = (float)best_y[j];
        }
        free(best_x);
        free(best_y);
        if (!entity_route_setup_and_marker_associate(entity, best_count,
                                                      route_x, route_y,
                                                      include_diagonals))
            return 0;
    }
    if (out_candidate != NULL) *out_candidate = best_index;
    return 1;
}

// EntityNearestCellRouteCommit
// FUNCTION: LEGORR 0x00446e40
int ledger_stub_00446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int mode)
{
    unsigned int candidate;
    if (entity == NULL ||
        !ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, goal_x, goal_y, mode, &candidate, 1)) return 0;
    entity->pos_pair_2f4 = (float)goal_x;
    entity->pos_pair_2f8 = (float)goal_y;
    if (mode != 0) entity->flags_3e8 |= 0x02000000u;
    else entity->flags_3e8 &= ~0x02000000u;
    if (entity->wp_count > 0) {
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].pending = (candidate & 1u) ? 2u : 1u;
    }
    return 1;
}

// RegisterRelativeStateUpdateFragment
// FUNCTION: LEGORR 0x0044702d
int ledger_stub_0044702d_RegisterRelativeStateUpdateFragment(
    EntitySlot* entity, float target_x, float target_y)
{
    int result;
    if (entity == NULL) return 0;
    result = entity_route_to_cell_commit(entity, (int)target_x,
                                         (int)target_y, 3u);
    if (result) entity->flags_3e8 &= ~0x02000000u;
    return result;
}

// EntityConditionalTypeFiveSpawn
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
// FUNCTION: LEGORR 0x004470d0
int ledger_stub_004470d0_EntityConditionalTypeFiveSpawn(EntitySlot* entity)
{
    GameContext* ctx;
    LevelState* level;
    int destination[2];
    int slot;
    EntitySlot* spawned;
    float route_x;
    float route_y;

    if (entity == NULL || (entity->flags_3e8 & 0x04000000u) == 0u ||
        entity->owner_ctx == NULL) return 0;
    if (entity_stats_float_at90(entity) >= entity->field_340) return 0;
    ctx = entity->owner_ctx;
    level = ctx->level;
    if (level == NULL) return 0;
    if (!ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
            entity, &destination[0], &destination[1], 1, 0, 0))
        return 0;

    slot = entity_create(ctx, 5, "Type5", (float)destination[0],
                         (float)destination[1], 0.0f, 0.0f);
    if (slot < 0) return 0;
    spawned = (EntitySlot*)entity_at(ctx, 5, slot);
    if (spawned == NULL) return 0;
    entity_world_position_set(spawned, (float)destination[0],
                              (float)destination[1], 0.0f, 0);
    route_x = (float)destination[0];
    route_y = (float)destination[1];
    if (!entity_route_setup_and_marker_associate(entity, 1, &route_x,
                                                  &route_y, 0)) {
        entity_type5_finalize_if_allowed(spawned);
        return 0;
    }
    entity->link_2fc = (uintptr_t)spawned;
    spawned->attach_304 = entity;
    entity->field_340 = 0.0f;
    return 1;
}

// TryCreateLinkedEntityAtPosition
// FUNCTION: LEGORR 0x004471b0
int ledger_stub_004471b0_TryCreateLinkedEntityAtPosition(
    EntitySlot* entity, float cell_x, float cell_y, EntitySlot* linked)
{
    LevelState* level;
    int source_cell[2];
    AiWaypoint* step;
    float weight;
    unsigned int direction;
    float route_x;
    float route_y;

    if (entity == NULL || linked == NULL
        || (entity->flags_3e0 & 0x400u) != 0u) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    if (!entity_route_cell_candidate_accept(entity, level,
                                             (int)cell_x, (int)cell_y, 1)) {
        return 0;
    }
    route_x = cell_x;
    route_y = cell_y;
    if (!entity_route_setup_and_marker_associate(entity, 1, &route_x,
                                                  &route_y, 0)) {
        return 0;
    }
    ENTITY_WAYPOINTS(entity)[0].pending = 3u;
    entity->link_2fc = (uintptr_t)linked;
    linked->attach_304 = entity;
    return 1;
}

// EntityAdjacentPlacementCellSearch
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target);
// FUNCTION: LEGORR 0x00447210
int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target)
{
    static const int offsets[12][2] = {
        { 2, 0 }, { 0, 2 }, { -2, 0 }, { 0, -2 },
        { 2, -1 }, { 2, 1 }, { 1, 2 }, { -1, 2 },
        { -2, 1 }, { -2, -1 }, { -1, -2 }, { 1, -2 }
    };
    LevelState* level;
    int target_cell[2];
    unsigned int start;
    int direction;
    unsigned int pass;
    unsigned int i;
    if (entity == NULL || target == NULL || entity->type != 3 ||
        entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    start = neighbor_pick_rotation_peek() % 12u;
    direction = (start & 1u) ? -1 : 1;
    for (pass = 0; pass != 2; ++pass) {
        for (i = 0; i < 12; ++i) {
            const unsigned int index = (start +
                (unsigned int)((int)i * direction + 12 * 12)) % 12u;
            const int x = target_cell[0] + offsets[index][0];
            const int y = target_cell[1] + offsets[index][1];
            if (pass == 0 &&
                !entity_route_cell_candidate_accept(entity, level, x, y, 1))
                continue;
            if (ledger_stub_004473b0_RouteToCellCommandIssue(
                    entity, x, y, target)) return 1;
        }
    }
    entity_attached_object_list_detach(entity, 0);
    return 0;
}

// RouteToCellCommandIssue
// FUNCTION: LEGORR 0x004473b0
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (target_cell[0] == target_x && target_cell[1] == target_y) return 0;
    if (!entity_route_to_cell_commit(entity, target_x, target_y, 5u)) return 0;
    if (entity->carry_link_308 != NULL)
        ((EntitySlot*)entity->carry_link_308)->link_2fc = (uintptr_t)target;
    return 1;
}

// LinkedEntityProximityResponse
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode);
// FUNCTION: LEGORR 0x004474f0
void ledger_stub_004474f0_LinkedEntityProximityResponse(EntitySlot* entity)
{
    EntitySlot* linked;
    float entity_position[3];
    float linked_position[3];
    float dx, dy, distance_squared, radius;
    if (entity == NULL) return;
    linked = (EntitySlot*)entity->link_2fc;
    entity->link_2fc = 0;
    if (linked == NULL ||
        !entity_world_center_position_read(entity, entity_position) ||
        !entity_world_center_position_read(linked, linked_position)) return;
    dx = linked_position[0] - entity_position[0];
    dy = linked_position[1] - entity_position[1];
    distance_squared = dx * dx + dy * dy;
    radius = entity_collision_radius_get(entity) * 2.0f +
             entity_collision_radius_get(linked);
    if (distance_squared < radius * radius) {
        const float direction[2] = { dx, dy };
        ledger_stub_00447950_EntityDirectionalActivityStart(linked,
                                                            direction, 1);
        entity_damage_apply(linked, 10.0f, 1);
    }
}

// EntityProgressAndNegativeDeltaHandler
// FUNCTION: LEGORR 0x004475c0
int ledger_stub_004475c0_EntityProgressAndNegativeDeltaHandler(
    EntitySlot* entity)
{
    EntitySlot* target;
    float delta;
    int complete = 0;
    if (entity == NULL) return 1;
    target = (EntitySlot*)entity->link_2fc;
    if (target == NULL) return 1;
    if ((target->flags_3e4 & 0x00100000u) != 0u) return 0;

    if ((entity->flags_3e8 & 0x00080000u) != 0u) {
        if (!entity_shared_eligibility_check(target, 0)) return 1;
        if (entity->activity_timer >= target->activity_timer) {
            target->wake_timer = 0.0f;
            entity->activity_timer = 0.0f;
        }
    }
    else if ((entity->flags_3e8 & 0x00400000u) != 0u) {
        entity_state_reset(target, 0, 1);
        entity->flags_3e8 &= ~0x00400000u;
    }
    else {
        delta = entity_stats_float_at88(entity);
        if (entity->type == 3) target->flags_3e4 &= ~0x00000020u;
        if (delta >= 0.0f) {
            target->field_340 += delta;
        }
        else {
            entity_damage_apply(target, -delta, 1);
            target->flags_3e4 |= 0x40000000u;
        }
        if (target->field_340 < 100.0f) {
            target->activity = 0;
        }
        else {
            target->field_340 = 100.0f;
            complete = 1;
        }
    }
    if (target->field_340 <= 0.0f) complete = 1;
    if (complete) entity->link_2fc = 0;
    return complete;
}

// PathStepLookaheadEntryWrite
// FUNCTION: LEGORR 0x004477d0
void ledger_stub_004477d0_PathStepLookaheadEntryWrite(EntitySlot* entity)
{
    EntitySlot* linked;
    EntitySlot* target;
    AiWaypoint* waypoint;
    float dx;
    float dy;
    float length;

    if (entity == NULL || entity->wp_count <= 0
        || (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) {
        return;
    }
    linked = (EntitySlot*)entity->carry_link_308;
    if (linked == NULL) {
        return;
    }
    target = (EntitySlot*)linked->link_2fc;
    if (target == NULL) {
        return;
    }

    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    dx = target->x - waypoint->x;
    dy = target->y - waypoint->y;
    length = (float)sqrt((double)(dx * dx + dy * dy));
    if (length != 0.0f) {
        waypoint->x += (dx / length) * 0.1f;
        waypoint->y += (dy / length) * 0.1f;
    }
    waypoint->pending = (uint8_t)(waypoint->pending | 8u);
    linked->pos_pair_2f4 = target->x;
    linked->pos_pair_2f8 = target->y;
    linked->link_2fc = 0;
}

// EntityDirectionalActivityStart
// FUNCTION: LEGORR 0x00447950
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode)
{
    float forward_x;
    float forward_y;
    float length;
    float dot;
    float cross;
    int activity;

    if (entity == NULL || (entity->flags_3e0 & 0x1000u) != 0u
        || !entity_shared_eligibility_check(entity, 1)) {
        return;
    }

    if (direction == NULL) {
        activity = (mode != 0) ? 0x1c : 0x24;
    }
    else {
        /* The reference uses the entity's normalized facing vector. The
         * portable slot keeps that orientation as a heading in radians; its
         * velocity pair is used when a caller has already supplied one. */
        forward_x = entity->velocity_x;
        forward_y = entity->velocity_y;
        length = (float)sqrt((double)(forward_x * forward_x
                                      + forward_y * forward_y));
        if (length == 0.0f) {
            forward_x = (float)cos((double)entity->rot);
            forward_y = (float)sin((double)entity->rot);
        }
        else {
            forward_x /= length;
            forward_y /= length;
        }
        dot = forward_x * direction[0] + forward_y * direction[1];
        if (dot <= 0.2928932f) {
            if (dot >= -0.2928932f) {
                cross = forward_y * direction[0]
                      - forward_x * direction[1];
                activity = (cross <= 0.0f) ? 0x17 : 0x18;
            }
            else {
                activity = 0x19;
            }
        }
        else {
            activity = 0x1a;
        }
    }

    entity_action_sequence_reset(entity, 0);
    ai_set_activity(entity, activity, 1);
    entity->flags_3e0 |= 0x1000u;
    if (entity->type == ENTITY_TYPE_OBJ) {
        entity->flags_3e8 &= 0x7fffffffu;
    }
}

// EntityActivityTick
// FUNCTION: LEGORR 0x00447b30
float ledger_stub_00447b30_EntityActivityTick(
    EntitySlot* entity, float delta, const EntityActivityTickHooks* hooks)
{
    float scaled_delta;
    EntityTypeActivityTickHook tick = NULL;
    float timer_result = 0.0f;
    if (entity == NULL || hooks == NULL) return delta;
    scaled_delta = hooks->delta_scale == NULL ? delta
        : hooks->delta_scale(hooks->context, entity, delta);
    timer_result = entity->activity_timer;
    if ((entity->flags_3e0 & 0x4000u) == 0u) {
        switch (entity->type) {
        case 1: tick = hooks->type1_tick; break;
        case 2: tick = hooks->type2_tick; break;
        case 3: tick = hooks->type3_tick; break;
        case 4: tick = hooks->type4_tick; break;
        default: break;
        }
        if (tick != NULL) {
            timer_result = tick(hooks->context, entity, scaled_delta);
        }
        else if ((entity->flags_3e8 & 0x20000u) != 0u) {
            if (hooks->behavior_update != NULL)
                hooks->behavior_update(hooks->context, entity, delta);
        }
        else if (entity->type == 10 && hooks->type10_tick != NULL) {
            timer_result = hooks->type10_tick(hooks->context, entity,
                                               scaled_delta);
        }
    }
    if (timer_result > 0.0f && hooks->activity_finish != NULL)
        hooks->activity_finish(hooks->context, entity);
    return scaled_delta;
}

// EntityTypeBehaviorUpdate
// FUNCTION: LEGORR 0x00447ea0
void ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks)
{
    int animation_done;
    if (entity == NULL || hooks == NULL) return;
    switch (entity->type) {
    case 5:
        if ((entity->flags_3e8 & 0x2000u) != 0u &&
            hooks->boulder_behavior != NULL)
            hooks->boulder_behavior(hooks->context, entity, delta);
        return;
    case 6:
        if (entity->attach_304 == NULL && hooks->animation_tick != NULL)
            hooks->animation_tick(hooks->context, entity, delta * 0.01f);
        return;
    case 8:
    case 13:
        if ((entity->flags_3e8 & 0x10000u) == 0u &&
            (entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            entity->flags_3e8 |= 0x10000u;
            if (hooks->animation_play != NULL)
                hooks->animation_play(hooks->context, entity, "TickDown");
            if (hooks->terrain_activity != NULL)
                hooks->terrain_activity(hooks->context, entity);
            return;
        }
        if ((entity->flags_3e8 & 0x10000u) != 0u) {
            animation_done = hooks->animation_tick == NULL ? 0
                : hooks->animation_tick(hooks->context, entity, delta);
            if (animation_done || entity->field_340 <= 0.0f) {
                entity->flags_3e8 &= ~0x10000u;
                entity->flags_3e8 |= 0x800000u;
                entity->flags_3e0 |= 0x80000u;
                if (hooks->terrain_activity != NULL)
                    hooks->terrain_activity(hooks->context, entity);
            }
            return;
        }
        if (hooks->animation_tick != NULL)
            hooks->animation_tick(hooks->context, entity, delta);
        return;
    case 11:
        if ((entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            if (hooks->terrain_activity != NULL)
                hooks->terrain_activity(hooks->context, entity);
        }
        return;
    case 9:
    case 12:
    case 14:
    case 16:
    case 17:
    case 19:
        if (hooks->animation_tick != NULL)
            hooks->animation_tick(hooks->context, entity, delta);
        return;
    default:
        return;
    }
}

// EntityMoveToCellCoordinates
// FUNCTION: LEGORR 0x00448800
int ledger_stub_00448800_EntityMoveToCellCoordinates(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 8u) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (!entity_move_destination_cell_usable(entity, level, x, y)) {
        return 0;
    }

    /* The source queues the route through its command record. The portable
     * model keeps the same destination in its inline waypoint list. */
    ENTITY_WAYPOINTS(entity)[0].x = (float)x;
    ENTITY_WAYPOINTS(entity)[0].y = (float)y;
    ENTITY_WAYPOINTS(entity)[0].pending = 9u;
    entity->wp_count = 1;
    entity->wp_index = 0;
    entity->pos_pair_2f4 = (float)x;
    entity->pos_pair_2f8 = (float)y;
    entity->flags_3e0 |= 0x00000008u;
    return 1;
}

// RecordGatedStepCommandIssue
// FUNCTION: LEGORR 0x00448880
int ledger_stub_00448880_RecordGatedStepCommandIssue(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;
    float route_x;
    float route_y;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 0x0du) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (!entity_move_destination_cell_usable(entity, level, x, y)) {
        return 0;
    }

    route_x = (float)x;
    route_y = (float)y;
    if (!entity_route_setup_and_marker_associate(entity, 1, &route_x,
                                                  &route_y, 0)) {
        return 0;
    }
    ENTITY_WAYPOINTS(entity)[0].pending = 9u;
    entity->pos_pair_2f4 = route_x;
    entity->pos_pair_2f8 = route_y;
    return 1;
}

// EntityNeighborCellMoveCommandHandle
static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y);
static float entity_route_weight_random(float minimum, float maximum);
// FUNCTION: LEGORR 0x00448a60
int ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(
    EntitySlot* entity, const int target_cell[2])
{
    LevelState* level;
    int source_cell[2];
    AiWaypoint* step;
    float weight;
    unsigned int direction;
    if (entity == NULL || target_cell == NULL || entity->owner_ctx == NULL
        || entity->activity != 11) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !level_walkable(level, target_cell[0], target_cell[1]))
        return 0;
    /* The reference uses TerrainNeighborPathCheck when the requested cell is
     * not the entity's current cell, then installs activity step 9. The
     * shared route commit owns that search, route storage, and target pair. */
    entity_position_to_terrain_cell(entity, level, &source_cell[0],
                                    &source_cell[1]);
    if (!entity_route_to_cell_commit(entity, target_cell[0], target_cell[1],
                                     9u) || entity->wp_count <= 0) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 9u;
    if (entity->type == 1) {
        direction = entity_route_cardinal_direction(source_cell[0],
            source_cell[1], target_cell[0], target_cell[1]);
        weight = entity_route_weight_random(0.55f, 0.75f);
        switch (direction) {
        case 0u: step->weight_x = weight; step->weight_y = 0.5f; break;
        case 1u: step->weight_x = 0.5f; step->weight_y = weight; break;
        case 2u: step->weight_x = 1.0f - weight; step->weight_y = 0.5f; break;
        default: step->weight_x = 0.5f;
                 step->weight_y = 1.0f - weight; break;
        }
        step->pending = (uint8_t)(step->pending | 8u);
    }
    return 1;
}

// CardinalStepCommandIssueAndLink
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker);
static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y)
{
    int dx = target_x - source_x;
    int dy = target_y - source_y;
    if (abs(dx) <= abs(dy)) return dy <= 0 ? 2u : 0u;
    return dx <= 0 ? 3u : 1u;
}

static float entity_route_weight_random(float minimum, float maximum)
{
    if (s_cell_step_random_hook != NULL)
        return s_cell_step_random_hook(minimum, maximum,
                                       s_cell_step_hook_user);
    return (minimum + maximum) * 0.5f;
}

// FUNCTION: LEGORR 0x00448c90
int ledger_stub_00448c90_CardinalStepCommandIssueAndLink(
    EntitySlot* entity, EntitySlot* target, int link_mode)
{
    LevelState* level;
    int target_cell[2];
    int source_cell[2];
    AiWaypoint* step;
    unsigned int direction;
    float weight;
    if (entity == NULL || target == NULL || target->type != 4) return 0;
    if (entity->type == 2)
        return ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
            entity, target, 1);
    if (entity->type != 1 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    entity_position_to_terrain_cell(entity, level, &source_cell[0],
                                    &source_cell[1]);
    if (!entity_route_to_cell_commit(entity, target_cell[0], target_cell[1],
                                     15u)) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    direction = entity_route_cardinal_direction(source_cell[0], source_cell[1],
                                                 target_cell[0], target_cell[1]);
    weight = entity_route_weight_random(0.55f, 0.75f);
    switch (direction) {
    case 0u: step->weight_x = weight; step->weight_y = 0.5f; break;
    case 1u: step->weight_x = 0.5f; step->weight_y = weight; break;
    case 2u: step->weight_x = 1.0f - weight; step->weight_y = 0.5f; break;
    default: step->weight_x = 0.5f; step->weight_y = 1.0f - weight; break;
    }
    step->step_kind = 15u;
    step->pending = (uint8_t)(step->pending | 4u | direction);
    entity->link_2fc = (uintptr_t)target;
    target->route_partner_300 = entity;
    entity->command_mode_404 = link_mode;
    return 1;
}

// ProximityStepCommandIssueWithJitter
// FUNCTION: LEGORR 0x00448eb0
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, 1) ||
        entity->wp_count <= 0) return 0;
    direction = (candidate - 2u) & 3u;
    edge_weight = entity_route_weight_random(0.1f, 0.9f);
    step_fraction = 4.0f / entity_surface_scale_get();
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = marker ? 15u : 14u;
    step->pending = (uint8_t)(step->pending | 4u | direction |
                              (marker ? 0x20u : 0u));
    entity->activity = marker ? 15 : 14;
    entity->activity_timer = 0.0f;
    entity->pos_pair_2f4 = (float)target_cell[0];
    entity->pos_pair_2f8 = (float)target_cell[1];
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// EntityKind6RecordApproachRouteIssue
int ledger_stub_0044a3d0_PlaceCellNearestResolve(const EntitySlot* target,
                                                 int cell_xy_out[2]);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount);
// FUNCTION: LEGORR 0x00449010
int ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(EntitySlot* entity)
{
    int cell[2];
    EntitySlot* kind_six_record;
    AiWaypoint* step;
    if (entity->type != 2 || (entity->flags_3e0 & 0x400u) == 0u) return 0;
    kind_six_record = (EntitySlot*)entity->carry_link_308;
    if (kind_six_record->type != 6
        || (kind_six_record->flags_3e8 & 0x80000000u) == 0u) return 0;
    if (!ledger_stub_0044a3d0_PlaceCellNearestResolve(entity, cell)) return 0;
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(
            entity, (unsigned int)cell[0], (unsigned int)cell[1], 3.0f))
        return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 18u;
    entity->pos_pair_2f4 = (float)cell[0];
    entity->pos_pair_2f8 = (float)cell[1];
    return 1;
}

// ProximityStepCommandIssueWithLink
// FUNCTION: LEGORR 0x004490a0
int ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
    EntitySlot* entity, EntitySlot* target, int mode, int set_global_gate)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || target->type != 4 ||
        (entity->type != 2 && entity->type != 3) ||
        entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, entity->type != 3) ||
        entity->wp_count <= 0) return 0;
    direction = (candidate - 2u) & 3u;
    if (entity->type == 3 && (entity->flags_3e8 & 0x80000u) == 0u)
        edge_weight = 0.5f;
    else
        edge_weight = entity_route_weight_random(0.2f, 0.8f);
    step_fraction = 4.0f / entity_surface_scale_get();
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = 6u;
    step->pending = (uint8_t)(step->pending | 4u | direction |
                              (set_global_gate ? 0x20u : 0u));
    entity->activity = 6;
    entity->activity_timer = 0.0f;
    entity->pos_pair_2f4 = (float)target_cell[0];
    entity->pos_pair_2f8 = (float)target_cell[1];
    entity->link_2fc = (uintptr_t)target;
    /* The original writes the caller's mode to +0x404 and sets the mode bit
     * in the +0x3ec activity flags when requested. */
    if (set_global_gate) entity->flags_3ec |= 0x00400000u;
    entity->command_mode_404 = mode;
    return 1;
}

// EntityWalkableTargetCellRouteIssue
// FUNCTION: LEGORR 0x00449240
int ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (!terrain_cell_allows_placement(entity, level, target_xy[0],
                                       target_xy[1])) return 0;
    return entity_route_to_cell_commit(entity, target_xy[0], target_xy[1], 2u);
}

// CellStepCommandIssueWithTargetMark
// FUNCTION: LEGORR 0x004492b0
int ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    int result;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !level_walkable(level, target_xy[0], target_xy[1]))
        return 1;
    if ((entity_state_flags_get(entity) & 0x10000u) == 0u) return 1;
    result = entity_route_to_cell_commit(entity, target_xy[0], target_xy[1], 4u);
    if (result && entity->type == 1) {
        terrain_cell_flags_set_and_refresh(level, target_xy[0], target_xy[1]);
        terrain_cell_flag100_set_and_track(level, target_xy[0], target_xy[1]);
    }
    return result;
}

// EntityTargetCellRouteAndAssociate
// FUNCTION: LEGORR 0x00449430
int ledger_stub_00449430_EntityTargetCellRouteAndAssociate(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &cell[0], &cell[1]);
    if (!entity_route_to_cell_commit(entity, cell[0], cell[1], 0x11u))
        return 0;
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// EntityTargetCellRouteMarker14
// FUNCTION: LEGORR 0x00449520
int ledger_stub_00449520_EntityTargetCellRouteMarker14(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &cell[0], &cell[1]);
    return entity_route_to_cell_commit(entity, cell[0], cell[1], 0x14u);
}

// EntityCoordinateRouteAndMark
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker)
{
    LevelState* level;
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0;
    float route_x[AI_WAYPOINTS_MAX];
    float route_y[AI_WAYPOINTS_MAX];
    uint32_t i;

    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !entity_route_cell_candidate_accept(
            entity, level, target_x, target_y, 1)) return 0;
    entity_position_to_terrain_cell(entity, level, &start_x, &start_y);
    if (start_x == target_x && start_y == target_y) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    else if (!terrain_neighbor_path_check(level, (uint32_t)start_x,
                                           (uint32_t)start_y,
                                           (uint32_t)target_x,
                                           (uint32_t)target_y,
                                           &path_x, &path_y,
                                           &path_count, NULL, NULL)) {
        if (abs(target_x - start_x) + abs(target_y - start_y) != 1) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    if (path_count == 0 || path_count > AI_WAYPOINTS_MAX) {
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    free(path_x);
    free(path_y);
    if (!entity_route_setup_and_marker_associate(entity, path_count,
                                                  route_x, route_y, 0))
        return 0;
    entity->activity = (int)marker;
    entity->activity_timer = 0.0f;
    entity->pos_pair_2f4 = (float)target_x;
    entity->pos_pair_2f8 = (float)target_y;
    return 1;
}

// FUNCTION: LEGORR 0x00449610
int ledger_stub_00449610_EntityCoordinateRouteAndMark(
    EntitySlot* entity, int target_x, int target_y, unsigned int marker)
{
    return entity_route_to_cell_commit(entity, target_x, target_y, marker);
}

int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2]);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);

// RouteStepCommandIssue
// FUNCTION: LEGORR 0x00449700
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity)
{
    LevelState* level;
    int destination[2];
    int found;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    if ((entity->flags_3e8 & 0x00100000u) != 0u) {
        found = ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
            entity, destination);
        if (!found || !entity_route_to_cell_commit(entity, destination[0],
                                                    destination[1], 10u))
            return 0;
    }
    else {
        found = ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
            entity, &destination[0], &destination[1], 1, 0, 1);
        if (!found || !ledger_stub_00449880_BlockedCellApproachWeightWrite(
                entity, (unsigned int)destination[0],
                (unsigned int)destination[1], 3.0f)) return 0;
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 10u;
    }
    entity->pos_pair_2f4 = (float)destination[0];
    entity->pos_pair_2f8 = (float)destination[1];
    return 1;
}

// BlockedCellApproachWeightWrite
// FUNCTION: LEGORR 0x00449880
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount)
{
    LevelState* level;
    unsigned int direction;
    float scaled;
    AiWaypoint* step;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !entity_route_cell_candidate_accept(
            entity, level, (int)cell_x, (int)cell_y, 1)) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, cell_x, cell_y, 0, &direction, 1)) return 0;
    if (entity->wp_count <= 0) return 0;
    scaled = amount / entity_surface_scale_get();
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->weight_x = 0.5f;
    step->weight_y = 0.5f;
    switch (direction & 3u) {
    case 0u: step->weight_x = scaled; break;
    case 1u: step->weight_y = scaled; break;
    case 2u: step->weight_x = 1.0f - scaled; break;
    case 3u: step->weight_y = 1.0f - scaled; break;
    }
    step->pending = (uint8_t)(step->pending | 8u);
    return 1;
}

// ProximityStepCommandIssueClearingGate
// FUNCTION: LEGORR 0x00449ac0
int ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(
    EntitySlot* entity, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int direction;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &direction, 1)) {
        entity->flags_3e8 &= ~0x00000800u;
        return 0;
    }
    entity->activity = 0x0b;
    entity->activity_param = 0;
    entity->activity_timer = 0.0f;
    entity->link_2fc = (uintptr_t)target;
    if (entity->wp_count > 0)
        ENTITY_WAYPOINTS(entity)[0].pending = (uint8_t)(ENTITY_WAYPOINTS(entity)[0].pending | 4u |
                                                   (direction & 3u));
    return 1;
}

// SceneObjectStateHandleApplyValue
typedef struct SceneObjectStateHandle {
    float value;
    uint32_t state;
    void* owner;
} SceneObjectStateHandle;
typedef void (*SceneObjectStateApplyHook)(void* owner, float value,
                                          uint32_t state, void* context);
// FUNCTION: LEGORR 0x00449cd0
int ledger_stub_00449cd0_SceneObjectStateHandleApplyValue(
    SceneObjectStateHandle* handle, float value, uint32_t state,
    SceneObjectStateApplyHook apply, void* context)
{
    if (handle == NULL) return 0;
    handle->value = value;
    handle->state = state;
    if (apply != NULL) apply(handle->owner, value, state, context);
    return 1;
}

// EntityTypeOperationDispatch
typedef int (*EntityTypeOperationHook)(void* context, EntitySlot* entity,
                                       uint32_t operation);
typedef struct EntityTypeOperationEntry {
    EntityTypeOperationHook dispatch;
    void* context;
} EntityTypeOperationEntry;
// FUNCTION: LEGORR 0x00449f50
int ledger_stub_00449f50_EntityTypeOperationDispatch(
    EntitySlot* entity, uint32_t operation,
    const EntityTypeOperationEntry* table, uint32_t table_count)
{
    if (entity == NULL || table == NULL || entity->type < 0 ||
        (uint32_t)entity->type >= table_count ||
        table[entity->type].dispatch == NULL) return 0;
    return table[entity->type].dispatch(table[entity->type].context, entity,
                                         operation);
}

// EntityTypeOperationRelease
// FUNCTION: LEGORR 0x00449ff0
void ledger_stub_00449ff0_EntityTypeOperationRelease(
    EntitySlot* entity, const EntityTypeOperationEntry* table,
    uint32_t table_count, void (*release)(void* context, EntitySlot* entity))
{
    if (entity == NULL) return;
    if (table != NULL && entity->type >= 0 &&
        (uint32_t)entity->type < table_count &&
        table[entity->type].dispatch != NULL)
        (void)table[entity->type].dispatch(table[entity->type].context,
                                           entity, 0xffffffffu);
    if (release != NULL) {
        void* context = NULL;
        if (table != NULL && entity->type >= 0 &&
            (uint32_t)entity->type < table_count)
            context = table[entity->type].context;
        release(context, entity);
    }
}

// EntityWorkObjectPositionGet
// FUNCTION: LEGORR 0x0044a2a0
int ledger_stub_0044a2a0_EntityWorkObjectPositionGet(
    const EntitySlot* entity, float position[3])
{
    if (entity == NULL || position == NULL) return 0;
    return entity_world_center_position_read(entity, position);
}

// ObjectMovementInputAccumulate
// FUNCTION: LEGORR 0x0044a310
void ledger_stub_0044a310_ObjectMovementInputAccumulate(EntitySlot* object,
    int input_y, int input_x, float input_z)
{
    unsigned char* bytes = (unsigned char*)object;
    int type;
    uint32_t capability;
    unsigned char flags;
    int* accumulators;
    float* accumulator_z;
    void* linked;
    memcpy(&type, bytes, sizeof(type));
    if (type == ENTITY_TYPE_MINIFIG) {
        memcpy(&linked, bytes + 0x36c, sizeof(linked));
        if (linked != NULL) {
            bytes = (unsigned char*)linked;
        }
    }
    flags = bytes[0x3e0];
    if ((flags & 1u) != 0)
        entity_action_sequence_reset((EntitySlot*)bytes, 0);
    capability = entity_capability_flags_get((EntitySlot*)bytes);
    if ((capability & 4u) == 0) input_x = 0;
    accumulators = (int*)(bytes + 0x2bc);
    accumulator_z = (float*)(bytes + 0x2c4);
    *accumulator_z = input_z + *accumulator_z;
    accumulators[0] += input_x;
    accumulators[1] += input_y;
}

// Type11CellCoordinateAppend
// FUNCTION: LEGORR 0x0044a390
void ledger_stub_0044a390_Type11CellCoordinateAppend(const int* coordinate)
{
    volatile unsigned int* count = &s_type11_cell_coordinate_count;
    volatile int* coordinates = (volatile int*)s_type11_cell_coordinates;
    volatile const int* input = coordinate;
    const unsigned int first = *count;
    unsigned int second;
    if (first >= 10u) return;
    coordinates[first * 2u] = input[0];
    second = *count;
    coordinates[second * 2u + 1u] = input[1];
    ++*count;
}

// PlaceCellNearestResolve
// FUNCTION: LEGORR 0x0044a3d0
int ledger_stub_0044a3d0_PlaceCellNearestResolve(const EntitySlot* target,
                                                 int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    unsigned int i;
    unsigned int best_index = 0xffffffffu;
    if (target == NULL) return 0;
    entity_transform_position_get(target, target_xy);
    for (i = 0; i < s_type11_cell_coordinate_count; ++i) {
        float world[3];
        float dx;
        float dy;
        float distance;
        int x = s_type11_cell_coordinates[i][0];
        int y = s_type11_cell_coordinates[i][1];
        if (!terrain_cell_world_position_get(x, y, world)) continue;
        dy = target_xy[1] - world[1];
        dx = target_xy[0] - world[0];
        distance = sqrtf(dy * dy + dx * dx);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    if (best_index == 0xffffffffu) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = s_type11_cell_coordinates[best_index][0];
        cell_xy_out[1] = s_type11_cell_coordinates[best_index][1];
    }
    return 1;
}

// NearestCellCoordinateWorldResolve
// FUNCTION: LEGORR 0x0044a4f0
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    unsigned int i;
    unsigned int count = level_predug_coordinate_count_get();
    int best_cell[2] = { 0, 0 };
    int found = 0;
    if (target == NULL) return 0;
    entity_transform_position_get(target, target_xy);
    for (i = 0; i < count; ++i) {
        int cell[2];
        float world[3];
        float dx;
        float dy;
        float distance;
        if (!level_predug_coordinate_get(i, cell) ||
            !terrain_cell_world_position_get(cell[0], cell[1], world)) continue;
        dy = target_xy[1] - world[1];
        dx = target_xy[0] - world[0];
        distance = sqrtf(dy * dy + dx * dx);
        if (distance < best_distance) {
            best_distance = distance;
            best_cell[0] = cell[0];
            best_cell[1] = cell[1];
            found = 1;
        }
    }
    if (!found) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = best_cell[0];
        cell_xy_out[1] = best_cell[1];
    }
    return 1;
}

typedef struct NearestOpenCellCandidate {
    int x;
    int y;
    float distance;
} NearestOpenCellCandidate;

static int nearest_open_cell_candidate_compare(const void* first,
                                               const void* second)
{
    const NearestOpenCellCandidate* a =
        (const NearestOpenCellCandidate*)first;
    const NearestOpenCellCandidate* b =
        (const NearestOpenCellCandidate*)second;
    if (a->distance < b->distance) return -1;
    if (a->distance > b->distance) return 1;
    return 0;
}

// NearestOpenCellCoordSearch
// FUNCTION: LEGORR 0x0044a5d0
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20)
{
    NearestOpenCellCandidate candidates[100];
    LevelState* level;
    float entity_position[2];
    int center_x = 0, center_y = 0;
    int x, y;
    unsigned int count = 0u;
    float minimum_distance;
    if (entity == NULL || entity->owner_ctx == NULL ||
        output_x == NULL || output_y == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    (void)terrain_world_xy_to_cell(level->ptr_terrain, entity_position[0],
                                   entity_position[1], &center_x, &center_y);
    minimum_distance = terrain_blocksize_get(level->ptr_terrain);
    for (y = center_y - 10; y < center_y + 10; ++y) {
        for (x = center_x - 10; x < center_x + 10; ++x) {
            float cell_position[2];
            float dx, dy, distance;
            if (!level_cell_walkable_flag(level, x, y) ||
                (allow_flag20 == 0 &&
                 level_cell_flag_20_bounded(level, x, y) != 0u) ||
                (allow_maskc0 == 0 && level_cell_mask_c0(level, x, y) != 0u)) {
                continue;
            }
            if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
                                                  x, y, cell_position)) {
                continue;
            }
            dx = cell_position[0] - entity_position[0];
            dy = cell_position[1] - entity_position[1];
            distance = sqrtf(dx * dx + dy * dy);
            if (require_minimum_distance != 0 &&
                minimum_distance > distance) continue;
            if (count < 100u) {
                candidates[count].x = x;
                candidates[count].y = y;
                candidates[count].distance = distance;
                ++count;
            }
        }
    }
    if (count == 0u) return 0;
    crt_array_sort_with_comparator(candidates, (size_t)count,
                                  sizeof(candidates[0]),
                                  nearest_open_cell_candidate_compare);
    *output_x = candidates[0].x;
    *output_y = candidates[0].y;
    return 1;
}

// PolyVariantSectionModelBuild
typedef int (*PolyVariantSectionBuildHook)(void* context, void* section,
                                           uint32_t variant);
// FUNCTION: LEGORR 0x0044a7d0
int ledger_stub_0044a7d0_PolyVariantSectionModelBuild(
    void* section, uint32_t variant_count, PolyVariantSectionBuildHook build,
    void* context)
{
    uint32_t variant;
    if (section == NULL || build == NULL) return 0;
    for (variant = 0u; variant < variant_count; ++variant)
        if (!build(context, section, variant)) return 0;
    return 1;
}

// BuildingPlacementPreviewUpdate
static BuildingPlacementPreviewHooks s_building_placement_preview_hooks;
static int s_building_placement_preview_owner_bound;
// FUNCTION: LEGORR 0x0044a910
int ledger_stub_0044a910_BuildingPlacementPreviewUpdate(
    const BuildingPlacementPreviewHooks* hooks)
{
    float position[3];
    uint32_t record_key = 0u;
    uint32_t direction = 0u;
    uint32_t footprint_count = 0u;
    const int* footprint_xy = NULL;
    TerrainSurfaceGrid* surface_grid = NULL;
    int cell_xy[2] = { 0, 0 };
    int emit_boundary_commands = 0;
    int valid;
    if (hooks == NULL || hooks->position_get == NULL ||
        hooks->placement_check == NULL || hooks->preview_set == NULL ||
        !hooks->position_get(hooks->context, position)) return 0;
    valid = hooks->placement_check(hooks->context, position);
    if (valid && hooks->path_selection_get != NULL &&
        hooks->path_selection_get(hooks->context, position, &record_key,
                                  cell_xy, &direction, &footprint_xy,
                                  &footprint_count, &surface_grid,
                                  &emit_boundary_commands) &&
        (footprint_count == 0u || footprint_xy != NULL)) {
        path_record_surface_grid_bind(surface_grid);
        (void)ledger_stub_00409790_CellPathRecordRegisterApply(
            record_key, cell_xy, direction, footprint_xy, footprint_count);
        if (emit_boundary_commands) {
            (void)ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
                cell_xy, footprint_xy, footprint_count);
        }
    }
    hooks->preview_set(hooks->context, position, valid);
    return valid;
}

void building_placement_preview_owner_bind(
    const BuildingPlacementPreviewHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_building_placement_preview_hooks, 0,
               sizeof(s_building_placement_preview_hooks));
        s_building_placement_preview_owner_bound = 0;
        return;
    }
    s_building_placement_preview_hooks = *hooks;
    s_building_placement_preview_owner_bound = 1;
}

int building_placement_preview_owner_update(void)
{
    if (!s_building_placement_preview_owner_bound) return 0;
    return ledger_stub_0044a910_BuildingPlacementPreviewUpdate(
        &s_building_placement_preview_hooks);
}

// ScatterSpawnGateSet is implemented in spawn.cpp.

// EntityAttachedSpawnRecordProcess
// FUNCTION: LEGORR 0x0044af90
void ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(EntitySlot* owner)
{
    if (owner != NULL && owner->carry_ref_348 != NULL) {
        const uint32_t count = *(const uint32_t*)owner->carry_ref_348;
        lego::random_kind_six_scatter_spawn(owner, (int)count, 0);
    }
}

// ConfiguredTinyMonsterSpawnAndTeardown
// FUNCTION: LEGORR 0x0044afb0
void ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(
    EntitySlot* source)
{
    static const char* const configured_names[3] = {
        "RockMonster", "Lavamonster", "Icemonster"
    };
    int resolved_type, resolved_row, source_type, source_row;
    int selected = -1, i;
    const char* replacement;
    float position[2];
    EntitySlot* monster = NULL;
    unsigned int successes = 0u;
    if (source == NULL || source->owner_ctx == NULL) return;
    if (source->type_name[0] == '\0' ||
        !entity_type_row_for_name(source->type_name, &source_type,
                                  &source_row)) return;
    for (i = 0; i < 3; ++i) {
        if (entity_type_row_for_name(configured_names[i], &resolved_type,
                                     &resolved_row) &&
            source_type == resolved_type && source_row == resolved_row) {
            selected = i;
            break;
        }
    }
    if (selected < 0) return;
    replacement = selected == 2 ? "TinyIM" : "TinyRM";
    if (!entity_type_row_for_name(replacement, &resolved_type, &resolved_row))
        return;
    entity_transform_position_get(source, position);
    while (successes < 3u) {
        monster = entity_create_with_position(
            source->owner_ctx, resolved_type, replacement, 0,
            position[0], position[1], (float)(level_rand_u16() % 7));
        if (monster == NULL) return;
        monster->field_340 = -1.0f;
        if (!ledger_stub_00449700_RouteStepCommandIssue(monster)) break;
        ++successes;
        source->flags_3e4 |= 0x20u;
    }
    if (successes == 3u) return;
    if (monster != NULL) (void)entity_teardown(monster);
}

// SmallSpiderCellSwarmSpawn
// FUNCTION: LEGORR 0x0044b140
void ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
    int cell_x, int cell_y, uint32_t variation)
{
    int type, row, i, count = (int)(variation & 3u) + 2;
    float vertices[12];
    float min_x, max_x, min_y, max_y;
    GameContext* context = entity_pool_callback_context_get();
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    if (context == NULL || context->level == NULL || grid == NULL ||
        !entity_type_row_for_name("SmallSpider", &type, &row) ||
        !terrain_cell_surface_vertices_get(grid, (uint32_t)cell_x,
                                           (uint32_t)cell_y, vertices)) return;
    min_x = max_x = vertices[0];
    min_y = max_y = vertices[1];
    for (i = 1; i < 4; ++i) {
        if (vertices[i * 3] < min_x) min_x = vertices[i * 3];
        if (vertices[i * 3] > max_x) max_x = vertices[i * 3];
        if (vertices[i * 3 + 1] < min_y) min_y = vertices[i * 3 + 1];
        if (vertices[i * 3 + 1] > max_y) max_y = vertices[i * 3 + 1];
    }
    (void)row;
    for (i = 0; i < count; ++i) {
        int rotation = level_rand_u16() % 7;
        float x = random_float_range(min_x + 1.0f, max_x - 1.0f);
        float y = random_float_range(min_y + 1.0f, max_y - 1.0f);
        EntitySlot* spider = entity_create_with_position(
            context, type, "SmallSpider", 0, x, y, (float)rotation);
        if (spider != NULL) spider->flags_3e0 |= 0x10000u;
    }
}

// PairingActivityRaiseAndMutualLink
// FUNCTION: LEGORR 0x0044b250
int ledger_stub_0044b250_PairingActivityRaiseAndMutualLink(
    EntitySlot* first, EntitySlot* second)
{
    if (first == NULL || second == NULL ||
        !entity_shared_eligibility_check(second, 1) ||
        (second->flags_3e4 & 0x100u) != 0u ||
        (second->flags_3e4 & 2u) != 0u ||
        (first->flags_3e4 & 1u) != 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(first) ||
        !(entity_stats_float_ate8(first) < first->field_340)) return 0;
    lego::pairing_activity_prepare(second);
    entity_state_reset(second, 0, 1);
    entity_state_reset(first, 0, 1);
    ai_set_activity(first, 0x25, 1);
    ai_idle_chain_record_attach(first);
    ai_set_activity(second, 0x26, 1);
    ai_idle_chain_record_attach(second);
    first->flags_3e4 |= 1u;
    second->flags_3e4 |= 2u;
    first->pair_link_3c0 = (uintptr_t)second;
    second->pair_link_3c0 = (uintptr_t)first;
    return 1;
}

// TypeTwoEntityTargetResolveAndApply
// FUNCTION: LEGORR 0x0044b4c0
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code)
{
    int group;
    /* The reference treats an unmapped code, and every non-minifigure,
     * permissively. A mapped code is a permitted-activity group id. */
    if (entity == NULL || entity->type != 2) return 1;
    if (!entity_grouped_code_table_find_value_index(code, &group)) return 1;
    return ai_activity_scan(entity, group);
}

// ObjectPositionStepCommandIssueAndLink
// FUNCTION: LEGORR 0x0044b500
int ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
    EntitySlot* entity, const float target_position[2], uint32_t flags)
{
    int target_xy[2];
    LevelState* level;
    if (entity == NULL || target_position == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    target_xy[0] = level_cell_of(target_position[0]);
    target_xy[1] = level_cell_of(target_position[1]);
    return entity_route_to_cell_commit(entity, target_xy[0], target_xy[1], flags);
}

// EntitySceneObjectTreeAttach
typedef int (*EntitySceneTreeAttachHook)(void* context, EntitySlot* entity,
                                        void* scene_object, void* parent);
// FUNCTION: LEGORR 0x0044b680
int ledger_stub_0044b680_EntitySceneObjectTreeAttach(
    EntitySlot* entity, void* scene_object, void* parent,
    EntitySceneTreeAttachHook attach, void* context)
{
    if (entity == NULL || scene_object == NULL || attach == NULL) return 0;
    return attach(context, entity, scene_object, parent);
}

/* Adapter only: EntityMotionToSceneNodeSync (FUN_0044bc30) takes an entity
 * and delta, and drives its registered scene node. The additional hooks
 * object has no installed production owner in this port. */
void ledger_stub_0044bc30_EntityMotionToSceneNodeSync(
    EntitySlot* entity, float delta,
    const EntityMotionSceneSyncHooks* hooks)
{
    entity_motion_to_scene_node_sync(entity, delta, hooks);
}

/* Adapter only: EntityAuxiliaryPayloadRelease (FUN_0044be30) takes one
 * entity and operates on its attached node plus a global deferred-release
 * list. The explicit release hooks are not its ABI or installed owner. */
void ledger_stub_0044be30_EntityAuxiliaryPayloadRelease(
    EntitySlot* entity, const EntityAuxiliaryReleaseHooks* hooks)
{
    entity_auxiliary_payload_release(entity, hooks);
}

// FUNCTION: LEGORR 0x0044c1b0
void ledger_stub_0044c1b0_EntityHeadingStepMove(
    EntityHeadingStepState* state, void* entity, float delta,
    const EntityHeadingStepHooks* hooks)
{
    entity_heading_step_move(state, entity, delta, hooks);
}

// DriftOffsetStepApply
void ledger_stub_0044d0d0_DriftOffsetStepApply(float delta)
{
    motion_drift_offset_step(delta);
}

// RandomWaypointTimerStepApply
// FUNCTION: LEGORR 0x0044d280
void ledger_stub_0044d280_RandomWaypointTimerStepApply(
    MotionWaypointTimerState* state, float delta,
    PortLoopRandomRangeHook random_range, void* context)
{
    int i;
    float first_step;
    if (state == NULL || random_range == NULL || (state->flags & 4u) == 0u)
        return;
    if ((state->flags & 8u) == 0u) {
        state->delay -= delta;
        if (state->delay > 0.0f) return;
        state->delay = random_range(context, state->delay_min,
                                    state->delay_max);
        state->duration = random_range(context, state->duration_min,
                                       state->duration_max);
        {
            const float fraction = random_range(context, 0.0f, 1.0f);
            for (i = 0; i < 3; ++i) {
                state->target[i] =
                    (float)fabs((double)(state->bounds_max[i] -
                                          state->bounds_min[i])) * fraction +
                    state->bounds_min[i];
                state->progress[i] = 0.0f;
            }
        }
        {
            const float scale = random_range(context, state->scale_min,
                                              state->scale_max);
            for (i = 0; i < 3; ++i)
                state->velocity[i] = state->target[i] / scale;
        }
        state->flags |= 0x18u;
        return;
    }
    if ((state->flags & 0x10u) == 0u) {
        if ((state->flags & 0x20u) == 0u) {
            state->duration -= delta;
            if (state->duration > 0.0f) return;
            for (i = 0; i < 3; ++i)
                state->progress[i] = state->target[i];
            {
                const float scale = random_range(context, state->scale_min,
                                                  state->scale_max);
                for (i = 0; i < 3; ++i)
                    state->velocity[i] = state->target[i] / scale;
            }
            state->flags |= 0x20u;
            return;
        }
        first_step = delta * state->velocity[0];
        for (i = 0; i < 3; ++i) {
            const float step = delta * state->velocity[i];
            state->position[i] -= step;
            state->progress[i] -= step;
        }
        if ((first_step < 0.0f && state->progress[0] >= 0.0f) ||
            (first_step >= 0.0f && state->progress[0] <= 0.0f)) {
            for (i = 0; i < 3; ++i)
                state->position[i] -= state->progress[i];
            state->flags &= ~0x38u;
        }
        return;
    }
    first_step = delta * state->velocity[0];
    for (i = 0; i < 3; ++i) {
        const float step = state->velocity[i] * delta;
        state->position[i] += step;
        state->progress[i] += step;
    }
    if ((first_step < 0.0f && state->progress[0] <= state->target[0]) ||
        (first_step >= 0.0f && state->progress[0] >= state->target[0])) {
        for (i = 0; i < 3; ++i)
            state->position[i] += state->target[i] - state->progress[i];
        state->flags &= ~0x10u;
    }
}

// JitterStepApply
// FUNCTION: LEGORR 0x0044d760
void ledger_stub_0044d760_JitterStepApply(
    MotionJitterState* state, float delta,
    PortLoopRandomRangeHook random_range, const MotionJitterHooks* hooks)
{
    float length;
    float candidate[3];
    int i;
    int valid;
    if (state == NULL || random_range == NULL || hooks == NULL ||
        hooks->random_vector == NULL || hooks->candidate_valid == NULL ||
        (state->flags & 0x40u) == 0u) return;
    if ((state->flags & 0x80u) == 0u) {
        state->delay -= delta;
        if (state->delay > 0.0f) return;
        state->delay = random_range(hooks->context, state->delay_min,
                                    state->delay_max);
        state->lifetime = random_range(hooks->context, state->lifetime_min,
                                      state->lifetime_max);
        state->remaining = random_range(hooks->context, state->magnitude_min,
                                       state->magnitude_max);
        if (!hooks->random_vector(hooks->context, state->direction)) return;
        length = (float)sqrt((double)(state->direction[0] * state->direction[0] +
                                      state->direction[1] * state->direction[1] +
                                      state->direction[2] * state->direction[2]));
        if (length == 0.0f) return;
        for (i = 0; i < 3; ++i)
            state->direction[i] = state->direction[i] / length * state->remaining;
        state->flags |= 0x80u;
        return;
    }
    for (i = 0; i < 3; ++i)
        candidate[i] = state->position[i] + state->direction[i];
    valid = hooks->candidate_valid(hooks->context, candidate);
    if (!valid) {
        for (i = 0; i < 3; ++i)
            candidate[i] = state->position[i] - state->direction[i];
        valid = hooks->candidate_valid(hooks->context, candidate);
    }
    if (!valid) {
        state->flags &= ~0x80u;
        return;
    }
    for (i = 0; i < 3; ++i) state->position[i] = candidate[i];
    if (hooks->position_apply != NULL)
        hooks->position_apply(hooks->context, state->position);
    /* The source decrements the sampled magnitude by its sampled lifetime
     * after each accepted position update; delta is only used by the delay. */
    state->remaining -= state->lifetime;
    if (state->remaining <= 0.0f) state->flags &= ~0x80u;
}

// KeyValueEqualsFileLoad
// FUNCTION: LEGORR 0x0044db50
int ledger_stub_0044db50_KeyValueEqualsFileLoad(
    const MotionKeyValueLoadRequest* request, MotionKeyValueTable* table,
    const MotionKeyValueLoadHooks* hooks)
{
    char line[256];
    void* profile = NULL;
    uint32_t i;
    if (request == NULL || table == NULL || hooks == NULL) return 0;
    memset(table, 0, sizeof(*table));
    if (hooks->string_resolve != NULL) {
        table->name_a = hooks->string_resolve(hooks->context,
                                              request->name_a_id);
        table->name_b = hooks->string_resolve(hooks->context,
                                              request->name_b_id);
        if (request->optional_name_id != 0)
            table->optional_name = hooks->string_resolve(
                hooks->context, request->optional_name_id);
        if (request->text_resource_id != 0)
            table->text_resource = hooks->string_resolve(
                hooks->context, request->text_resource_id);
    }
    for (i = 0; i < 4u; ++i) table->values[i] = request->values[i];
    if (hooks->profile_open != NULL)
        profile = hooks->profile_open(hooks->context, request->config,
                                      request->profile_id);
    for (i = 0; i < MOTION_KEY_VALUE_ENTRY_CAPACITY; ++i) {
        char* key = NULL;
        char* value = NULL;
        if (profile == NULL || hooks->profile_line_get == NULL ||
            hooks->line_split == NULL || hooks->string_duplicate == NULL ||
            !hooks->profile_line_get(hooks->context, profile, i, line) ||
            hooks->line_split(hooks->context, line, &key, &value) != 2 ||
            key == NULL || value == NULL) continue;
        table->entries[i].key = hooks->string_duplicate(hooks->context, key);
        if (table->entries[i].key == NULL) continue;
        table->entries[i].direct_value = 0;
        table->entries[i].fallback_value = hooks->integer_parse == NULL
            ? atoi(value) : hooks->integer_parse(hooks->context, value);
    }
    if (hooks->deferred_callback_register != NULL)
        hooks->deferred_callback_register(hooks->context, 0x0044dec0u);
    return 1;
}

uint32_t motion_config_profile_entries_release(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks)
{
    void* profile = NULL;
    uint32_t i;
    uint32_t released = 0u;
    if (entries == NULL) return 0u;
    if (hooks != NULL && hooks->profile_open != NULL)
        profile = hooks->profile_open(hooks->context, config);
    for (i = 0; i < count; ++i) {
        MotionConfigProfileEntry* entry = &entries[i];
        if (entry->key == NULL) continue;
        if (profile != NULL && hooks != NULL && hooks->profile_write != NULL) {
            hooks->profile_write(hooks->context, profile, entry->key, "=",
                entry->direct_value != 0 ? entry->direct_value
                                         : entry->fallback_value);
        }
        if (hooks != NULL && hooks->key_release != NULL)
            hooks->key_release(hooks->context, entry->key);
        entry->key = NULL;
        entry->direct_value = 0;
        entry->fallback_value = 0;
        ++released;
    }
    return released;
}

// EngineLoaderProfileEntriesRelease
// FUNCTION: LEGORR 0x0044dd40
uint32_t ledger_stub_0044dd40_EngineLoaderProfileEntriesRelease(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks)
{
    return motion_config_profile_entries_release(config, entries, count,
                                                 hooks);
}

// TerrainCellMaterialTransitionBegin
/* Adapter only: TerrainCellMaterialTransitionBegin (FUN_0044e6d0) reserves
 * and fills one of ten live transition slots before calling the material
 * setter. This wrapper performs only the final cell material update. */
void ledger_stub_0044e6d0_TerrainCellMaterialTransitionBegin(
    TerrainSurfaceGrid* grid, int x, int y, unsigned int material,
    unsigned int rotation)
{
    terrain_cell_material_set(grid, (unsigned int)x, (unsigned int)y,
                              (unsigned char)material,
                              (unsigned char)rotation);
}

// SurfaceVertexSpanValidate
int ledger_stub_0044e8c0_SurfaceVertexSpanValidate(
    TerrainSurfaceGrid* grid, const int* cell_xy, unsigned int cell_count,
    float maximum_height_span, float maximum_edge_delta)
{
    return terrain_surface_vertex_span_validate(grid, cell_xy, cell_count,
                                                maximum_height_span,
                                                maximum_edge_delta);
}

// TerrainCellListSurfaceHeightClampMutate
void ledger_stub_0044ead0_TerrainCellListSurfaceHeightClampMutate(
    TerrainSurfaceGrid* grid, const int* cell_xy, unsigned int cell_count,
    float maximum_offset)
{
    terrain_cell_surface_height_clamp(grid, cell_xy, cell_count,
                                      maximum_offset);
}

// TerrainCellSurfaceTriangleVerticesSelect
// FUNCTION: LEGORR 0x0044fab0
void ledger_stub_0044fab0_TerrainCellSurfaceTriangleVerticesSelect(
    TerrainSurfaceGrid* grid, float x, float z, unsigned int out_vertices[3])
{
    unsigned int cell_x;
    unsigned int cell_y;
    unsigned int cell_index;
    unsigned int triangle_offset;
    unsigned int corner;
    float fraction_x;
    float fraction_z;
    int flipped;
    const unsigned int* triangle;
    if (grid == NULL || out_vertices == NULL || grid->cell_records == NULL ||
        grid->triangulation_flags == NULL || x < 0.0f || z < 0.0f)
        return;
    cell_x = (unsigned int)x;
    cell_y = (unsigned int)z;
    if (cell_x >= grid->width || cell_y >= grid->height) return;
    cell_index = cell_y * grid->width + cell_x;
    fraction_x = x - (float)cell_x;
    fraction_z = z - (float)cell_y;
    flipped = (grid->triangulation_flags[cell_index] & 1u) != 0;
    if (!flipped)
        triangle_offset = fraction_x + fraction_z < 1.0f ? 0u : 3u;
    else
        triangle_offset = fraction_z <= fraction_x ? 0u : 3u;
    triangle = (const unsigned int*)(grid->cell_records +
                 cell_index * 0x28u + triangle_offset * sizeof(unsigned int));
    for (corner = 0; corner < 3; ++corner)
        out_vertices[corner] = triangle[corner];
}

// TerrainSurfacePointProjectOffset
// FUNCTION: LEGORR 0x0044fb90
int ledger_stub_0044fb90_TerrainSurfacePointProjectOffset(
    TerrainSurfaceGrid* grid, float x, float z, int orientation,
    float offset, float* out_x, float* out_z)
{
    return terrain_surface_point_project_offset(grid, x, z, orientation,
                                                offset, out_x, out_z);
}

// ScreenRayTerrainCellPick
// FUNCTION: LEGORR 0x004502e0
int ledger_stub_004502e0_ScreenRayTerrainCellPick(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, uint32_t depth_iterations,
    int* out_cell_x, int* out_cell_y, float out_hit_xyz[3])
{
    return screen_ray_terrain_cell_pick(grid, ray_hooks, screen_x, screen_y,
                                        depth_iterations, out_cell_x,
                                        out_cell_y, out_hit_xyz);
}

// TerrainCellRayHitTest
// FUNCTION: LEGORR 0x004515e0
int ledger_stub_004515e0_TerrainCellRayHitTest(
    TerrainSurfaceGrid* grid, int cell_x, int cell_y, const float origin[3],
    const float direction[3], float hit[3])
{
    if (cell_x < 0 || cell_y < 0) return 0;
    return terrain_cell_ray_hit_test(grid, (uint32_t)cell_x,
                                     (uint32_t)cell_y, origin, direction, hit);
}



// TutorialNerpScriptFileLoad
int ledger_stub_00452df0_TutorialNerpScriptFileLoad(const char* path)
{
    return tutorial_script_file_load(path);
}

// ObjectiveAdvisorTokenLoad
int ledger_stub_00452e70_ObjectiveAdvisorTokenLoad(
    uint32_t source_limit, char** output_tokens)
{
    return tutorial_objective_token_load(source_limit, output_tokens);
}

// AdvisorDisplayStateResourcesRelease
int ledger_stub_00453220_AdvisorDisplayStateResourcesRelease()
{
    return tutorial_display_resources_release();
}

// IndirectHandleResolveInPlace
void ledger_stub_004532e0_IndirectHandleResolveInPlace(uint32_t* value)
{
    tutorial_indirect_handle_resolve(value);
}

// TutorialScriptCommandDispatch
void ledger_stub_00453320_TutorialScriptCommandDispatch(float delta)
{
    tutorial_script_command_dispatch(delta);
}


// AdvisorScrollStepAdvance
void ledger_stub_00453920_AdvisorScrollStepAdvance()
{
    tutorial_advisor_scroll_step_advance();
}


// CameraFocusTweenUpdate
void ledger_stub_00453da0_CameraFocusTweenUpdate(float delta)
{
    tutorial_camera_focus_tween_update(delta);
}

// SoundAliasPlayGlobal
// AdvisorInlineTokenRender
void ledger_stub_00456830_AdvisorInlineTokenRender(
    char* source, char* output, int draw)
{
    tutorial_inline_token_render(source, output, draw);
}

// CoordinateListCallbackVisit
// FUNCTION: LEGORR 0x00456cb0
void ledger_stub_00456cb0_CoordinateListCallbackVisit(
    void* list_key, CoordinateListVisitCallback callback, void* user)
{
    coordinate_list_callback_visit(list_key, callback, user);
}

// InputDeviceRelease
void ledger_stub_004588e0_InputDeviceRelease()
{
    input_device_release();
}

// PopEntityStateOverride
int ledger_stub_00459240_PopEntityStateOverride(EntitySlot* entity)
{
    return entity_state_override_pop(entity);
}

// RenderThreePassUpdate
// FUNCTION: LEGORR 0x00459530
void ledger_stub_00459530_RenderThreePassUpdate(BlockFile* values,
                                                 BlockFile* names)
{
    legacy_health_bar_config_load(values, names);
    legacy_hunger_images_position_config_load(values, names);
    legacy_bubble_images_position_config_load(values, names);
}

// HealthBarConfigLoadAndShadeDerive
int ledger_stub_00459560_HealthBarConfigLoadAndShadeDerive(
    BlockFile* values, BlockFile* names)
{
    return legacy_health_bar_config_load(values, names);
}

// HungerImagesPositionConfigLoad
int ledger_stub_00459900_HungerImagesPositionConfigLoad(
    BlockFile* values, BlockFile* names)
{
    return legacy_hunger_images_position_config_load(values, names);
}

// LoadObjectHungerMeterImages
void ledger_stub_004599c0_LoadObjectHungerMeterImages(BlockFile* values,
                                                       BlockFile* names)
{
    legacy_hunger_images_load(values, names);
}

// BubbleImagesPositionConfigLoad
// FUNCTION: LEGORR 0x00459a50
int ledger_stub_00459a50_BubbleImagesPositionConfigLoad(
    BlockFile* values, BlockFile* names)
{
    return legacy_bubble_images_position_config_load(values, names);
}

// RenderEntityScreenRing
void ledger_stub_00459b00_RenderEntityScreenRing(void* entity, int x, int y)
{
    legacy_entity_screen_ring_render(entity, x, y);
}

// UiPanelNameTablesInit
void ledger_stub_0045a030_UiPanelNameTablesInit()
{
    ui_panel_name_tables_init();
}

// ScrollPanelControlsInitialize
void ledger_stub_0045a240_ScrollPanelControlsInitialize()
{
    legacy_scroll_panel_controls_initialize();
}

// PanelCategoryInteractionStateReset
void ledger_stub_0045a270_PanelCategoryInteractionStateReset()
{
    legacy_panel_category_interaction_state_reset();
}

// OverlayImageRecordLoad
void ledger_stub_0045a2e0_OverlayImageRecordLoad(BlockFile* cfg, int panel,
                                                  const char* path)
{
    legacy_overlay_image_record_load(cfg, panel, path);
}

// CaseInsensitiveStringTableLookup
int ledger_stub_0045a370_CaseInsensitiveStringTableLookup(
    const char* name, int* out_index)
{
    return ui_panel_name_table_lookup(name, out_index);
}

// PanelSlotCoordinateHitTest
int ledger_stub_0045a3b0_PanelSlotCoordinateHitTest(int panel, int x, int y)
{
    return legacy_panel_slot_coordinate_hit_test(panel, x, y);
}

// CaseInsensitiveIndexedStringLookup
int ledger_stub_0045a410_CaseInsensitiveIndexedStringLookup(
    unsigned int table_index, const char* name, unsigned int* out_index)
{
    return ui_panel_indexed_name_lookup(table_index, name, out_index);
}

// PanelImageSlotEntryAllocate
void* ledger_stub_0045a500_PanelImageSlotEntryAllocate(
    int panel, const char* path, const int rect[4], uint32_t flags)
{
    return legacy_panel_image_slot_entry_allocate(panel, path, rect, flags);
}

// ScrollingAdvisorTextAppend
// FUNCTION: LEGORR 0x0045a590
void ledger_stub_0045a590_ScrollingAdvisorTextAppend(void* record,
                                                      const char* text)
{
    legacy_scrolling_advisor_text_append(record, text);
}

// UITextOutputPositionAndDraw
// FUNCTION: LEGORR 0x0045a5b0
void ledger_stub_0045a5b0_UITextOutputPositionAndDraw(
    void* record, uint32_t start_line, float line_scale)
{
    void* output;
    void* offsets;
    int base_x, base_y;
    float offset_x, offset_y;
    int x, y;
    if (record == NULL) return;
    memcpy(&output, (char*)record, sizeof(output));
    memcpy(&base_y, (char*)record + 4, sizeof(base_y));
    memcpy(&base_x, (char*)record + 8, sizeof(base_x));
    memcpy(&offsets, (char*)record + 12, sizeof(offsets));
    if (output == NULL || offsets == NULL) return;
    memcpy(&offset_y, (char*)offsets + 0x18, sizeof(offset_y));
    memcpy(&offset_x, (char*)offsets + 0x1c, sizeof(offset_x));
    x = (int)floor((double)((float)base_x + offset_x));
    y = (int)floor((double)((float)base_y + offset_y));
    float_pair_at_offset4_set_from_ints(output, x, y);
    (void)text_buffer_draw_wrapped(output, start_line, line_scale, NULL);
}

// SimCursorInteractionTick
// FUNCTION: LEGORR 0x0045adb0
int ledger_stub_0045adb0_SimCursorInteractionTick(
    const SimCursorInteractionHooks* hooks, int x, int y,
    unsigned int action)
{
    return sim_cursor_interaction_tick(hooks, x, y, action);
}

// SimCategoryCellHitTest
// FUNCTION: LEGORR 0x0045b310
int ledger_stub_0045b310_SimCategoryCellHitTest(
    const SimCategoryHitEntry* entries, uint32_t count, int x, int y,
    int* category_out, int* item_out)
{
    return sim_category_cell_hit_test(entries, count, x, y,
                                      category_out, item_out);
}



// PanelRotationControlConfigLoad
// FUNCTION: LEGORR 0x0045b9d0
int ledger_stub_0045b9d0_PanelRotationControlConfigLoad(
    PanelRotationControlState* state,
    const PanelRotationControlHooks* hooks)
{
    return panel_rotation_control_config_load(state, hooks);
}

// CameraDragFromCursorApply
// FUNCTION: LEGORR 0x0045bcd0
int ledger_stub_0045bcd0_CameraDragFromCursorApply(
    CameraDragState* state, const CameraDragHooks* hooks, int x, int y,
    float delta)
{
    return camera_drag_from_cursor_apply(state, hooks, x, y, delta);
}

// PairCountHistogramBuild
// FUNCTION: LEGORR 0x0045c0d0
void ledger_stub_0045c0d0_PairCountHistogramBuild(
    const uint32_t pair_values[12], uint32_t counts[2][6],
    uint32_t dominant[2])
{
    pair_count_histogram_build(pair_values, counts, dominant);
}

// UiOverlayAnimationApply
// FUNCTION: LEGORR 0x0045c5f0
void ledger_stub_0045c5f0_UiOverlayAnimationApply(
    const UiOverlayAnimationState* state,
    void (*draw)(void*, void*, int, const float*, const float*),
    void* context)
{
    ui_overlay_animation_apply(state, draw, context);
}

// AdvisorAnimationSceneLookupAndRequest
// FUNCTION: LEGORR 0x0045c670
int ledger_stub_0045c670_AdvisorAnimationSceneLookupAndRequest(
    int scene, int animation, const AdvisorAnimationRequestHooks* hooks)
{
    return advisor_animation_scene_lookup_and_request(scene, animation,
                                                       hooks);
}


// PointerCursorConfigLoad
// FUNCTION: LEGORR 0x0045ca70
uint32_t ledger_stub_0045ca70_PointerCursorConfigLoad(
    PointerCursorState* state, void* section, const PointerCursorHooks* hooks)
{
    return pointer_cursor_config_load(state, section, hooks);
}

// PointerCursorConfigParse
// FUNCTION: LEGORR 0x0045cbd0
int ledger_stub_0045cbd0_PointerCursorConfigParse(
    const PointerCursorState* state, const char* name, int* index_out)
{
    return pointer_cursor_name_resolve(state, name, index_out);
}

// PointerCursorDraw
// FUNCTION: LEGORR 0x0045cc70
void ledger_stub_0045cc70_PointerCursorDraw(
    PointerCursorState* state, const PointerCursorHooks* hooks,
    uint32_t x, uint32_t y)
{
    pointer_cursor_draw(state, hooks, x, y);
}

// PriorityImagesConfigLoad
uint32_t ledger_stub_0045cdc0_PriorityImagesConfigLoad(
    PriorityConfigState* state, const PriorityImageConfigRow* rows,
    uint32_t count)
{
    return priority_images_config_load(state, rows, count);
}


// PrioritiesConfigLoad
uint32_t ledger_stub_0045cf50_PrioritiesConfigLoad(
    PriorityConfigState* state, const PriorityConfigRow* rows,
    uint32_t count, const PriorityConfigHooks* hooks)
{
    return priorities_config_load(state, rows, count, hooks);
}

// PrioritiesImagePositionsConfigLoad
uint32_t ledger_stub_0045d060_PrioritiesImagePositionsConfigLoad(
    PriorityConfigState* state, const PriorityImagePositionRow* rows,
    uint32_t count)
{
    return priorities_image_positions_config_load(state, rows, count);
}

// RegionContainmentScanUpdate
// FUNCTION: LEGORR 0x0045d110
int ledger_stub_0045d110_RegionContainmentScanUpdate(
    RegionContainmentEntry* entries, uint32_t count, int x, int y,
    void (*changed)(void*, int, int), void* context)
{
    return region_containment_scan_update(entries, count, x, y, changed,
                                          context);
}

// LevelIdentifierRemapTableLoad
// FUNCTION: LEGORR 0x0045d7e0
void ledger_stub_0045d7e0_LevelIdentifierRemapTableLoad(
    const uint32_t* identifier_pairs, uint32_t pair_count)
{
    level_identifier_remap_table_install(identifier_pairs, pair_count);
}

// TerrainViewTileGeometryBuild
/* Adapter only: TerrainViewTileGeometryBuild (FUN_0045dbc0) consumes global
 * render-view and terrain state. The explicit grid callback seam is not its
 * zero-argument ABI or complete geometry owner. */
void ledger_stub_0045dbc0_TerrainViewTileGeometryBuild(TerrainViewTileGrid* grid)
{
    terrain_view_tile_geometry_build(grid);
}

// TerrainTileStyleResolve
// FUNCTION: LEGORR 0x0045e9e0
int ledger_stub_0045e9e0_TerrainTileStyleResolve(
    void* output, uint32_t x, uint32_t y, uint32_t style, void* context,
    TerrainTileStyleHook resolver)
{
    return terrain_tile_style_resolve(output, x, y, style, context, resolver);
}

/* Adapter only: StartLevelSelected (FUN_00460220) operates on the selected
 * level and global game state with no arguments. This explicit GameContext
 * seam currently only performs the separate checkpoint query from FUN_00460200. */
void ledger_stub_00460220_StartLevelSelected(GameContext* ctx)
{
    selected_level_start_object_update(ctx);
}

// FUNCTION: LEGORR 0x00460290
uint32_t ledger_stub_00460290_EntityType4IndexListBuild(
    GameContext* ctx, EntitySlot** entries, uint32_t capacity)
{
    int i, total;
    uint32_t count = 0;
    if (ctx == NULL || (entries == NULL && capacity != 0u)) return 0;
    total = entity_count(ctx, 4);
    for (i = 0; i < total && count < capacity; ++i) {
        const EntitySlot* entity = entity_at(ctx, 4, i);
        if (entity != NULL) entries[count++] = (EntitySlot*)entity;
    }
    return count;
}

// FUNCTION: LEGORR 0x00460310
int ledger_stub_00460310_EntityType4IndexListAppend(
    EntitySlot** entries, uint32_t* count, uint32_t capacity,
    EntitySlot* entity)
{
    if (entries == NULL || count == NULL || entity == NULL
        || entity->type != 4 || *count >= capacity) return 0;
    entries[*count] = entity;
    ++*count;
    return 1;
}

int ledger_stub_00460360_CheckpointBlobTaskProgressRefresh()
{
    return mission_checkpoint_task_progress_refresh_bound();
}

int ledger_stub_00460f00_RewardStatsPanelDraw(void)
{
    return reward_stats_panel_draw_bound();
}

int ledger_stub_00461070_RewardStatsTextFormat(void)
{
    return reward_stats_text_format_bound();
}

int ledger_stub_00461410_RewardScreenTick(void)
{
    return reward_screen_tick_bound();
}

void ledger_stub_00461790_RewardDisplayTick(GameTask* task,
                                            uint32_t draw_mask,
                                            uint32_t task_number,
                                            float elapsed)
{
    reward_display_task_abi(task, draw_mask, task_number, elapsed);
}

/* Portable framebuffer text adapter; the retail address expects task-count
 * and elapsed-time inputs and is implemented in mission.cpp. */
void ledger_stub_00461c90_RewardScoreTaskTextDraw(
    const TextureSurface* font, void* pixels, int pitch, int width, int height,
    int x, int y, const char* text)
{
    if (text != NULL)
        reward_text_draw(font, pixels, pitch, width, height, x, y, "%s", text);
}

void ledger_stub_00462390_RewardScreenTransitionTick(
    int* displayed_task_count, int* transition_stage, int* score_text_gate,
    float* elapsed)
{
    reward_screen_transition_tick_abi(displayed_task_count, transition_stage,
                                      score_text_gate, elapsed);
}

// FUNCTION: LEGORR 0x00462460
int ledger_stub_00462460_RewardFlhContextReload(Flic** player,
                                                 const char* path)
{
    Flic* opened;
    if (player == NULL) return 0;
    if (*player != NULL) flic_close(*player);
    *player = NULL;
    opened = flic_open(path);
    *player = opened;
    return opened != NULL;
}

void ledger_stub_004624a0_RewardFlhContextTick(int* displayed_task_count,
                                                int* transition_stage,
                                                float* elapsed)
{
    reward_flic_context_tick_abi(displayed_task_count, transition_stage,
                                 elapsed);
}

// FUNCTION: LEGORR 0x00462600
int ledger_stub_00462600_RewardTaskTextDrawAndScoreGate(
    GameContext* ctx, int task_count, int* score_gate,
    const TextureSurface* font, void* pixels, int pitch, int width, int height,
    int x, int y, int line_height, const char* const task_text[10])
{
    int i, limit;
    if (score_gate != NULL) *score_gate = 0;
    if (ctx == NULL || ctx->checkpoint_blob == NULL || task_text == NULL)
        return 0;
    if (task_count < 0) task_count = 0;
    limit = task_count > 10 ? 10 : task_count;
    if (score_gate != NULL && task_count >= 10) *score_gate = 1;
    for (i = 0; i < limit; ++i) {
        if (task_text[i] != NULL)
            reward_text_draw(font, pixels, pitch, width, height, x,
                             y + i * line_height, "%s", task_text[i]);
    }
    return limit;
}

void ledger_stub_00462700_PointerOverrideExpiryTick(
    float delta)
{
    reward_pointer_override_expiry_tick_bound(delta);
}

// RewardScrollTickerAdvanceAndDraw
int ledger_stub_00462ad0_RewardScrollTickerAdvanceAndDraw(
    RewardScrollTickerState* state)
{
    return reward_scroll_ticker_advance_and_draw(state);
}

// TerrainCellQuadGeometryEmit
/* Adapter only: TerrainCellQuadGeometryEmit (FUN_00462f20) takes two
 * coordinates and four separate vertex pointers, using global quad state.
 * The explicit grid plus packed vertex array has a different ABI. */
void ledger_stub_00462f20_TerrainCellQuadGeometryEmit(
    TerrainCellQuadEmitGrid* grid, unsigned int x, unsigned int y,
    const float vertices[4][3])
{
    terrain_cell_quad_geometry_emit(grid, x, y, vertices);
}

// TerrainDirtyCellQuadNormalEmitPass
/* Adapter only: TerrainDirtyCellQuadNormalEmitPass (FUN_00463230) is a
 * zero-argument pass over global dirty-quad state; the pass pointer below is
 * a reconstruction seam without the retail global owner. */
void ledger_stub_00463230_TerrainDirtyCellQuadNormalEmitPass(
    TerrainDirtyQuadNormalPass* pass)
{
    terrain_dirty_quad_normal_emit_pass(pass);
}

// TerrainCellSurfaceNormalGet
// FUNCTION: LEGORR 0x00463590
int ledger_stub_00463590_TerrainCellSurfaceNormalGet(uint32_t x, uint32_t y,
                                                      float normal[3])
{
    return terrain_cell_surface_normal_get(x, y, normal);
}

/* Adapter only: InfoPanelStringEntriesInitialize (FUN_004636b0) takes four
 * fixed arguments followed by a variable stack record list and writes a
 * global 0x20-byte table. This explicit table/array resolver has a different
 * ABI and no live global table owner. */
uint32_t ledger_stub_004636b0_InfoPanelStringEntriesInitialize(
    UiInfoPanelStringTable* table, const UiInfoPanelStringInit* entries,
    uint32_t count, void* context, UiInfoPanelStringResolveHook resolve)
{
    return ui_info_panel_string_entries_initialize(table, entries, count,
                                                   context, resolve);
}

// SimTrackCursorHitTest
// FUNCTION: LEGORR 0x004638a0
int ledger_stub_004638a0_SimTrackCursorHitTest(
    UiTrackCursorState* state, int x, int y, int mode, int allow_mode_change,
    const UiTrackCursorHitHooks* hooks, unsigned int* slot_index_out)
{
    return ui_sim_track_cursor_hit_test(state, x, y, mode,
                                        allow_mode_change, hooks,
                                        slot_index_out);
}

// SceneQuadGridEmit
// FUNCTION: LEGORR 0x00463f00
SceneQuadGridRecord* scene_quad_grid_emit(
    void* scene, uint32_t parameter, const SceneQuadGridEmitHooks* hooks)
{
    static const uint32_t base_indices[6] = { 0u, 1u, 3u, 1u, 2u, 3u };
    uint32_t indices[30];
    SceneQuadGridRecord* record;
    uint32_t face;
    if (hooks == NULL || hooks->mesh_create == NULL ||
        hooks->mesh_record_allocate_or_append == NULL ||
        hooks->face_vertices_get == NULL ||
        hooks->face_vertices_emit == NULL ||
        hooks->face_texture_bind_by_mode == NULL ||
        hooks->scene_render_lock_toggle == NULL)
        return NULL;
    record = (SceneQuadGridRecord*)malloc(sizeof(*record));
    if (record == NULL) return NULL;
    record->mesh = NULL;
    record->parameter = parameter;
    record->mesh = hooks->mesh_create(hooks->context, scene, 3);
    if (record->mesh == NULL) {
        free(record);
        return NULL;
    }
    for (face = 0; face < 5u; ++face) {
        uint32_t i;
        for (i = 0; i < 6u; ++i)
            indices[face * 6u + i] = base_indices[i] + face * 4u;
    }
    /* FUN_00463f00 creates one shared 20-vertex, 10-row mesh record before
     * populating the ten face groups. */
    hooks->mesh_record_allocate_or_append(hooks->context, record->mesh,
                                           20u, 10u, 3u, indices);
    for (face = 0; face < 10u; ++face) {
        uint32_t quad;
        for (quad = 0; quad < 5u; ++quad) {
            float vertices[4][9];
            if (!hooks->face_vertices_get(hooks->context, face, quad,
                                           vertices) ||
                !hooks->face_vertices_emit(hooks->context, record->mesh,
                                           face, quad, vertices)) goto fail;
        }
        hooks->face_texture_bind_by_mode(hooks->context, record->mesh,
                                         face, 3u);
    }
    hooks->scene_render_lock_toggle(hooks->context, record->mesh, 1);
    return record;
fail:
    if (hooks->mesh_release != NULL)
        hooks->mesh_release(hooks->context, record->mesh);
    free(record);
    return NULL;
}

SceneQuadGridRecord* ledger_stub_00463f00_SceneQuadGridEmit(
    void* scene, unsigned int parameter, const SceneQuadGridEmitHooks* hooks)
{
    return scene_quad_grid_emit(scene, parameter, hooks);
}

/* Adapter only: BuildingPlacementFootprintRenderAndValidate (FUN_004641c0)
 * consumes the selected object, placement state, global terrain, and render
 * layer in seven arguments. This explicit surface/hooks seam is not its ABI. */
const int* ledger_stub_004641c0_BuildingPlacementFootprintRenderAndValidate(
    TerrainSurfaceGrid* surface, const int offset[2], const int* source_xy,
    unsigned int count, int rotation, float maximum_height_span,
    float maximum_edge_delta, unsigned int minimum_routes,
    const BuildingFootprintPreviewHooks* hooks)
{
    return building_footprint_render_and_validate(
        surface, offset, source_xy, count, rotation, maximum_height_span,
        maximum_edge_delta, minimum_routes, hooks);
}

// FUNCTION: LEGORR 0x00465190
void ledger_stub_00465190_SoundHandleStopForward(int buffer_handle)
{
    sound_buffer_stop_handle(buffer_handle);
}

void ledger_stub_004653a0_TransientEffectTexturesPreload(
    const char* filename_prefix, const char* filename_format,
    uint32_t texture_count)
{
    transient_effect_textures_preload(filename_prefix, filename_format,
                                      texture_count);
}

// FUNCTION: LEGORR 0x00465430
void* ledger_stub_00465430_TransientEffectEmitterCreate(
    int scene_context, uint32_t position_x, uint32_t position_y,
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

// FUNCTION: LEGORR 0x00465f40
void ledger_stub_00465f40_InstanceQuadVerticesBuild(
    void* mesh, float radius, const float start[3], const float end[3],
    float red, float green, float blue, float alpha)
{
    mesh_build_eight_sided_segment(mesh, radius, start, end, red, green,
                                    blue, alpha);
}

/* ExpandCellEntitySpawn (FUN_00466250) is intentionally not mapped to
 * CreateCellObjectIfAvailable: its decompile creates a type-12 entity,
 * records it on the voxel, sets +0x3e0 bit 0x100, and starts "Expand".
 * The port has no production cell-eligibility/world-position/create/action
 * binding for that sequence yet. */
int ledger_stub_00466250_ExpandCellEntitySpawn(int x, int y)
{
    return create_cell_object_if_available(x, y);
}

// FUNCTION: LEGORR 0x00466490
int ledger_stub_00466490_CellSightRayCheckAndFlag(
    const EntitySlot* observer, const EntitySlot* target,
    uint32_t* visibility_flags)
{
    int visible = !entity_pair_ray_hit_check(observer, target);
    if (visibility_flags != NULL) {
        if (visible) *visibility_flags |= 1u;
        else *visibility_flags &= ~1u;
    }
    return visible;
}

unsigned int ledger_stub_004667e0_EntityStatsConfigLoad(
    void* config, void* section, const EntityStatsConfigHooks* hooks)
{
    return entity_stats_config_load(config, section, hooks);
}

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

/* Adapter only: UpgradeLevelTableParse (FUN_0046c160) receives the live
 * table record and two config owners, then parses the sixteen Upgrades level
 * groups. This hook-driven port table does not match that record/config ABI. */
void ledger_stub_0046c160_UpgradeLevelTableParse(
    UpgradeLevelTable* table, void* context,
    UpgradeLevelEntryReadHook read_entry)
{
    upgrade_level_table_parse(table, context, read_entry);
}


// VehicleTypeDefinitionParse
int ledger_stub_0046c6f0_VehicleTypeDefinitionParse(
    VehicleTypeDefinition* definition, void* context,
    VehicleTypeFieldReadHook read_field)
{
    return vehicle_type_definition_parse(definition, context, read_field);
}

/* Adapter only: AttachedModelOrientationUpdate (FUN_0046d380) takes a
 * render record, four callback arguments, and directly mutates attached scene
 * nodes. This compact state/apply-hook model is not its ABI. */
int ledger_stub_0046d380_AttachedModelOrientationUpdate(
    AttachedModelOrientationState* state, void* context,
    AttachedModelOrientationApplyHook apply)
{
    return attached_model_orientation_update(state, context, apply);
}

// CellQuadRunStage
// FUNCTION: LEGORR 0x0046db90
void ledger_stub_0046db90_CellQuadRunStage(
    LevelState* level, void* context, TerrainCellTypeReadHook read_cell_type,
    TerrainCellQuadRunFlushHook flush_run,
    TerrainCellQuadRunFinishHook finish)
{
    terrain_cell_quad_run_stage(level, context, read_cell_type, flush_run,
                                finish);
}

// StagedCellCandidateSelect
void ledger_stub_0046dd10_StagedCellCandidateSelect(
    void* unused, LevelState* level)
{
    staged_cell_candidate_select(unused, level);
}

/* Adapter only: CellQuadRunEmit (FUN_0046de80) takes the scene and level
 * owners, then consumes the global cell-quad run table. This explicit
 * records/count/hooks entry has no production run-table owner. */
void ledger_stub_0046de80_CellQuadRunEmit(
    LevelState* level, TerrainCellQuadRunRecord* records,
    unsigned int record_count, const TerrainCellQuadRunEmitHooks* hooks)
{
    terrain_cell_quad_run_emit(level, records, record_count, hooks);
}

// CellSurfaceQuadHeightFill
// FUNCTION: LEGORR 0x0046e220
int ledger_stub_0046e220_CellSurfaceQuadHeightFill(
    TerrainSurfaceGrid* surface, TerrainCellQuadRunRecord* records,
    unsigned int record_count, unsigned int x, unsigned int y,
    const TerrainCellQuadSlotMutationHooks* hooks)
{
    return terrain_cell_surface_quad_height_fill(surface, records,
                                                  record_count, x, y, hooks);
}

// StagedCellCandidateCollect
void* ledger_stub_0046e9d0_StagedCellCandidateCollect(
    unsigned int row, unsigned int first_cell, unsigned int end_cell)
{
    return staged_cell_candidate_collect(row, first_cell, end_cell);
}

// EntityIndexedFloatLookupWithMinusOneFallback
float ledger_stub_0046f1d0_EntityIndexedFloatLookupWithMinusOneFallback(
    int weapon_type, const void* entity_record)
{
    return lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity_record);
}

// TypeMotionScaleApplyDispatch
// FUNCTION: LEGORR 0x0046f270
int ledger_stub_0046f270_TypeMotionScaleApplyDispatch(
    EntitySlot* entity, int weapon_type, int apply_index, float scale,
    void* finish_argument, const TypeMotionScaleHooks* hooks)
{
    float base_value;
    float entry_scale = 0.0f;
    int entry_value = 0;
    int entry_present = 0;
    if (entity == NULL || hooks == NULL || hooks->motion_apply == NULL)
        return 0;
    base_value = lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity);
    if ((entity_state_flags_get(entity) & 0x00040000u) == 0u ||
        !entity_shared_eligibility_check(entity, 1)) return 0;
    if (hooks->indexed_entry_get != NULL) {
        entry_present = hooks->indexed_entry_get(hooks->context, weapon_type,
                                                 &entry_scale, &entry_value);
    }
    if (entry_present) {
        if (apply_index) {
            hooks->motion_apply(hooks->context, entity,
                                entry_scale * base_value, 1, scale);
            if (hooks->entry_state_store != NULL)
                hooks->entry_state_store(hooks->context, entity, weapon_type,
                                         entry_value);
            if (hooks->finish_apply != NULL)
                hooks->finish_apply(hooks->context, entity, finish_argument, 1);
        }
        else {
            hooks->motion_apply(hooks->context, entity, base_value * scale,
                                1, scale);
        }
        return 1;
    }
    if (entity->type == ENTITY_TYPE_OBJ && hooks->type3_entry_miss != NULL)
        hooks->type3_entry_miss(hooks->context, entity);
    hooks->motion_apply(hooks->context, entity, base_value, 1, scale);
    if (apply_index && hooks->finish_apply != NULL)
        hooks->finish_apply(hooks->context, entity, finish_argument, 1);
    return 1;
}

// RayHitScanAndEffectApply
// FUNCTION: LEGORR 0x0046f3b0
void ledger_stub_0046f3b0_RayHitScanAndEffectApply(
    CombatRayState* state, const CombatRayHooks* hooks)
{
    combat_ray_hit_scan_and_effect_apply(state, hooks);
}

// LaserSpatialSoundAndImpact
// FUNCTION: LEGORR 0x0046ffb0
void ledger_stub_0046ffb0_LaserSpatialSoundAndImpact(
    void* request, int kind, float scale, const float origin[3],
    const float direction[3], const LaserImpactHooks* hooks)
{
    laser_spatial_sound_and_impact(request, kind, scale, origin, direction,
                                   hooks);
}

// EntitySurfaceSegmentCollisionCheck
// FUNCTION: LEGORR 0x00470260
int ledger_stub_00470260_EntitySurfaceSegmentCollisionCheck(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    int check_side, const EntityQuadCollisionHooks* quad_hooks)
{
    if (entity == NULL) return 0;
    if ((entity_state_flags_get(entity) & 0x100u) != 0u) {
        return entity_surface_quad_segment_intersect(
            entity, start_xy, end_xy, NULL, 0, quad_hooks);
    }
    return entity_segment_center_proximity_check(entity, start_xy, end_xy,
                                                 check_side);
}

// EntitySurfaceQuadSegmentIntersect
// FUNCTION: LEGORR 0x004702b0
int ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side,
    const EntityQuadCollisionHooks* hooks)
{
    return entity_surface_quad_segment_intersect(entity, start_xy, end_xy,
                                                 out_xy, check_side, hooks);
}

// VelocitySlotSteerDispatchAndSound
// FUNCTION: LEGORR 0x00471d20
int ledger_stub_00471d20_VelocitySlotSteerDispatchAndSound(
    void* entity, void* argument, float delta)
{
    return entity_velocity_steer_dispatch_and_sound(entity, argument, delta);
}

// DDiSceneObjectDestroy
void ledger_stub_00472a50_DDiSceneObjectDestroy(void* scene_object,
                                                 int mode);

// FUNCTION: LEGORR 0x00472a40
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object)
{
    ledger_stub_00472a50_DDiSceneObjectDestroy(scene_object, 0);
}

// DDiSceneObjectDestroy
// FUNCTION: LEGORR 0x00472a50
void ledger_stub_00472a50_DDiSceneObjectDestroy(void* scene_object, int mode)
{
    ddiscene_object_destroy_with_mode(scene_object, mode);
}

// SceneActionNamePresent
// FUNCTION: LEGORR 0x00473340
uint32_t ledger_stub_00473340_SceneActionNamePresent(
    void* scene_object, void* action_name)
{
    return scene_action_name_present(scene_object, action_name);
}

// DDiSceneLightCreate
// FUNCTION: LEGORR 0x004734f0
void* ledger_stub_004734f0_DDiSceneLightCreate(
    void* owner, uint32_t value_a, uint32_t value_b,
    uint32_t value_c, uint32_t value_d)
{
    return ddiscene_light_create(owner, value_a, value_b, value_c, value_d);
}

// DDiSceneObjectKindModeSetup
// FUNCTION: LEGORR 0x00473560
void* ledger_stub_00473560_DDiSceneObjectKindModeSetup(void* owner, int mode)
{
    return ddiscene_object_kind_mode_setup(owner, mode);
}

// EntityVisualResourceLoad
void* ledger_stub_00473690_EntityVisualResourceLoad(void* scene_object)
{
    return entity_visual_resource_load(scene_object);
}

// ResourceNameRegisterOrQuery
typedef struct PortableResourceTreeNode {
    const char* name;
    void* resource;
    struct PortableResourceTreeNode** children;
    uint32_t child_count;
} PortableResourceTreeNode;
typedef struct PortableResourceNameQuery {
    const char* name;
    void* resource;
    int compare_prefix;
} PortableResourceNameQuery;
static int portable_resource_tree_walk(
    PortableResourceTreeNode* node,
    int (*callback)(PortableResourceTreeNode*, void*), void* context)
{
    uint32_t i;
    int result;
    if (node == NULL) return 0;
    if (callback != NULL && callback(node, context)) return 1;
    for (i = 0; i < node->child_count; ++i) {
        result = portable_resource_tree_walk(node->children[i], callback,
                                             context);
        if (result) return result;
    }
    return 0;
}
static int portable_resource_name_match(PortableResourceTreeNode* node,
                                       void* context)
{
    PortableResourceNameQuery* query = (PortableResourceNameQuery*)context;
    const char* a;
    const char* b;
    if (node == NULL || query == NULL || node->name == NULL ||
        query->name == NULL) return 0;
    a = node->name;
    b = query->name;
    while (*a != '\0' && *b != '\0') {
        int ca = (unsigned char)*a;
        int cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    if (query->compare_prefix) {
        if (*b != '\0') return 0;
    }
    else if (*a != '\0' || *b != '\0') return 0;
    query->resource = node->resource;
    return 1;
}
// FUNCTION: LEGORR 0x00473bc0
void* ledger_stub_00473bc0_ResourceNameRegisterOrQuery(
    PortableResourceTreeNode* root, const char* name, int mode,
    void** resource_out)
{
    PortableResourceNameQuery query;
    if (resource_out != NULL) *resource_out = NULL;
    if (root == NULL || name == NULL ||
        (mode != 0 && mode != 1 && mode != 2)) return NULL;
    query.name = name;
    query.resource = NULL;
    query.compare_prefix = (mode == 2);
    if (!portable_resource_tree_walk(root, portable_resource_name_match,
                                     &query)) return NULL;
    if (mode == 1) {
        if (resource_out != NULL) *resource_out = query.resource;
        return NULL;
    }
    return query.resource;
}

// XfTextureNameFormat
// FUNCTION: LEGORR 0x00473c60
int ledger_stub_00473c60_XfTextureNameFormat(
    char* output, uint32_t output_size, const char* prefix,
    const char* texture_name, uint32_t frame)
{
    int result;
    if (output == NULL || output_size == 0u || texture_name == NULL) return 0;
    if (prefix == NULL) prefix = "";
#if defined(_MSC_VER)
    result = _snprintf(output, output_size - 1u, "%s%s%u", prefix,
                       texture_name, (unsigned int)frame);
#else
    result = snprintf(output, output_size - 1u, "%s%s%u", prefix,
                      texture_name, (unsigned int)frame);
#endif
    output[output_size - 1u] = '\0';
    return result >= 0 && (uint32_t)result < output_size - 1u;
}

typedef void* (*TextureSurfaceCreateHook)(void* context, uint32_t width,
                                          uint32_t height, uint32_t format,
                                          uint32_t flags);
static TextureSurfaceCreateHook s_texture_surface_create_hook;
static void* s_texture_surface_create_context;

void texture_surface_create_hook_set(TextureSurfaceCreateHook hook,
                                     void* context)
{
    s_texture_surface_create_hook = hook;
    s_texture_surface_create_context = context;
}

// FUNCTION: LEGORR 0x004744f0
void* ledger_stub_004744f0_TextureSurfaceCreate(
    uint32_t width, uint32_t height, uint32_t format, uint32_t flags)
{
    if (width == 0u || height == 0u || s_texture_surface_create_hook == NULL)
        return NULL;
    return s_texture_surface_create_hook(s_texture_surface_create_context,
                                         width, height, format, flags);
}

static void* s_interface_forward_sync_items[4];

static void interface_array_forward_sync_slot80(void* object,
                                                 uint32_t* count,
                                                 void** items)
{
    void** vtable;
    typedef void (*Method)(void*, void*, void*);
    if (object == NULL || count == NULL) return;
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[0x80 / 4] == NULL) return;
    ((Method)vtable[0x80 / 4])(object, count, items);
}

static void interface_array_forward_sync_slot(void* object,
                                               unsigned int byte_offset,
                                               void* value)
{
    void** vtable;
    typedef void (*Method)(void*, void*);
    if (object == NULL) return;
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[byte_offset / 4] == NULL) return;
    ((Method)vtable[byte_offset / 4])(object, value);
}

// ResourceListUpdateMatchingKey supplies the cached object, scene record, and
// request word.  The record's kind chooses the active
// interface; slot +0x80 enumerates values into the shared four-entry scratch
// array, and slots +0xa0/+0x48 forward them to the paired interfaces.
// FUNCTION: LEGORR 0x00474760
void ledger_stub_00474760_InterfaceArrayForwardSync(
    void* source_value, void* scene_record, uint32_t request)
{
    unsigned char* record = (unsigned char*)scene_record;
    uint32_t kind;
    uint32_t count;
    uint32_t i;
    void* primary;
    void* secondary;
    void* secondary_child;

    if (record == NULL) return;
    memcpy(&kind, record + 0x10, sizeof(kind));
    memcpy(&primary, record + (kind == 6u ? 0x00 : 0x04), sizeof(primary));
    if (primary == NULL) return;

    if (request != 0u) {
        count = request;
        interface_array_forward_sync_slot80(primary, &count, NULL);
        if (count > 4u) count = 4u;
        if (count != 0u) {
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            if (count > 4u) count = 4u;
            for (i = 0u; i < count; ++i) {
                interface_array_forward_sync_slot(primary, 0xa0,
                    s_interface_forward_sync_items[i]);
            }
        }

        memcpy(&secondary, record + 0x08, sizeof(secondary));
        if (secondary != NULL) {
            uint32_t secondary_count = 0u;
            interface_array_forward_sync_slot80(secondary,
                                                 &secondary_count, NULL);
            if (secondary_count > 4u) secondary_count = 4u;
            if (secondary_count != 0u) {
                interface_array_forward_sync_slot80(secondary,
                    &secondary_count, s_interface_forward_sync_items);
                if (secondary_count > 4u) secondary_count = 4u;
                memcpy(&secondary_child, (unsigned char*)secondary + 0x08,
                       sizeof(secondary_child));
                for (i = 0u; i < secondary_count; ++i) {
                    void* item = s_interface_forward_sync_items[i];
                    interface_array_forward_sync_slot(primary, 0x48, item);
                    interface_array_forward_sync_slot(secondary_child, 0xa0,
                                                       item);
                }
            }
        }
        *(uint32_t*)(record + 0x14) &= ~4u;
        if (source_value != NULL) {
            void* selector_owner = NULL;
            uint32_t selector_owner_flags = 0u;
            void* selector_data = NULL;
            void* selector = NULL;
            memcpy(&selector_owner, (unsigned char*)source_value + 0x0c,
                   sizeof(selector_owner));
            if (selector_owner != NULL) {
                memcpy(&selector_owner_flags,
                       (unsigned char*)selector_owner + 0x08,
                       sizeof(selector_owner_flags));
                if (selector_owner_flags != 0u) {
                    interface_array_forward_sync_slot(primary, 0x48, primary);
                    *(uint32_t*)(record + 0x14) |= 4u;
                }
                else {
                    memcpy(&selector_data,
                           (unsigned char*)selector_owner + 0x0c,
                           sizeof(selector_data));
                    selector = NULL;
                    if (selector_data != NULL)
                        memcpy(&selector,
                               (unsigned char*)selector_data + 0x0c,
                               sizeof(selector));
                    interface_array_forward_sync_slot(primary, 0x48,
                                                       selector);
                    *(uint32_t*)(record + 0x14) |= 4u;
                }
            }
        }
    }
    else {
        count = 0u;
        interface_array_forward_sync_slot80(primary, &count,
                                             NULL);
        if (count > 4u) count = 4u;
        if (count != 0u) {
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            if (count > 4u) count = 4u;
            memcpy(&secondary, record + 0x08, sizeof(secondary));
            secondary_child = NULL;
            if (secondary != NULL)
                memcpy(&secondary_child, (unsigned char*)secondary + 0x08,
                       sizeof(secondary_child));
            for (i = 0u; i < count; ++i) {
                void* item = s_interface_forward_sync_items[i];
                interface_array_forward_sync_slot(primary, 0x48, item);
                interface_array_forward_sync_slot(secondary_child, 0xa0,
                                                   item);
            }
        }
        *(uint32_t*)(record + 0x14) &= ~4u;
    }
}

// MeshRecordAllocateOrAppend
// FUNCTION: LEGORR 0x004748f0
int ledger_stub_004748f0_MeshRecordAllocateOrAppend(
    void* object, uint32_t vertex_count, uint32_t index_rows,
    uint32_t index_columns, const uint16_t* source_indices)
{
    return mesh_record_allocate_or_append(object, vertex_count, index_rows,
                                          index_columns, source_indices);
}

// AnimatedTextureFrameCount
// FUNCTION: LEGORR 0x00474a20
int ledger_stub_00474a20_AnimatedTextureFrameCount(const void* texture)
{ return lego::animated_texture_frame_count(texture); }

// SceneSurfaceTextureBindByMode
// FUNCTION: LEGORR 0x00474a60
void ledger_stub_00474a60_SceneSurfaceTextureBindByMode(
    void* texture, void* frame, int mode)
{ lego::animated_texture_frame_bind(texture, frame, mode); }

// AnimatedTextureFrameResolve
// FUNCTION: LEGORR 0x00474c00
int ledger_stub_00474c00_AnimatedTextureFrameResolve(void* interface_object,
                                                       int frame,
                                                       void** resolved_out)
{ return lego::animated_texture_frame_resolve(interface_object, frame,
                                               resolved_out); }

// AnimatedTextureFrameSet
// FUNCTION: LEGORR 0x00474da0
void ledger_stub_00474da0_AnimatedTextureFrameSet(
    void* texture, uint32_t frame_index, const void* frame_state)
{
    void* owner;
    void* direct_count;
    void* interface_object;
    void* resolved_frame = NULL;
    if (texture == NULL) return;
    memcpy(&owner, (unsigned char*)texture + 0x0c, sizeof(owner));
    if (owner == NULL) return;
    memcpy(&direct_count, (unsigned char*)owner + 0x0c,
           sizeof(direct_count));
    if (direct_count != NULL) {
        if (frame_state != NULL)
            lego::animated_texture_frame_surface_set(direct_count,
                                                       (int)frame_index,
                                                       frame_state);
        return;
    }
    memcpy(&interface_object, (unsigned char*)owner + 8,
           sizeof(interface_object));
    if (interface_object == NULL) return;
    if (lego::animated_texture_frame_resolve(interface_object,
            (int)frame_index, &resolved_frame) && resolved_frame != NULL)
        lego::animated_texture_frame_bind(texture, resolved_frame, 0);
}

// RenderObjectHierarchyFloat3Set
// FUNCTION: LEGORR 0x00474e90
uint32_t ledger_stub_00474e90_RenderObjectHierarchyFloat3Set(
    void* scene_object, float x, float y, float z)
{
    float xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    return render_object_hierarchy_float3_set(scene_object, xyz);
}

// SceneObjectBoundsUnion
uint32_t ledger_stub_00474f10_SceneObjectBoundsUnion(
    void* scene_object, float bounds_min_max[6])
{
    return scene_object_bounds_union(scene_object, bounds_min_max);
}

// RenderObjectSetPackedRgbaFromFloatChannels
void ledger_stub_004750b0_RenderObjectSetPackedRgbaFromFloatChannels(
    void* scene_object, float red, float green, float blue, float alpha)
{
    render_object_set_packed_rgba_from_float_channels(
        scene_object, red, green, blue, alpha);
}

// DDiSceneAdvanceByTime
void ledger_stub_00475120_DDiSceneAdvanceByTime(
    void* scene_object, float delta)
{
    ddiscene_advance_by_time(scene_object, delta);
}

// DDiSceneFrameUpdateAndCallbacks
float ledger_stub_00475140_DDiSceneFrameUpdateAndCallbacks(
    void* scene_object, float time)
{
    return ddiscene_frame_update_and_callbacks(scene_object, time);
}

/* The original boot path registers an SFX sample hook and an entity scene-end
 * hook. These adapters use the port's alias registry and EntitySlot flags;
 * owner-token resolution remains a renderer integration boundary. */
static EntityDDiSceneOwnerResolveHook s_ddiscene_owner_resolve;
static void* s_ddiscene_owner_resolve_context;

static void entity_ddiscene_sample_callback(const char* sample_name,
                                            void* scene_object,
                                            void* context)
{
    int alias;
    (void)context;
    if (sample_name != NULL && scene_object != NULL &&
        sound_alias_lookup(sample_name, &alias)) {
        sound_submission_route_wrapper(scene_object, alias, 0, 1, NULL);
    }
}

static void entity_ddiscene_end_callback(void* scene_object, void* context)
{
    EntitySlot* entity;
    uint32_t owner_token;
    (void)context;
    if (scene_object == NULL || s_ddiscene_owner_resolve == NULL) return;
    owner_token = scene_object_owner_get(scene_object);
    if (owner_token == 0u) return;
    entity = s_ddiscene_owner_resolve(owner_token,
                                     s_ddiscene_owner_resolve_context);
    if (entity != NULL) entity->flags_3e4 |= 0x00010000u;
}

void entity_ddiscene_callbacks_install(
    EntityDDiSceneOwnerResolveHook owner_resolve, void* context)
{
    s_ddiscene_owner_resolve = owner_resolve;
    s_ddiscene_owner_resolve_context = context;
    ddiscene_frame_callbacks_set(entity_ddiscene_sample_callback, NULL, 1,
                                 entity_ddiscene_end_callback, NULL);
}

// DDiSceneApplyActFrameTiming
void ledger_stub_00475300_DDiSceneApplyActFrameTiming(void* scene_object)
{
    ddiscene_apply_act_frame_timing(scene_object);
}

// DDiSceneObjectLengthGet
float ledger_stub_00475390_DDiSceneObjectLengthGet(void* scene_object)
{
    return ddiscene_object_length_get(scene_object);
}

// DDiSceneObjectFramePositionGet
uint32_t ledger_stub_004753f0_DDiSceneObjectFramePositionGet(
    void* scene_object)
{
    return ddiscene_object_frame_position_get(scene_object);
}

// VideoObjectAspectRatioGet
// FUNCTION: LEGORR 0x004756d0
float ledger_stub_004756d0_VideoObjectAspectRatioGet(void* video_object)
{
    return video_object_aspect_ratio_get(video_object);
}

// DDiSceneObjectAttachOrUnlink
void ledger_stub_00475710_DDiSceneObjectAttachOrUnlink(
    void* pair_record, void* source_pair_record)
{
    ddiscene_scene_object_attach_or_unlink(pair_record, source_pair_record);
}

// DDiSceneContextGet
// FUNCTION: LEGORR 0x004757a0
void* ledger_stub_004757a0_DDiSceneContextGet(void* scene_object)
{
    return ddiscene_context_get(scene_object);
}

// TypedChildResourceScalarGet
float ledger_stub_004757f0_TypedChildResourceScalarGet(void* scene_object)
{
    return typed_child_resource_scalar_get(scene_object);
}

// SceneObjectFirstNonKind6ChildGet
void* ledger_stub_00475830_SceneObjectFirstNonKind6ChildGet(
    void* child_interface)
{
    return scene_object_first_non_kind6_child_get(child_interface);
}

// SceneObjectOwnerLinkEnsure
void* ledger_stub_00475880_SceneObjectOwnerLinkEnsure(void* owner_interface)
{
    return scene_object_owner_link_ensure(owner_interface);
}

// DDiSceneObjectArrayGrow
void ledger_stub_004759f0_DDiSceneObjectArrayGrow()
{
    lego::ddiscene_object_array_grow();
}

// SceneActionRecordActFrameScan
unsigned int ledger_stub_00475a70_SceneActionRecordActFrameScan(
    void* scene_object, void** records, int* frame_indices, char** names)
{
    return scene_action_record_act_frame_scan(scene_object, records,
                                              frame_indices, names);
}

// DDiSceneAnimationCallbacksSet
// FUNCTION: LEGORR 0x00475c00
void ledger_stub_00475c00_DDiSceneAnimationCallbacksSet(
    void* scene_object, void* callback_a, void* callback_b,
    void* callback_c, void* callback_d)
{
    ddiscene_animation_callbacks_set(scene_object, callback_a, callback_b,
                                     callback_c, callback_d);
}

// DDiScenePayloadRelease
void ledger_stub_00475c80_DDiScenePayloadRelease(void* scene_object)
{
    ddiscene_payload_release(scene_object);
}

// CallbackRecordSetFlag10AndRefresh
// FUNCTION: LEGORR 0x00475e10
void ledger_stub_00475e10_CallbackRecordSetFlag10AndRefresh(void* property)
{
    callback_record_set_flag10_and_refresh(property);
}

// DDiSceneFrameRecordFind
// FUNCTION: LEGORR 0x00475e40
void* ledger_stub_00475e40_DDiSceneFrameRecordFind(
    void* action_interface, const char* action_name, int selector)
{
    return ddiscene_frame_record_find(action_interface, action_name, selector);
}

// DDiSceneAnimationHookCreate
void ledger_stub_00475f70_DDiSceneAnimationHookCreate(
    void* scene_object, int callback, const char* name,
    const uint32_t* value_0c, int value_08, const uint32_t* value_10,
    const uint32_t* value_14, int value_18, int value_24,
    const uint32_t* value_20)
{
    ddiscene_animation_hook_create(scene_object, callback, name, value_0c,
                                   value_08, value_10, value_14, value_18,
                                   value_24, value_20);
}

// AttachedObjectDataReleaseAndClear
void ledger_stub_004760e0_AttachedObjectDataReleaseAndClear(void* property)
{
    attached_object_data_release_and_clear(property);
}



// ScenePropertyFloatAt14Get
// FUNCTION: LEGORR 0x004761c0
float ledger_stub_004761c0_ScenePropertyFloatAt14Get(void* property)
{
    return scene_property_float_at14_get(property);
}

// RenderInterfacePairUpdate
// FUNCTION: LEGORR 0x00476220
void ledger_stub_00476220_RenderInterfacePairUpdate(
    void* first, void* second)
{
    render_interface_pair_update(first, second);
}

// SceneObjectSetFormattedString
// FUNCTION: LEGORR 0x00476270
void ledger_stub_00476270_SceneObjectSetFormattedString(
    void* scene_object, const char* format, ...)
{
    va_list arguments;
    char local[1024];
    if (scene_object == NULL || format == NULL) return;
    va_start(arguments, format);
#if defined(_MSC_VER)
    _vsnprintf(local, sizeof(local) - 1, format, arguments);
#else
    vsnprintf(local, sizeof(local) - 1, format, arguments);
#endif
    va_end(arguments);
    local[sizeof(local) - 1] = '\0';
    scene_object_set_formatted_string(scene_object, "%s", local);
}


// ResourceTreeCallbackWalk
// FUNCTION: LEGORR 0x00476330
PortableResourceTreeNode* ledger_stub_00476330_ResourceTreeCallbackWalk(
    PortableResourceTreeNode* root, uint32_t depth,
    int (*callback)(PortableResourceTreeNode*, void*), void* context)
{
    uint32_t i;
    PortableResourceTreeNode* found;
    if (root == NULL || callback == NULL) return NULL;
    if (callback(root, context)) return root;
    if (depth > 64u) return NULL;
    for (i = 0; i < root->child_count; ++i) {
        found = ledger_stub_00476330_ResourceTreeCallbackWalk(
            root->children[i], depth + 1u, callback, context);
        if (found != NULL) return found;
    }
    return NULL;
}

// ResourceNameMatchCallback
// FUNCTION: LEGORR 0x00476410
int ledger_stub_00476410_ResourceNameMatchCallback(
    PortableResourceTreeNode* node, PortableResourceNameQuery* query)
{
    int matched = portable_resource_name_match(node, query);
    if (matched && query != NULL) query->resource = node->resource;
    return matched;
}

// DDiSceneResourceNameResolve
// FUNCTION: LEGORR 0x00476770
char* ledger_stub_00476770_DDiSceneResourceNameResolve(const char* name)
{
    return ddiscene_resource_name_resolve(name);
}

// DDiSceneResourceExists
typedef int (*DdiSceneResourceExistsHook)(const char* name, void* scene,
                                          void* context);
typedef void* (*DdiSceneResourceOpenHook)(const char* name, void* scene,
                                          int optional, void* context);
static DdiSceneResourceExistsHook s_ddiscene_resource_exists_hook;
static DdiSceneResourceOpenHook s_ddiscene_resource_open_hook;
static void* s_ddiscene_resource_hook_context;
void ddiscene_resource_hooks_set(DdiSceneResourceExistsHook exists_hook,
                                 DdiSceneResourceOpenHook open_hook,
                                 void* context)
{
    s_ddiscene_resource_exists_hook = exists_hook;
    s_ddiscene_resource_open_hook = open_hook;
    s_ddiscene_resource_hook_context = context;
}
// FUNCTION: LEGORR 0x004767e0
int ledger_stub_004767e0_DDiSceneResourceExists(const char* name,
                                                 void* scene)
{
    char* resolved;
    if (name == NULL || scene == NULL ||
        s_ddiscene_resource_exists_hook == NULL) return 0;
    resolved = ddiscene_resource_name_resolve(name);
    if (resolved == NULL || resolved[0] == '\0') return 0;
    return s_ddiscene_resource_exists_hook(resolved, scene,
                                           s_ddiscene_resource_hook_context);
}

// DDiSceneResourceOpen
// FUNCTION: LEGORR 0x00476850
void* ledger_stub_00476850_DDiSceneResourceOpen(const char* name,
                                                 void* scene,
                                                 int optional)
{
    char* resolved;
    if (name == NULL || scene == NULL ||
        s_ddiscene_resource_open_hook == NULL) return NULL;
    resolved = ddiscene_resource_name_resolve(name);
    if (resolved == NULL || resolved[0] == '\0') return NULL;
    return s_ddiscene_resource_open_hook(resolved, scene, optional,
                                         s_ddiscene_resource_hook_context);
}



/* Adapter only: retail RenderTargetViewportScaleSet queries the global
 * Direct3DRM device and allocates a bounded screen surface. This explicit
 * dimension/hook function is not its ABI or complete behavior. */
typedef void (*RenderTargetViewportScaleHook)(void* context, void* target,
                                              float scale_x, float scale_y);
int ledger_stub_00476dc0_RenderTargetViewportScaleSet(
    void* target, float viewport_width, float viewport_height,
    float target_width, float target_height,
    RenderTargetViewportScaleHook set_scale, void* context)
{
    float x, y;
    if (target == NULL || set_scale == NULL || viewport_width <= 0.0f ||
        viewport_height <= 0.0f || target_width <= 0.0f ||
        target_height <= 0.0f) return 0;
    x = target_width / viewport_width;
    y = target_height / viewport_height;
    set_scale(context, target, x, y);
    return 1;
}

// FUNCTION: LEGORR 0x00476e50
ScreenBoundedSurfaceRecord*
ledger_stub_00476e50_ScreenBoundedSurfaceResourceAllocate(
    uint32_t width, uint32_t height, void* camera, int x, int y)
{
    return screen_bounded_surface_allocate(width, height, camera, x, y);
}


// DDiSceneDrawBegin
// FUNCTION: LEGORR 0x00477010
void ledger_stub_00477010_DDiSceneDrawBegin(void* render_object, int begin)
{
    ddiscene_draw_begin(render_object, begin);
}




// ComPropertyResultGet
void* ledger_stub_00477310_ComPropertyResultGet(void* resource_owner)
{
    return com_resource_enumeration_result_get(resource_owner);
}

// RenderResourceTrackedReferencesRelease
// FUNCTION: LEGORR 0x004773e0
void ledger_stub_004773e0_RenderResourceTrackedReferencesRelease()
{ lego::scene_resource_tracked_references_release_all(); }

// MainLoopFramePump
// FUNCTION: LEGORR 0x00477f30
void ledger_stub_00477f30_MainLoopFramePump(int mode_transition)
{
    main_loop_frame_pump(mode_transition);
}

// DisplayDeviceCapsQueryDiscard
void ledger_stub_00477f80_DisplayDeviceCapsQueryDiscard()
{
    window_runtime_display_caps_query_discard();
}

// EngineCallbackTripletSet
// FUNCTION: LEGORR 0x00477fd0
int ledger_stub_00477fd0_EngineCallbackTripletSet(const uintptr_t* triplet)
{
    return window_runtime_callback_triplet_set(triplet);
}

// WindowMessageDispatchFilter
// FUNCTION: LEGORR 0x00478010
int ledger_stub_00478010_WindowMessageDispatchFilter(uint32_t message)
{
    return window_runtime_message_filter(message);
}

// WindowRectAdjustForWindowedMode
int ledger_stub_00478310_WindowRectAdjustForWindowedMode(WindowPortRect* rect)
{
    return window_runtime_adjust_rect(rect);
}

// GameWindowProcedure
// FUNCTION: LEGORR 0x004784c0
ptrdiff_t ledger_stub_004784c0_GameWindowProcedure(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    return window_runtime_game_window_proc(window, message, wparam, lparam);
}

// MainWindowActivationAndCloseHandler
// FUNCTION: LEGORR 0x004786c0
ptrdiff_t ledger_stub_004786c0_MainWindowActivationAndCloseHandler(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    return window_runtime_activation_close_proc(window, message, wparam,
                                                lparam);
}

// FUNCTION: LEGORR 0x004789a0
void ledger_stub_004789a0_MusicMixerVolumeGet(float* music, float* sfx)
{
    (void)audio_mixer_control_init(music, sfx, 0);
}

// NestedRecordListRemoveMatchingKey
typedef struct NestedResourceRecord {
    int key;
    unsigned char reserved[12];
    struct NestedResourceRecord* child_or_next;
} NestedResourceRecord;
typedef void (*NestedRecordRemoveHook)(void* owner, int key, void* context);
static NestedRecordRemoveHook s_nested_record_remove_hook;
static void* s_nested_record_remove_context;
void nested_record_remove_hook_set(NestedRecordRemoveHook hook, void* context)
{
    s_nested_record_remove_hook = hook;
    s_nested_record_remove_context = context;
}
// FUNCTION: LEGORR 0x0047b330
int ledger_stub_0047b330_NestedRecordListRemoveMatchingKey(
    void* owner, int key, NestedResourceRecord* record)
{
    NestedResourceRecord* child;
    if (record == NULL) return 0;
    child = record->child_or_next;
    if (ledger_stub_0047b330_NestedRecordListRemoveMatchingKey(
            owner, key, child)) {
        if (s_nested_record_remove_hook != NULL)
            s_nested_record_remove_hook(owner, key,
                                       s_nested_record_remove_context);
        record->child_or_next = child->child_or_next;
        engine_heap_free(child);
    }
    return record->key == key;
}

// FUNCTION: LEGORR 0x0047c100
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on == 0) return rolloff;
    if (rolloff < 0.0f) rolloff = 0.0f;
    if (rolloff > 10.0f) rolloff = 10.0f;
    return audio_listener_rolloff_set(rolloff);
}

// InterfaceMethod18StructApply
// FUNCTION: LEGORR 0x0047e300
int ledger_stub_0047e300_InterfaceMethod18StructApply(
    void* destination, const void* source, size_t size,
    void (*apply)(void* destination, const void* source, size_t size,
                  void* context), void* context)
{
    if (destination == NULL || source == NULL || size == 0u) return 0;
    if (apply != NULL) apply(destination, source, size, context);
    else memcpy(destination, source, size);
    return 1;
}

// InterfaceMethodResultRecordInitialize
// FUNCTION: LEGORR 0x0047e3e0
void ledger_stub_0047e3e0_InterfaceMethodResultRecordInitialize(
    unsigned int* result, void* device, unsigned int interface_value,
    void* context, unsigned int first_color, unsigned int second_color)
{
    if (result == NULL) return;
    if (s_small_record_free == NULL) {
        fixed_size_small_record_pool_grow();
    }
    result[1] = interface_value;
    result[7] = (unsigned int)result;
    result[6] = 0;
    result[2] = (unsigned int)context;
    result[0] = (unsigned int)device;
    result[5] = first_color;
    result[3] = font_device_color_convert(device, first_color);
    result[4] = font_device_color_convert(device, second_color);
}

// ComObjectDestroyAndOptionallyFree
// FUNCTION: LEGORR 0x0047e460
void ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(void* object);
void ledger_stub_0047e460_ComObjectDestroyAndOptionallyFree(void* object,
                                                              unsigned char flags)
{
    if (object == 0) return;
    ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(object);
    if ((flags & 1u) != 0) ui_render_record_free(object);
}

// TwoInterfaceFieldsReleaseAndReset
// FUNCTION: LEGORR 0x0047e480
void ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(void* object)
{
    typedef unsigned long (*ReleaseMethod)(void*);
    unsigned char* bytes = (unsigned char*)object;
    unsigned int* slots;
    void* interface_object;
    void** vtable;
    if (bytes == 0) return;
    slots = (unsigned int*)bytes;
    slots[0] = 0;
    interface_object = *(void**)(bytes + 24);
    if (interface_object != 0) {
        vtable = *(void***)interface_object;
        if (vtable != 0 && vtable[2] != 0) {
            ReleaseMethod release = (ReleaseMethod)vtable[2];
            release(interface_object);
        }
        *(void**)(bytes + 24) = 0;
    }
    interface_object = *(void**)(bytes + 16);
    if (interface_object != 0) {
        vtable = *(void***)interface_object;
        if (vtable != 0 && vtable[2] != 0) {
            ReleaseMethod release = (ReleaseMethod)vtable[2];
            release(interface_object);
        }
        *(void**)(bytes + 16) = 0;
    }
}

// FUNCTION: LEGORR 0x0047e4c0
void ledger_stub_0047e4c0_AviPlaybackStateInitialize(
    AviPlaybackState* state, uint32_t width, uint32_t height,
    uint32_t bits_per_pixel, int fullscreen, void* display_device)
{
    avi_playback_state_initialize(state, width, height, bits_per_pixel,
                                  fullscreen, display_device);
}

int ledger_stub_0047e510_DirectDrawDisplayModeSetup(
    AviPlaybackState* state, uint32_t width, uint32_t height,
    uint32_t bits_per_pixel, int fullscreen, void* display_device)
{
    return avi_display_mode_setup(state, width, height, bits_per_pixel,
                                  fullscreen, display_device);
}

uint32_t ledger_stub_0047e760_AviSurfaceLock(AviPlaybackState* state)
{
    return avi_surface_lock(state);
}

uint32_t ledger_stub_0047e7b0_AviFrameSurfaceUnlock(AviPlaybackState* state)
{
    return avi_surface_unlock(state);
}

void ledger_stub_0047e980_AviPlayerDestroy(AviPlaybackState* state,
                                            uint8_t flags)
{
    avi_player_destroy(state, flags);
}

void ledger_stub_0047e9a0_AviFrameRetrieverDestroy(AviPlaybackState* state)
{
    avi_frame_retriever_destroy(state);
}

// FUNCTION: LEGORR 0x0047e9e0
int ledger_stub_0047e9e0_AviFrameDecodeAndCopy(Movie* movie,
                                               uint8_t* destination,
                                               int width, int height,
                                               int pitch)
{
    MovieOut frame;
    MovieInfo info;
    int result;
    if (movie == NULL || destination == NULL) return -1;
    result = movie_next(movie, &frame);
    if (result != 0) return result;
    if (movie_info(movie, &info) != 0) return -1;
    movie_scale_rgba(frame.rgba, info.width, info.height, frame.rgba_pitch,
                     destination, width, height, pitch);
    return 0;
}

int ledger_stub_0047eb20_AviFrameUpdateAndBlit(AviPlaybackState* state)
{
    return avi_frame_update_and_blit(state);
}

uint32_t ledger_stub_0047ebe0_AviStreamFrameCountGet(
    const AviPlaybackState* state)
{
    return avi_stream_frame_count_get(state);
}

struct AviFrameSeekRebaseMethod {
    void seek(uint32_t frame_index);
};

void AviFrameSeekRebaseMethod::seek(uint32_t frame_index)
{
    avi_frame_seek_and_clock_rebase((AviPlaybackState*)this, frame_index);
}

uint32_t ledger_stub_0047ec60_AviPlaybackFrameIndexGet(
    const AviPlaybackState* state)
{
    return avi_playback_frame_index_get(state);
}

// FUNCTION: LEGORR 0x0047ec80
void ledger_stub_0047ec80_AviFileInitialize(void* context)
{
    avi_file_initialize(context);
}

void ledger_stub_0047e800_AviFrameRetrieverOpen(AviPlaybackState* state,
                                                 const char* path)
{
    avi_frame_retriever_open(state, path);
}

// FUNCTION: LEGORR 0x0047ecf0
uint8_t ledger_stub_0047ecf0_AviFrameUpdate(AviPlaybackState* state)
{
    return (uint8_t)state->frame_update_and_blit();
}

void ledger_stub_0047ed00_AviPlaybackFrameLoop(AviPlaybackState* state,
                                                void* destination)
{
    avi_playback_frame_loop(state, destination);
}

// FUNCTION: LEGORR 0x0047ed40
void ledger_stub_0047ed40_AviFrameIndexAndClockReset(
    AviPlaybackState* state, uint32_t frame_index)
{
    ((AviFrameSeekRebaseMethod*)state)->seek(frame_index);
}

// FUNCTION: LEGORR 0x0047ed50
uint32_t ledger_stub_0047ed50_AviCurrentFrameIndexGet(
    const AviPlaybackState* state)
{
    return state->playback_frame_index_get();
}

// FUNCTION: LEGORR 0x0047ed60
uint32_t ledger_stub_0047ed60_AviStreamFrameCountGet(
    const AviPlaybackState* state)
{
    return state->stream_frame_count_get();
}

// SharedTextureRootRegister
int ledger_stub_004805b0_SharedTextureRootRegister(const char* root)
{
    return lego::shared_texture_root_register(root);
}

// SharedTextureRootInitialize
int ledger_stub_00480650_SharedTextureRootInitialize()
{
    return lego::shared_texture_root_initialize();
}

// FUNCTION: LEGORR 0x00480870
EngineTaskCallbackRecord* ledger_stub_00480870_TaskCallbackRecordAllocateAndInitialize(
    void* manager, void* owner, void* callback, int mode,
    const EngineTaskCallbackHooks* hooks)
{
    return engine_task_callback_record_allocate_and_initialize(
        manager, owner, callback, mode, hooks);
}

// CachedRenderResourceInstanceCreate
typedef struct CachedRenderResourceInstance {
    void* resource;
    void* instance;
    uint32_t generation;
} CachedRenderResourceInstance;
typedef void* (*CachedRenderResourceCreateHook)(void* context,
                                                 void* resource);
// FUNCTION: LEGORR 0x00480900
void* ledger_stub_00480900_CachedRenderResourceInstanceCreate(
    CachedRenderResourceInstance* cache, void* resource,
    CachedRenderResourceCreateHook create, void* context)
{
    void* instance;
    if (cache == NULL || resource == NULL || create == NULL) return NULL;
    if (cache->resource == resource && cache->instance != NULL)
        return cache->instance;
    instance = create(context, resource);
    if (instance == NULL) return NULL;
    cache->resource = resource;
    cache->instance = instance;
    ++cache->generation;
    return instance;
}

// LwoModelEntryLoad (implemented in gods98/model.cpp)
Model* ledger_stub_004809e0_LwoModelEntryLoad(const char* path, void* flags,
                                              void* context)
{
    return lwo_model_entry_load(path, flags, context);
}

/* Portable adapter only. Retail MaterialPass takes (device, task/material
 * record, parsed model, flags) and owns geometry/scene submission. */
int ledger_stub_00480ac0_MaterialPass(void* device, void* model, void* layer)
{
    return material_pass_execute(device, model, layer);
}

/* Portable adapter only. Retail BindModelTextures takes (device, native
 * model record, native material-record array, force-source-color). */
int ledger_stub_00481820_BindModelTextures(
    void* device, const ModelTextureInput* inputs,
    ModelTextureBinding* bindings, uint32_t count, int force_source_color)
{
    return bind_model_textures(device, inputs, bindings, count,
                               force_source_color);
}

// AnimationTrackApplyWrappedFrameOffset
void ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
    void* texture, int frame_index, const void* frame_state);
// FUNCTION: LEGORR 0x00482040
int ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset(
    void* animation_owner, int frame_offset)
{
    unsigned char* owner = (unsigned char*)animation_owner;
    unsigned char* records;
    uint32_t count;
    uint32_t flags;
    uint32_t slot;
    if (owner == NULL) return 0;
    memcpy(&count, owner, sizeof(count));
    memcpy(&flags, owner + 0x2c, sizeof(flags));
    if ((flags & 4u) == 0u) return 0;
    memcpy(&records, owner + 8, sizeof(records));
    if (records == NULL) return 1;
    for (slot = 0; slot < count; ++slot) {
        unsigned char* record = records + (size_t)slot * 0x70u;
        unsigned char* track;
        uint32_t track_flags;
        int frame_count;
        int frame_base;
        int frame_index;
        void* frames;
        void* frame;
        memcpy(&track, record + 0x68, sizeof(track));
        if (track == NULL) continue;
        memcpy(&track_flags, track + 0x34, sizeof(track_flags));
        if ((track_flags & 0x80u) == 0u) continue;
        memcpy(&frame_count, track + 8, sizeof(frame_count));
        memcpy(&frame_base, track + 0x38, sizeof(frame_base));
        memcpy(&frames, track, sizeof(frames));
        if (frame_count <= 0 || frames == NULL) continue;
        frame_index = (frame_base + frame_offset) % frame_count;
        /* The reference clamps a negative remainder to frame zero. */
        if (frame_index < 0) frame_index = 0;
        memcpy(&frame, (unsigned char*)frames +
               (size_t)frame_index * sizeof(frame), sizeof(frame));
        ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
            animation_owner, (int)slot, frame);
    }
    return 1;
}

// MeshRecordAllocateAndFill (FUN_00482220): append one 0x70-byte render
// record to the pool.  The pool starts at ten slots (0x460 bytes); on the
// first call it seeds count at 1, and when the count reaches capacity it
// grows capacity by 150 percent through the pool's allocator.  The record
// gets an index_count of rows*columns copied from every other source word
// and a zeroed 0x20-byte-per-vertex block, then reports through the pool's
// registration seam (FUN_00483170).
static void* ledger_mesh_record_default_allocate(void*, size_t bytes)
{
    return engine_heap_alloc((long)bytes);
}

static void ledger_mesh_record_default_release(void*, void* memory)
{
    engine_heap_free(memory);
}

// FUNCTION: LEGORR 0x00482220
int ledger_stub_00482220_MeshRecordAllocateAndFill(
    MeshRecordPool* pool, uint32_t vertex_count, uint32_t index_rows,
    uint32_t index_columns, const uint16_t* source_indices)
{
    void* (*allocate)(void*, size_t);
    void (*release)(void*, void*);
    uint32_t index_count;
    uint32_t index;
    size_t slot_bytes;
    size_t index_bytes;
    size_t vertex_bytes;
    MeshRenderRecord* record;
    uint32_t i;

    if (pool == NULL || (index_rows != 0 &&
        index_columns > 0xffffffffu / index_rows)) return -1;
    index_count = index_rows * index_columns;
    if (index_count != 0 && source_indices == NULL) return -1;
    if (vertex_count > (size_t)-1 / 32u ||
        index_count > (size_t)-1 / sizeof(uint16_t)) return -1;
    allocate = pool->allocate != NULL ? pool->allocate :
                                        ledger_mesh_record_default_allocate;
    release = pool->release != NULL ? pool->release :
                                      ledger_mesh_record_default_release;

    if (pool->records == NULL) {
        pool->capacity = 10;
        pool->count = 1;
        slot_bytes = (size_t)pool->capacity * sizeof(MeshRenderRecord);
        pool->records = (MeshRenderRecord*)allocate(pool->context, slot_bytes);
        if (pool->records == NULL) {
            pool->count = 0;
            pool->capacity = 0;
            return -1;
        }
    }
    else {
        index = pool->count;
        pool->count++;
        if (index == pool->capacity) {
            uint32_t new_capacity = (pool->capacity * 150u) / 100u;
            MeshRenderRecord* grown;
            if (new_capacity <= pool->capacity)
                new_capacity = pool->capacity + 1;
            slot_bytes = (size_t)new_capacity * sizeof(MeshRenderRecord);
            grown = (MeshRenderRecord*)allocate(pool->context, slot_bytes);
            if (grown == NULL) return -1;
            memcpy(grown, pool->records,
                   (size_t)pool->capacity * sizeof(MeshRenderRecord));
            release(pool->context, pool->records);
            pool->records = grown;
            pool->capacity = new_capacity;
        }
    }

    index = pool->count - 1u;
    record = &pool->records[index];
    memset(record, 0, sizeof(*record));
    index_bytes = (size_t)index_count * sizeof(uint16_t);
    record->indices = index_bytes != 0 ?
        (uint16_t*)allocate(pool->context, index_bytes) : NULL;
    if (index_bytes != 0 && record->indices == NULL) return -1;
    record->index_count = index_count;
    for (i = 0; i < index_count; ++i)
        record->indices[i] = source_indices[i * 2u];

    vertex_bytes = (size_t)vertex_count * 32u;
    record->vertices = vertex_bytes != 0 ?
        allocate(pool->context, vertex_bytes) : NULL;
    if (vertex_bytes != 0 && record->vertices == NULL) {
        if (record->indices != NULL) release(pool->context, record->indices);
        memset(record, 0, sizeof(*record));
        return -1;
    }
    if (record->vertices != NULL) memset(record->vertices, 0, vertex_bytes);
    record->vertex_count = vertex_count;
    if (pool->record_registered != NULL)
        pool->record_registered(pool->context, index);
    return (int)index;
}



// ModelRenderPassDispatch
void ledger_stub_00482d60_ModelRenderPassDispatch(void* device)
{
    model_render_pass_dispatch(device);
}

// AnimatedTextureFrameSurfaceSet (implemented in gods98/scene_resources.cpp)
void ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
    void* texture, int frame_index, const void* frame_state)
{
    lego::animated_texture_frame_surface_set(texture, frame_index,
                                              frame_state);
}

// IndexedRecordInterfaceRelease
// FUNCTION: LEGORR 0x00483140
void ledger_stub_00483140_IndexedRecordInterfaceRelease(void* owner, int index)
{
    unsigned char* bytes = (unsigned char*)owner;
    void* records;
    void* interface_object;
    void** vtable;
    typedef uint32_t (*ReleaseMethod)(void*);
    if (bytes == 0 || index < 0) return;
    memcpy(&records, bytes + 8, sizeof(records));
    if (records == 0) return;
    bytes = (unsigned char*)records + (size_t)index * 0x70u;
    memcpy(&interface_object, bytes + 0x60, sizeof(interface_object));
    if (interface_object == 0) return;
    vtable = *(void***)interface_object;
    if (vtable != 0 && vtable[2] != 0) {
        ReleaseMethod release = (ReleaseMethod)vtable[2];
        release(interface_object);
    }
    memset(bytes + 0x60, 0, sizeof(void*));
}

// FUNCTION: LEGORR 0x00483810
int ledger_stub_00483810_InputActionStateToggle(unsigned int action,
                                                 const void* data)
{
    return input_action_state_toggle(&s_input_action_toggle_state, NULL,
                                     action, data);
}

// FUNCTION: LEGORR 0x00483f00
int ledger_stub_00483f00_FlicPlayerClose(Flic* player)
{
    if (player == NULL) return 0;
    flic_close(player);
    return 1;
}

// FUNCTION: LEGORR 0x00483f60
int ledger_stub_00483f60_FlicFileHeaderValidate(const char* path,
                                                int* context)
{
    Resource* resource;
    unsigned char* header;
    uint32_t declared_size;
    uint16_t magic;
    int32_t file_size;
    int result = 0;
    if (path == NULL || context == NULL) return 0;
    resource = resource_open(path, RESOURCE_AUTO);
    if (resource == NULL) return 0;
    file_size = resource_read(resource);
    header = (unsigned char*)context + 0x640;
    if (file_size >= 0x88) {
        memcpy(header, resource->data, 0x88);
        memcpy(&declared_size, header, sizeof(declared_size));
        memcpy(&magic, header + 4, sizeof(magic));
        if (declared_size == (uint32_t)file_size &&
            (magic == 0x9119u || magic == 0x1234u ||
             magic == 0xaf11u || magic == 0xaf12u || magic == 0xaf43u)) {
            result = 1;
        }
    }
    resource_free(resource);
    return result;
}

// FUNCTION: LEGORR 0x00484610
int ledger_stub_00484610_FlicSubrecordOpcodeDispatch(void* record)
{
    uint8_t* rec = (uint8_t*)record;
    int16_t opcode;
    if (rec == NULL) return 0;
    memcpy(&opcode, rec + 0x6d4, sizeof(opcode));
    if (opcode == 4u) return flic_decode16_bit_row_records(rec);
    if (opcode == 0x0bu || opcode == 0x0cu || opcode == 0x0du
        || opcode == 0x5344u || opcode == 0x5555u) return 1;
    if (opcode == 0x0fu || opcode == 0x19u)
        return flic_opcode25_decode_dispatch(rec);
    if (opcode == 0x10u) return flic_opcode16_mode_dispatch(rec + 0x6d0);
    /* FUN_00484610 routes both 0x07 and 0x1b through FUN_00485030. */
    if (opcode == 0x07u || opcode == 0x1bu)
        return flic_opcode27_decode_dispatch(rec);
    return 0;
}

// LoadLWSCScene
// FUNCTION: LEGORR 0x004869f0
void* ledger_stub_004869f0_LoadLWSCScene(
    const char* path, void* context,
    void* (*scene_load)(const char* path, void* context))
{
    if (path == NULL || path[0] == '\0' || scene_load == NULL) return NULL;
    return scene_load(path, context);
}

// SharedObjectDirectoryScan
typedef int (*SharedObjectDirectoryEntryHook)(void* context,
                                              const char* name,
                                              uint32_t attributes);
typedef int (*SharedObjectDirectoryWalkHook)(void* context,
    const char* directory, SharedObjectDirectoryEntryHook entry_hook,
    void* entry_context);
typedef struct SharedObjectDirectoryScanState {
    const char* directory;
    SharedObjectDirectoryWalkHook walk;
    SharedObjectDirectoryEntryHook entry;
    void* context;
} SharedObjectDirectoryScanState;
// FUNCTION: LEGORR 0x004876c0
int ledger_stub_004876c0_SharedObjectDirectoryScan(
    SharedObjectDirectoryScanState* state)
{
    if (state == NULL || state->directory == NULL || state->walk == NULL ||
        state->entry == NULL) return 0;
    return state->walk(state->context, state->directory, state->entry,
                       state->context);
}

// SceneEntrySlotTableExpand
typedef struct SceneEntrySlotTable {
    void** entries;
    uint32_t count;
    uint32_t capacity;
} SceneEntrySlotTable;
// FUNCTION: LEGORR 0x004877e0
int ledger_stub_004877e0_SceneEntrySlotTableExpand(
    SceneEntrySlotTable* table, uint32_t minimum_capacity)
{
    void** entries;
    uint32_t capacity;
    if (table == NULL) return 0;
    if (minimum_capacity <= table->capacity) return 1;
    if (table->count > table->capacity) return 0;
    capacity = table->capacity != 0u ? table->capacity : 8u;
    while (capacity < minimum_capacity) {
        if (capacity > 0x3fffffffu) return 0;
        capacity *= 2u;
    }
    entries = (void**)engine_heap_alloc((size_t)capacity * sizeof(void*));
    if (entries == NULL) return 0;
    memset(entries, 0, (size_t)capacity * sizeof(void*));
    if (table->entries != NULL && table->count != 0u)
        memcpy(entries, table->entries,
               (size_t)table->count * sizeof(void*));
    if (table->entries != NULL) engine_heap_free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
    return 1;
}

// SharedObjectClone
typedef void* (*SharedObjectCloneAllocHook)(size_t size, void* context);
typedef void (*SharedObjectCloneRetainHook)(void* clone, const void* source,
                                            void* context);
// FUNCTION: LEGORR 0x00487a00
void* ledger_stub_00487a00_SharedObjectClone(
    const void* source, size_t size, SharedObjectCloneAllocHook allocate,
    SharedObjectCloneRetainHook retain, void* context)
{
    void* clone;
    if (source == NULL || size == 0u || allocate == NULL) return NULL;
    clone = allocate(size, context);
    if (clone == NULL) return NULL;
    memcpy(clone, source, size);
    if (retain != NULL) retain(clone, source, context);
    return clone;
}

// SharedObjectAnimationAndTimedEventUpdate
typedef struct SharedObjectAnimationHooks {
    float (*clock_get)(void* context);
    void (*timed_event_update)(void* owner, void* track, float time,
                              void* context);
    void* context;
} SharedObjectAnimationHooks;
// FUNCTION: LEGORR 0x00487ba0
void ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
    void* shared_object, float time, const void* hook_context)
{
    unsigned char* owner = (unsigned char*)shared_object;
    const SharedObjectAnimationHooks* hooks =
        (const SharedObjectAnimationHooks*)hook_context;
    uint16_t duration;
    uint16_t track_count;
    uint32_t flags;
    unsigned char* tracks;
    float current_time;
    uint16_t i;
    if (owner == NULL) return;
    memcpy(&current_time, owner + 0x10, sizeof(current_time));
    memcpy(owner + 0x0c, &current_time, sizeof(current_time));
    memcpy(&duration, owner + 2, sizeof(duration));
    memcpy(&flags, owner + 0x34, sizeof(flags));
    if ((flags & 1u) != 0u && hooks != NULL && hooks->clock_get != NULL)
        time = hooks->clock_get(hooks->context);
    else if (time > (float)duration)
        time = (float)duration;
    memcpy(owner + 0x10, &time, sizeof(time));
    current_time = time;
    memcpy(&track_count, owner + 0x24, sizeof(track_count));
    memcpy(&tracks, owner + 0x20, sizeof(tracks));
    if (tracks == NULL) return;
    for (i = 0; i < track_count; ++i) {
        unsigned char* track = tracks + (size_t)i * 0x34u;
        uint16_t vector_count;
        uint16_t scalar_count;
        uint16_t segment = 0;
        float fraction;
        memcpy(&vector_count, track + 0x24, sizeof(vector_count));
        if (vector_count > 1u) {
            fraction = lego::keyframe_time_segment_fraction(
                track, current_time, &segment);
            lego::animation_track_keys_interpolate_apply(
                owner, track, segment, fraction);
        }
        memcpy(&scalar_count, track + 0x26, sizeof(scalar_count));
        if (scalar_count > 1u)
            lego::shared_object_float_track_interpolate_apply(owner, track,
                                                               segment, 0.0f);
        if (hooks != NULL && hooks->timed_event_update != NULL)
            hooks->timed_event_update(owner, track, current_time,
                                      hooks->context);
    }
}

// AnimationTrackFindKeyframeAndFraction
float ledger_stub_00487cb0_AnimationTrackFindKeyframeAndFraction(
    const void* track, float time, uint16_t* segment_out)
{
    return ::animation_track_find_keyframe_and_fraction(track, time,
                                                         segment_out);
}

// ComInterfaceQueryAndContextDispatch
typedef int (*ComInterfaceContextDispatchHook)(void* context, void* object,
                                                uint32_t method,
                                                void* argument);
// FUNCTION: LEGORR 0x00487d50
int ledger_stub_00487d50_ComInterfaceQueryAndContextDispatch(
    void* object, uint32_t method, void* argument,
    ComInterfaceContextDispatchHook dispatch, void* context)
{
    if (object == NULL || dispatch == NULL) return 0;
    return dispatch(context, object, method, argument);
}

// SharedObjectTimedSoundEventUpdate
typedef struct SharedObjectTimedSoundEvent {
    float time;
    uint32_t sound_id;
    uint32_t flags;
} SharedObjectTimedSoundEvent;
typedef void (*SharedObjectTimedSoundPlayHook)(void* owner,
    uint32_t sound_id, uint32_t flags, void* context);
// FUNCTION: LEGORR 0x00487de0
uint32_t ledger_stub_00487de0_SharedObjectTimedSoundEventUpdate(
    void* owner, const SharedObjectTimedSoundEvent* events,
    uint32_t event_count, float previous_time, float current_time,
    SharedObjectTimedSoundPlayHook play, void* context)
{
    uint32_t i;
    uint32_t played = 0u;
    if (owner == NULL || events == NULL || play == NULL ||
        current_time < previous_time) return 0u;
    for (i = 0u; i < event_count; ++i) {
        if (events[i].time > previous_time && events[i].time <= current_time) {
            play(owner, events[i].sound_id, events[i].flags, context);
            ++played;
        }
    }
    return played;
}

// SharedObjectFloatTrackInterpolateApply
// FUNCTION: LEGORR 0x00488070
void ledger_stub_00488070_SharedObjectFloatTrackInterpolateApply(
    void* owner, const void* track, uint32_t segment, float fraction)
{
    lego::shared_object_float_track_interpolate_apply(owner, track, segment,
                                                       fraction);
}

// AnimationTrackKeysInterpolate
void ledger_stub_00488170_AnimationTrackKeysInterpolate(
    void* owner, const void* track, uint32_t segment, float fraction)
{
    ::animation_track_keys_interpolate(owner, track, segment, fraction);
}

// SceneNodeTransformMatrixApply
void ledger_stub_004882e0_SceneNodeTransformMatrixApply(void* owner,
    const void* track, const float position[3], const float rotation[3],
    const float scale[3])
{
    ::scene_node_transform_matrix_apply(owner, track, position, rotation,
                                        scale);
}

// ModelResourceResolveOrLoad (implemented in gods98/model.cpp)
Model* ledger_stub_004885c0_ModelResourceResolveOrLoad(const char* prefix,
                                                       const char* path,
                                                       void* flags,
                                                       void* context)
{
    return model_resource_resolve_or_load(prefix, path, flags, context);
}

// SceneHierarchyInterfaceRegisterRecursive
// FUNCTION: LEGORR 0x004887c0
void ledger_stub_004887c0_SceneHierarchyInterfaceRegisterRecursive(
    unsigned short* next_index, void* root, void* node)
{
    scene_hierarchy_interface_register_recursive(next_index, root, node);
}

// ResourceTreeCallbackReferenceChainRelease
typedef void (*ResourceReferenceReleaseHook)(void* resource, void* context);
typedef struct ResourceReferenceChain {
    void* resource;
    struct ResourceReferenceChain* next;
} ResourceReferenceChain;
// FUNCTION: LEGORR 0x004889d0
uint32_t ledger_stub_004889d0_ResourceTreeCallbackReferenceChainRelease(
    ResourceReferenceChain* chain, ResourceReferenceReleaseHook release,
    void* context)
{
    uint32_t count = 0u;
    while (chain != NULL) {
        ResourceReferenceChain* next = chain->next;
        if (chain->resource != NULL && release != NULL)
            release(chain->resource, context);
        engine_heap_free(chain);
        chain = next;
        ++count;
    }
    return count;
}

// LoadRecordInterfacesReleaseFree
typedef struct LoadRecordInterfaceList {
    void** interfaces;
    uint32_t count;
} LoadRecordInterfaceList;
// FUNCTION: LEGORR 0x00488a70
void ledger_stub_00488a70_LoadRecordInterfacesReleaseFree(
    LoadRecordInterfaceList* list, void (*release)(void* interface_ptr))
{
    uint32_t i;
    if (list == NULL) return;
    for (i = 0u; i < list->count; ++i)
        if (list->interfaces != NULL && list->interfaces[i] != NULL &&
            release != NULL) release(list->interfaces[i]);
    if (list->interfaces != NULL) engine_heap_free(list->interfaces);
    list->interfaces = NULL;
    list->count = 0u;
}

// FUNCTION: LEGORR 0x00488bb0
void ledger_stub_00488bb0_CdAudioTrackStart(int track, int restart_when_done,
    MediaCdAudioCompletionHook callback)
{
    media_cd_audio_track_start(track, restart_when_done, callback);
}

// ComInterfaceQueryOrFallback
typedef int (*ComInterfaceQueryHook)(void* object, uint32_t interface_id,
                                     void** result, void* context);
// FUNCTION: LEGORR 0x004894c0
void* ledger_stub_004894c0_ComInterfaceQueryOrFallback(
    void* object, uint32_t interface_id, void* fallback,
    ComInterfaceQueryHook query, void* context)
{
    void* result = NULL;
    if (object != NULL && query != NULL &&
        query(object, interface_id, &result, context) && result != NULL)
        return result;
    return fallback;
}

// DDiSceneTimedHookDispatch
void ledger_stub_004897e0_DDiSceneTimedHookDispatch(
    void* hook_record, float time, void* output)
{
    ddiscene_timed_hook_dispatch(hook_record, time, output);
}

// ObjectHierarchyRecursiveClone
typedef struct PortableObjectHierarchyNode {
    const void* source;
    void* clone;
    struct PortableObjectHierarchyNode** children;
    uint32_t child_count;
} PortableObjectHierarchyNode;
typedef int (*PortableObjectCloneNodeHook)(void* context,
                                           const void* source,
                                           void** clone_out);
// FUNCTION: LEGORR 0x004899f0
int ledger_stub_004899f0_ObjectHierarchyRecursiveClone(
    PortableObjectHierarchyNode* node, PortableObjectCloneNodeHook clone_node,
    void* context, uint32_t depth)
{
    uint32_t i;
    if (node == NULL || node->source == NULL || clone_node == NULL ||
        depth > 64u) return 0;
    if (node->clone == NULL &&
        !clone_node(context, node->source, &node->clone)) return 0;
    for (i = 0u; i < node->child_count; ++i)
        if (!ledger_stub_004899f0_ObjectHierarchyRecursiveClone(
                node->children[i], clone_node, context, depth + 1u)) return 0;
    return 1;
}

// ObjectInterfaceArrayResourceCreate
// FUNCTION: LEGORR 0x00489b30
void** ledger_stub_00489b30_ObjectInterfaceArrayResourceCreate(
    uint32_t count, void* (*create)(uint32_t index, void* context),
    void (*destroy)(void* interface_ptr, void* context), void* context)
{
    void** interfaces;
    uint32_t i;
    if (count == 0u || create == NULL) return NULL;
    interfaces = (void**)engine_heap_alloc((size_t)count * sizeof(void*));
    if (interfaces == NULL) return NULL;
    memset(interfaces, 0, (size_t)count * sizeof(void*));
    for (i = 0u; i < count; ++i) {
        interfaces[i] = create(i, context);
        if (interfaces[i] == NULL) {
            while (i != 0u) {
                --i;
                if (interfaces[i] != NULL && destroy != NULL)
                    destroy(interfaces[i], context);
            }
            engine_heap_free(interfaces);
            return NULL;
        }
    }
    return interfaces;
}

// ComChildItemsVisit
typedef int (*ComChildItemVisitHook)(void* context, void* parent,
                                     void* child, uint32_t index);
// FUNCTION: LEGORR 0x00489bc0
uint32_t ledger_stub_00489bc0_ComChildItemsVisit(
    void* parent, void* const* children, uint32_t count,
    ComChildItemVisitHook visit, void* context)
{
    uint32_t i, visited = 0u;
    if (parent == NULL || children == NULL || visit == NULL) return 0u;
    for (i = 0u; i < count; ++i) {
        if (children[i] == NULL) continue;
        if (!visit(context, parent, children[i], i)) break;
        ++visited;
    }
    return visited;
}

// DirectXErrorMessageFormat
// FUNCTION: LEGORR 0x00489d90
void ledger_stub_00489d90_DirectXErrorMessageFormat(const char* name,
                                                    const char* description)
{
    if (name == NULL) name = "Unknown";
    if (description == NULL) description = "No description available";
#if defined(_MSC_VER)
    _snprintf(s_directx_error_message, sizeof(s_directx_error_message) - 1,
              "%s: %s", name, description);
#else
    snprintf(s_directx_error_message, sizeof(s_directx_error_message) - 1,
             "%s: %s", name, description);
#endif
    s_directx_error_message[sizeof(s_directx_error_message) - 1] = '\0';
}

// DirectXResultDiagnosticDispatch
// FUNCTION: LEGORR 0x00489dd0
int ledger_stub_00489dd0_DirectXResultDiagnosticDispatch(
    int result, int diagnostic_kind, const char* source_file, int source_line)
{
    char name[64];
    char detail[256];
    if (result >= 0) return result;
#if defined(_MSC_VER)
    _snprintf(name, sizeof(name) - 1, "DirectX 0x%08x", (unsigned int)result);
    _snprintf(detail, sizeof(detail) - 1, "%s:%d (kind %d)",
              source_file != NULL ? source_file : "?", source_line,
              diagnostic_kind);
#else
    snprintf(name, sizeof(name) - 1, "DirectX 0x%08x", (unsigned int)result);
    snprintf(detail, sizeof(detail) - 1, "%s:%d (kind %d)",
             source_file != NULL ? source_file : "?", source_line,
             diagnostic_kind);
#endif
    name[sizeof(name) - 1] = '\0';
    detail[sizeof(detail) - 1] = '\0';
    ledger_stub_00489d90_DirectXErrorMessageFormat(name, detail);
    return result;
}

// FUNCTION: LEGORR 0x0048b260
void ledger_stub_0048b260_StartupStreamStateReset()
{
    unported_ledger_stubs::s_startup_streams[0] = NULL;
    unported_ledger_stubs::s_startup_streams[1] = NULL;
    unported_ledger_stubs::s_startup_streams[2] = NULL;
    unported_ledger_stubs::s_startup_streams[3] = NULL;
    unported_ledger_stubs::s_startup_window_mode = 0u;
    window_mode_flag_set(0u);
}


// FUNCTION: LEGORR 0x0048b290
void ledger_stub_0048b290_StartupStreamsClose()
{
    if (unported_ledger_stubs::s_startup_streams[0] != NULL) {
        game_file_close(unported_ledger_stubs::s_startup_streams[0]);
    }
    if (unported_ledger_stubs::s_startup_streams[1] != NULL) {
        game_file_close(unported_ledger_stubs::s_startup_streams[1]);
    }
    if (unported_ledger_stubs::s_startup_streams[2] != NULL) {
        game_file_close(unported_ledger_stubs::s_startup_streams[2]);
    }
    unported_ledger_stubs::s_startup_streams[2] = NULL;
    unported_ledger_stubs::s_startup_streams[1] = NULL;
    unported_ledger_stubs::s_startup_streams[0] = NULL;
}

// FUNCTION: LEGORR 0x0048b390
int ledger_stub_0048b390_RegistryPathCreateKeys(
    void* root, const char* subkey_path, const char* value_name,
    void* output, uint32_t* size, uint32_t* value_type)
{
    return unported_ledger_stubs::registry_path_create_keys(
        root, subkey_path, value_name, output, size, value_type);
}

// LwoCurves
// FUNCTION: LEGORR 0x0048c360
uint32_t ledger_stub_0048c360_LwoCurves(GameFileHandle* stream,
                                         int byte_count)
{
    uint8_t* data;
    int remaining;
    int offset = 0;
    if (stream == NULL || byte_count < 0 || byte_count > 0x7ffffff0) return 0u;
    data = (uint8_t*)engine_heap_alloc((long)byte_count + 16L);
    if (data == NULL) return 0u;
    memset(data, 0, (size_t)byte_count + 16u);
    if (byte_count != 0 &&
        wad_file_read(data, byte_count, 1, stream) != 1) {
        engine_heap_free(data);
        return 0u;
    }

    /* CRVS is intentionally not retained by the model.  Walk each record so
     * the handler consumes its packed index list, then release the temporary
     * chunk copy just as the original loader does. */
    remaining = byte_count;
    while (remaining > 0) {
        uint32_t index_count;
        uint32_t index_bytes;
        if (remaining < 2) break;
        index_count = ((uint32_t)data[offset] << 8) | data[offset + 1];
        offset += 2;
        remaining -= 2;
        index_bytes = index_count * 2u;
        if (index_bytes > (uint32_t)remaining) break;
        offset += (int)index_bytes;
        remaining -= (int)index_bytes;
        /* The record ends with two words after its counted index list. */
        if (remaining < 4) break;
        offset += 4;
        remaining -= 4;
    }
    engine_heap_free(data);
    return 1u;
}

// VC5 LIBC.LIB realloc.obj supplies the heap reallocation entry.
// LIBRARY: LEGORR 0x0048dc80 SYMBOL
// _realloc
#if !defined(_MSC_VER)
void* ledger_stub_0048dc80_GrowHeapArray(void* allocation,
                                         uint32_t requested_size)
{
    uint32_t size = requested_size;
    if (allocation == NULL) {
        if (size == 0u) size = 1u;
        return engine_heap_alloc((long)size);
    }
    if (size == 0u) {
        engine_heap_free(allocation);
        return NULL;
    }
    /* The CRT rounds ordinary allocations to its 16-byte small-block unit. */
    if (size >= 0xffffffe1u) return NULL;
    size = (size + 15u) & ~15u;
    return realloc(allocation, (size_t)size);
}
#endif

/* SehScopeTableLookup (FUN_0048ee90): walk the nested active scope ranges
 * from the innermost table entry outwards. Invalid frame/table data returns
 * NULL in this portable model instead of invoking the CRT's fatal helper. */
// FUNCTION: LEGORR 0x0048ee90
void* seh_scope_table_lookup(const void* registration_frame,
                             int nesting_depth, uint32_t control_pc,
                             uint32_t* scope_index_out,
                             uint32_t* parent_index_out)
{
    const uint8_t* frame = (const uint8_t*)registration_frame;
    const uint8_t* entries;
    uint32_t count;
    uint32_t index;
    uint32_t previous;
    uint32_t last;
    if (scope_index_out != NULL) *scope_index_out = 0u;
    if (parent_index_out != NULL) *parent_index_out = 0u;
    if (frame == NULL || scope_index_out == NULL || parent_index_out == NULL)
        return NULL;
    memcpy(&count, frame + 0x0cu, sizeof(count));
    memcpy(&entries, frame + 0x10u, sizeof(entries));
    if (entries == NULL || count > 4096u) return NULL;

    index = count;
    previous = count;
    last = count;
    while (nesting_depth >= 0) {
        uint32_t start_pc;
        uint32_t end_pc;
        int matched;
        if (index == 0u) return NULL;
        --index;
        memcpy(&start_pc, entries + index * 0x14u + 4u,
               sizeof(start_pc));
        memcpy(&end_pc, entries + index * 0x14u + 8u,
               sizeof(end_pc));
        matched = start_pc < control_pc && control_pc <= end_pc;
        if (matched) {
            --nesting_depth;
            last = previous;
            previous = index;
        }
    }
    ++index;
    *scope_index_out = index;
    *parent_index_out = last;
    if (last > count || last < index) return NULL;
    return (void*)(entries + index * 0x14u);
}

// CharacterClassifyMask

// VC5 LIBC.LIB sbheap.obj supplies the in-place small-block resize routine.
// LIBRARY: LEGORR 0x00491cc0 SYMBOL
// ___sbh_resize_block
#if !defined(_MSC_VER)
int ledger_stub_00491cc0_SmallHeapBlockResizeInPlace(
    void* heap, uint32_t* group, uint8_t* block, uint32_t requested_units)
{
    uintptr_t group_offset;
    uint32_t* bucket;
    uint32_t old_units;
    if (heap == NULL || group == NULL || block == NULL ||
        requested_units > 0xffu) return 0;
    group_offset = (uintptr_t)group -
                   (uintptr_t)(*(uint8_t**) ((uint8_t*)heap + 0x10));
    bucket = (uint32_t*)((uint8_t*)heap + 0x18u +
                         (group_offset >> 12) * 8u);
    old_units = block[0];
    if (old_units <= requested_units) {
        uint8_t* new_end;
        uint8_t* scan;
        if (old_units == requested_units) return 0;
        new_end = block + requested_units;
        if (new_end > (uint8_t*)(group + 0x3e)) return 0;
        for (scan = block + old_units; scan < new_end; ++scan)
            if (*scan != 0u) return 0;
        block[0] = (uint8_t)requested_units;
        if ((uint8_t*)group[0] >= block && (uint8_t*)group[0] < new_end) {
            uint8_t* first_free = new_end;
            while (first_free < (uint8_t*)(group + 0x3e) &&
                   *first_free == 0u) ++first_free;
            group[0] = (uint32_t)first_free;
            group[1] = (uint32_t)(first_free - new_end);
        }
        bucket[0] += old_units - requested_units;
        return 1;
    }
    block[0] = (uint8_t)requested_units;
    bucket[1] = 0xf1u;
    bucket[0] += old_units - requested_units;
    return 1;
}
#endif

// LocaleCharacterMapString

// CrtLocaleCodePageTableUpdate

// CrtAnsiCodePageTablesInit

// CrtGetStringTypeACompat

// CrtCharacterToLowerLocale

// CrtMultibyteToWideChar

// CrtCharacterClassFlag8Check

} // namespace unported_ledger_stubs

void spawn_coordinate_pool_clear(void)
{
    unported_ledger_stubs::spawn_coordinate_pool_clear_impl();
}

/* The old NERP/advisor owner lives in DAT windows in the corpus.  Keep the
 * parser and interpreter state explicit here so callers can install the
 * operations that belong to the game or platform layer. */
typedef struct TutorialTokenEntry {
    char* name;
    uint32_t value;
    int kind;
} TutorialTokenEntry;

static TutorialScriptOps s_tutorial_script_ops;
static void* s_tutorial_script_data;
static uint32_t s_tutorial_script_size;
static uint32_t s_tutorial_program_words[8192];
static uint32_t s_tutorial_program_count;
static TutorialTokenEntry s_tutorial_tokens[512];
static uint32_t s_tutorial_token_count;
static uint32_t s_tutorial_scroll_pending;
static uint32_t s_tutorial_scroll_max;
static uint32_t s_tutorial_scroll_step;
static uint32_t s_tutorial_scroll_action;
static uint32_t s_tutorial_scroll_flags;
static uint32_t s_tutorial_focus_id;
static uint32_t s_tutorial_focus_valid;
static float s_tutorial_focus_offset_target;
static float s_tutorial_focus_angle_target;
static uint32_t s_tutorial_focus_offset_active;
static uint32_t s_tutorial_focus_angle_active;
static float s_tutorial_focus_offset;
static float s_tutorial_focus_angle;
static float s_tutorial_focus_angle_velocity;

static int tutorial_text_equal(const char* a, const char* b)
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

static void tutorial_tokens_clear(void)
{
    uint32_t i;
    for (i = 0; i < s_tutorial_token_count; ++i) {
        if (s_tutorial_tokens[i].kind == 2 &&
            s_tutorial_script_ops.resolved_token_release != NULL)
            s_tutorial_script_ops.resolved_token_release(
                s_tutorial_tokens[i].kind, s_tutorial_tokens[i].value,
                s_tutorial_script_ops.context);
        free(s_tutorial_tokens[i].name);
        s_tutorial_tokens[i].name = NULL;
        s_tutorial_tokens[i].value = 0;
        s_tutorial_tokens[i].kind = 0;
    }
    s_tutorial_token_count = 0;
}

static int tutorial_token_add(const char* start, uint32_t length, int kind)
{
    char* copy;
    uint32_t i;
    uint32_t value = 0;
    if (s_tutorial_token_count >= 512 || length == 0) return 0;
    copy = (char*)malloc((size_t)length + 1);
    if (copy == NULL) return 0;
    for (i = 0; i < length; ++i) {
        copy[i] = start[i] == '_' ? ' ' : start[i];
    }
    copy[length] = '\0';
    if (kind != 0 && s_tutorial_script_ops.resolve != NULL) {
        value = s_tutorial_script_ops.resolve(kind, copy,
                                              s_tutorial_script_ops.context);
    }
    s_tutorial_tokens[s_tutorial_token_count].name = copy;
    s_tutorial_tokens[s_tutorial_token_count].value = value;
    s_tutorial_tokens[s_tutorial_token_count].kind = kind;
    ++s_tutorial_token_count;
    return 1;
}

void tutorial_script_ops_set(const TutorialScriptOps* ops)
{
    if (ops != NULL) s_tutorial_script_ops = *ops;
    else memset(&s_tutorial_script_ops, 0, sizeof(s_tutorial_script_ops));
}

void tutorial_script_program_install(const uint32_t* words, uint32_t count)
{
    uint32_t i;
    if (count > 8192) count = 8192;
    if (words == NULL) count = 0;
    for (i = 0; i < count; ++i) s_tutorial_program_words[i] = words[i];
    s_tutorial_program_count = count;
}

// FUNCTION: LEGORR 0x00452df0
int tutorial_script_file_load(const char* path)
{
    uint32_t size = 0;
    void* data = NULL;
    tutorial_tokens_clear();
    if (s_tutorial_script_data != NULL) {
        if (s_tutorial_script_ops.release != NULL)
            s_tutorial_script_ops.release(s_tutorial_script_data,
                                          s_tutorial_script_ops.context);
        else free(s_tutorial_script_data);
    }
    s_tutorial_script_data = NULL;
    s_tutorial_script_size = 0;
    s_tutorial_program_count = 0;
    s_tutorial_scroll_pending = 0;
    s_tutorial_scroll_step = 0;
    if (s_tutorial_script_ops.state_reset != NULL)
        s_tutorial_script_ops.state_reset(s_tutorial_script_ops.context);
    if (s_tutorial_script_ops.resource_state_initialize != NULL)
        s_tutorial_script_ops.resource_state_initialize(
            s_tutorial_script_ops.context);
    if (s_tutorial_script_ops.mode_set != NULL)
        s_tutorial_script_ops.mode_set(1, s_tutorial_script_ops.context);
    if (s_tutorial_script_ops.load != NULL && path != NULL)
        data = s_tutorial_script_ops.load(path, &size,
                                         s_tutorial_script_ops.context);
    s_tutorial_script_data = data;
    s_tutorial_script_size = size;
    if (data != NULL && size >= sizeof(uint32_t)) {
        uint32_t count = size / sizeof(uint32_t);
        if (count > 8192) count = 8192;
        tutorial_script_program_install((const uint32_t*)data, count);
    }
    return data != NULL;
}

// FUNCTION: LEGORR 0x00452e70
int tutorial_objective_token_load(uint32_t source_limit, char** output_tokens)
{
    char* mutable_source;
    uint32_t length;
    uint32_t i = 0;
    if (s_tutorial_script_data == NULL) return 0;
    if (s_tutorial_script_ops.resource_state_initialize != NULL)
        s_tutorial_script_ops.resource_state_initialize(
            s_tutorial_script_ops.context);
    length = s_tutorial_script_size;
    if (source_limit != 0 && source_limit < length) length = source_limit;
    mutable_source = (char*)malloc((size_t)length + 1);
    if (mutable_source == NULL) return 0;
    memcpy(mutable_source, s_tutorial_script_data, length);
    mutable_source[length] = '\0';
    tutorial_tokens_clear();
    while (i < length && s_tutorial_token_count < 512) {
        uint32_t start;
        int kind = 0;
        while (i < length && (unsigned char)mutable_source[i] < 0x20) {
            mutable_source[i++] = '\0';
        }
        if (i >= length) break;
        if (mutable_source[i] == ':' || mutable_source[i] == '$') {
            kind = mutable_source[i] == ':' ? 1 : 2;
            ++i;
        }
        start = i;
        while (i < length && (unsigned char)mutable_source[i] >= 0x20) ++i;
        if (i > start) {
            uint32_t j;
            for (j = start; j < i; ++j) {
                if (mutable_source[j] == '_') mutable_source[j] = ' ';
            }
            tutorial_token_add(mutable_source + start, i - start, kind);
        }
        if (i < length) mutable_source[i++] = '\0';
    }
    free(mutable_source);
    (void)output_tokens;
    return 1;
}

// FUNCTION: LEGORR 0x00453220
int tutorial_display_resources_release(void)
{
    if (s_tutorial_script_data != NULL) {
        if (s_tutorial_script_ops.release != NULL)
            s_tutorial_script_ops.release(s_tutorial_script_data,
                                          s_tutorial_script_ops.context);
        else free(s_tutorial_script_data);
    }
    s_tutorial_script_data = NULL;
    s_tutorial_script_size = 0;
    s_tutorial_program_count = 0;
    tutorial_tokens_clear();
    s_tutorial_scroll_step = 0;
    s_tutorial_scroll_pending = 0;
    memset(s_tutorial_program_words, 0, sizeof(s_tutorial_program_words));
    if (s_tutorial_script_ops.state_reset != NULL)
        s_tutorial_script_ops.state_reset(s_tutorial_script_ops.context);
    if (s_tutorial_script_ops.resource_state_release != NULL)
        s_tutorial_script_ops.resource_state_release(
            s_tutorial_script_ops.context);
    return 1;
}

// FUNCTION: LEGORR 0x004532e0
void tutorial_indirect_handle_resolve(uint32_t* value)
{
    uint32_t index;
    if (value == NULL || (*value & 0xffff0000u) != 0x00020000u) return;
    index = *value & 0xffffu;
    if (s_tutorial_script_ops.indirect_resolve != NULL)
        *value = s_tutorial_script_ops.indirect_resolve(
            index, s_tutorial_script_ops.context);
}

static uint32_t tutorial_compare_values(uint32_t op, uint32_t left,
                                        uint32_t right)
{
    switch (op) {
    case 3: return left < right;
    case 4: return left > right;
    case 5: return left == right;
    case 6: return left <= right;
    case 7: return left >= right;
    case 8: return left != right;
    default: return right != 0;
    }
}

// FUNCTION: LEGORR 0x00453320
void tutorial_script_command_dispatch(float delta)
{
    uint32_t pc = 0;
    uint32_t condition = 1;
    uint32_t previous = 0;
    uint32_t compare_op = 0;
    while (pc < s_tutorial_program_count) {
        uint32_t word = s_tutorial_program_words[pc++];
        uint32_t command = word & 0xffffu;
        uint32_t kind;
        uint32_t argc = 0;
        uint32_t args[3];
        uint32_t result = previous;
        uint32_t i;
        if ((word & 0x20000u) == 0) {
            if ((word & 0x10000u) != 0) {
                if (command == 1) compare_op = 1;
                else if (command >= 2 && command <= 10) compare_op = command;
            } else if ((word & (0x40000u | 0x80000u)) != 0) {
                if ((word & 0x40000u) != 0) condition = 1;
                if ((word & 0x80000u) != 0) {
                    if (condition) pc = command < s_tutorial_program_count
                                            ? command : s_tutorial_program_count;
                    condition = 1;
                }
            } else {
                uint32_t literal = (uint32_t)(int32_t)(int16_t)command;
                if (compare_op == 1) result = literal == 0;
                else if (compare_op != 0)
                    result = tutorial_compare_values(compare_op, previous,
                                                     literal);
                else result = literal;
                previous = result;
                condition = result;
                compare_op = 0;
            }
            continue;
        }
        kind = s_tutorial_script_ops.command_kind != NULL
            ? s_tutorial_script_ops.command_kind(command,
                                                  s_tutorial_script_ops.context)
            : 0;
        if (kind > 6) kind = 0;
        if (kind == 3) argc = 0;
        else if (kind == 4 || kind == 1) argc = 1;
        else if (kind == 5 || kind == 2) argc = 2;
        else if (kind == 6) argc = 3;
        for (i = 0; i < argc; ++i) {
            args[i] = pc < s_tutorial_program_count
                    ? s_tutorial_program_words[pc++] : 0;
            tutorial_indirect_handle_resolve(&args[i]);
        }
        if (kind >= 3 && !condition) {
            if (kind == 3 || kind == 4 || kind == 5 || kind == 6) {
                condition = 0;
                previous = 0;
            }
            continue;
        }
        if (s_tutorial_script_ops.command != NULL)
            result = s_tutorial_script_ops.command(
                command, argc == 0 ? NULL : args, argc,
                s_tutorial_script_ops.context);
        previous = result;
        condition = result != 0;
        if (kind >= 3) {
            previous = 0;
            compare_op = 0;
        }
    }
    tutorial_camera_focus_tween_update(delta);
}

void tutorial_advisor_scroll_state_set(uint32_t pending, uint32_t maximum,
                                       uint32_t step, uint32_t action,
                                       uint32_t state_flags)
{
    s_tutorial_scroll_pending = pending;
    s_tutorial_scroll_max = maximum;
    s_tutorial_scroll_step = step;
    s_tutorial_scroll_action = action;
    s_tutorial_scroll_flags = state_flags;
}

// FUNCTION: LEGORR 0x00453920
void tutorial_advisor_scroll_step_advance(void)
{
    if (s_tutorial_scroll_pending != 0) {
        if (s_tutorial_scroll_step != 0) --s_tutorial_scroll_step;
        if (s_tutorial_scroll_step > 3) s_tutorial_scroll_step = 3;
        if (s_tutorial_scroll_step > s_tutorial_scroll_max)
            s_tutorial_scroll_step = s_tutorial_scroll_max;
        if (s_tutorial_script_ops.scroll_commit != NULL)
            s_tutorial_script_ops.scroll_commit(
                s_tutorial_scroll_step, s_tutorial_script_ops.context);
        if (s_tutorial_scroll_step == 0 && s_tutorial_script_ops.command != NULL) {
            uint32_t args[2];
            args[0] = s_tutorial_scroll_action;
            args[1] = 1;
            s_tutorial_script_ops.command(0, args, 2,
                                          s_tutorial_script_ops.context);
        }
        s_tutorial_scroll_flags |= 1u;
    } else {
        s_tutorial_scroll_pending = 0;
    }
}

void tutorial_camera_focus_state_set(uint32_t focus_id, uint32_t valid,
                                     float offset_target, float angle_target,
                                     uint32_t offset_active,
                                     uint32_t angle_active)
{
    s_tutorial_focus_id = focus_id;
    s_tutorial_focus_valid = valid;
    s_tutorial_focus_offset_target = offset_target;
    s_tutorial_focus_angle_target = angle_target;
    s_tutorial_focus_offset_active = offset_active;
    s_tutorial_focus_angle_active = angle_active;
    s_tutorial_focus_offset = 0.0f;
    s_tutorial_focus_angle = 0.0f;
    s_tutorial_focus_angle_velocity = 0.0f;
}

// FUNCTION: LEGORR 0x00453da0
void tutorial_camera_focus_tween_update(float delta)
{
    float scale = 1.0f;
    float step;
    if (s_tutorial_script_ops.delta_scale != NULL)
        scale = s_tutorial_script_ops.delta_scale(
            s_tutorial_script_ops.context);
    if (s_tutorial_focus_valid != 0 && s_tutorial_script_ops.camera_focus != NULL)
        s_tutorial_script_ops.camera_focus(s_tutorial_focus_id, 0.0f, 0.0f,
                                            s_tutorial_script_ops.context);
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
    }
    if (s_tutorial_script_ops.camera_state != NULL &&
        s_tutorial_script_ops.camera_state(s_tutorial_script_ops.context) != 0 &&
        s_tutorial_script_ops.camera_scroll != NULL)
        s_tutorial_script_ops.camera_scroll(100, 0.0f,
                                            s_tutorial_script_ops.context);
}

static void tutorial_inline_append(char* out, uint32_t* used, const char* text)
{
    uint32_t i;
    if (out == NULL || used == NULL || text == NULL) return;
    for (i = 0; text[i] != '\0' && *used < 511; ++i)
        out[(*used)++] = text[i];
    out[*used] = '\0';
}

// FUNCTION: LEGORR 0x00456830
void tutorial_inline_token_render(char* source, char* output, int draw)
{
    uint32_t used = 0;
    int text_x = 0;
    uint32_t i = 0;
    if (source == NULL) return;
    if (output != NULL) output[0] = '\0';
    while (source[i] != '\0') {
        char marker = source[i];
        if ((marker == '<' || marker == '#') && source[i + 1] != '\0') {
            uint32_t start = i + 1;
            uint32_t end = start;
            int kind = marker == '<' ? 1 : 2;
            char name[256];
            uint32_t len;
            uint32_t n;
            while (source[end] != '\0' && source[end] != (marker == '<' ? '>' : '#') &&
                   end - start < 255) ++end;
            if (source[end] == (marker == '<' ? '>' : '#')) {
                len = end - start;
                memcpy(name, source + start, len);
                name[len] = '\0';
                for (n = 0; n < s_tutorial_token_count; ++n) {
                    if (s_tutorial_tokens[n].kind == kind &&
                        tutorial_text_equal(s_tutorial_tokens[n].name, name)) {
                        if (kind == 1) {
                            tutorial_inline_append(output, &used,
                                                   s_tutorial_tokens[n].name);
                            if (draw && s_tutorial_script_ops.text_draw != NULL)
                                s_tutorial_script_ops.text_draw(
                                    s_tutorial_tokens[n].name, text_x, 0,
                                    s_tutorial_script_ops.context);
                            text_x += (int)strlen(s_tutorial_tokens[n].name);
                        } else if (s_tutorial_script_ops.command != NULL) {
                            s_tutorial_script_ops.command(
                                s_tutorial_tokens[n].value, NULL, 0,
                                s_tutorial_script_ops.context);
                        }
                        break;
                    }
                }
                i = end + 1;
                continue;
            }
        }
        if (output != NULL && used < 511) {
            output[used++] = source[i];
            output[used] = '\0';
        }
        if (draw && s_tutorial_script_ops.text_draw != NULL) {
            char one[2]; one[0] = source[i]; one[1] = '\0';
            s_tutorial_script_ops.text_draw(one, text_x, 0,
                                            s_tutorial_script_ops.context);
        }
        ++text_x;
        ++i;
    }
}

uint32_t tutorial_script_word_count(void) { return s_tutorial_program_count; }
uint32_t tutorial_token_count(void) { return s_tutorial_token_count; }
const char* tutorial_token_name_get(uint32_t index)
{
    return index < s_tutorial_token_count ? s_tutorial_tokens[index].name : NULL;
}
uint32_t tutorial_token_value_get(uint32_t index)
{
    return index < s_tutorial_token_count ? s_tutorial_tokens[index].value : 0;
}

typedef struct LegacyPanelPortRecord {
    float initial[4];
    float current[4];
    uint32_t flags;
    uint32_t image_flags;
    void* image;
    int image_mode;
    uint32_t selection_flags[64];
    /* Mirrors the original per-entry value word (record entry +0x24),
       cleared by the interaction-state reset alongside selection_flags. */
    uint32_t selection_values[64];
    uint32_t selection_count;
} LegacyPanelPortRecord;

typedef struct LegacyPanelImagePortEntry {
    void* surface;
    int x;
    int y;
    int width;
    int height;
    int panel;
    uint32_t flags;
} LegacyPanelImagePortEntry;

static LegacyHudOps s_legacy_hud_ops;
static LegacyHealthBarState s_legacy_health_bar;
static LegacyPanelPortRecord s_legacy_panels[18];
static void* s_legacy_hunger_images[5];
static float s_legacy_hunger_position[2];
static float s_legacy_bubble_position[2];
static uint32_t s_legacy_hud_state_flags;
static float s_legacy_entity_ring_radius;
static float s_legacy_entity_ring_height;

void legacy_hud_ops_set(const LegacyHudOps* ops)
{
    if (ops != NULL) s_legacy_hud_ops = *ops;
    else memset(&s_legacy_hud_ops, 0, sizeof(s_legacy_hud_ops));
}

static int legacy_config_pair(BlockFile* cfg, const char* path, int* x, int* y)
{
    char text[128];
    char* p;
    char* end;
    long a;
    long b;
    if (cfg == NULL || path == NULL || x == NULL || y == NULL) return 0;
    if (s_legacy_hud_ops.config_pair != NULL)
        return s_legacy_hud_ops.config_pair(cfg, path, x, y,
                                            s_legacy_hud_ops.context);
    if (!block_get_str(cfg, path, text, sizeof(text))) return 0;
    p = text;
    a = strtol(p, &end, 10);
    if (end == p) return 0;
    p = end;
    while (*p == ' ' || *p == '\t' || *p == ',' || *p == ';') ++p;
    b = strtol(p, &end, 10);
    if (end == p) return 0;
    *x = (int)a;
    *y = (int)b;
    return 1;
}

static int legacy_config_rgb(BlockFile* cfg, const char* path, float rgb[3])
{
    if (cfg == NULL || path == NULL || rgb == NULL) return 0;
    if (s_legacy_hud_ops.config_rgb != NULL)
        return s_legacy_hud_ops.config_rgb(cfg, path, rgb,
                                           s_legacy_hud_ops.context);
    return config_rgb_vector_read(cfg, path, &rgb[0], &rgb[1], &rgb[2]);
}

static float legacy_clamp_01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void legacy_color_pair(float source[3], float high[3], float low[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        high[i] = legacy_clamp_01(source[i] + source[i] * 0.4f);
        low[i] = legacy_clamp_01(source[i] - source[i] * 0.4f);
    }
}

// FUNCTION: LEGORR 0x00459560
int legacy_health_bar_config_load(BlockFile* values, BlockFile* names)
{
    int x, y;
    int width, height;
    BlockFile* cfg = values != NULL ? values : names;
    BlockFile* key_cfg = names != NULL ? names : values;
    int border;
    int vertical;
    float rgb[3];
    if (cfg == NULL || key_cfg == NULL) return 0;
    if (!legacy_config_pair(cfg, "ObjInfo::HealthBarPosition", &x, &y)) return 0;
    if (!legacy_config_pair(cfg, "ObjInfo::HealthBarWidthHeight",
                            &width, &height)) return 0;
    // The original derives this through an integer conversion of the raw
    // value string, so a missing or empty key yields 0 rather than 1.
    border = block_get_int(cfg, "ObjInfo::HealthBarBorderSize", 0);
    if (!legacy_config_rgb(cfg, "ObjInfo::HealthBarBorderRGB", rgb)) return 0;
    s_legacy_health_bar.x = (float)x;
    s_legacy_health_bar.y = (float)y;
    s_legacy_health_bar.width = (float)width;
    s_legacy_health_bar.height = (float)height;
    s_legacy_health_bar.border = (float)border;
    memcpy(s_legacy_health_bar.border_rgb, rgb, sizeof(rgb));
    legacy_color_pair(s_legacy_health_bar.border_rgb,
                      s_legacy_health_bar.border_high,
                      s_legacy_health_bar.border_low);
    if (!legacy_config_rgb(cfg, "ObjInfo::HealthBarBackgroundRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.background_rgb, rgb, sizeof(rgb));
    if (!legacy_config_rgb(cfg, "ObjInfo::HealthBarRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.health_rgb, rgb, sizeof(rgb));
    vertical = block_get_bool(cfg, "ObjInfo::HealthBarVertical");
    /* Mirrors the original's writes to the shared HUD state word
       (DAT_004ffe68): the loaded bit (2) is always set and the vertical
       bit (8) is set only when the key reads as 1; neither is cleared.
       The port models those two bits on the health-bar state record that
       the renderer gates on. */
    s_legacy_health_bar.vertical = vertical == 1 ? 1u : 0u;
    s_legacy_health_bar.flags |= 2u;
    if (vertical == 1) s_legacy_health_bar.flags |= 8u;
    return 1;
}

static void legacy_hud_resource_name(BlockFile* cfg, const char* path,
                                     char* out, uint32_t out_size)
{
    out[0] = '\0';
    if (cfg == NULL || path == NULL || out_size == 0) return;
    if (s_legacy_hud_ops.config_resource != NULL) {
        s_legacy_hud_ops.config_resource(cfg, path, out, out_size,
                                        s_legacy_hud_ops.context);
    } else {
        block_get_str(cfg, path, out, (size_t)out_size);
    }
}

// FUNCTION: LEGORR 0x004599c0
void legacy_hunger_images_load(BlockFile* values, BlockFile* names)
{
    int i;
    char path[96];
    char resource_name[256];
    BlockFile* cfg = values != NULL ? values : names;
    // The original re-reads the five ObjInfo::HungerImages::HungerImage%i
    // keys and overwrites each slot without releasing the previous surface
    // (the resource layer owns surface lifetime).
    for (i = 0; i < 5; ++i) {
        void* image = NULL;
        sprintf(path, "ObjInfo::HungerImages::HungerImage%i", i);
        legacy_hud_resource_name(cfg, path, resource_name,
                                 (uint32_t)sizeof(resource_name));
        if (resource_name[0] != '\0' &&
            s_legacy_hud_ops.surface_load != NULL) {
            image = s_legacy_hud_ops.surface_load(
                cfg, resource_name, 0, 0, s_legacy_hud_ops.context);
            if (image != NULL && s_legacy_hud_ops.surface_initialize != NULL)
                s_legacy_hud_ops.surface_initialize(
                    image, s_legacy_hud_ops.context);
        }
        s_legacy_hunger_images[i] = image;
    }
}

// FUNCTION: LEGORR 0x00459900
int legacy_hunger_images_position_config_load(BlockFile* values,
                                               BlockFile* names)
{
    int x, y;
    BlockFile* cfg = values != NULL ? values : names;
    legacy_hunger_images_load(values, names);
    if (cfg == NULL || !legacy_config_pair(cfg,
            "ObjInfo::HungerImagesPosition", &x, &y)) return 0;
    s_legacy_hunger_position[0] = (float)x;
    s_legacy_hunger_position[1] = (float)y;
    s_legacy_hud_state_flags |= 1u;
    return 1;
}

int legacy_bubble_images_position_config_load(BlockFile* values,
                                              BlockFile* names)
{
    int x, y;
    BlockFile* cfg = values != NULL ? values : names;
    if (cfg == NULL || !legacy_config_pair(cfg,
            "ObjInfo::BubbleImagesPosition", &x, &y)) return 0;
    s_legacy_bubble_position[0] = (float)x;
    s_legacy_bubble_position[1] = (float)y;
    s_legacy_hud_state_flags |= 0x10u;
    return 1;
}

void legacy_health_bar_config_peek(LegacyHealthBarState* state)
{
    if (state != NULL) *state = s_legacy_health_bar;
}

static uint32_t legacy_rgb_pack(const float rgb[3])
{
    uint32_t r = (uint32_t)(legacy_clamp_01(rgb[0]) * 255.0f);
    uint32_t g = (uint32_t)(legacy_clamp_01(rgb[1]) * 255.0f);
    uint32_t b = (uint32_t)(legacy_clamp_01(rgb[2]) * 255.0f);
    return (r << 16) | (g << 8) | b;
}

// FUNCTION: LEGORR 0x00459b00
void legacy_entity_screen_ring_render(void* entity, int x, int y)
{
    EntitySlot* slot = (EntitySlot*)entity;
    float rect[4];
    float health;
    float width;
    float height;
    float border;
    float health_size;
    int i;
    if (slot == NULL || (s_legacy_health_bar.flags & 2u) == 0) return;
    if (s_legacy_hud_ops.entity_renderable == NULL ||
        !s_legacy_hud_ops.entity_renderable(slot,
                                             s_legacy_hud_ops.context)) return;
    health = slot->field_340;
    if (health < 0.0f) health = 0.0f;
    if (health > 100.0f) health = 100.0f;
    border = s_legacy_health_bar.border;
    width = s_legacy_health_bar.width + border * 2.0f;
    height = s_legacy_health_bar.height + border * 2.0f;
    rect[0] = (float)x + s_legacy_health_bar.x;
    rect[1] = (float)y + s_legacy_health_bar.y;
    if (s_legacy_hud_ops.rect_draw != NULL) {
        if (s_legacy_health_bar.vertical == 0u) {
            health_size = health * width * 0.01f;
            rect[2] = health_size;
            rect[3] = height;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
            rect[0] += health_size;
            rect[2] = width - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
        }
        else {
            health_size = (100.0f - health) * height * 0.01f;
            rect[2] = width;
            rect[3] = health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
            rect[1] += health_size;
            rect[3] = height - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
        }
    }
    if (s_legacy_hud_ops.rect_draw == NULL || border <= 0.0f) return;
    for (i = 0; i < (int)border; ++i) {
        float inset = (float)i;
        float band = 1.0f;
        uint32_t high = legacy_rgb_pack(s_legacy_health_bar.border_high);
        uint32_t low = legacy_rgb_pack(s_legacy_health_bar.border_low);
        rect[0] = (float)x + s_legacy_health_bar.x + inset;
        rect[1] = (float)y + s_legacy_health_bar.y + inset;
        rect[2] = width - inset * 2.0f;
        rect[3] = band;
        s_legacy_hud_ops.rect_draw(rect, high, s_legacy_hud_ops.context);
        rect[1] = (float)y + s_legacy_health_bar.y + height - inset - band;
        s_legacy_hud_ops.rect_draw(rect, low, s_legacy_hud_ops.context);
        rect[0] = (float)x + s_legacy_health_bar.x + inset;
        rect[1] = (float)y + s_legacy_health_bar.y + inset;
        rect[2] = band;
        rect[3] = height - inset * 2.0f;
        s_legacy_hud_ops.rect_draw(rect, high, s_legacy_hud_ops.context);
        rect[0] = (float)x + s_legacy_health_bar.x + width - inset - band;
        s_legacy_hud_ops.rect_draw(rect, low, s_legacy_hud_ops.context);
    }
}

// FUNCTION: LEGORR 0x0045a240
void legacy_scroll_panel_controls_initialize(void)
{
    /* The original initialises the scroll info panel, creates the
       configured overlay element, then calls FUN_0045abb0(6, 1, 1) and
       FUN_0045abb0(6, 2, 1) — the panel-item request-state setter. */
    ui_scroll_info_panel_initialize(s_legacy_hud_ops.overlay_init_sink);
    ui_overlay_create_configured_element(s_legacy_hud_ops.overlay_init_sink);
    panel_item_flag32_set(6, 1, 1);
    panel_item_flag32_set(6, 2, 1);
}

// FUNCTION: LEGORR 0x0045a270
void legacy_panel_category_interaction_state_reset(void)
{
    int i;
    /* The original walks the twelve records between DAT_00500110 and
       0x500350 (stride 0x30), restores the two view-offset dwords from
       their initial pair, forces the record flags to (f & ~0xa) | 4, and
       clears each selection entry's value word and its 0x48 flag bits.
       It does not touch the HUD state-flag word. */
    for (i = 0; i < 12; ++i) {
        uint32_t j;
        LegacyPanelPortRecord* record = &s_legacy_panels[i];
        record->current[0] = record->initial[0];
        record->current[1] = record->initial[1];
        record->flags = (record->flags & ~0xau) | 4u;
        for (j = 0; j < record->selection_count && j < 64; ++j) {
            record->selection_values[j] = 0;
            record->selection_flags[j] &= ~0x48u;
        }
    }
    overlay_anim_float_install(0.0f);
}

// FUNCTION: LEGORR 0x0045a2e0
void legacy_overlay_image_record_load(BlockFile* cfg, int panel,
                                     const char* path)
{
    LegacyPanelPortRecord* record;
    /* The original clears the record's entry count, stores the caller's
       state word (mapped to 0 through this port seam), and then tries the
       primary resource load with mode 0 followed by initialisation, or the
       alternate resource load with mode 1.  It never releases the previous
       surface; the resource layer owns surface lifetime. */
    if (panel < 0 || panel >= 18) return;
    record = &s_legacy_panels[panel];
    record->selection_count = 0;
    record->image_mode = 0;
    record->image_flags = 0;
    record->image = NULL;
    if (path != NULL && s_legacy_hud_ops.surface_load != NULL)
        record->image = s_legacy_hud_ops.surface_load(
            cfg, path, 0, 0, s_legacy_hud_ops.context);
    if (record->image != NULL) {
        if (s_legacy_hud_ops.surface_initialize != NULL)
            s_legacy_hud_ops.surface_initialize(record->image,
                                                s_legacy_hud_ops.context);
        record->image_mode = 0;
        record->image_flags |= 1u;
        return;
    }
    if (path != NULL && s_legacy_hud_ops.surface_alternate_load != NULL) {
        /* The original's alternate path is FUN_00483c80(path, record, 3);
           the panel stands in for the record and 3 is its load mode. */
        record->image = s_legacy_hud_ops.surface_alternate_load(
            cfg, path, panel, 3, s_legacy_hud_ops.context);
        if (record->image != NULL) {
            record->image_mode = 1;
            record->image_flags |= 1u;
        }
    }
}

// FUNCTION: LEGORR 0x0045a3b0
int legacy_panel_slot_coordinate_hit_test(int panel, int x, int y)
{
    LegacyPanelPortRecord* record;
    int hit;
    /* Preserve the port record-array bound; the corpus directly indexes its
       panel record and has no corresponding range branch. */
    if (panel < 0 || panel >= 18) return 1;
    record = &s_legacy_panels[panel];
    if (record->image_mode != 0 || record->image == NULL ||
        s_legacy_hud_ops.surface_hit == NULL) return 1;
    hit = s_legacy_hud_ops.surface_hit(record->image,
        x - (int)record->current[0], y - (int)record->current[1],
        s_legacy_hud_ops.context);
    return hit ? panel == 0 : 1;
}

// FUNCTION: LEGORR 0x0045a500
void* legacy_panel_image_slot_entry_allocate(int panel, const char* path,
                                             const int rect[4],
                                             uint32_t flags)
{
    LegacyPanelImagePortEntry* entry;
    BlockFile* cfg = NULL;
    if (panel < 0 || panel >= 18 || rect == NULL) return NULL;
    entry = (LegacyPanelImagePortEntry*)engine_heap_alloc(
        (long)sizeof(LegacyPanelImagePortEntry));
    if (entry == NULL) return NULL;
    memset(entry, 0, sizeof(*entry));
    entry->x = rect[0];
    entry->y = rect[1];
    entry->width = rect[2];
    entry->height = rect[3];
    entry->panel = panel;
    entry->flags = flags;
    if (path != NULL && s_legacy_hud_ops.surface_load != NULL)
        entry->surface = s_legacy_hud_ops.surface_load(
            cfg, path, rect[2], rect[3], s_legacy_hud_ops.context);
    return entry;
}

void legacy_scrolling_advisor_text_append(void* record, const char* text)
{
    if (record == NULL || text == NULL) return;
    text_buffer_append_format(record, 0, "%s", text);
}

void legacy_ui_text_output_position_and_draw(void* record, const char* text,
                                             const char* fallback)
{
    LegacyPanelImagePortEntry* entry = (LegacyPanelImagePortEntry*)record;
    const char* draw_text = text != NULL ? text : fallback;
    int x, y;
    if (entry == NULL || draw_text == NULL) return;
    x = entry->x;
    y = entry->y;
    if (entry->panel >= 0 && entry->panel < 18) {
        x += (int)s_legacy_panels[entry->panel].current[0];
        y += (int)s_legacy_panels[entry->panel].current[1];
    }
    if (s_legacy_hud_ops.text_measure != NULL)
        s_legacy_hud_ops.text_measure(draw_text, s_legacy_hud_ops.context);
    if (s_legacy_hud_ops.text_draw != NULL)
        s_legacy_hud_ops.text_draw(draw_text, x, y,
                                   s_legacy_hud_ops.context);
}

uint32_t legacy_panel_category_flags_get(int panel)
{
    return panel >= 0 && panel < 18 ? s_legacy_panels[panel].flags : 0;
}

void legacy_panel_category_flags_set(int panel, uint32_t flags)
{
    if (panel >= 0 && panel < 18) s_legacy_panels[panel].flags = flags;
}

void legacy_panel_category_record_install(int panel, const float rect[4],
                                         uint32_t flags,
                                         const uint32_t* selections,
                                         uint32_t selection_count)
{
    LegacyPanelPortRecord* record;
    uint32_t i;
    if (panel < 0 || panel >= 18 || rect == NULL) return;
    record = &s_legacy_panels[panel];
    memcpy(record->initial, rect, sizeof(record->initial));
    memcpy(record->current, rect, sizeof(record->current));
    record->flags = flags;
    if (selection_count > 64) selection_count = 64;
    record->selection_count = selection_count;
    for (i = 0; i < selection_count; ++i) {
        record->selection_flags[i] = selections != NULL ? selections[i] : 0;
        record->selection_values[i] = 0;
    }
}

void* legacy_panel_image_get(int panel)
{
    return panel >= 0 && panel < 18 ? s_legacy_panels[panel].image : NULL;
}

void legacy_panel_image_slot_entry_release(void* opaque)
{
    LegacyPanelImagePortEntry* entry = (LegacyPanelImagePortEntry*)opaque;
    if (entry == NULL) return;
    if (entry->surface != NULL && s_legacy_hud_ops.surface_release != NULL)
        s_legacy_hud_ops.surface_release(entry->surface,
                                         s_legacy_hud_ops.context);
    engine_heap_free(entry);
}

/* ObjectiveSidecarAppendGate (FUN_0043bc40): the corpus has many engine
 * callbacks in this gate. This stateful port preserves its early gates and
 * action order while exposing those engine transitions as a narrow hook. */
// FUNCTION: LEGORR 0x0043bc40
int objective_sidecar_append_gate(ObjectiveSidecarSnapshot* state,
                                  void* context,
                                  ObjectiveSidecarActionHook action)
{
    if (state == NULL) return 0;
    if (state->type != 4
        && ((state->status & 0x08000000u) != 0u
            || (state->state & 0x00200000u) != 0u)) return 0;
    if (state->activity_timer > 0.0f) return 0;
    if ((state->status & 0x80000070u) != 0u && state->linked) {
        if (action == NULL) return 0;
        action(context, 1, state); /* cancel linked special activity */
        state->linked = 0;
    }
    if ((state->mode_flags & 0x00800000u) != 0u) {
        if (action == NULL) return 0;
        action(context, 2, state); /* release sidecar and stop motion */
        if (state->type == 4 || state->type == 5)
            action(context, state->type == 4 ? 3 : 4, state);
        else action(context, 5, state);
        return 1;
    }
    if (state->type == 3) {
        if ((state->status & 0x8000u) == 0u) {
            if ((state->state & 0x00100000u) == 0u) return 0;
            if (action == NULL) return 0;
            state->state |= 0x00020000u;
            action(context, 6, state);
            return 1;
        }
        if ((state->state & 0x00100000u) != 0u) return 0;
        if (action == NULL) return 0;
        action(context, 7, state);
        return 1;
    }
    if (state->type == 6) {
        state->mode_flags |= 0x00800000u;
        return 1;
    }
    if ((state->type == 4 || state->type == 6)
        && (state->entity_flags & 0x40000u) == 0u) return 0;
    if ((state->state & 0x00400000u) != 0u) return 0;
    if (action == NULL) return 0;
    action(context, 8, state); /* capture state and detach stale owner */
    if (state->type == 0xb) action(context, 9, state);
    action(context, 10, state); /* append/update objective sidecar */
    return 1;
}

/* UpgradeLevelTableParse (FUN_0046c160): enumerate the sixteen level groups
 * in order, append each parsed row to the fixed 1000-record table, and track
 * which level bitmasks were present. */
void upgrade_level_table_parse(UpgradeLevelTable* table, void* context,
                               UpgradeLevelEntryReadHook read_entry)
{
    uint32_t level;
    if (table == NULL) return;
    memset(table, 0, sizeof(*table));
    if (read_entry == NULL) return;
    for (level = 0; level < 16u; ++level) {
        uint32_t ordinal = 0;
        for (;;) {
            UpgradeLevelEntry entry;
            memset(&entry, 0, sizeof(entry));
            if (!read_entry(context, level, ordinal, &entry)) break;
            if (table->count >= 1000u) return;
            entry.level = level;
            table->entries[table->count++] = entry;
            table->level_mask |= level;
            ++ordinal;
        }
    }
}

/* VehicleTypeDefinitionParse (FUN_0046c6f0): the source parser zeroes its
 * 0x7b-word vehicle record before reading config. Keep that initialization
 * and provide an indexed field reader for the config library boundary. */
int vehicle_type_definition_parse(VehicleTypeDefinition* definition,
                                  void* context,
                                  VehicleTypeFieldReadHook read_field)
{
    uint32_t index;
    uint32_t present = 0;
    if (definition == NULL) return 0;
    memset(definition, 0, sizeof(*definition));
    if (read_field == NULL) return 0;
    for (index = 0; index < 0x7bu; ++index) {
        uint32_t value = 0;
        if (read_field(context, index, &value)) {
            definition->words[index] = value;
            ++present;
        }
    }
    return present != 0u;
}

static int port_loop_vector_normalize(float vector[3])
{
    float length = (float)sqrt((double)(vector[0] * vector[0]
                              + vector[1] * vector[1]
                              + vector[2] * vector[2]));
    if (length < 0.000001f) return 0;
    vector[0] /= length; vector[1] /= length; vector[2] /= length;
    return 1;
}

/* AttachedModelOrientationUpdate (FUN_0046d380): derive an averaged surface
 * normal from the attached points, smooth it against the previous normal,
 * then orthogonalize the stored up vector and publish the resulting basis. */
int attached_model_orientation_update(AttachedModelOrientationState* state,
                                      void* context,
                                      AttachedModelOrientationApplyHook apply)
{
    float a[3], b[3], normal[3], up[3], right[3];
    uint32_t i;
    if (state == NULL || apply == NULL || state->count < 3u || state->count > 8u)
        return 0;
    a[0] = state->points[1][0] - state->points[0][0];
    a[1] = state->points[1][1] - state->points[0][1];
    a[2] = state->points[1][2] - state->points[0][2];
    b[0] = state->points[2][0] - state->points[0][0];
    b[1] = state->points[2][1] - state->points[0][1];
    b[2] = state->points[2][2] - state->points[0][2];
    normal[0] = a[1] * b[2] - a[2] * b[1];
    normal[1] = a[2] * b[0] - a[0] * b[2];
    normal[2] = a[0] * b[1] - a[1] * b[0];
    if (!port_loop_vector_normalize(normal)) return 0;
    for (i = 3; i < state->count; ++i) {
        float c[3], d[3], n[3];
        c[0] = state->points[i - 1][0] - state->points[0][0];
        c[1] = state->points[i - 1][1] - state->points[0][1];
        c[2] = state->points[i - 1][2] - state->points[0][2];
        d[0] = state->points[i][0] - state->points[0][0];
        d[1] = state->points[i][1] - state->points[0][1];
        d[2] = state->points[i][2] - state->points[0][2];
        n[0] = c[1] * d[2] - c[2] * d[1];
        n[1] = c[2] * d[0] - c[0] * d[2];
        n[2] = c[0] * d[1] - c[1] * d[0];
        if (port_loop_vector_normalize(n)) {
            normal[0] += n[0]; normal[1] += n[1]; normal[2] += n[2];
        }
    }
    if (!port_loop_vector_normalize(normal)) return 0;
    if (port_loop_vector_normalize(state->normal)) {
        normal[0] = normal[0] * 0.6f + state->normal[0] * 0.4f;
        normal[1] = normal[1] * 0.6f + state->normal[1] * 0.4f;
        normal[2] = normal[2] * 0.6f + state->normal[2] * 0.4f;
        if (!port_loop_vector_normalize(normal)) return 0;
    }
    up[0] = state->up[0]; up[1] = state->up[1]; up[2] = state->up[2];
    {
        float projection = up[0] * normal[0] + up[1] * normal[1]
                         + up[2] * normal[2];
        up[0] -= projection * normal[0];
        up[1] -= projection * normal[1];
        up[2] -= projection * normal[2];
    }
    if (!port_loop_vector_normalize(up)) {
        up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
        if (fabs((double)(up[0] * normal[0] + up[1] * normal[1]
                          + up[2] * normal[2])) > 0.95) {
            up[0] = 1.0f; up[1] = 0.0f; up[2] = 0.0f;
        }
        {
            float projection = up[0] * normal[0] + up[1] * normal[1]
                             + up[2] * normal[2];
            up[0] -= projection * normal[0];
            up[1] -= projection * normal[1];
            up[2] -= projection * normal[2];
        }
        if (!port_loop_vector_normalize(up)) return 0;
    }
    right[0] = up[1] * normal[2] - up[2] * normal[1];
    right[1] = up[2] * normal[0] - up[0] * normal[2];
    right[2] = up[0] * normal[1] - up[1] * normal[0];
    if (!port_loop_vector_normalize(right)) return 0;
    state->normal[0] = normal[0]; state->normal[1] = normal[1];
    state->normal[2] = normal[2];
    state->up[0] = up[0]; state->up[1] = up[1]; state->up[2] = up[2];
    apply(context, right, up, normal);
    return 1;
}
