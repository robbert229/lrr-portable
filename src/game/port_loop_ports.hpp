// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_PORT_LOOP_PORTS_HPP
#define LEGORRR_GAME_PORT_LOOP_PORTS_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct EngineContext;
struct LevelState;
struct TerrainSurfaceGrid;
typedef struct BlockRec BlockFile;
typedef struct BlockRec BlockRec;

typedef struct BuildingPlacementPreviewHooks {
    void* context;
    int (*position_get)(void* context, float position[3]);
    int (*placement_check)(void* context, const float position[3]);
    void (*preview_set)(void* context, const float position[3], int valid);
    int (*path_selection_get)(void* context, const float position[3],
                              uint32_t* record_key, int cell_xy[2],
                              uint32_t* direction,
                              const int** footprint_xy,
                              uint32_t* footprint_count,
                              struct TerrainSurfaceGrid** surface_grid,
                              int* emit_boundary_commands);
} BuildingPlacementPreviewHooks;
void building_placement_preview_owner_bind(
    const BuildingPlacementPreviewHooks* hooks);
int building_placement_preview_owner_update(void);

int objective_sidecar_append_gate(struct EntitySlot* entity);

typedef struct UpgradeLevelEntry {
    uint32_t level;
    uint32_t amount;
    uint32_t value;
    uint32_t resource_id;
    char resource_name[64];
} UpgradeLevelEntry;
typedef int (*UpgradeLevelEntryReadHook)(void* context, uint32_t level,
                                         uint32_t ordinal,
                                         UpgradeLevelEntry* entry);
typedef struct UpgradeLevelTable {
    UpgradeLevelEntry entries[1000];
    uint32_t count;
    uint32_t level_mask;
} UpgradeLevelTable;
void upgrade_level_table_parse(UpgradeLevelTable* table, void* context,
                               UpgradeLevelEntryReadHook read_entry);

typedef struct VehicleTypeDefinition {
    uint32_t words[0x7b];
} VehicleTypeDefinition;
typedef int (*VehicleTypeFieldReadHook)(void* context, uint32_t field,
                                        uint32_t* value);
int vehicle_type_definition_parse(VehicleTypeDefinition* definition,
                                  void* context,
                                  VehicleTypeFieldReadHook read_field);

typedef struct AttachedModelOrientationState {
    uint32_t count;
    float anchor[3];
    float points[8][3];
    float normal[3];
    float up[3];
    float radius;
} AttachedModelOrientationState;
typedef void (*AttachedModelOrientationApplyHook)(void* context,
                                                   const float right[3],
                                                   const float up[3],
                                                   const float forward[3]);
int attached_model_orientation_update(AttachedModelOrientationState* state,
                                      void* context,
                                      AttachedModelOrientationApplyHook apply);

typedef float (*PortLoopRandomRangeHook)(void* context, float minimum,
                                         float maximum);
struct MotionWaypointTimerState {
    uint32_t flags;
    float delay;
    float delay_min;
    float delay_max;
    float duration;
    float duration_min;
    float duration_max;
    float scale_min;
    float scale_max;
    float configured_position[3];
    float scale_rate[3];
    float bounds_min[3];
    float bounds_max[3];
    float target[3];
    float position[3];
    float progress[3];
    float velocity[3];
};
typedef struct MotionJitterState {
    uint32_t flags;
    float delay;
    float delay_min;
    float delay_max;
    float lifetime;
    float lifetime_min;
    float lifetime_max;
    float magnitude_min;
    float magnitude_max;
    float position[3];
    float direction[3];
    float remaining;
    float center[3];
    float tolerance[3];
} MotionJitterState;
typedef struct MotionConfigProfileEntry {
    char* key;
    int direct_value;
    int fallback_value;
} MotionConfigProfileEntry;
typedef struct MotionConfigProfileReleaseHooks {
    void* context;
    void* (*profile_open)(void* context, void* config);
    void (*profile_write)(void* context, void* profile, const char* key,
                         const char* separator, int value);
    void (*profile_close)(void* context, void* profile);
    void (*key_release)(void* context, char* key);
} MotionConfigProfileReleaseHooks;
uint32_t motion_config_profile_entries_release(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks);
#define MOTION_KEY_VALUE_ENTRY_CAPACITY 50
typedef struct MotionKeyValueLoadRequest {
    void* config;
    int profile_id;
    int name_a_id;
    int name_b_id;
    int optional_name_id;
    int text_resource_id;
    uint32_t values[4];
} MotionKeyValueLoadRequest;
typedef struct MotionKeyValueEntry {
    char* key;
    int direct_value;
    int fallback_value;
} MotionKeyValueEntry;
typedef struct MotionKeyValueTable {
    void* name_a;
    void* name_b;
    void* optional_name;
    void* text_resource;
    uint32_t values[4];
    MotionKeyValueEntry entries[MOTION_KEY_VALUE_ENTRY_CAPACITY];
} MotionKeyValueTable;
typedef struct MotionKeyValueLoadHooks {
    void* context;
    void* (*profile_open)(void* context, void* config, int profile_id);
    int (*profile_line_get)(void* context, void* profile, uint32_t line,
                            char buffer[256]);
    int (*line_split)(void* context, char* line, char** key, char** value);
    char* (*string_duplicate)(void* context, const char* text);
    void* (*string_resolve)(void* context, int text_id);
    int (*integer_parse)(void* context, const char* text);
    void (*string_release)(void* context, char* text);
    void (*profile_close)(void* context, void* profile);
    void (*deferred_callback_register)(void* context, uint32_t callback_id);
} MotionKeyValueLoadHooks;
int motion_key_value_equals_file_load(
    const MotionKeyValueLoadRequest* request, MotionKeyValueTable* table,
    const MotionKeyValueLoadHooks* hooks);
