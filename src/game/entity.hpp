// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — entity table + `.ol` object list (findings 10/07/12).
 *
 * `EntityPointer` (corpus engine-region.c, 0x42ed70) keeps four typed
 * entity arrays: type 1 = vehicles (stride 0x1ec), type 2 = minifigures
 * (stride 0x74 — DAT_005561d8), type 3 (stride 0x74 — 005561dc),
 * type 4 = buildings/objects (stride 0x14c — 005561e0); the `.ol` object
 * list the LevelLoad `OListFile` key names (`M01.ol`) is a block-text file
 * whose top-level `name { … }` records describe placed objects.
 *
 * The corpus's `.ol` consumer (`FUN_0042e020`) and its type resolver
 * (`FUN_0042e8b0` = ObjectTypeByName) are decompiled and mirrored here: the
 * *grammar* is corpus-verified by `entity_test --real` against all 44
 * shipped object lists (root `Lego*`, one record per object named
 * `Object1..` / other names, fields `type <name>`, `xPos`, `yPos`,
 * `heading`, `health` (default 100), optional `driving <ref>`; `Model`
 * appears nowhere in the shipped data and is kept as the port's extension
 * key), and the *codes* are decompile-verified — 1..4 from the shipped
 * `Lego.cfg` `…Types` groups, 5..19 special kinds, TVCamera −1, 0 = not
 * registered.
 */
#ifndef LEGORRR_GAME_ENTITY_HPP
#define LEGORRR_GAME_ENTITY_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TypeConfigTable TypeConfigTable;
struct InputState;
void entity_motion_inputs_initialize(EntitySlot* entity);
void entity_motion_input_accumulate(EntitySlot* entity, int forward,
                                    int strafe, float yaw);
void entity_motion_inputs_capture(const struct InputState* input);
/* Negative means unavailable/pending, zero is a checked no-motion result.
 * Unsupported nonzero input remains owned and is never cleared as zero. */
int entity_motion_idle_gate_checked(EntitySlot* entity, float delta);
int entity_motion_scale_checked(EntitySlot* entity, float delta,
                                 float* movement_delta, float* scale);
int entity_neighbor_step_idle_checked(EntitySlot* entity, int mode);
/* ACT worker boundary; production defaults to the actual retained actor
 * instance. A success/result pair is published only for the same activity. */
typedef int (*EntityActorActivityAdvanceHook)(void* context, EntitySlot* entity,
    float delta, uint32_t repeat_count, float* overflow);
void entity_actor_activity_advance_hook_set(EntityActorActivityAdvanceHook hook,
                                            void* context);
struct EntityQuadCollisionHooks;
void entity_quad_collision_hooks_set(
    const struct EntityQuadCollisionHooks* hooks);

/* Entity type constants (ObjectTypeByName FUN_0042e8b0 — decompile-verified).
 * 1..4 are the four typed tables (the `.ol`-placeable kinds); 5..19 are the
 * special-object kinds the corpus stores in per-kind singletons, which the
 * port groups under the shared special table. -1 = TVCamera (follow-camera
 * branch, no entity); 0 = unregistered. */
#define ENTITY_TYPE_VEHICLE  1
#define ENTITY_TYPE_MINIFIG  2
#define ENTITY_TYPE_OBJ      3   /* creatures/general objects (RockMonsterTypes) */
#define ENTITY_TYPE_BUILDING 4
#define ENTITY_TYPE_SPECIAL  5   /* first code of the special-object range */

/* A placed-object record produced by the `.ol` loader. */
typedef struct ObjectPlacement {
    char  name[64];
    int   type;          /* corpus code (1..19, -1 camera)           */
    char  type_name[24]; /* .ol `type` name (e.g. "SmallDigger")     */
    float x, y, z;       /* resolved world position, z is zero       */
    float rot;           /* alias of the resolved heading            */
    float heading;       /* .ol degrees converted to radians         */
    float health;        /* .ol `health` (default 100)                */
    char  driving[32];   /* .ol `driving` target object name, or ""  */
    char  model[128];    /* optional `Model` key (port extension)    */
} ObjectPlacement;

/* Six zeroed lookup maps allocated by FUN_0041aa00. The original keeps two
 * independent table capacities; this owner makes those capacities explicit.
 * Zero-initialize the owner before its first allocation; later allocations
 * release and replace its previous arrays. */
typedef struct EntityTableMaps {
    uint32_t* primary_a;
    uint8_t* flags_a;
    uint32_t* secondary_a;
    uint32_t* primary_b;
    uint8_t* flags_b;
    uint32_t* secondary_b;
    unsigned int count_a;
    unsigned int count_b;
} EntityTableMaps;
void entity_table_maps_allocate_for_counts(EntityTableMaps* maps,
                                           unsigned int count_a,
                                           unsigned int count_b);
extern "C" void entity_table_maps_allocate(void); /* FUN_0041aa00 */
void entity_table_maps_current_release(void);
const EntityTableMaps* entity_table_maps_current_peek(void);
void entity_table_maps_release(EntityTableMaps* maps);
typedef int (*EntitySurfaceVectorGetHook)(void* surface, float out_xy[2]);
void entity_surface_vector_hook_set(EntitySurfaceVectorGetHook hook);
void entity_surface_vector_normalize(void* surface, float out_xy[2]); /* FUN_004422a0 */
int entity_surface_vector_normalize_if_available(void* surface,
                                                  float out_xy[2]);
float entity_vector_direction_angle_radians_get(void* surface); /* FUN_00442480 */
extern "C" void entity_action_sequence_reset(EntitySlot* entity, int preserve_mode); /* FUN_00441940 */
/* The retail route list is represented by EntitySlot's 0x14-byte waypoint
 * records. These entry points advance the matching portable path cursor and
 * movement vector state. */
float entity_path_step_advance_and_arrival_check(EntitySlot* entity,
                                                 float delta); /* 0x4438b0 */
int entity_step_movement_vector_resolve(EntitySlot* entity, int advance,
                                        int use_saved_vector,
                                        int steering_mode); /* 0x444680 */
void entity_state_reset(EntitySlot* entity, int clear_mode, int reset_mode); /* FUN_00441b30 */
void entity_directional_activity_start(EntitySlot* entity,
                                        const float direction[2], int mode); /* FUN_00447950 */
typedef void (*EntityQueuedRecordProcessHook)(EntitySlot* source,
                                               EntitySlot* target,
                                               void* record);
typedef void (*EntityQueuedRecordReleaseHook)(void* record);
typedef int (*EntitySimulationImmediateModeHook)(EntitySlot* source);
void entity_simulation_queue_hooks_set(EntityQueuedRecordProcessHook process,
                                       EntityQueuedRecordReleaseHook release);
void entity_simulation_queue_mode_hook_set(
    EntitySimulationImmediateModeHook immediate_mode);
int entity_simulation_queue_attach(EntitySlot* source, EntitySlot* target,
                                   void* record, int record_kind);
void entity_simulation_queue_flush(EntitySlot* source); /* FUN_0043eea0 */
typedef void (*EntityFlagDispatchHook)(EntitySlot* entity, uint32_t argument_a,
                                       uint32_t argument_b, uint32_t argument_c,
                                       int* result);
void entity_flag_dispatch_hooks_set(EntityFlagDispatchHook first,
                                    EntityFlagDispatchHook second);
int entity_flag_gated_callback_dispatch(EntitySlot* entity,
                                        uint32_t argument_a,
                                        uint32_t argument_b,
                                        uint32_t argument_c); /* FUN_00445d70 */
int entity_directed_line_side_check(const EntitySlot* entity,
                                    const float point_xy[2],
                                    const float direction_xy[2]); /* FUN_00470630 */
typedef float (*EntitySegmentRadiusGetHook)(const EntitySlot* entity);
void entity_segment_radius_hook_set(EntitySegmentRadiusGetHook hook);
int entity_segment_center_proximity_check(const EntitySlot* entity,
                                          const float start_xy[2],
                                          const float end_xy[2],
                                          int check_side); /* FUN_00470540 */
typedef struct EntityRayHitSweepState {
    void* hit_object;
    float max_distance;
    const float* start;
    const float* direction;
    int hit;
    void* ignored_object; /* 0x471860 fifth arg; the ray's source object */
} EntityRayHitSweepState;
typedef struct EntityRayHitSweepOps {
    void* context;
    int (*object_position_get)(void* context, void* object,
                               float position_out[3]);
    int (*see_through_walls_enabled)(void* context);
    void (*scene_trace_prepare)(void* context,
                                EntityRayHitSweepState* state);
    int (*grid_dimensions_get)(void* context, uint32_t* width_out,
                               uint32_t* height_out);
    int (*world_to_cell)(void* context, const float point[3],
                         uint32_t* x_out, uint32_t* y_out);
    int (*cell_intersection)(void* context, uint32_t x, uint32_t y,
                             const float start[3], const float direction[3],
                             float hit_point_out[3]);
} EntityRayHitSweepOps;
void ray_hit_cell_sweep_ops_set(const EntityRayHitSweepOps* ops);
void ray_hit_cell_distance_minimize_sweep(
    EntityRayHitSweepState* state); /* FUN_00471630 */
int scene_segment_trace_nearest_hit(const float start[3],
                                   const float direction[3],
                                   void** hit_object_out,
                                   float* max_distance_io,
                                   void* ignored_object); /* FUN_00471860 */
int object_pair_ray_hit_check(void* first, void* second); /* FUN_004718d0 */
extern "C" int see_through_walls_ray_check(void* first, void* second); /* FUN_00471960 */
typedef struct EntityVelocitySteerState {
    void* primary;
    void* secondary;
    float target[3];
    float velocity[3];
    float heading;
    float max_vertical_velocity;
} EntityVelocitySteerState;
typedef void (*EntitySteerVelocityApplyFn)(
    void* context, void* entity, const float velocity[3],
    float heading, int secondary_pass);
void entity_steer_velocity_apply_hook_set(EntitySteerVelocityApplyFn hook,
                                          void* context);
void entity_velocity_steer_blend_update(EntityVelocitySteerState* state,
                                        const float target[3], float elapsed,
                                        int update_target); /* FUN_00471a60 */
int pose_offset_velocity_slot_blend_update(
    EntityVelocitySteerState* state, const float offset[3],
    const float translation[3], float elapsed); /* FUN_00471f00 */
typedef struct EntityVelocitySlotDispatchOps {
    void* context;
    int (*state_get)(void* context, void* entity, uint32_t* flags_out,
                     uint32_t* slot_count_out);
    int (*pose_slot_get)(void* context, void* entity, void* argument,
                         uint32_t slot, EntityVelocitySteerState** state_out,
                         float offset_out[3], float translation_out[3],
                         float* elapsed_out);
    void (*steer_sound_dispatch)(void* context, void* entity,
                                void* argument, uint32_t slot);
} EntityVelocitySlotDispatchOps;
void entity_velocity_slot_dispatch_ops_set(
    const EntityVelocitySlotDispatchOps* ops);
int entity_velocity_slot_dispatch(void* entity, void* argument); /* FUN_00471fc0 */
typedef struct EntityVelocitySteerDispatchOps {
    void* context;
    int (*steer_input_get)(void* context, void* entity, void* argument,
                           float delta, EntityVelocitySteerState** state_out,
                           float target_out[3], float* current_velocity_out);
    int (*effect_state_get)(void* context, uint32_t* global_flags_out);
    int (*active_entity_get)(void* context, void** entity_out);
    int (*candidate_ready)(void* context, void* entity, void* argument,
                           uint32_t candidate);
    float (*candidate_speed_scale)(void* context, void* entity,
                                   uint32_t candidate);
    int (*candidate_update)(void* context, void* entity, void* argument,
                           uint32_t candidate, float speed_scale);
    void (*first_candidate_initialize)(void* context, void* entity,
                                      void* argument);
} EntityVelocitySteerDispatchOps;
void entity_velocity_steer_dispatch_ops_set(
    const EntityVelocitySteerDispatchOps* ops);
