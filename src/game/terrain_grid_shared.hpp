// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/terrain_grid.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_TERRAIN_GRID_SHARED_HPP
#define LRR_GAME_TERRAIN_GRID_SHARED_HPP

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

#endif /* LRR_GAME_TERRAIN_GRID_SHARED_HPP */
