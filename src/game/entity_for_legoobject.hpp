// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from entity.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_FOR_LEGOOBJECT_HPP
#define LRR_GAME_ENTITY_FOR_LEGOOBJECT_HPP

#include "entity_for_lego.hpp"

typedef struct EntityActionConsumeCounterSidecar {
    EntitySlot* owner;
    uint32_t* values;
    struct EntityActionConsumeCounterSidecar* next;
} EntityActionConsumeCounterSidecar;

typedef struct EntityPathMotionRuntime {
    EntitySlot* owner;
    int waypoint_index;
    float total_length;
    float progress;
    float carry_progress;
    float start[2];
    float target[2];
    float vector[2];
    uint32_t sample_count;
    float samples[50][2];
    float segment_lengths[49];
} EntityPathMotionRuntime;

typedef struct EntityRouteMarkerSearchRequest {
    const float* position;
    uint32_t flags;
    float best_distance;
    int cell_x;
    int cell_y;
    int action_argument;
    int requested_state;
    EntitySlot* match;
} EntityRouteMarkerSearchRequest;

#define POOL_CHUNK_COUNT_SLOTS 32 /* deviation: bounded chunk table */

namespace unported_ledger_stubs {
// SpatialEntityCallbackQuery
typedef struct SpatialEntityQueryRequest {
    EntitySlot* subject;
    float position[2];
    float values[3];
    int callback_kind;
} SpatialEntityQueryRequest;
} // namespace unported_ledger_stubs

int entity_pool_find_by_callback_filtered(GameContext* ctx, int (*callback)(EntitySlot*, void*), void* payload);
namespace unported_ledger_stubs {
int entity_path_waypoint_world_get(const EntitySlot* entity, unsigned int index, float position[2], int project_surface);
void ledger_stub_00447950_EntityDirectionalActivityStart( EntitySlot* entity, const float direction[2], int mode);
} // namespace unported_ledger_stubs
extern EntityActorActivityAdvanceHook s_actor_activity_advance_hook;
extern void* s_actor_activity_advance_context;
extern char* s_entity_kind_names[16];
extern uint32_t s_entity_kind_name_counts[16];
extern const char** s_object_name_kind1;
extern const char** s_object_name_kind2;
extern const char** s_object_name_kind3;
extern const char** s_object_name_kind4;
extern const char** s_object_name_kind10;
extern const char** s_object_the_name_kind1;
extern const char** s_object_the_name_kind2;
extern const char** s_object_the_name_kind3;
extern const char** s_object_the_name_kind4;
extern const char** s_object_the_name_kind10;
extern const char* s_object_name_special[9];
extern const char* s_object_the_name_special[9];
extern EntityHealthThresholdReadHook s_entity_health_threshold_read_hook;
extern int s_entity_pair_activity_sentinel;
extern EntityVisualResourcePresentHook s_entity_visual_resource_present_hook;
extern EntityVisualResourceCreateHook s_entity_visual_resource_create_hook;
extern EntityTeardownHooks s_entity_teardown_hooks;
extern void* s_entity_visual_resource_context;
extern EntityIndexedFlagPublishHook s_entity_indexed_flag_publish_hook;
extern void* s_entity_indexed_flag_context;
extern EntityToolNameLoadHook s_entity_tool_name_load_hook;
extern void* s_entity_tool_name_context;
extern void* s_entity_tool_name_handles[11];
extern EntityTypeTwoEventHook s_entity_type_two_event_hook;
extern void* s_entity_type_two_event_context;
extern EntityTypeTwoPositionedHooks s_entity_type_two_positioned_hooks;
extern GameContext* s_entity_task_count_context;
extern EntityTaskCoordinateMatchHook s_entity_task_coordinate_match_hook;
extern void* s_entity_task_coordinate_match_user;
extern EntityBuildingDepositHook s_entity_building_deposit_hook;
extern void* s_entity_building_deposit_context;
extern EntityDepositActivityHooks s_entity_deposit_activity_hooks;
extern EntityBuildPlacementHooks s_entity_build_placement_hooks;
extern GameContext* s_entity_build_placement_context;
extern EntityActionIssueHooks s_entity_action_issue_hooks;
extern EntityActionConsumeHooks s_entity_action_consume_hooks;
extern EntityActionConsumeCounterSidecar* s_entity_action_counter_sidecars;
extern EntityClippedRectangleHooks s_entity_clipped_rectangle_hooks;
extern void* s_entity_health_threshold_context;
uint32_t* entity_action_consume_counter_sidecar_get(EntitySlot* owner, int create);
extern EntityPathMotionRuntime s_entity_path_motion_runtime[ ENTITY_TYPES * ENTITY_CAP];
void entity_path_motion_runtime_clear(EntitySlot* entity);
extern EntityPeriodicTickHook s_entity_periodic_tick_hook;
extern void* s_entity_periodic_tick_context;
extern float s_periodic_timer25;
extern uint32_t s_entity_finalize_mode_flags;
extern EntityFinalizeHook s_entity_type5_timeout_hook;
extern EntityFinalizeHook s_entity_type5_finalize_hook;
extern EntitySurfaceVectorGetHook s_entity_surface_vector_get_hook;
extern EntityFlagDispatchHook s_entity_flag_dispatch_first;
extern EntityFlagDispatchHook s_entity_flag_dispatch_second;
extern EntityVelocityApplyHook s_entity_velocity_hooks[5];
extern EntityRouteSegmentAcceptHook s_entity_route_segment_accept_hook;
extern EntityScenePoseHooks s_entity_scene_pose_hooks;
extern EntityRouteStateHook s_entity_route_state_hook;
extern GameContext* s_entity_pool_callback_context;
extern EntityCallbackDispatchHook s_entity_callback_dispatch_hook;
extern void* s_entity_callback_dispatch_context;
extern EntityActionPrerequisiteHooks s_entity_action_prerequisite_hooks;
extern EntityRouteMarkerCandidateHook s_entity_route_marker_candidate_hook;
extern void* s_entity_route_marker_candidate_context;
extern EntityKindRecordUpdateHooks s_entity_kind_record_update_hooks;
extern GridObjectRecord s_grid_object_records[200];
extern unsigned int s_grid_object_record_count;
extern GridObjectRecordsReleaseHook s_grid_object_records_release_tail;
extern void* s_grid_object_records_release_context;
extern QueuedGridAnimationHooks s_queued_grid_animation_hooks;
extern float s_queued_grid_animation_fraction;
extern RoverWorkQueueHooks s_rover_work_queue_hooks;
extern BuildingUpgradeAdmissionHooks s_building_upgrade_admission_hooks;
extern BuildingUpgradeCommitHooks s_building_upgrade_commit_hooks;
extern DeferredOListEntityRecord s_deferred_olist_records[200];
extern unsigned int s_deferred_olist_record_count;
extern DeferredOListEntityHooks s_deferred_olist_hooks;
void* entity_spatial_object_query_hook( void* context, const SpatialObjectQueryRequest* query);
extern uint32_t s_entity_iteration_guard_flags;
extern EntityOneShotAttachedEffectHooks s_one_shot_attached_effect_hooks;
namespace unported_ledger_stubs {
extern CellStepPathResolveHook s_cell_step_path_hook;
extern CellStepRandomRangeHook s_cell_step_random_hook;
extern void* s_cell_step_hook_user;
extern EntityPairPoseHooks s_entity_pair_pose_hooks;
extern EntityObjectPositionSceneResolveHook s_entity_object_position_scene_resolve_hook;
extern void* s_entity_object_position_scene_resolve_context;
int entity_spatial_query_dispatch_kind(EntitySlot* subject, const float position[2], const float values[3], int callback_kind);
float entity_movement_default_surface_scale_get(void* context, void* entity);
float entity_movement_default_entity_scale_get(void* context, void* entity);
int entity_movement_default_terrain_data_get( void* context, void* entity, int* cell_cost, int* terrain_type, int* occupiable, int* entity_scale_gate);
float entity_movement_default_base_speed_get(void* context, void* entity, int kind);
float entity_movement_default_animation_scale_get(void* context, void* entity);
} // namespace unported_ledger_stubs