typedef float (*EntityTypeActivityTickHook)(void* context,
                                           struct EntitySlot* entity,
                                           float scaled_delta);
typedef void (*EntityBehaviorTickHook)(void* context, struct EntitySlot* entity,
                                       float input_delta);
typedef void (*EntityActivityFinishHook)(void* context,
                                         struct EntitySlot* entity);
typedef struct EntityActivityTickHooks {
    void* context;
    float (*delta_scale)(void* context, struct EntitySlot* entity,
                         float input_delta);
    EntityTypeActivityTickHook type1_tick;
    EntityTypeActivityTickHook type2_tick;
    EntityTypeActivityTickHook type3_tick;
    EntityTypeActivityTickHook type4_tick;
    EntityTypeActivityTickHook type10_tick;
    EntityBehaviorTickHook behavior_update;
    EntityActivityFinishHook activity_finish;
} EntityActivityTickHooks;
float entity_activity_tick_step(struct EntitySlot* entity, float delta,
                                const EntityActivityTickHooks* hooks);
typedef struct EntityTypeBehaviorHooks {
    void* context;
    int (*animation_tick)(void* context, struct EntitySlot* entity,
                          float delta);
    void (*animation_play)(void* context, struct EntitySlot* entity,
                           const char* animation);
    void (*boulder_behavior)(void* context, struct EntitySlot* entity,
                             float delta);
    void (*terrain_activity)(void* context, struct EntitySlot* entity);
} EntityTypeBehaviorHooks;
void entity_type_behavior_update(struct EntitySlot* entity, float delta,
                                 const EntityTypeBehaviorHooks* hooks);
typedef struct EntityCellPathEffectHooks {
    void* context;
    int (*placement_cells_get)(void* context, struct EntitySlot* entity,
                               const int** cells, uint32_t* count);
    int (*rotation_get)(void* context, struct EntitySlot* entity);
    void (*cell_prepare)(void* context, int cell_x, int cell_y);
    void (*cell_effect_apply)(void* context, struct EntitySlot* entity,
                              int cell_x, int cell_y, int repeated_cell);
    void (*cell_refresh)(void* context, int cell_x, int cell_y);
    void (*placement_commands_build)(void* context,
                                     struct EntitySlot* entity);
} EntityCellPathEffectHooks;
void entity_cell_path_effects_apply(
    struct EntitySlot* entity, struct LevelState* level,
    const EntityCellPathEffectHooks* hooks);
typedef struct EntityDebugCellRetargetHooks {
    void* context;
    uint32_t (*target_state_flags)(void* context,
                                   struct EntitySlot* target);
    int (*target_flag20_query)(void* context, struct EntitySlot* target,
                               uint32_t state);
    int (*fallback_retarget)(void* context, struct EntitySlot* entity,
                            struct EntitySlot* target,
                            struct LevelState* level);
} EntityDebugCellRetargetHooks;
int entity_debug_cell_retarget_dispatch(
    struct EntitySlot* entity, struct EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks);