int entity_velocity_steer_dispatch_and_sound(void* entity, void* argument,
                                             float delta); /* FUN_00471d20 */
typedef void (*EntityVelocityApplyHook)(EntitySlot* entity, float x, float y,
                                        float z);
void entity_velocity_apply_hooks_set(EntityVelocityApplyHook vehicle,
                                     EntityVelocityApplyHook minifig,
                                     EntityVelocityApplyHook object,
                                     EntityVelocityApplyHook building,
                                     EntityVelocityApplyHook special);
void entity_velocity_vector_dispatch(EntitySlot* entity, float angle,
                                     const float vector_or_null[3]); /* FUN_00442300 */
typedef int (*EntityRouteSegmentAcceptHook)(EntitySlot* entity,
                                            float x0, float y0,
                                            float x1, float y1);
void entity_route_segment_accept_hook_set(EntityRouteSegmentAcceptHook hook);
extern "C" int entity_route_setup_and_marker_associate(EntitySlot* entity,
                                            unsigned int count,
                                            const float* x_values,
                                            const float* y_values,
                                            const float* final_marker_position); /* FUN_00441700 */
int entity_waypoint_storage_reserve(EntitySlot* entity,
                                    unsigned int count);
/* Cell route -> world point, including the route's subcell weights and the
 * terrain origin/blocksize/Y inversion. The cursor, not pending, owns progress. */
int entity_route_waypoint_world_get(const EntitySlot* entity,
                                    unsigned int index, float position[2]);
int entity_route_adjacent_step_append(EntitySlot* entity, int cell_x,
                                      int cell_y); /* FUN_004468c0 */
typedef float (*EntitySurfaceScaleReadHook)(void* context);
void entity_surface_scale_read_hook_set(EntitySurfaceScaleReadHook hook,
                                        void* context);
float entity_surface_scale_get(void);
typedef struct CellEntityKindRegistry {
    uint32_t width;
    uint32_t height;
    void** records;
    void* context;
    int (*grid_to_world)(void* context, uint32_t x, uint32_t y,
                         float* world_x, float* world_y);
    void* (*create_kind14)(void* context, float world_x, float world_y);
    void (*release_entity)(void* context, void* entity);
} CellEntityKindRegistry;
void cell_entity_kind_create_or_remove(CellEntityKindRegistry* registry,
                                       uint32_t x, uint32_t y,
                                       int create); /* FUN_0040e350 */
typedef struct WorldObjectSpawnRequest {
    uint32_t object_kind;
    uint32_t orientation_quadrant;
    uint32_t reserved;
    int32_t origin_x;
    int32_t origin_y;
    const int32_t* coordinate_pairs;
    uint32_t coordinate_pair_count;
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
    uint32_t reserved_tail[44];
#else
    uint32_t reserved_tail[46];
#endif
} WorldObjectSpawnRequest;
typedef struct WorldObjectCreateHooks {
    void* context;
    void (*terrain_pairs_prepare)(void* context,
                                  const int32_t* coordinate_pairs,
                                  uint32_t coordinate_pair_count,
                                  float extent);
    void (*neighbor_cells_refresh)(void* context,
                                   const int32_t position[2]);
    /* Converts the request coordinates and creates a type-4 entity via
     * FUN_00438610. */
    EntitySlot* (*entity_create_type4)(
        void* context, const WorldObjectSpawnRequest* request,
        float orientation_radians);
    void (*event8_dispatch)(void* context, EntitySlot* entity);
    void (*matching_pair_prepare)(void* context, int32_t x, int32_t y);
    void (*matching_pair_attach)(void* context, EntitySlot* entity,
                                 const int32_t pair[2]);
    void (*boundary_pair_special)(void* context, const int32_t pair[2]);
    void (*boundary_pair_unlink)(void* context, const int32_t pair[2]);
    void (*boundary_pair_apply)(void* context, int32_t x, int32_t y);
    void (*boundary_pair_secondary)(void* context, const int32_t pair[2]);
    void (*cardinal_neighbors_apply)(void* context, int32_t x, int32_t y);
    void (*pair_refresh)(void* context, int32_t x, int32_t y);
} WorldObjectCreateHooks;
EntitySlot* world_object_spawn_request_create(
    const WorldObjectCreateHooks* hooks,
    const WorldObjectSpawnRequest* request,
    int notify); /* FUN_00409b30 */
void world_object_create_with_transform(const WorldObjectCreateHooks* hooks,
                                        uint32_t object_kind,
                                        const int32_t position[2],
                                        uint32_t subtype,
                                        const int32_t* coordinate_pairs,
                                        uint32_t coordinate_pair_count,
                                        int notify); /* FUN_00409a90 */
typedef struct EntityScenePose {
    float position[3];
    float orientation[7];
} EntityScenePose;
typedef struct EntityScenePoseHooks {
    void* context;
    int (*poses_get)(void* context, EntitySlot* entity,
                     EntityScenePose** source, EntityScenePose** target);
} EntityScenePoseHooks;
void entity_scene_pose_hooks_set(const EntityScenePoseHooks* hooks);
void scene_object_pose_position_sync(EntitySlot* entity); /* FUN_00443510 */
typedef struct EntityPairPoseHooks {
    void* context;
    int (*pose_read)(void* context, EntitySlot* entity,
                     EntityScenePose* pose);
    int (*scene_present)(void* context, EntitySlot* entity);
    void (*pose_apply)(void* context, EntitySlot* entity,
                       const EntityScenePose* pose);
    void (*activity_raise)(void* context, EntitySlot* entity, int activity);
} EntityPairPoseHooks;
void entity_pair_pose_hooks_set(const EntityPairPoseHooks* hooks);
int entity_record_pair_pose_copy_and_activity_raise(EntitySlot* entity);
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
void set_vector_and_notify_listeners(float* state,
                                     const float* delta); /* FUN_0040fde0 */
int entity_face_target_action_start(EntitySlot* entity, EntitySlot* target,
                                    int mode); /* FUN_00441ed0 */
void entity_finish_state_1a(EntitySlot* entity); /* FUN_0043c4c0 */
void entity_mark_and_finish_state_1a(EntitySlot* entity); /* FUN_0043c530 */

/* Allocate a slot of `type`; returns the slot index or -1 when the table is
 * full or type is out of 1..4. */
int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot);

/* Corpus strides by type (EntityPointer, finding 10 §"Kind registry" note). */
int  entity_stride(int type);
int  entity_count(GameContext* ctx, int type);
const EntitySlot* entity_at(GameContext* ctx, int type, int index);

/* EntityPoolFindByCallback / EntityPoolFindByCallbackFiltered
 * (FUN_004377d0 / FUN_004377b0): scan entity slots in table/slot order,
 * accepting only the self-pointer marker at +0x408, optionally skipping
 * records with +0x3e8 bit 0x80000, and stop at the first callback match. */
int entity_pool_find_by_callback(GameContext* ctx,
                                int (*callback)(EntitySlot*, void*),
                                void* payload, int filter_flagged);
int entity_pool_find_by_callback_filtered(GameContext* ctx,
                                          int (*callback)(EntitySlot*, void*),
                                          void* payload);
/* Retail ABI wrappers read the active pool context (FUN_004377d0/b0). */
int entity_pool_find_by_callback_current(
    int (*callback)(EntitySlot*, void*), void* payload, int filter_flagged);
extern "C" int entity_pool_find_by_callback_filtered_current(
    int (*callback)(EntitySlot*, void*), void* payload);
void entity_pool_callback_context_set(GameContext* ctx);
GameContext* entity_pool_callback_context_get(void);
int entity_pool_callback_search(int (*callback)(EntitySlot*, void*),
                                void* payload); /* FUN_00406540 */
typedef void (*EntityCallbackDispatchHook)(EntitySlot* entity, void* context);
/* Optional host observer after the retail linked-route cleanup walk. */
void entity_callback_dispatch_hook_set(EntityCallbackDispatchHook hook,
                                       void* context);
void entity_callback_dispatch_unless_type_five(EntitySlot* entity); /* 0x4374f0 */
/* EntityTeardown (FUN_00437540): execute the engine-owned cleanup stages in
 * the observed order while this module owns flag clearing and pool relinking. */