extern "C" float random_float_range(float minimum, float maximum);

extern "C" float random_float_range(float minimum, float maximum);

namespace unported_ledger_stubs {
static inline int entity_movement_scale_impl(
    void* entity, float delta, float* movement_delta,
    float* movement_scale)
{
    EntityMovementTickScaleState state;
    EntityMovementTickScaleHooks hooks;
    const EntitySlot* slot = (const EntitySlot*)entity;
    if (slot == NULL || movement_delta == NULL || movement_scale == NULL)
        return 0;
    memset(&state, 0, sizeof(state));
    state.kind = slot->type;
    state.flags_f8 = slot->flags_3e0;
    state.flags_f9 = slot->flags_3e4;
    state.flags_fa = slot->flags_3e8;
    state.stats_flags = entity_stats_word_get(slot);
    state.surface_scale = entity_stats_float_atd4(slot);
    hooks.base_speed_get = entity_movement_default_base_speed_get;
    hooks.terrain_data_get = entity_movement_default_terrain_data_get;
    hooks.surface_scale_get = entity_movement_default_surface_scale_get;
    hooks.entity_scale_get = entity_movement_default_entity_scale_get;
    hooks.animation_scale_get = entity_movement_default_animation_scale_get;
    hooks.context = NULL;
    return entity_movement_tick_scale_and_delta_get(
        &state, entity, delta, movement_delta, movement_scale, &hooks);
}
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_ENTITY_FOR_LEGOOBJECT_HPP */