typedef struct RewardScrollTickerEntry {
    int kind;
    int x;
    int y;
    int line_height;
    uint32_t flags;
    void* text_handle;
    void* surface_handle;
} RewardScrollTickerEntry;
typedef struct RewardScrollTickerState {
    float minimum_y;
    float current_y;
    float lower_clip;
    float scroll_speed;
    float scroll_height;
    uint32_t entry_count;
    uint32_t flags;
    void* context;
    int (*entry_get)(void* context, uint32_t index,
                     RewardScrollTickerEntry* entry);
    void (*entry_draw)(void* context, const RewardScrollTickerEntry* entry,
                       int x, int y, int clipped);
    int (*debug_key_get)(void* context, uint32_t key_index);
} RewardScrollTickerState;
int reward_scroll_ticker_advance_and_draw(RewardScrollTickerState* state);
typedef struct SceneQuadGridRecord {
    void* mesh;
    uint32_t parameter;
} SceneQuadGridRecord;
typedef struct SceneQuadGridEmitHooks {
    void* context;
    void* (*mesh_create)(void* context, void* scene, int kind);
    int (*mesh_record_allocate_or_append)(void* context, void* mesh,
                                          uint32_t vertex_count,
                                          uint32_t index_rows,
                                          uint32_t index_columns,
                                          const uint32_t* indices);
    int (*face_vertices_get)(void* context, uint32_t face, uint32_t quad,
                             float vertices[4][9]);
    int (*face_vertices_emit)(void* context, void* mesh, uint32_t face,
                              uint32_t quad, const float vertices[4][9]);
    int (*face_texture_bind_by_mode)(void* context, void* mesh,
                                     uint32_t face, uint32_t mode);
    int (*scene_render_lock_toggle)(void* context, void* mesh, int enabled);
    void (*mesh_release)(void* context, void* mesh);
} SceneQuadGridEmitHooks;
/* A created mesh is owned by this record. Vertex, texture and lock hooks
 * return nonzero on success. Append returns the new zero-based group index;
 * when omitted, the renderer-owned mesh_record_allocate_or_append is used.
 * A failure destroys the entire partial mesh and never publishes a record.
 * Keep the same mesh_release owner available until record destruction. */
SceneQuadGridRecord* scene_quad_grid_emit(
    void* scene, uint32_t parameter, const SceneQuadGridEmitHooks* hooks);
int scene_quad_grid_release(SceneQuadGridRecord* record,
                             const SceneQuadGridEmitHooks* hooks);
typedef int (*MotionJitterVectorHook)(void* context, float out[3]);
typedef int (*MotionJitterCandidateHook)(void* context,
                                         const float position[3]);
typedef void (*MotionJitterPositionHook)(void* context,
                                         const float position[3]);
typedef struct MotionJitterHooks {
    void* context;
    MotionJitterVectorHook random_vector;
    MotionJitterCandidateHook candidate_valid;
    MotionJitterPositionHook position_apply;
} MotionJitterHooks;
void motion_jitter_state_step(MotionJitterState* state, float delta,
                              PortLoopRandomRangeHook random_range,
                              const MotionJitterHooks* hooks);

#ifdef __cplusplus
}

namespace unported_ledger_stubs {
uint32_t ledger_stub_0040e1e0_CellCornerOccupancyScan(
    struct LevelState* level, int cell_x, int cell_y);
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const struct EntitySlot* entity, const int cell_xy[2],
    uint32_t filter_a, uint32_t filter_b);
void ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
    int cell_x, int cell_y, uint32_t variation);
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    struct EntitySlot* entity, struct EntitySlot* target,
    const struct EntityDebugCellRetargetHooks* hooks);
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    struct EntitySlot* entity, struct EntitySlot* target, int marker);
void terrain_cell_refresh_eight_neighbors_current(const int cell_xy[2]);
void entity_cell_path_effects_apply_current(struct EntitySlot* entity);
/* StartupStreamsClose/StartupStreamStateReset (FUN_0048b290/260): the
 * four file handles and window-mode word that the startup stream routines
 * own. `slot` follows the reference dword order at 0x575ce0..0x575cec. */
void startup_stream_handle_set(int slot, void* handle);
void* startup_stream_handle_get(int slot);
void startup_stream_window_mode_set(uint32_t value);
uint32_t startup_stream_window_mode_get(void);
void startup_stream_state_reset(void);
void startup_streams_close(void);
/* StartupConfigStreamLoad (FUN_0048b2f0). The path globals are initialized
 * outside the recovered routine, so the host supplies them here. */
void startup_config_stream_paths_set(const char* output_path,
                                     const char* token_list_path);