typedef struct EntityTeardownHooks {
    void* context;
    void (*operation)(EntitySlot* entity, unsigned int operation,
                      void* context);
    void* (*resource_get)(EntitySlot* entity, unsigned int record_word,
                          void* context);
    void (*resource_release)(void* resource, void* context);
    void (*type_resource_release)(EntitySlot* entity, int type,
                                  void* context);
    void (*counter_increment)(unsigned int slot, void* context);
    uint32_t (*state_flags_get)(EntitySlot* entity, void* context);
    void (*free_list_link)(EntitySlot* entity, void* previous_head,
                           void* context);
} EntityTeardownHooks;
enum EntityTeardownOperation {
    ENTITY_TEARDOWN_CALLBACK = 0,
    ENTITY_TEARDOWN_TYPE_THREE_PREPARE = 1,
    ENTITY_TEARDOWN_DETACH_PARENT = 2,
    ENTITY_TEARDOWN_RELEASE_SCENE = 3,
    ENTITY_TEARDOWN_RELEASE_LINK = 4,
    ENTITY_TEARDOWN_RESET_REFERENCE = 5,
    ENTITY_TEARDOWN_RESET_ACTION = 6,
    ENTITY_TEARDOWN_RELEASE_ACTIVITY = 7,
    ENTITY_TEARDOWN_RELEASE_AI = 8,
    ENTITY_TEARDOWN_RELEASE_TYPE_DATA = 9,
    ENTITY_TEARDOWN_FINALIZE = 10
};
void entity_teardown_hooks_set(const EntityTeardownHooks* hooks);
extern "C" int entity_teardown(EntitySlot* entity); /* 0x437540 */
typedef struct EntityCommandStatusFeedbackHooks {
    void* context;
    const char* (*command_text_get)(EntitySlot* entity, void* context);
    const char* (*status_text_get)(EntitySlot* entity, int status,
                                   void* context);
    void (*feedback_dispatch)(EntitySlot* entity, const char* text,
                              void* context);
    void (*empty_command_clear)(EntitySlot* entity, void* context);
    int (*status_get)(EntitySlot* entity, void* context);
} EntityCommandStatusFeedbackHooks;
void entity_command_status_feedback_hooks_set(
    const EntityCommandStatusFeedbackHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
void entity_command_status_feedback_dispatch(EntitySlot* entity); /* 0x428020 */
typedef struct EntityActionPrerequisiteHooks {
    void* context;
    int (*action_ready)(EntitySlot* entity, int action, void* context);
    int (*action_values_get)(EntitySlot* entity, int action,
                             uint32_t values[3], void* context);
    uint32_t (*resource_get)(unsigned int resource, void* context);
} EntityActionPrerequisiteHooks;
void entity_action_prerequisite_hooks_set(
    const EntityActionPrerequisiteHooks* hooks);
extern "C" int entity_action_kind_prerequisite_check(EntitySlot* entity,
                                          int action); /* 0x4388b0 */
typedef int (*EntityRouteMarkerCandidateHook)(EntitySlot* candidate,
                                               const float position[2],
                                               uint32_t flags,
                                               void* context);
void entity_route_marker_candidate_hook_set(
    EntityRouteMarkerCandidateHook hook, void* context);
extern "C" EntitySlot* entity_route_marker_candidate_find(EntitySlot* source,
                                               int include_secondary); /* 0x4389e0 */
typedef struct EntityKindRecordUpdateHooks {
    void* context;
    void (*type_four_update)(EntitySlot* entity, void* context);
    /* Notification after the required type-5 delivery record is created. */
    void (*action_dispatch)(EntitySlot* entity, int action, void* context);
    void (*type_four_global_action)(void* context);
    void (*type_one_record_update)(EntitySlot* entity, void* context);
    void (*global_refresh)(void* context);
    void (*event_emit)(EntitySlot* entity, int type, uint32_t flags,
                       void* context);
    void (*type_four_counter_increment)(void* context);
} EntityKindRecordUpdateHooks;
void entity_kind_record_update_hooks_set(
    const EntityKindRecordUpdateHooks* hooks);
extern "C" void entity_kind_record_update_dispatch(EntitySlot* entity); /* 0x438460 */
/* 0x447b30: returns scaled frame delta; completion lives in activity_timer
 * and is supplied by the actual type-specific animation owner. */
float entity_activity_tick(EntitySlot* entity, float delta);
typedef struct GridObjectRecord {
    void* first;
    void* second;
    uint32_t reserved[9];
} GridObjectRecord;
typedef void (*GridObjectRecordsReleaseHook)(void* context);
void grid_object_record_state_install(const GridObjectRecord* records,
                                      unsigned int count,
                                      GridObjectRecordsReleaseHook tail,
                                      void* context);
void grid_object_records_release(void); /* 0x4378e0 */
typedef struct QueuedGridAnimationHooks {
    void* context;
    float (*mode_scale_get)(int mode, void* context);
    int (*anchor_position_get)(void* anchor, float position[2],
                               void* context);
    EntitySlot* (*entity_find)(const float position[2], int type,
                               int filter_a, int filter_b, void* context);
    EntitySlot* (*animation_entity_get)(EntitySlot* entity, int type,
                                       int filter_a, int filter_b,
                                       void* context);
    void (*work_queue_update)(EntitySlot* entity, int mode, void* context);
} QueuedGridAnimationHooks;
void queued_grid_animation_hooks_set(const QueuedGridAnimationHooks* hooks);
void queued_grid_animation_accumulator_clear(void);
void process_queued_grid_animations(void* anchor, int mode,
                                    float delta); /* 0x437300 */
typedef struct RoverWorkQueueHooks {
    void* context;
    void* (*model_get)(EntitySlot* entity, void* context);
    unsigned int (*animation_count_get)(void* model, void* context);
    void (*position_get)(int mode, float position[3], void* context);
    void (*animation_position_set)(void* model, unsigned int index,
                                   const float position[3], float scale,
                                   void* context);
    void (*animation_update)(void* model, unsigned int index,
                             const float position[3], void* context);
} RoverWorkQueueHooks;
void rover_work_queue_hooks_set(const RoverWorkQueueHooks* hooks);
void rover_work_queue_update(EntitySlot* entity, int mode); /* 0x4383b0 */
typedef void (*EntitySpawnSpecialFinalizeHook)(EntitySlot* entity,
                                               int value, void* context);
void entity_spawn_special_finalize_hook_set(
    EntitySpawnSpecialFinalizeHook hook, void* context);
EntitySlot* entity_create_with_position(GameContext* context, int type,
                                        const char* name, int value,
                                        float x, float y,
                                        float heading); /* 0x438610 */
/* Unnamed resource creation carries the raw subtype independently of the
 * post-create animation selector. Named objects retain config resolution. */
EntitySlot* entity_create_with_position_subtype(GameContext* context, int type,
                                                int subtype, const char* name,
                                                int value, float x, float y,
                                                float heading);
typedef struct BuildingUpgradeAdmissionHooks {
    void* context;
    uint32_t (*maximum_level_get)(EntitySlot* building, void* context);
    uint32_t (*current_level_get)(EntitySlot* building, void* context);
    uint32_t (*cost_get)(unsigned int resource, void* context);
    uint32_t (*resource_get)(unsigned int resource, void* context);
} BuildingUpgradeAdmissionHooks;
void building_upgrade_admission_hooks_set(
    const BuildingUpgradeAdmissionHooks* hooks);
extern "C" int building_upgrade_resource_admission_check(EntitySlot* building,
                                               uint32_t* out_secondary_cost); /* 0x438770 */
typedef struct BuildingUpgradeCommitHooks {
    void* context;
    void (*resource_spend)(EntitySlot* building, unsigned int resource,
                           uint32_t amount, void* context);
    void (*upgrade_request_clear)(EntitySlot* building, void* context);
    void (*payload_level_set)(EntitySlot* building, unsigned int level,
                              void* context);
    void (*stats_level_set)(EntitySlot* building, unsigned int level,
                            void* context);
    void (*upgrade_effect_dispatch)(EntitySlot* building, unsigned int level,
                                    void* context);
} BuildingUpgradeCommitHooks;
void building_upgrade_commit_hooks_set(
    const BuildingUpgradeCommitHooks* hooks);
extern "C" void building_upgrade_commit(EntitySlot* building); /* 0x4387f0 */
typedef struct DeferredOListEntityRecord {
    int cell_x;
    int cell_y;
    float x;
    float y;
    float heading;
    int source_kind;
    int entity_type;
    int variant;
    int record_flags;
    char* name;
    char* link_name;
} DeferredOListEntityRecord;
typedef struct DeferredOListEntityHooks {
    void* context;
    EntitySlot* (*entity_create)(const DeferredOListEntityRecord* record,
                                 void* context);
    EntitySlot* (*special_building_create)(
        const DeferredOListEntityRecord* record, unsigned int level,
        void* context);
    void (*control_initialize)(EntitySlot* entity, void* context);
    void (*type_two_event)(EntitySlot* entity, void* context);
    void (*type_eleven_finalize)(EntitySlot* entity, void* context);
    void (*link_finalize)(EntitySlot* entity, EntitySlot* target,
                          unsigned int mode, void* context);
    void (*record_release)(DeferredOListEntityRecord* record,
                           void* context);
} DeferredOListEntityHooks;
void deferred_olist_entity_hooks_set(const DeferredOListEntityHooks* hooks);
void deferred_olist_entity_queue_install(
    const DeferredOListEntityRecord* records, unsigned int count);
extern "C" void deferred_olist_entity_create_and_link(const int cell_xy[2]); /* 0x437940 */
void deferred_olist_entity_level_begin(GameContext* context);
void deferred_olist_entity_level_end(struct LevelState* level);
typedef int (*EntitySpatialQueryHook)(void* context, EntitySlot* subject,
                                      EntitySlot* candidate,
                                      const float position[2],
                                      const float values[3]);
void entity_spatial_query_hook_set(EntitySpatialQueryHook hook, void* context);
int entity_spatial_query_dispatch(EntitySlot* subject,
                                  const float position[2],
                                  const float values[3]);
/* EntityLookupAtWorldPosition (FUN_00438df0): run the mask-8 spatial query
 * around an explicit world XY point with radius 10,000. */
extern "C" EntitySlot* entity_lookup_at_world_position(const float world_position[2]);
void entity_indexed_value_table_clear(void);
void entity_indexed_value_append(int group, uint32_t value); /* FUN_00469a90 */
extern "C" int entity_grouped_code_table_find_value_index(uint32_t value,
                                                          int* out_group); /* FUN_0046a120 */
void entity_indexed_record_table_set(int group, const void* const* records,
                                     uint32_t count);
extern "C" uint32_t entity_indexed_record_word_at_b0_get(int group, int index,
                                                         int slot); /* FUN_00469ac0 */
extern "C" uint32_t entity_indexed_record_word_at_b4_get(int group, int index,
                                                         int slot); /* FUN_00469af0 */
uint32_t entity_indexed_record_word_at_b8_get(int group, int index,
                                              int slot); /* FUN_00469b20 */
uint32_t entity_indexed_record_word_at_f0_get(int group, int index,
                                              int slot); /* FUN_00469be0 */
extern "C" uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot); /* FUN_0046a0c0 */
typedef int (*PeriodicEntityCallback)(EntitySlot* entity, void* payload,
                                      void* context);
void periodic_entity_callback_install(PeriodicEntityCallback callback,
                                      void* context);
void periodic_entity_callback_dispatch(void* payload); /* FUN_00425b40 */
int entity_default_transform_apply_if_flagged(EntitySlot* entity,
                                              void* argument);
void entity_default_transform_apply_all(void* argument);
EntitySlot* entity_at_grid_position_find(const int cell_xy[2]); /* 0x00438670 */
extern "C" void entity_activity_record_remove_by_type(void* entity, int type,
                                           void* action_context);
void entity_activity_records_refresh(void* entity);
typedef void (*ActivityRecordClearFn)(void* head, void* user);
typedef int (*ActivityRunnerChangeFn)(void* scene_owner, void* state,
                                      void* value, void* user);
void activity_runner_hooks_set(ActivityRecordClearFn clear_records,
                               ActivityRunnerChangeFn change, void* user);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * renamed and uncalled so that the symbol count of every includer is
 * unchanged (ADR 0005 entropy rule 1). */
int activity_runner_update_superseded(void* runner, void* state, void* value);
typedef struct EntityActivityRunnerResourceOps {
    void* context;
    void* (*scene_clone)(void* context, uint32_t scene_token);
    void (*scene_release)(void* context, void* scene);
    void (*scene_owner_bind)(void* context, void* scene,
                             EntitySlot* entity);
    void* (*resource_chain_clone)(void* context, uint32_t chain_token);
    void (*resource_chain_release)(void* context, void* chain);
    void* (*indexed_resource_resolve)(void* context, void* scene,
                                      uint32_t source_token,
                                      uint32_t index);
    void (*scene_time_update)(void* context, void* scene, void* value);
    int (*scene_component_gate_read)(void* context, void* scene,
                                     uint32_t* value_out);
} EntityActivityRunnerResourceOps;
void entity_activity_runner_resource_ops_set(
    const EntityActivityRunnerResourceOps* ops);
int entity_activity_runner_type_config_bind(EntitySlot* entity);
void* entity_activity_runner_indexed_get(EntitySlot* entity, int type,
                                         uint32_t index);
int entity_type1_scene_resource_bind(EntitySlot* entity);
void* entity_type1_scene_resource_indexed_get(
    EntitySlot* entity, uint32_t index, uint32_t* component_present);
void entity_activity_runner_cache_clear(EntitySlot* entity, int type);
int entity_activity_runner_activity_update(EntitySlot* entity, int type,
                                           void* activity, void* value);
void entity_activity_runner_release(EntitySlot* entity);
typedef void* (*EntityTrackedCleanupFn)(int operation, void* entity,
                                        uint32_t flags, float position[3],
                                        void* user);
void entity_tracked_cleanup_context_set(void** tracked_entities,
                                       EntityTrackedCleanupFn callback,
                                       void* user);
void entity_destroy_tracked_pointer_cleanup(void* entity); /* 0x00409ff0 */
typedef void (*EntityPairCountUpdateFn)(uint32_t count, void* user);
typedef struct EntityPairRecord { void* first; void* second; } EntityPairRecord;
void entity_pair_count_records_install(EntityPairRecord* records,
                                       uint32_t count,
                                       EntityPairCountUpdateFn update,
                                       void* user);
