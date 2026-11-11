// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what map_view.cpp sees from
 * terrain_grid.cpp. A verbatim snapshot of terrain_grid_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_TERRAIN_GRID_FOR_MAP_VIEW_HPP
#define LRR_GAME_TERRAIN_GRID_FOR_MAP_VIEW_HPP

typedef struct GridCellObjectNode {
    void* object;
    int x;
    int y;
    float flood_gate; /* retail linked record +0x0c */
    /* Retail record +0x10 is the pool link while free, and is self-linked
     * while the record is installed in a cell. Keep one pointer here so the
     * x86 record retains the original five-word (0x14-byte) layout. */
    struct GridCellObjectNode* next;
} GridCellObjectNode;

typedef struct GridCellObjectChunk {
    struct GridCellObjectChunk* next;
    GridCellObjectNode* nodes;
    size_t node_count;
} GridCellObjectChunk;

typedef struct LevelCellKindGrid {
    struct LevelState* owner;
    uint32_t width;
    uint32_t height;
    void* retail_records; /* target allocation: width*height*0x14 */
    void** object_heads;  /* widened pointer view of each cell's +0 head */
    void** scene_objects; /* widened pointer view of each cell's +4 object */
    uint32_t* auxiliary;  /* widened sidecar view of each cell's +8 word */
    TerrainCellSceneObjectReleaseHook* scene_releases;
    void** scene_release_contexts;
    GridCellObjectChunk* chunks;
    GridCellObjectNode* free_nodes;
    uint32_t chunk_count;
} LevelCellKindGrid;

#define s_map_cell_record_grid (s_active_cell_kind_grid->retail_records)

#define s_map_cell_record_level (s_active_cell_kind_grid->owner)

#define s_grid_cell_object_heads (s_active_cell_kind_grid->object_heads)

extern TerrainCellInBoundsHook s_terrain_cell_in_bounds_hook;
extern TerrainCellStateUpdateHook s_terrain_cell_state_update_hook;
extern void* s_terrain_cell_neighbor_context;
extern LevelCellKindGrid s_empty_cell_kind_grid;
extern LevelCellKindGrid* s_active_cell_kind_grid;
extern GridCellObjectCoordinatesHook s_grid_object_coordinates_hook;
extern GridCellObjectFlagHook s_grid_object_flag_hook;
extern void* s_grid_cell_object_context;
extern TerrainCellObjectAvailableHook s_cell_object_available_hook;
extern TerrainCellObjectCreateHook s_cell_object_create_hook;
extern TerrainCellObjectDeleteHook s_cell_object_delete_hook;
extern void* s_cell_object_context;
extern TerrainActivationPrepareHook s_terrain_activation_prepare_hook;
extern TerrainActivationResolveHook s_terrain_activation_resolve_hook;
extern TerrainActivationSpecialCellHook s_terrain_activation_special_hook;
extern TerrainActivationFloodHook s_terrain_activation_flood_hook;
extern void* s_terrain_activation_context;
extern TerrainFloodStateHook s_terrain_flood_state_hook;
extern TerrainFloodEffectHook s_terrain_flood_effect_hook;
extern void* s_terrain_flood_context;
extern FenceSparkCellCenterHook s_fence_spark_center_hook;
extern FenceSparkHeightHook s_fence_spark_height_hook;
extern FenceSparkEntityRadiusHook s_fence_spark_radius_hook;
extern FenceSparkFinalizeHook s_fence_spark_finalize_hook;
extern void* s_fence_spark_context;
extern TerrainFenceSparkEntityPositionHook s_fence_spark_entity_position_hook;
extern TerrainFenceSparkCellLifetimeHook s_fence_spark_lifetime_hook;
extern TerrainFenceSparkEligibilityHook s_fence_spark_eligibility_hook;
extern TerrainFenceSparkDurationHook s_fence_spark_duration_hook;
extern void* s_fence_spark_interaction_context;
void level_cell_kind_grid_storage_release(LevelCellKindGrid* grid);

extern NestedPointerReleaseHook s_nested_handle_release_hook;
extern NestedPointerReleaseHook s_nested_memory_release_hook;

typedef struct TerrainWorldSpatialRecord {
    TerrainCellQuadRunRecord run;
    float* candidate_xy; /* four candidate coordinates per run item */
} TerrainWorldSpatialRecord;

typedef struct TerrainWorldSpatialMerge {
    uint32_t destination;
    uint32_t source;
} TerrainWorldSpatialMerge;

typedef struct TerrainWorldSpatialRecords {
    LevelState* owner;
    TerrainWorldSpatialRecord* records;
    uint32_t count;
    uint32_t capacity;
    TerrainWorldSpatialMerge* merges;
    uint32_t merge_count;
    uint32_t merge_capacity;
} TerrainWorldSpatialRecords;

extern const StagedCellCandidateRecord* s_staged_candidate_records;
extern uint32_t s_staged_candidate_record_count;
extern StagedCellCandidateLinks* s_staged_candidate_links;
extern StagedCellCandidateSelectionRecord* s_staged_selection_records;
extern uint32_t s_staged_selection_record_count;
void terrain_world_spatial_record_release( TerrainWorldSpatialRecord* record);
TerrainWorldSpatialRecord* terrain_world_spatial_record_find_at( LevelState* level, int x, int y, uint32_t* item_index);

#endif /* LRR_GAME_TERRAIN_GRID_FOR_MAP_VIEW_HPP */