void startup_config_stream_load(void);
typedef int (*BufferedCharReadHook)(void* context, void* stream);
void buffered_char_stream_read_hook_set(BufferedCharReadHook read_char,
                                        void* context);
int buffered_char_stream_read_to_string(void* stream, char* output);
typedef void* (*RegistryCreateKeyHook)(void* parent, const char* component,
                                       void* context);
typedef int (*RegistryQueryValueHook)(void* key, const char* value_name,
                                      void* output, uint32_t* size,
                                      uint32_t* value_type, void* context);
typedef void (*RegistryCloseKeyHook)(void* key, void* context);
void registry_path_hooks_set(RegistryCreateKeyHook create_key,
                             RegistryQueryValueHook query_value,
                             RegistryCloseKeyHook close_key, void* context);
int registry_path_create_keys(void* root, const char* subkey_path,
                              const char* value_name, int query_mode,
                              void* output, uint32_t* size,
                              uint32_t* value_type);
int registry_path_accessor(const char* subkey_path, const char* value_name,
                           int query_mode, void* output,
                           uint32_t output_size);
typedef int (*DependencyConfigEntryHook)(void* context, int level,
                                         const char* entity_name,
                                         const char* dependency_value);
void dependency_config_entry_hook_set(DependencyConfigEntryHook hook,
                                      void* context);
void dependency_grid_config_load(BlockRec* level_block,
                                 BlockFile* global_config); /* 0x0040ab70 */
void dependency_grid_level_load(struct LevelState* level,
                                BlockRec* level_block,
                                BlockFile* global_config);
void dependency_grid_level_release(struct LevelState* level);
void dependency_grid_mode_zero_input_set(struct LevelState* level, int value);
uint32_t dependency_grid_mode_zero_input_get(const struct LevelState* level);
int dependency_grid_cell_entries_validate(const struct LevelState* level,
    uint32_t kind, uint32_t row, uint32_t variant);
uint32_t dependency_grid_cell_flags_get(const struct LevelState* level,
    uint32_t kind, uint32_t row, uint32_t variant);
int dependency_config_entry_should_load(int level, const char* name,
                                        const char* value);

#define DEPENDENCY_GRID_ROWS 20u
#define DEPENDENCY_GRID_COLUMNS 15u
#define DEPENDENCY_GRID_VARIANTS 16u
#define DEPENDENCY_GRID_CELL_SLOTS 49u
#define DEPENDENCY_GRID_MATCH_CAPACITY 32u
#define DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY 30u
typedef struct DependencyGridTuple {
    uint32_t first;
    uint32_t second;
    uint32_t selector;
    uint32_t wildcard;
} DependencyGridTuple;
typedef struct DependencyGridRecordList {
    const DependencyGridTuple* records;
    uint32_t count;
} DependencyGridRecordList;
typedef struct DependencyGridCell {
    uint32_t variant_records_present;
    /* The byte flag table advances 0x31 dwords per cell. Variant records
     * use its first 16 fields; the remaining fields are still addressable
     * by the direct flag helpers' caller-supplied selector. */
    uint32_t flags[DEPENDENCY_GRID_CELL_SLOTS];
    DependencyGridRecordList base_records;
    DependencyGridRecordList variant_records[DEPENDENCY_GRID_VARIANTS];
} DependencyGridCell;
typedef struct DependencyGridState {
    DependencyGridCell cells[DEPENDENCY_GRID_ROWS][DEPENDENCY_GRID_COLUMNS];
    /* DAT_004c7178 is process-wide in retail.  Keep an explicit per-level
     * value when a caller binds one; otherwise use the global fallback. */
    uint32_t mode_zero_input;
    uint32_t mode_zero_input_bound;
} DependencyGridState;
/* Native-width view of TerrainCellRecordGet's pointer/count pair. The retail
 * table stores a 32-bit pointer; the LevelState producer owns host pointers in
 * its sidecar, so native consumers must use this accessor rather than the
 * low-word ABI mirror. x/y select the 20x15 cell and field selects one of its
 * 16 variants when that cell has variant records. The returned records stay
 * valid until that LevelState's dependency grid is reloaded or released. */
int dependency_grid_cell_record_get_native(const struct LevelState* level,
    int x, int y, int field, const DependencyGridTuple** records_out,
    uint32_t* count_out);