void entity_pair_count_refresh(void* first, void* second); /* 0x0041ec30 */
typedef int (*EntityActionTargetCellValidateFn)(EntitySlot* entity,
                                                void* user);
void entity_action_target_cell_validator_set(
    EntityActionTargetCellValidateFn validator, void* user);
extern "C" int entity_action_target_cell_is_valid(EntitySlot* entity); /* 0x0043fdc0 */
typedef int (*EntityExplicitRouteFn)(EntitySlot* actor, EntitySlot* target,
                                     float* x_values, float* y_values,
                                     unsigned int* count, void* user);
void entity_explicit_route_hook_set(EntityExplicitRouteFn hook, void* user);
int entity_route_to_explicit_target(EntitySlot* actor,
                                    EntitySlot* target); /* 0x0043fe70 */
int entity_route_marker_setup(EntitySlot* actor); /* 0x0043fff0 */
typedef int (*EntityHurtSoundAvailableFn)(void* user);
typedef void (*EntityHurtSoundSubmitFn)(EntitySlot* entity, void* user);
typedef float (*EntityHurtSoundDurationFn)(void* user);
void entity_hurt_sound_hooks_set(EntityHurtSoundAvailableFn available,
                                 EntityHurtSoundSubmitFn submit,
                                 EntityHurtSoundDurationFn duration,
                                 void* user);
int entity_damage_hurt_sound_submit(EntitySlot* entity, float time,
                                    float damage); /* 0x0043fb40 */
typedef void (*EntityLinkedEventSubmitFn)(uint32_t event, EntitySlot* entity,
                                          void* user);
void entity_linked_event_submit_hook_set(EntityLinkedEventSubmitFn hook,
                                         void* user);
extern "C" void entity_linked_activity_event_dispatch(EntitySlot* entity,
                                           uint32_t events); /* 0x0043f5b0 */
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
void direction_driven_heading_angle_step(void* state, const float* direction);
void kind_two_steering_torque_apply(void* state, float direction[3]);
void steering_physics_step(void* unused, float* state, const float* delta);
int entity_flag_mask_callbacks_all_pass(uint32_t mask); /* FUN_0046a4c0 */
typedef int (*EntityFlagMaskPredicate)(EntitySlot*, uint32_t, void*);
int entity_flag_mask_callbacks_predicate_all_pass(
    GameContext* ctx, uint32_t mask, EntityFlagMaskPredicate predicate,
    void* user);
extern "C" void entity_iteration_callback_guard(void); /* FUN_0043b320 */
uint32_t entity_iteration_guard_flags_peek(void); /* port hook */
void entity_iteration_guard_flags_set(uint32_t flags); /* FUN_00436c20 / 437050 */
typedef int (*SceneObjectStateCallback)(EntitySlot*, uint32_t, uint32_t,
                                        uint32_t, void*);
void scene_object_state_dispatch_words_install(uint32_t point_low,
                                               uint32_t point_high,
                                               uint32_t word_c,
                                               uint32_t word14);
void scene_object_state_dispatch(GameContext* ctx, uint32_t selector,
                                 uint32_t mode, uint32_t flags,
                                 SceneObjectStateCallback callback,
                                 void* user); /* FUN_0046a510 */

/* EntitySpawnRequestQueueAdd/Process (FUN_0043f150/0x0043f190): bounded
 * nine-entry deferred spawn request queue. The port processes the first
 * valid entity-type request when an eligible requester is ticked. */
extern "C" int entity_spawn_request_queue_add(GameContext* ctx, int type,
                                  int request_kind);
void entity_spawn_request_queue_process(EntitySlot* requester);

/* EntityQueryFlagCount (FUN_0041b3c0): catalog-kind query over the stats
 * flag words and the own-storage catalog count/pair windows (install hooks
 * below; counts are installed from the live type config before Stats loads. */
int entity_query_flag_count(int query);           /* FUN_0041b3c0 */
void     catalog_count_install(int which, uint32_t value); /* port hook */
uint32_t catalog_count_peek(int which);           /* port hook */
void     catalog_name_table_install(int kind, const void* names); /* hook */
const void* catalog_name_table_peek(int kind);    /* port hook */
void     catalog_id_pair_install(int entry, uint32_t size); /* port hook */

/* CatalogCallbackByIdDispatch (FUN_0041b680): callback dispatch over the
 * catalog query lanes and the key-group code band shared with the resource
 * module's stride-8 window (the pair word is that window's count). */
int catalog_callback_by_id_dispatch(const GameContext* ctx, int query,
                                    int (*callback)(int, int, int, void*),
                                    void* payload);     /* FUN_0041b680 */

/* Resolver pair (FUN_0042e8b0, name -> (type, registry row)): the row slot
 * is zeroed at entry (the corpus's blind `mov [ebx],0` at 0x42e8b8; the port
 * guards it for NULL-threading callers); table matches then write both
 * outputs, the special names write only their dedicated rows, so everything
 * else answers a zero row.
 * The (name, &type, &row, NULL) corpus call form; NULL name answers 0. */
int entity_type_row_for_name(const char* name, int* type_out, int* row_out);

/* Load the level's `.ol` object list (`OListFile` key) from `cfg` into the
 * entity table. Returns the number of objects placed, or -1 when the
 * resource is missing/unparsable. */
int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name);
/* Load the config-backed entity display-name tables after Lego.cfg opens. */
void entity_names_config_load(BlockFile* cfg);
const TypeConfigTable* entity_type_config_table_get(uint32_t row_kind);
const TypeConfigTable* entity_type_config_table_for_entity_type(int type);

/* Load a `.ol` object list by resource path directly. Returns one only when
 * the full required-entry parse succeeds; the placed count is in GameContext. */
int object_list_load_file(GameContext* ctx, const char* olist);
/* OListEntityRecordParse's ten-slot ObjectPointerRegistry. Retail-shaped
 * dwords are retained for diagnostics; runtime callers use full host
 * pointers so 64-bit EntitySlot addresses are not truncated. */
void object_pointer_registry_reset(void);
void object_pointer_registry_append(EntitySlot* entity);
void object_pointer_registry_entity_remove(EntitySlot* entity);
uint32_t object_pointer_registry_count_get(void);
int32_t object_pointer_registry_word_get(uint32_t index);
EntitySlot* object_pointer_registry_entity_get(uint32_t index);

/* Attach a model to a slot by resource path (`.ol Model` key). The first
 * load per path goes through `model_open`; later loads reuse a process-
 * lifetime cache (the demo never frees models — documented). Empty path
 * clears. */
void entity_set_model(EntitySlot* e, const char* path);

/* The parsed LWO Model* for a slot (NULL when none/load failed). */
const void* entity_model(const EntitySlot* e);

/* --- entity record accessors (finding 457: the corpus +0x334 record) -----
 * Corpus families FUN_00469cb0 … FUN_0046a310 read fixed fields from the
 * entity's selected stats record (*(entity + 0x334)). The port mirrors that
 * record as EntitySlot.stats_record; NULL resolves to a zero template,
 * matching the shipped static templates. The startup Stats loader fills
 * the supported fields into retained 0x150-byte per-level records; fields
 * whose key/offset mapping is not recovered still read as zero. The
 * corpus's float fields return long double (x87 float10); the port hands
 * back the plain float stored in the record. Offsets are record-relative;
 * names are offset-preserving where semantics are unproved. */

#define ENTITY_STATS_RECORD_SIZE 0x150 /* the corpus record stride */

/* RouteSpeed is raw WORLD units per reference tick (25 ticks/second).
 * This query applies the existing 0x446610 terrain/state scale, with no motion.
 * Missing Stats returns zero; callers must explicitly convert their time unit. */
float entity_route_speed(EntitySlot* entity);
float entity_stats_float_at20(const EntitySlot* e);   /* 0x469cb0: RouteSpeed */
extern "C" float entity_stats_float_at24_indexed(const EntitySlot* e, int index);
                                      /* 0x469cc0: drill times at +24..34, in reference ticks */
float entity_stats_float_atd4(const EntitySlot* e);   /* FUN_00469ce0 */
float entity_stats_float_at9c(const EntitySlot* e);   /* FUN_00469d00 */
float entity_stats_float_atd8(const EntitySlot* e);   /* FUN_00469d20 */
float entity_stats_float_at6c(const EntitySlot* e);   /* FUN_00469d40 */
extern "C" float entity_stats_float_at74(const EntitySlot* e);   /* FUN_00469d50 */
extern "C" float entity_stats_float_at78(const EntitySlot* e);   /* FUN_00469d60 */
extern "C" float entity_stats_float_ate8(const EntitySlot* e);   /* FUN_00469d70 */
void entity_stats_float_ate8_set(EntitySlot* e, float value); /* tutorial write */
float entity_stats_float_at70(const EntitySlot* e);   /* FUN_00469d90 */
void* entity_stats_field_at_a0(const EntitySlot* e);  /* FUN_00469da0: the
                                         * record+0xa0 address itself */
extern "C" float entity_stats_float_at7c(const EntitySlot* e);   /* FUN_00469db0 */
float entity_stats_float_at80(const EntitySlot* e);   /* FUN_00469dc0 */
float entity_energy_decay_rate_get(const EntitySlot* e); /* FUN_00469de0 (+84) */
extern "C" float entity_stats_float_atdc(const EntitySlot* e);   /* FUN_00469e20 */
uint32_t entity_stats_word_atac(const EntitySlot* e); /* FUN_00469e40 */
extern "C" uint32_t entity_stats_word_get(const EntitySlot* e);  /* FUN_00469e60 (+144) */
extern "C" uint32_t entity_state_flags_get(const EntitySlot* e); /* FUN_00469e80 (+148) */
typedef struct EntityStateOverrideEntry {
    uint32_t flags_3f0;
    uint32_t value_330;
    char name[12];
} EntityStateOverrideEntry;
typedef void (*EntityStateOverrideValue330ApplyHook)(EntitySlot* entity,
                                                     uint32_t value,
                                                     void* context);
typedef void (*EntityStateOverrideRenameHook)(EntitySlot* entity,
                                              const char* name,
                                              void* context);
void entity_state_override_hooks_set(
    EntityStateOverrideValue330ApplyHook value_330_apply,
    EntityStateOverrideRenameHook rename, void* context);
int entity_state_override_stack_set(const EntityStateOverrideEntry* entries,
                                    uint32_t count);
int entity_state_override_stack_push(const EntityStateOverrideEntry* entry);
extern "C" int entity_state_override_pop(EntitySlot* entity); /* FUN_00459240 */
extern "C" void entity_record_flag400_toggle_and_refresh(EntitySlot* e); /* FUN_0046a330 */
extern "C" uint32_t entity_capability_flags_get(const EntitySlot* e); /* FUN_00469ea0 (+14c) */
float entity_stats_float_at88(const EntitySlot* e);   /* FUN_00469f20
                                         * (the "screen shake" reader) */
extern "C" uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector);
/* Record +0xec value used by tutorial action 0x14/0x15. */
uint32_t entity_stats_word_at_ec(const EntitySlot* e);
                                      /* FUN_00469f90: +c4 if selector
                                       * nonzero, else +c0 */
extern "C" uint32_t entity_stats_word_atc8(const EntitySlot* e); /* FUN_00469fc0 */
float entity_stats_float_at8c(const EntitySlot* e);   /* FUN_00469fe0 */
float entity_stats_float_at90(const EntitySlot* e);   /* FUN_0046a000 */
extern "C" float entity_stats_float_at98(const EntitySlot* e);   /* FUN_0046a020 */
float entity_stats_float_at94(const EntitySlot* e);   /* FUN_0046a040 */
float entity_stats_float_atf4(const EntitySlot* e);   /* FUN_0046a080 */
float entity_speed_component_get(const EntitySlot* e); /* FUN_0046a0a0 (+f8) */
/* EntityTypeComponentGet (FUN_0044a200): inspect the legacy entity-record
 * layout and return the type-specific renderer component handle. */
extern "C" void* entity_type_component_get(const void* entity_record);
void entity_slot_type_component_set(EntitySlot* entity, void* component);
void entity_follow_target_pose_get(EntitySlot* entity, int index,
                                   float position[3], float facing[3]);
                                  /* FUN_0044a070 */
extern "C" uint32_t entity_type_operation_release(const EntitySlot* entity); /* FUN_00449ff0 */
void entity_type_operation_fallback_handle_set(EntitySlot* entity,
                                               void* handle_1c);
float entity_stats_float_at130(const EntitySlot* e);  /* FUN_0046a170 */
float entity_stats_float_at134(const EntitySlot* e);  /* FUN_0046a190 */
float entity_stats_float_at120(const EntitySlot* e);  /* FUN_0046a1b0 */
float entity_stats_float_at128(const EntitySlot* e);  /* FUN_0046a1d0 */
float entity_stats_float_at124(const EntitySlot* e);  /* FUN_0046a1f0 */
uint32_t entity_stats_word_at13c(const EntitySlot* e); /* FUN_0046a210 */
float entity_stats_float_at12c(const EntitySlot* e);  /* FUN_0046a230 */
float entity_stats_float_at138(const EntitySlot* e);  /* FUN_0046a250 */
float entity_collision_radius_get(const EntitySlot* e); /* FUN_0046a270 (+e4) */
float entity_stats_float_at114(const EntitySlot* e);  /* FUN_0046a290 */
float entity_stats_float_at118(const EntitySlot* e);  /* FUN_0046a2b0 */
float entity_stats_float_at11c(const EntitySlot* e);  /* FUN_0046a2d0 */
float entity_stats_float_at110(const EntitySlot* e);  /* FUN_0046a2f0 */
float entity_stats_float_at10c(const EntitySlot* e);  /* FUN_0046a310 */

/* --- entity-state eligibility predicates (findings 318/319/471/472) -------
 * Pure reads over the +0x3e0/+0x3e4/+0x3e8 flag mirrors and the +0x340
 * float — no cell or context state. */

/* FUN_0043baf0 — true when either flag word intersects its literal mask:
 * +0x3e0 & 0x107c33de or +0x3e4 & 0x01c00203. */
extern "C" int entity_flags_mask107c33de_or_1c00203_any_test(const EntitySlot* e);

/* FUN_004385b0 — the shared eligibility gate the command and activity
 * layers consult: no +0x3e0 bits 0x700100 or bit 0x20000, no +0x3e4 bit
 * 0x100000, the +0x340 float not negative, and — when the caller passes a
 * zero second argument — +0x3e8 bit 0x20000000 set with bit 0x80000000
 * clear. */
extern "C" int entity_shared_eligibility_check(const EntitySlot* e, int relaxed);

/* FUN_004436f0 — kind-3 predicate: type == 3 and +0x3e8 bit 0x04000000. */
extern "C" int entity_type3_flag04000000_test(const EntitySlot* e);

/* VehicleUnloadAdjacentCellFind and its two stat-flag gates (0x439d50,
 * 0x439e10, 0x439e40): direct level-backed neighbor selection. */
int entity_vehicle_unload_adjacent_cell_find(const EntitySlot* entity,
                                             int out_xy[2]);
extern "C" int entity_vehicle_unload_cell_find_if_eligible(const EntitySlot* entity,
                                                int out_xy[2]);
extern "C" int entity_record_flag20_query(const EntitySlot* entity, int out_xy[2]);
extern "C" int entity_accepts_target_for_task_check(const EntitySlot* entity,
                                         const EntitySlot* target,
                                         int apply_extra_gate); /* 0x00439bd0 */
int entity_gated_step_command_issue(EntitySlot* entity,
                                    EntitySlot* target); /* 0x00439a20 */
extern "C" int entity_task_target_cell_eligibility_check(
    const EntitySlot* entity, const EntitySlot* target); /* 0x00439b80 */
void entity_pair_interaction_activity_sentinel_set(int activity);
void entity_pair_interaction_gate(EntitySlot* entity,
                                  const EntitySlot* partner); /* 0x0043a9f0 */
typedef int (*EntityVisualResourcePresentHook)(const EntitySlot* entity,
                                               void* resource,
                                               void* context);
typedef void (*EntityVisualResourceCreateHook)(EntitySlot* entity,
                                               void* context);
void entity_visual_resource_hooks_set(EntityVisualResourcePresentHook present,
                                      EntityVisualResourceCreateHook create,
                                      void* context);
void entity_visual_resource_get_or_create(EntitySlot* entity); /* 0x0043a5f0 */
void entity_proximity_activity_raise(EntitySlot* entity); /* 0x0043aab0 */
typedef void (*EntityBuildingDepositHook)(EntitySlot* entity, void* context);
void entity_building_deposit_hook_set(EntityBuildingDepositHook hook,
                                      void* context);
void entity_building_deposit_state_monitor(EntitySlot* entity); /* 0x0043bb20 */
typedef struct EntityDepositActivityHooks {
    void* context;
    uint32_t (*activity_flags)(void* context, const EntitySlot* entity);
    int (*target_busy)(void* context, const EntitySlot* target, int mode);
    int (*route_to_target)(void* context, EntitySlot* entity,
                           EntitySlot* target, int* direction);
    void (*deposit_cancel)(void* context, EntitySlot* entity);
    void (*deposit_start)(void* context, EntitySlot* entity,
                          EntitySlot* target, int direction);
    /* Locator boundary only: 1 found, 0 authored locator absent, -1 owner
     * unavailable. The gate always owns path search and installation.
     * target_busy/route_to_target/deposit_start above are legacy adapters;
     * they cannot substitute for the target's Stats or a real route. */
    int (*approach_pose_get)(void* context, EntitySlot* target, int tool_null,
                              float position[3], float facing[3]);
} EntityDepositActivityHooks;
void entity_deposit_activity_hooks_set(const EntityDepositActivityHooks* hooks);
int entity_deposit_activity_gate(EntitySlot* entity,
                                 EntitySlot* target); /* FUN_0043a300 */
void entity_kind_four_post_create_resolve(EntitySlot* entity); /* FUN_0043c570 */
typedef void (*EntityTypeTwoEventHook)(const EntitySlot* entity,
                                       uint32_t argument_a,
                                       uint32_t argument_b, void* context);
void entity_type_two_event_hook_set(EntityTypeTwoEventHook hook,
                                    void* context);
extern "C" void entity_type_two_event_dispatch(const EntitySlot* entity,
                                    uint32_t argument_a,
                                    uint32_t argument_b); /* 0x0043b820 */
typedef struct EntityTypeTwoPositionedHooks {
    void* context;
    int (*position_get)(void* context, void* scene_object, float out_xyz[3]);
    /* Apply the enabled state to the renderer tracks selected by state:
     * state 1 uses +0x64 for types 2/3, state 2 uses +0x68, and type 1
     * updates its paired tracks for either state. */
    void (*render_tracks_set)(void* context, EntitySlot* entity, int state,
                              int enabled);
} EntityTypeTwoPositionedHooks;
void entity_type_two_positioned_hooks_set(
    const EntityTypeTwoPositionedHooks* hooks);
extern "C" void entity_type_two_positioned_callback_dispatch(
    const EntitySlot* entity, void* scene_object, float inner_range,
    float outer_range, int mode); /* 0x0043b850 */
typedef int (*EntityTaskCoordinateMatchHook)(void* context,
                                              const EntitySlot* entity,
                                              uint32_t x, uint32_t y);
void entity_task_count_context_set(GameContext* context,
                                   EntityTaskCoordinateMatchHook match,
                                   void* user);
extern "C" int entity_task_at_grid_coordinate_find(uint32_t x, uint32_t y,
                                        EntitySlot** out_entity);
typedef void (*EntityIndexedFlagPublishHook)(EntitySlot* entity,
                                             uint32_t flags,
                                             uint32_t event_id,
                                             void* context);
void entity_indexed_flag_publish_hook_set(EntityIndexedFlagPublishHook hook,
                                          void* context);
void entity_indexed_flag_set_and_refresh(EntitySlot* entity); /* 0x00439c80 */
typedef void* (*EntityToolNameLoadHook)(void* string_loader, void* config,
                                        const char* category, int slot,
                                        void* context);
void entity_tool_name_load_hook_set(EntityToolNameLoadHook hook,
                                    void* context);
/* FUN_00439950 args are the parsed config file and config root path. When no
 * override is installed, loads each known ToolNames value and duplicates it
 * into the indexed handle table. */
extern "C" void entity_tool_name_strings_load(void* string_loader,
                                   void* config); /* 0x00439950 */
void* entity_tool_name_string_get(int slot);
typedef struct EntityBuildPlacementHooks {
    void* context;
    int (*position_choose)(void* context, const int cell_xy[2],
                          float out_world_xy[2]);
    EntitySlot* (*entity_create)(void* context, int kind, int selector,
                                 int entity_id, float x, float y);
    void (*queue_entity)(void* context, int opcode, EntitySlot* entity);
    void (*event_raise)(void* context, int event_id, const int cell_xy[2]);
    void (*cell_counter_increment)(void* context, const int cell_xy[2],
                                   int field_offset);
    int (*batch_counts_read)(void* context, const int cell_xy[2],
                             uint32_t counts[4]);
} EntityBuildPlacementHooks;
void entity_build_placement_hooks_set(const EntityBuildPlacementHooks* hooks);
void entity_build_placement_context_bind(GameContext* context);
int entity_build_placement_command_setup(const int cell_xy[2], int entity_id,
                                         const float world_xy[2],
                                         int update_state, int alternate);
void entity_build_placement_command_pair_create(const int cell_xy[2],
                                                int entity_id);
void entity_build_placement_command_batch_expand(const int cell_xy[2]);
void entity_placement_commands_build_once(EntitySlot* entity); /* FUN_00409f20 */
typedef struct EntityActionIssueHooks {
    void* context;
    EntitySlot* (*create)(void* context, EntitySlot* source, int kind,
                          int selector, int argument);
    void (*kind_event)(void* context, int kind, int selector);
    void (*queue)(void* context, int opcode, EntitySlot* created);
    void (*label_set)(void* context, EntitySlot* created, const char* label);
} EntityActionIssueHooks;
void entity_action_issue_hooks_set(const EntityActionIssueHooks* hooks);
extern "C" EntitySlot* entity_action_kind_entity_create_and_command_issue(
    EntitySlot* source, int kind, int selector, int argument); /* 0x0043a650 */
typedef struct EntityActionConsumeHooks {
    void* context;
    void (*event_dispatch)(void* context, int type, int reference);
    void (*record_release)(void* context, EntitySlot* record);
    void (*entity_records_cleanup)(void* context, EntitySlot* entity);
} EntityActionConsumeHooks;
void entity_action_consume_hooks_set(const EntityActionConsumeHooks* hooks);
uint32_t entity_action_consume_counter_get(const EntitySlot* entity,
                                           uint32_t slot);