typedef struct DependencyGridMatches {
    /* Retail output arrays hold 32 dwords each at offsets 0, 0x80, 0x100,
     * and 0x180; count is at +0x200. */
    uint32_t x[DEPENDENCY_GRID_MATCH_CAPACITY];
    uint32_t y[DEPENDENCY_GRID_MATCH_CAPACITY];
    uint32_t variant[DEPENDENCY_GRID_MATCH_CAPACITY];
    uint32_t variant_present[DEPENDENCY_GRID_MATCH_CAPACITY];
    uint32_t count;
} DependencyGridMatches;
typedef struct DependencyGridCategoryCandidates {
    uint32_t count;
    uint32_t category;
    uint32_t row;
    uint32_t selector;
    uint32_t x[DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY];
    uint32_t y[DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY];
    uint32_t variant[DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY];
} DependencyGridCategoryCandidates;
void dependency_grid_state_bind(DependencyGridState* state);
void dependency_grid_matches_collect(uint32_t first, uint32_t second,
                                     uint32_t selector,
                                     DependencyGridMatches* output);
void dependency_grid_occupancy_pass(void);
uint32_t dependency_grid_flag4_get(int x, int y, int field);
int dependency_grid_flag100_set(int x, int y, int field);
uint32_t dependency_grid_flag100_get(int x, int y, int field);
void dependency_grid_category_candidates_build(uint32_t category,
                                               uint32_t row,
                                               uint32_t selector);
const DependencyGridCategoryCandidates*
dependency_grid_category_candidates_get(void);

#define UI_HIT_ENTRY_CAPACITY 40u
typedef struct UiEntryHitRecord {
    uint32_t fields[3];
    int32_t height_offset;
    float vertical_offset;
} UiEntryHitRecord;
typedef struct UiEntryHitTestState {
    UiEntryHitRecord* entries;
    uint32_t count;
    uint32_t flags;
    void* context;
    int (*object_bounds_get)(void* context, uint32_t object_index,
                             int* width, int* height);
} UiEntryHitTestState;
void ui_entry_hit_test_state_bind(UiEntryHitTestState* state);

int object_task_target_filter_find(
    uint32_t type, uint32_t selector, uint32_t companion);

/* Explicit view of the legacy entity/task fields consumed by
 * PlaceCommandEligibilityCheck (0x404fc0). The compact EntitySlot does not
 * own all of these stats/config words, so the runtime owner supplies them. */
typedef struct ObjectTaskEligibilityEntityView {
    int32_t type;
    uint32_t flags_3e8;
    uint32_t flags_f8;
    uint32_t flags_f9;
    uint32_t flags_fb;
    uint32_t flags_fc;
    int32_t field_bc;
    int32_t field_db;
    float field_d0;
    float field_d1;
    float field_f5;
    int32_t nested_type_2fc;
    uint32_t activity_flags;
    uint32_t task_owner_cc;
} ObjectTaskEligibilityEntityView;
typedef struct ObjectTaskEligibilityRecord {
    int32_t kind;
    struct EntitySlot* target;
    int32_t x;
    int32_t y;
    uint32_t flags_5c;
    uint32_t field_38;
    uint32_t field_3c;
    uint32_t field_5c;
    uint32_t field_17;
    uint32_t field_e;
    uint32_t field_f;
    float task_scalar_18;
} ObjectTaskEligibilityRecord;
typedef struct ObjectTaskEligibilityOps {
    void* context;
    int (*entity_view_get)(void*, const struct EntitySlot*,
                           ObjectTaskEligibilityEntityView*);
    uint32_t* global_feature_word;
    float activity_scale;
    int (*entity_special_gate)(void*, const struct EntitySlot*);
    int (*kind1_activity_gate)(void*, const struct EntitySlot*);
    int (*kind3_target_gate)(void*, const struct EntitySlot*, const struct EntitySlot*, int);
    int (*entity_can_target_kind)(void*, const struct EntitySlot*, int);
    int (*entity_target_gate)(void*, const struct EntitySlot*, const struct EntitySlot*);
    int (*entity_radius_gate)(void*, const struct EntitySlot*, const struct EntitySlot*);
    int (*entity_ray_gate)(void*, const struct EntitySlot*, const struct EntitySlot*);
    int (*entity_eligible)(void*, const struct EntitySlot*, int);
    int (*target_kind_resolve)(void*, const struct EntitySlot*);
    int (*target_activity_flags)(void*, const struct EntitySlot*);
    int (*target_position_get)(void*, const struct EntitySlot*, float*, float*);
    int (*position_to_world)(void*, int, int, float*, float*);
    int (*terrain_walkable)(void*, const struct EntitySlot*, int, int, int);
    int (*entity_target_relation)(void*, const struct EntitySlot*,
                                  const struct EntitySlot*);
    int (*target_route_position_get)(void*, const struct EntitySlot*, float*, float*);
    int (*target_type4_activity_get)(void*, const struct EntitySlot*, int*);
    int (*target_activity_position_get)(void*, const struct EntitySlot*, float*, float*);
    int (*target_distance_check)(void*, const struct EntitySlot*, float);
    float (*simulation_time_get)(void*);
    int (*target_world_position_get)(void*, const struct EntitySlot*, float*, float*);
    int (*task_coordinate_check)(void*, int, int);
    int (*command_kind_gate)(void*, const struct EntitySlot*, int);
    int (*target_visibility_check)(void*, const struct EntitySlot*, int);
    int (*target_activity_gate)(void*, const struct EntitySlot*);
    struct EntitySlot* (*kind13_target_resolve)(void*, const struct EntitySlot*,
                                                uint32_t);
} ObjectTaskEligibilityOps;
void object_task_eligibility_ops_bind(ObjectTaskEligibilityOps* ops);
typedef struct CellVisualChannelTable {
    void* roots[2];
    void* resources[2][100];
    uint32_t resource_count;
} CellVisualChannelTable;
typedef void (*CellVisualChannelReleaseHook)(void* context, void* resource);
void cell_visual_channel_release_hook_set(CellVisualChannelReleaseHook hook,
                                           void* context);