void entity_action_record_consume_and_cleanup(
    EntitySlot* entity, EntitySlot* record); /* 0x0043a850 */
typedef int (*EntityPairRayHitCheckHook)(const EntitySlot* first,
                                         const EntitySlot* second,
                                         void* context);
void entity_pair_ray_hit_check_hook_set(EntityPairRayHitCheckHook hook,
                                        void* context);
int entity_pair_ray_hit_check(const EntitySlot* first,
                              const EntitySlot* second);
int entity_type26_command_completion_gate(const EntitySlot* entity); /* 0x0043c3e0 */
typedef struct EntityClippedRectangleHooks {
    void* context;
    void (*visit_entities)(void* context, void* payload,
                           const float bounds[4]);
    void (*queue_command)(void* context, int opcode);
} EntityClippedRectangleHooks;
void entity_clipped_rectangle_hooks_set(
    const EntityClippedRectangleHooks* hooks);
extern "C" void entity_clipped_rectangle_callback_dispatch(
    void* payload, const float rect_a[2],
    const float rect_b[2]); /* 0x0043b6c0 */

/* Route candidate helpers from the terrain/movement predicate band. */
int entity_cell_type_speed_multiplier_resolve(const EntitySlot* entity,
                                              const struct LevelState* level,
                                              int x, int y, float scale,
                                              float* out_scale); /* FUN_00433180 */
extern "C" int entity_cell_type_speed_multiplier_resolve_current(
    const EntitySlot* entity, int x, int y, float scale,
    float* out_scale); /* FUN_00433180 retail ABI */
int entity_route_cell_candidate_accept(const EntitySlot* entity,
                                       const struct LevelState* level,
                                       int x, int y, int mode); /* FUN_00443840 */
extern "C" void entity_transform_position_get_outputs(const EntitySlot* entity,
                                           float* out_x,
                                           float* out_y); /* FUN_00442260 */
void entity_transform_position_get(const EntitySlot* entity,
                                   float out_xy[2]);
/* World pose -> corner-based fractional grid coordinates (cell centers are
 * n+0.5). Used by software drawing/picking; false without a valid terrain. */
int entity_position_grid_coordinates_get(const EntitySlot* entity,
                                          const struct LevelState* level,
                                          float out_xy[2]);
int entity_work_object_position_get(const EntitySlot* entity,
                                    float out_xy[2]); /* FUN_0044a2a0 */
int entity_world_center_position_read(const EntitySlot* entity,
                                      float out_xyz[3]); /* FUN_00471ca0 */
int entity_centers_within_configured_radius(const EntitySlot* first,
                                            const EntitySlot* second); /* FUN_00472080 */
extern "C" int entity_centers_within_object_radius(const EntitySlot* first,
                                        const EntitySlot* second); /* FUN_00472130 */
extern "C" void entity_world_position_set(EntitySlot* entity, float x, float y,
                               float heading, int update_heading); /* FUN_004428a0 */
typedef void (*EntityFinalizeHook)(EntitySlot* entity);
void entity_type5_finalize_environment_set(uint32_t mode_flags,
                                          EntityFinalizeHook timeout_hook,
                                          EntityFinalizeHook finalize_hook);
extern "C" void entity_type5_finalize_if_allowed(EntitySlot* entity); /* FUN_00441ea0 */
typedef int (*EntityRouteStateHook)(EntitySlot* entity);
void entity_route_state_hook_set(EntityRouteStateHook hook);
void entity_action_route_state_initialize(EntitySlot* entity); /* FUN_00442210 */
extern "C" void entity_enter_event21_state_once(EntitySlot* entity); /* FUN_00444620 */

/* FUN_00447730 — routing-progress advance (findings 377/448): add 25 to
 * the +0x344 progress float, clear the +0x2fc link field, clamp to the
 * entity's +0x340 bound, and answer 1 when that bound is reached. */
int entity_progress_advance_by_25(EntitySlot* e);

/* LinkedEntityRouteFlagClear (FUN_0043f100): clear route bit 0x40 from
 * the entity and its optional +0x2fc linked entity. */
void entity_linked_route_flag_clear(GameContext* ctx, EntitySlot* e);

/* EntityClearNestedState8Fields (FUN_004487c0): when gated and linked to a
 * type-8 entity, clear its +0x2f4 word and detach attached objects. */
extern "C" void entity_clear_nested_state_8_fields(EntitySlot* e);

/* TypeFourEntityCallbackDispatch (FUN_00447900): on a type-4 source,
 * reset the action sequence of each live type-5 entity whose +0x2fc link
 * points back to it and whose +0x304 attachment is present. */
void entity_type_four_callback_dispatch(EntitySlot* source);

/* EntityEffectCountersApplyFromRecord (FUN_0042b2f0): when state flag 8
 * is set, apply record +4 through SimulationUpdateCounterAdd and repeat the
 * resource-count/sound update from record +8. */
void entity_effect_counters_apply_from_record(EntitySlot* e,
                                              const void* record);

/* EntityDamageApply (FUN_0043f6a0): apply damage to the entity health
 * mirror after the shared eligibility/attached-object gate. */
void entity_damage_apply(EntitySlot* e, float damage, int emit_effects);

/* EntityPoolFindWithLocalPredicate (FUN_0043f560): rough linked-reference
 * cleanup pass used during entity teardown/state changes. */
int entity_pool_find_with_local_predicate(GameContext* ctx,
                                          EntitySlot* target);

/* PendingTypeValueSlotSelect/ResetObjectAuxiliaryState (FUN_00407a10 /
 * 0x00407370): select a pending value slot by entity flags and command
 * type, then clear auxiliary state when the refresh gate is raised. */
void pending_type_value_set(uint32_t original_address, uint32_t value);
uint32_t pending_type_value_get(uint32_t original_address);
void pending_type_value_slot_select(const EntitySlot* e, uint32_t* out);
void entity_reset_auxiliary_state(EntitySlot* e);

typedef struct EntityCursorTargetHooks {
    void* context;
    const void* projection_adapter;
    int (*scene_position_get)(void* context, const EntitySlot* entity,
                              float out_xyz[3]);
    int (*point_project)(void* context, const float point_xyz[3],
                         float out_xy[2]);
} EntityCursorTargetHooks;
void entity_cursor_target_hooks_set(const EntityCursorTargetHooks* hooks);
int entity_cursor_target_screen_position_get(const EntitySlot* entity,
                                             float out_xy[2]);

/* Entity payload ownership hooks and scene-slot mirror
 * (FUN_00406ec0/0x004083a0/0x00408940/0x004089a0). The native entity's
 * +0x9c list is represented by host-width EntitySceneSlotNode sidecars; the
 * target's +0xa0/+0xb8/+0xc4/+0xd8 windows and +0x144 count are mirrored in
 * EntitySlot's scene-slot fields. */
typedef void (*EntityPayloadReleaseFn)(void* resource);
void entity_payload_release_hooks_set(EntityPayloadReleaseFn scene_release,
                                      EntityPayloadReleaseFn chain_release);
void entity_scene_slot_table_rebuild(EntitySlot* e, int mode);
typedef struct EntitySceneSlotRuntimeOps {
    void* context;
    /* Supplied by the renderer owner for the four DDiScene/resource
     * operations whose records are not represented by the portable scene API. */
    void (*renderer_position_prepare)(void* context, EntitySlot* entity,
                                      EntitySceneSlotNode* node);
    void (*renderer_state_apply)(void* context,
                                 EntitySceneSlotNode* node);
    void (*node_animation_select)(void* context, EntitySceneSlotNode* node,
                                  int mode);
    void* (*resource_slot_register)(void* context,
                                    EntitySceneSlotNode* node,
                                    uint32_t owner_key, uint32_t index);
} EntitySceneSlotRuntimeOps;
void entity_scene_slot_runtime_ops_set(const EntitySceneSlotRuntimeOps* ops);
void entity_scene_slot_owner_keys_set(EntitySlot* entity, uint32_t key_1c,
                                      uint32_t key_20, uint32_t key_24);
EntitySceneSlotNode* entity_scene_slot_node_append(EntitySlot* entity,
                                                   void* renderer_record,
                                                   void* payload);
void entity_scene_slot_nodes_clear(EntitySlot* entity);
extern "C" void entity_payload_state_set(EntitySlot* e, int state);
extern "C" void entity_payload_release_subslots(EntitySlot* e);
extern "C" void entity_payload_teardown(EntitySlot* e);
typedef struct EntityHeadingStepState {
    float position[2];
    float direction[2];
    float remaining;
    uint32_t flags;
} EntityHeadingStepState;
typedef struct EntityHeadingStepHooks {
    void* context;
    int (*position_to_cell)(void* context, float x, float y,
                            int* cell_x, int* cell_y);
    int (*cell_step_allowed)(void* context, void* entity, int cell_x,
                             int cell_y);
    int (*position_transition)(void* context, void* entity,
                               const float position[2]);
    void (*position_set)(void* context, void* entity,
                         const float position[2]);
} EntityHeadingStepHooks;
void entity_heading_step_move(EntityHeadingStepState* state, void* entity,
                              float delta,
                              const EntityHeadingStepHooks* hooks); /* 0x0044c1b0 */
/* The action maps the NUMERIC selector exactly once. The resolver consumes
 * that ordinal in the genuine per-building/current-activity ToolNull cache.
 * Return a real DDi-compatible scene wrapper, borrowed through the pose read;
 * never cast a host record or LwscFrameRef into that wrapper. NULL fails. */
typedef void* (*EntityObjectPositionSceneResolveHook)(
    void* context, EntitySlot* spatial_candidate, uint32_t ordinal);
typedef struct EntityAuxiliaryReleaseRecord {
    void* scene_object;
    struct EntityAuxiliaryReleaseRecord* next;
    uint32_t flags;
    float value;
} EntityAuxiliaryReleaseRecord;
typedef struct EntityAuxiliaryReleaseHooks {
    void* context;
    EntityAuxiliaryReleaseRecord* (*record_allocate)(void* context);
    void (*record_release)(void* context, EntityAuxiliaryReleaseRecord* record);
    void* (*scene_object_get)(void* context, EntitySlot* entity);
    float (*value_get)(void* context, EntitySlot* entity);
    int (*scene_point_get)(void* context, void* scene_object, float point[3]);
    int (*scene_callback_ready)(void* context, void* scene_object);
    void (*scene_callback_apply)(void* context, void* scene_object,
                                 uint32_t callback_id, const float point[3]);
    void (*scene_object_dispose)(void* context, void* scene_object);
} EntityAuxiliaryReleaseHooks;
void entity_auxiliary_payload_release(EntitySlot* entity,
                                      const EntityAuxiliaryReleaseHooks* hooks); /* 0x0044be30 */
void entity_auxiliary_release_records_flush(
    const EntityAuxiliaryReleaseHooks* hooks);