CellVisualChannelTable* cell_visual_channel_table_create(void);
int cell_visual_channel_table_resource_set(CellVisualChannelTable* table,
                                            uint32_t channel,
                                            uint32_t index, void* resource);
void cell_visual_channel_table_resources_clear(CellVisualChannelTable* table);
void cell_visual_channel_table_dispose(CellVisualChannelTable* table);

typedef struct InputDeviceReleaseHooks {
    void* context;
    void (*primary_release)(void* context);
    void (*secondary_reenable)(void* context, int enabled);
    void (*advisor_clear)(void* context);
    void (*stream_stop)(void* context, int mode);
    void (*transition_control)(void* context, int a, int b);
    void (*sound_cleanup)(void* context, int mode);
    int (*system_state_get)(void* context, int argument);
} InputDeviceReleaseHooks;
void input_device_release_state_set(uint32_t input_flags,
                                    uint32_t system_flags,
                                    int active_track);
void input_device_release_hooks_set(const InputDeviceReleaseHooks* hooks);
void input_device_release(void);
uint32_t input_device_release_flags_get(void);
uint32_t input_device_release_system_flags_get(void);
int input_device_release_active_track_get(void);

extern "C" void ledger_stub_00425c30_TransitionControl(int enabled,
                                            uint32_t parameter);

/* CellGridPointHitTest reads a global row of 16-byte action records and two
 * optional cursor rectangles.  The native owner binds the active row and
 * size query here; the function itself owns the reference hit-test order. */
typedef struct CellGridPointHitRecord {
    uint32_t action;
    uint32_t x;
    uint32_t y;
    uint32_t reserved;
} CellGridPointHitRecord;
typedef struct CellGridPointHitTestState {
    uint32_t flags;
    const CellGridPointHitRecord* records;
    uint32_t record_count;
    int rectangles_present[2];
    float rectangle_width[2];
    float rectangle_height[2];
    void* context;
    int (*record_size_get)(void* context, uint32_t record_word0,
                           uint32_t record_word1,
                           float* width, float* height);
} CellGridPointHitTestState;
void cell_grid_point_hit_test_state_bind(CellGridPointHitTestState* state);

typedef struct GlobalSubsystemResetState {
    struct EngineContext* engine;
    void* text_buffer;
    uint32_t* word_00503618;
} GlobalSubsystemResetState;
void global_subsystem_reset_state_bind(GlobalSubsystemResetState* state);
void ledger_stub_004263c0_GlobalSubsystemStateReset(void);
}
#endif

#ifdef __cplusplus
void registry_path_hooks_set(
    unported_ledger_stubs::RegistryCreateKeyHook create_key,
    unported_ledger_stubs::RegistryQueryValueHook query_value,
    unported_ledger_stubs::RegistryCloseKeyHook close_key, void* context);
extern "C" int registry_path_accessor(const char* subkey_path, const char* value_name,
                           int query_mode, void* output,
                           uint32_t output_size);
#endif

#endif