typedef struct EntityMotionSceneSyncHooks {
    void* context;
    void* (*scene_record_get)(void* context, EntitySlot* entity);
    void (*wake_transition)(void* context, EntitySlot* entity);
    int (*record_timer_active)(void* context, void* record);
    void (*record_timer_advance)(void* context, void* record, float delta);
    float (*entity_timer_get)(void* context, EntitySlot* entity);
    void (*entity_action)(void* context, EntitySlot* entity,
                          int a, int b, int c);
    int (*scene_position_get)(void* context, EntitySlot* entity,
                              float position[3]);
    void* (*scene_object_get)(void* context, EntitySlot* entity);
    int (*scene_object_registered)(void* context, void* record);
    int (*scene_should_wake)(void* context, void* scene_object);
    void (*record_timer_clear)(void* context, void* record);
    void (*scene_callback_apply)(void* context, void* scene_object,
                                 uint32_t callback_id, const float values[4]);
    void (*record_transform_apply)(void* context, void* record,
                                   uint32_t operation, const float values[3]);
    void (*periodic_callback)(void* context, EntitySlot* entity,
                              float delta, float speed,
                              uint32_t mode_a, uint32_t mode_b,
                              uint32_t mode_c);
} EntityMotionSceneSyncHooks;
void entity_motion_to_scene_node_sync(EntitySlot* entity, float delta,
                                      const EntityMotionSceneSyncHooks* hooks); /* 0x0044bc30 */
#define ENTITY_STATS_CONFIG_FIELD_COUNT 129u
typedef struct EntityStatsConfigRecord {
    char name[64];
    int type;
    int row;
    uint32_t level_count;
    float level_limit;
    char field_names[ENTITY_STATS_CONFIG_FIELD_COUNT][32];
    float fields[ENTITY_STATS_CONFIG_FIELD_COUNT];
    float level_fields[16][ENTITY_STATS_CONFIG_FIELD_COUNT];
    float levels[16];
} EntityStatsConfigRecord;
typedef struct EntityStatsConfigHooks {
    void* context;
    unsigned int (*entry_count)(void* context, void* config, void* section);
    const char* (*entry_name)(void* context, void* config, void* section,
                              unsigned int index);
    int (*type_row_resolve)(void* context, const char* name, int* type,
                            int* row);
    unsigned int (*type_capacity)(void* context, int type);
    int (*float_value_get)(void* context, const char* name,
                           const char* field, unsigned int level,
                           float* value);
    void (*record_commit)(void* context,
                          const EntityStatsConfigRecord* record);
} EntityStatsConfigHooks;
unsigned int entity_stats_config_load(void* config, void* section,
                                     const EntityStatsConfigHooks* hooks); /* 0x004667e0 */
unsigned int entity_stats_config_load_from_block(BlockFile* config);

/* --- object pointer registry (2026-10-24) ---------------------------------
 * ObjectPointerRegistryClearMatch / ObjectPointerRegistryGetSlot
 * (FUN_0042dfc0 / FUN_0042dff0, findings 292/456/247): the corpus's
 * global table of ten entity pointers plus its live count. Its only
 * feeder — the object-list record parse — is modeled without pushing
 * entries, so the count stays zero: clearing answers the no-op, and the
 * slot lookup never fires (documented deviation). The peek is a
 * tests/diagnostics view (the message_active_count precedent). */
void object_pointer_registry_clear_match(int32_t pointer);
int object_pointer_registry_get_slot(uint32_t index, int32_t* out);
uint32_t object_pointer_registry_count_peek(void);

/* --- context-selected globals (2026-10-24) ---------------------------------
 * ContextSelectedGlobalPointerGet / EntityMatchesCurrentSelectionOrOwner
 * (FUN_004295a0 / FUN_004295c0, findings 313/324/478): the corpus's
 * context-selected pointer leaf — the DAT_005560f8 / DAT_00556104 pair
 * chosen by the context-mode global — and the "matches current selection
 * or owner" predicate over the current-selection pointer (DAT_00556160)
 * and the entity's +0x36c owner pointer. The mode and current-selection
 * globals are modeled by the media layer (media_min_distance_mode /
 * _context, fed by the ported SoundMinimumDistanceUpdateMode); only the
 * pointer pair has no ported feeder, so both land zero-initialized
 * (documented deviation). The set/peek functions are the
 * tests/diagnostics view. The +0x36c owner read is pointer-sized on the
 * port (the corpus reads the 32-bit word). */
uint32_t context_selected_global_pointer_get(void);
int entity_matches_current_selection_or_owner(const void* entity);
uint32_t context_selected_pointer_pair_peek(int second);
void context_selected_pointer_pair_set(uint32_t first, uint32_t second);

/* SelectionCandidateEligibilityCheck (FUN_00452870, findings 77/85/468):
 * the selection filter's eligibility predicate — requires the +0x3e8
 * state bit 0x100, the shared-eligibility predicate, no +0x3e4 low-byte
 * bits 1/3 (the corpus's `(byte)word & 10`), no +0x3e0 bits 0x24000000, and
 * — the corpus's documented quirk branch `*param_1 != 1 || (word &
 * 0x40000)`'s right term never fires for non-type-1 entities; type 1
 * rejects on +0x3e0 bit 0x40000 (finding 468). NULL rejects (deviation). */
int selection_candidate_eligibility_check(const EntitySlot* e);

/* EntityFlag40000ThresholdCompare (FUN_00437cc0, findings 362/476): the
 * record-queue validator — the state flags (stats record +0x148) must carry
 * bit 0x40000 and the nested float at +0xe8 must fall below the entity's
 * own +0x340 bound; missing state records read zero, so the test fails. */
extern "C" int entity_flag40000_threshold_compare(const EntitySlot* e);

/* EntityFlagsContainRequestedSingleBit (FUN_00438ed0, findings 327/449):
 * the selector is accepted when it is 1/2/4/8/0x10/0x20 and the state-flag
 * word's corresponding bit 4/8/0x10/0x20/0x40/0x80 is set — the corpus's
 * chain of word re-reads, collapsed to the equivalent switch. Feeds
 * TaskType23CreateForCategoryMask (cmd.*). */
extern "C" int entity_flags_contain_requested_single_bit(const EntitySlot* e,
                                              int selector);

/* TaskCounterIncrementByBranch (Finding 464): count its zero/nonzero paths;
 * the corpus has not established a gameplay interpretation for the totals. */
void entity_task_counter_increment_by_branch(int nonzero);

#ifdef __cplusplus
}
#endif

/* EntityStatsWordFlag10Test/20Test/40Test (FUN_00439cf0/00439d10/00439d30):
 * the stats-word bit tests — the corpus masks the stats reader's word and
 * hands back the masked result (a bare bit, not a bool). */
extern "C" uint32_t entity_stats_flag10_test(const EntitySlot* e); /* FUN_00439cf0 */
extern "C" uint32_t entity_stats_flag20_test(const EntitySlot* e); /* FUN_00439d10 */
extern "C" uint32_t entity_stats_flag40_test(const EntitySlot* e); /* FUN_00439d30 */

/* GlobalAnimationStateSet (FUN_004383a0): stores the global animation-state
 * counter word (DAT_004eadc4 — the unowned global lands zero-init, finding
 * 464). The corpus's other feeder is the kind-4 dispatch increment, whose
 * dispatch row stays blocked. */
void     entity_global_animation_state_set(uint32_t value); /* FUN_004383a0 */
uint32_t entity_global_animation_state_peek(void);          /* port hook */
/* --- eligibility + admission leaves (2026-10-24; findings 332/460/251/435/471)
 * EntityFlag100AndSharedEligibilityCheck (FUN_00471ce0): with the stats
 * word's bit 0x100 set, consults the shared eligibility gate — the relaxed
 * clause is skipped only for type-4 entities (the corpus's second argument
 * is its own type test). EntityQuadMeshRenderRecordAllowed (FUN_0040aae0):
 * allowed with stats bit 0x08000000 clear and the type neither 8 nor 13.
 * TaskTypeAndConditionCheck (FUN_0043c490): the gate in relaxed mode over
 * the target and the caller's condition; the corpus's first argument is
 * discarded in the body (calling-convention residue, 0x460200 precedent). */
int entity_flag100_and_shared_eligibility_check(const EntitySlot* e);
                                                /* FUN_00471ce0 */
extern "C" int entity_quad_mesh_render_record_allowed(const EntitySlot* e);
                                                /* FUN_0040aae0 */
int task_type_and_condition_check(const void* unused,
                                 const EntitySlot* target, int condition);
                                                /* FUN_0043c490 */

/* EntityTypeAndStateFlagPredicate (FUN_0045e400, findings 324/105): the
 * predicate answers the inverted bit 0x20000000 of the entity's stats state word
 * (the ported +0x148 reader) for entity kinds outside the corpus's excluded
 * set {4, 5, 8, 9, 0xC, 0xD, 0xE, 0x10, 0x11, 0x12, 0x13}; those kinds answer
 * zero whatever the flags say.
 * Deviation: the corpus strides the entity blindly; the port guards it. */
int entity_type_and_state_flag_predicate(const EntitySlot* e); /* FUN_0045e400 */

/* ObjectFilterMatchesActionTarget (FUN_00404f50, findings 318/535): the
 * action-target filter. Type 5 matches unconditionally (subject to the
 * identity pair); types 7/9/0xF additionally require the referenced
 * entity to pass the shared eligibility check when a handle is present;
 * every other type answers 0. When the filter carries a name target
 * (+0x20), the target record's identity words must agree: word 0 equals
 * the name, word 1 equals the filter selector (+0x24) unless that is -1,
 * and word 2 equals the filter's +0x28 companion.
 * Deviations: the corpus strides both records blindly (the port guards);
 * the +0x10 entity handle rides the port's widened-link convention. */
int object_filter_matches_action_target(const void* filter_rec,
                                        const void* target_rec);
                                                /* FUN_00404f50 */

/* EntityDetachCarriedObjectAndResetState (FUN_0043bbc0): the carrier
 * detach. Only type-3 carriers act: a linked type-6 carry loses the pair
 * (link, state word, attach back-pointer, the flag word's 0x400 gate) and
 * both +0x340 floats drop to -1.0, and a linked reference has its dword
 * folded into the level's field 0x9c. The flag word's bit 0x800000 always
 * rides along with the type-3 pass.
 * Deviations: the corpus strides the slot words blindly (the port guards
 * the slot pointer); the field-0x9c fold threads the level block the
 * corpus reads through its global. */
void entity_detach_carried_object_and_reset_state(struct EntitySlot* e,
                                                  struct LevelState* lvl);
                                                /* FUN_0043bbc0 */

/* EntityClearLinkedObjectState (FUN_00447780): the linked-object clear.
 * With the flag word's gate bit 0x400 raised: the carry link and state
 * word drop, the gate drops, the carried slot's flag word bit 0x2000
 * raises, and the entity's kind-4 command-chain records drop through the
 * shared chain drop (the corpus's dispatch extras only mean the carried
 * reference / store flag for kinds 7/0xc/0x17).
 * Deviations: the corpus dereferences a missing carried slot blindly
 * (the port guards); the null slot and gate skip is the faithful shape. */
void entity_clear_linked_object_state(EntitySlot* e);
                                                /* FUN_00447780 */

/* PooledRecordChunkAllocateAndLink (FUN_00438310): grows a power-of-two
 * chunk of entity records onto the pool free list — stride links join the
 * records and the last link joins the previous chunk head, with the chunk
 * table tracking a byte counter. Deviations: widened link slot (stride
 * 0x410 vs the corpus's 0x40c) and a bounded chunk table. */
void pooled_record_chunk_allocate_and_link(void); /* FUN_00438310 */
void* pooled_record_head_peek(void);
uint32_t pooled_record_chunk_count_peek(void);

/* PooledRecordAllocateAndReset (FUN_004382c0): pops the pool's free-list
 * head, zero-clears the record's first 0x40c bytes, self-links the record
 * in the link slot (the corpus's busy mark), and seeds the two float
 * words at +0x2f4 / +0x2f8 with 0xbf800000. When the head runs empty the
 * corpus grows a chunk first; the port answers NULL only if that grow
 * finds the allocator empty (deviation: the corpus strides the word). */
void* pooled_record_allocate_and_reset(void);
void pooled_record_pool_initialize(void); /* reset pool storage for 0x436c20 */
/* EntityPoolResourcesRelease (FUN_00437050): release all tracked pool blocks
 * and clear the allocator head/count so a later initialization can regrow. */
extern "C" void pooled_record_pool_resources_release(void);

/* ResolvedEntityOperationDispatch (FUN_00460910): resolves the name through
 * the corpus resolver's (type, row) pair and feeds the count-difference
 * scan — row to the column slot (unknown names answer 0). Corpus shape:
 * the resolver zeroes the row slot at entry and only table-row matches
 * (and the ore pair) write it, so special names ride the scan with zero. */
int resolved_entity_operation_dispatch(const char* name, int slot,
                                       int subtract);          /* FUN_00460910 */

/* --- DAT_00502bd8 indexed stats-type tables (2026-10-25) -------------------
 * The corpus family FUN_00469b20 … FUN_00469be0 / FUN_00469e00 /
 * FUN_00469ec0 … FUN_00469f00 resolves a template record through
 * DAT_00502bd8's two-level type/row pointer table
 * and reads fixed fields or stride-0x150 instance blocks from the template.
 * The Stats config loader installs guarded own storage for each type/row;
 * each registered window carries its record count, while reads from an
 * empty cell answer zero. Offsets stay corpus-relative. */
#define STATS_TYPE_TABLE_TYPES 20 /* stat types 0..19; retail allocates by type */
#define STATS_TYPE_TABLE_ROWS 15  /* DAT_00502c28 uses type * 15 + row */

void stats_type_table_install(int type, int row, const void* record,
                              int stride_count);
void stats_type_table_level_limit_install(int type, int row, float level_limit);
const void* stats_type_table_cell(int type, int row);
uint32_t stats_table_word_at_cc(int type, int row, int stride_index);
int entity_task_state_capacity_bind(EntitySlot* entity);
/* FUN_00469f40: raw float word for the stats loader's parsed level count. */
extern "C" uint32_t building_upgrade_admission_check(int type, int row);

extern "C" uint32_t stats_table_word_at_b8(int type, int row, int stride_index = 0); /* FUN_00469b20 */
uint32_t stats_table_word_at_b0(int type, int row, int stride_index);
float stats_table_float_at_dc(int type, int row);    /* FUN_00469e00 */
extern "C" uint32_t stats_table_word_at_144(int type, int row); /* FUN_00469ec0 */
extern "C" uint32_t stats_table_word_at_148(int type, int row); /* FUN_00469ee0 */
extern "C" uint32_t stats_table_word_at_14c(int type, int row); /* FUN_00469f00 */

/* Strided triplets: the corpus takes the instance index as its third
 * argument (offset + index * 0x150 into the template). */
uint32_t stats_table_word_at_b4(int type, int row, int stride_index);
uint32_t stats_table_word_at_d0(int type, int row, int stride_index);
                                                   /* FUN_00469f60 */
uint32_t stats_table_word_at_100(int type, int row, int stride_index);
                                                   /* FUN_0046a0f0 */

/* Entity-indexed twins: the corpus's parameter is a config-block window
 * holding the type dword at +0x00, the row dword at +0x04, and the
 * instance index dword at +0x330. */
uint32_t stats_instance_word_at_bc(const void* config_block); /* FUN_00469b50 */
uint32_t stats_instance_word_at_e0(const void* config_block); /* FUN_00469b80 */
extern "C" uint32_t stats_instance_word_at_ec(const void* config_block); /* FUN_00469bb0 */
uint32_t stats_instance_word_at_f0(const void* config_block); /* FUN_00469be0 */

/* CreateRepeatedResourceInstances (FUN_00408e50): increments the matching
 * path-record sequence and issues the requested number of commands. */
void entity_repeated_resource_instances_create(
    const int cell_xy[2], int kind, int variant, uint32_t value,
    int count); /* FUN_00408e50 */
void entity_kind_name_tables_install(int kind, const char* const* names,
                                     uint32_t count);
extern "C" const char* item_name_by_kind_lookup(uint32_t kind, int index); /* FUN_004396c0 */
void entity_localized_name_tables_install(int kind,
                                          const char* const* names,
                                          const char* const* the_names,
                                          uint32_t count);
extern "C" const char* entity_display_name_lookup(const EntitySlot* entity); /* FUN_004395e0 */
extern "C" const char* localized_object_name_get(uint32_t kind, int index); /* FUN_00439860 */
const char* localized_object_the_name_get(uint32_t kind,
                                          int index); /* FUN_00439790 */
/* Optional override for EntityFlag400AndHealthThresholdCheck's four source
 * words. Without an installed override, the implementation reads +0x328,
 * Stats +0xec, +0x324, and +0x3d4 from their typed EntitySlot/Stats fields. */
typedef struct EntityHealthThresholdState {
    uint32_t upper_limit;
    uint32_t secondary_limit;
    uint32_t required_value;
    float health_percent;
} EntityHealthThresholdState;
typedef int (*EntityHealthThresholdReadHook)(const EntitySlot* entity,
                                             EntityHealthThresholdState* state,
                                             void* context);
void entity_health_threshold_read_hook_install(
    EntityHealthThresholdReadHook hook, void* context);
extern "C" int entity_flag400_and_health_threshold_check(
    const EntitySlot* entity); /* FUN_004399c0 */
int entity_animation_state_set(EntitySlot* entity,
                               uint32_t state); /* FUN_00439cd0 */
/* Resolve the configured type row and attach its selected 0x150-byte Stats
 * record. EntityCreateByType selects level zero during creation. */
int entity_stats_record_bind(EntitySlot* entity);
typedef EntitySlot* (*EntityDDiSceneOwnerResolveHook)(uint32_t owner_token,
                                                       void* context);
/* Bind the in-project SFX sample and entity completion callbacks. The host
 * supplies owner-token resolution because renderer owner links are opaque. */
namespace unported_ledger_stubs {
/* Matches the existing implementation namespace; the old global C declaration
 * had no definition or production installer. */
void entity_object_position_scene_resolve_hook_set(
    EntityObjectPositionSceneResolveHook hook, void* context);
/* Numeric selector ABI. -1 means no production scene owner installed;
 * 0 is a real resolve/path/install failure; 1 is route issuance, not arrival. */
int ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
    EntitySlot* entity, uint32_t selector);
void entity_ddiscene_callbacks_install(
    EntityDDiSceneOwnerResolveHook owner_resolve, void* context);
}
uint32_t entity_ddiscene_owner_token_get(const EntitySlot* entity);
void entity_ddiscene_scene_owner_bind(EntitySlot* entity, void* scene_object);
int entity_record_flag20_query(EntitySlot* entity,
                               uint32_t state); /* FUN_00439e40 */
typedef void (*EntityPeriodicTickHook)(EntitySlot* entity, float delta,
                                        void* context);
void entity_periodic_tick_hook_install(EntityPeriodicTickHook hook,
                                       void* context);
void periodic_timer25_advance(GameContext* ctx, float delta); /* FUN_0043c2b0 */
float periodic_timer25_peek(void);
void entity_activity_counters_advance(EntitySlot* entity,
                                      float elapsed); /* FUN_0043c6b0 */
typedef struct EntityOneShotAttachedEffectHooks {
    void* context;
    int (*scene_pose_get)(void* context, EntitySlot* owner,
                          float position_out[2], float* angle_out);
    EntitySlot* (*child_create)(void* context, EntitySlot* owner, int kind,
                                const float position[2], float angle);
    void (*child_action_start)(void* context, EntitySlot* child,
                               const char* action);
} EntityOneShotAttachedEffectHooks;
void entity_one_shot_attached_effect_hooks_set(
    const EntityOneShotAttachedEffectHooks* hooks);
int entity_one_shot_attached_effect_start(EntitySlot* entity,
                                          float duration); /* 0x0044c030 */
int entity_pair_scalar_fields_set(EntitySlot* entity, const int32_t pair[2],
                                  int32_t scalar); /* 0x0044c150 */

namespace unported_ledger_stubs {
void ledger_stub_00444fb0_EntityTurnToVectorActivityIssue(
    EntitySlot* entity, const float target[2]);
typedef int (*CellStepPathResolveHook)(int start_x, int start_y,
                                       int target_x, int target_y,
                                       int out_xy[2], void* user);
typedef float (*CellStepRandomRangeHook)(float minimum, float maximum,
                                         void* user);
typedef struct EmergenceSpawnHooks {
    void* user;
    struct LevelState* level;
    uint16_t (*random_u16)(void* user);
    EntitySlot* (*spawn_try)(int entity_index, int owner_index,
                             int cell_x, int cell_y, void* user);
    int (*cell_eligible)(int cell_x, int cell_y, void* user);
    int (*cell_world_position)(int cell_x, int cell_y, float out_xy[2],
                               void* user);
    EntitySlot* (*create)(void* base_entity, int type, int owner_index,
                          float x, float y, float heading, void* user);
    void (*scene_action_attach)(EntitySlot* entity, int action,
                                int argument_a, int argument_b,
                                int argument_c, void* user);
    void (*post_create)(EntitySlot* entity, int cell_x, int cell_y,
                        void* user);
} EmergenceSpawnHooks;
void emergence_spawn_hooks_set(const EmergenceSpawnHooks* hooks);
EntitySlot* emergence_random_spawn_attempt(int entity_index, int owner_index,
                                           int grid_width, int grid_height);
extern "C" EntitySlot* creature_spawn_from_emergence(void* base_entity, int mode_token,
                                                     int owner_index, int cell_x,
                                                     int cell_y); /* FUN_0043af30 */
void entity_cell_step_hooks_set(CellStepPathResolveHook path_hook,
                                CellStepRandomRangeHook random_hook,
                                void* user);
int entity_cell_step_action_peek(const EntitySlot* entity, int* type_out,
                                 float out_xy[2], uint32_t* flags_out);
float entity_cell_step_random_range(float minimum, float maximum);
int entity_cell_step_command_issue_with_jitter(
    EntitySlot* entity, const int target_xy[2], const float input_xy[2],
    int action_mode, int mark_pending); /* FUN_0043a120 */
extern "C" void entity_attached_object_list_detach(EntitySlot* entity,
                                        int reset_mode); /* FUN_00439e70 */
extern "C" int entity_capacity_or_state_predicate(
    const EntitySlot* entity); /* FUN_0043c650 */
extern "C" void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int cell_xy[2], uint32_t filter_a, uint32_t filter_b,
    uint32_t filter_c);
extern "C" void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b);
void* ledger_stub_00438b80_SpatialObjectQueryCenterMaskFromArgument(
    const EntitySlot* entity, uint32_t mask);
extern "C" void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const EntitySlot* entity);
void* ledger_stub_00438c60_EntityCandidateSearch(const EntitySlot* entity);
void* ledger_stub_00438cd0_SpatialObjectQueryCenterMask20(
    const float center[2]);
void* ledger_stub_00438d30_SpatialObjectQueryCenterMask10(
    const float center[2]);
void* ledger_stub_00438d90_SpatialObjectQueryCenterMask200(
    const float center[2]);
void* ledger_stub_00438df0_EntityLookupAtWorldPosition(
    const float center[2]);
void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra);
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict);
}

#endif /* LEGORRR_GAME_ENTITY_HPP */
