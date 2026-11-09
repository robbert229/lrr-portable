// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_TERRAIN_GRID_HPP
#define LEGORRR_GAME_TERRAIN_GRID_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
struct EntitySlot;

#define TERRAIN_GRID_CHANGE_CAPACITY 512

typedef struct TerrainSurfaceGrid {
    uint32_t width;
    uint32_t height;
    uint32_t record_width; /* +0x28 table stride; map width, not quad width */
    uint8_t* surface_values;
    float* surface_vectors;
    uint8_t* triangulation_flags;
    uint8_t* cell_records; /* corpus cell record stride 0x28 */
    uint8_t* texture_corner_values; /* four reference corner codes per cell */
    void* material_context;
    uint32_t (*material_handle_resolve)(void* context, uint8_t material);
    void (*surface_palette_apply)(void* context, uint32_t cell_index,
                                  uint32_t handle);
    int16_t changed_xy[TERRAIN_GRID_CHANGE_CAPACITY][2];
    uint32_t changed_count;
    void* initialize_context;
    void (*initialize_cell)(void* context, uint32_t x, uint32_t y);
    const void* terrain_window; /* original cell-variant/normal sampling view */
    struct LevelState* owner_level; /* owning level's transition sidecar */
} TerrainSurfaceGrid;

/* Tile visual grid owned by a level record (LoadTileMeshes, FUN_0042b330).
 * Each cell stores the TileModelEntityRegister result, whose native record
 * is a SceneCellVisualRelayRecord with two roots and two 100-slot channels.
 * The grid dimensions are SurfTextWidth/SurfTextHeight, not map dimensions. */
struct SceneCellVisualRelayRecord;
/* A successful registrar returns a complete scene relay record; it must
 * release any temporary models/resources before returning NULL. The grid
 * owns successful records and falls back to the native scene-object release
 * path when no explicit release callback is installed. */
typedef struct SceneCellVisualRelayRecord* (*TerrainTileMeshRegisterHook)(
    void* scene, const char* bitmap_a, const char* bitmap_b,
    void* material, void* options, void* context);
typedef void (*TerrainTileMeshReleaseHook)(
    struct SceneCellVisualRelayRecord* visual, void* context);
typedef struct TerrainTileMeshGrid {
    struct LevelState* owner_level;
    uint32_t width;
    uint32_t height;
    uint32_t surface_text_width;
    uint32_t surface_text_height;
    struct SceneCellVisualRelayRecord** meshes;
    void* scene;
    void* material;
    void* options;
    float cell_projection_scale;
    void* visual_lookup_table;
    uint32_t* visual_lookup_entries;
    const char* bitmap_prefix;
    int editor_mode;
    void* context;
    TerrainTileMeshRegisterHook register_mesh;
    TerrainTileMeshReleaseHook release_mesh;
    /* TextureSetConfigLoad's static cell bitmap handles are owned by this
     * grid. The RoofTexture animation handle is owned by the indexed-surface
     * resource state, not by this level grid. */
    struct TerrainCellResourceGrid* textures;
    const char* mesh_basename;
    const char* texture_basename;
    void* surface_text_map;
    void (*surface_text_map_insert)(void* map, uint8_t key, uint8_t value);
    TerrainSurfaceGrid* surface_grid;
    void (*surface_texture_grid_attach)(void* scene,
                                        struct TerrainCellResourceGrid* grid,
                                        void* context);
} TerrainTileMeshGrid;
int terrain_tile_mesh_grid_load(TerrainTileMeshGrid* grid,
                                const char* bitmap_prefix);
int terrain_tile_mesh_grid_load_level(struct LevelState* level,
                                      const char* bitmap_prefix); /* 0x0042b330 */
int entity_neighbor_blocked_cell_step_probe(struct EntitySlot* entity,
                                             float cadence, int mode);
void terrain_tile_mesh_grid_release(TerrainTileMeshGrid* grid);
struct SceneCellVisualRelayRecord* terrain_tile_mesh_visual_channels_get(
    TerrainTileMeshGrid* grid, uint8_t packed_surface);
struct BlockFile;
void terrain_texture_set_config_context_bind(struct BlockFile* config);
int terrain_texture_set_config_load(TerrainTileMeshGrid* grid,
                                    const char* group_name);
int terrain_texture_set_config_load_level(struct LevelState* level,
                                          const char* group_name);
int terrain_texture_set_config_load_current(TerrainTileMeshGrid* grid,
                                            const char* group_name);
void terrain_texture_set_release(TerrainTileMeshGrid* grid);
typedef struct TerrainCellResourceGrid {
    uint32_t width;
    uint32_t height;
    /* TextureSurfaceCreate records, released through TextureRelease. */
    void** handles;
} TerrainCellResourceGrid;
/* Explicit bitmap-resource grid release adapter; retail 0x434530 clears the
 * current level's visual-channel resources without freeing its cell records. */
void terrain_grid_entry_resources_release_all(TerrainCellResourceGrid* grid);
void terrain_grid_entry_resources_release_all_current(void); /* 0x00434530 */
TerrainCellResourceGrid* terrain_cell_resource_grid_load(
    const char* bitmap_prefix, uint32_t width, uint32_t height); /* 0x0040b310 */
void terrain_cell_resource_grid_dispose(TerrainCellResourceGrid* grid);

void terrain_surface_grid_initialize(TerrainSurfaceGrid* grid,
                                     uint32_t width, uint32_t height,
                                     uint8_t* surface_values,
                                     float* surface_vectors,
                                     uint8_t* triangulation_flags);
void terrain_surface_grid_material_hooks_set(
    TerrainSurfaceGrid* grid, uint8_t* cell_records,
    uint8_t* texture_corner_values, void* context,
    uint32_t (*resolve)(void*, uint8_t),
    void (*apply)(void*, uint32_t, uint32_t));
void terrain_surface_grid_terrain_window_set(TerrainSurfaceGrid* grid,
                                             const void* terrain_window);
void terrain_cell_variant_table_insert(void* terrain_window, uint8_t key,
                                      uint8_t value);
void terrain_cell_material_set(TerrainSurfaceGrid* grid, uint32_t x,
                               uint32_t y, uint8_t material,
                               uint8_t rotation); /* FUN_0044f230 */
void terrain_cell_material_transition_begin(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            int material,
                                            int rotation); /* 0x0044e6d0 */
void terrain_material_transition_tick(TerrainSurfaceGrid* grid,
                                      float delta); /* 0x0044f090 */
/* GridCellSetPayloadIndex @ 0x0044e860 forwards a width*row+column lookup
 * through the terrain's detail object; its return is discarded. */
void grid_cell_set_payload_index(void* terrain, int x, int y);
/* Host gameplay predicate for TerrainCreate's mirrored detail-enable bit. */
int grid_cell_detail_enabled_get(void* terrain, int x, int y);
void terrain_grid_cell_initializer_set(TerrainSurfaceGrid* grid, void* context,
                                       void (*initialize_cell)(void*, uint32_t,
                                                               uint32_t));
void terrain_grid_cells_initialize(TerrainSurfaceGrid* grid); /* 0x0044e880; default worker is 0x00450f30 */
typedef void (*TerrainGridOperationHook)(void* context, uint32_t cell_index,
                                        uint32_t operation);
void terrain_grid_operation_apply_all(TerrainSurfaceGrid* grid,
                                     uint32_t operation, void* context,
                                     TerrainGridOperationHook hook); /* FUN_0044f490 */
typedef struct TerrainPaletteGrid {
    uint32_t width;
    uint32_t height;
    const uint8_t* cell_records; /* 0x48-byte level-cell records */
    void* context;
    void (*palette_resource_select)(void* context, uint32_t packed_index);
} TerrainPaletteGrid;
void terrain_cell_palette_resource_at_grid_position(
    const TerrainPaletteGrid* grid, uint32_t x, uint32_t y);
void* terrain_cell_palette_resource_at_grid_position_current(
    const int cell_xy[2]); /* 0x00429160 */
typedef struct TerrainVertexBuildGrid {
    uint32_t width;
    uint32_t height;
    uint32_t map_width;
    uint32_t map_height;
    float vertical_scale;
    float spacing;
    float x_origin;
    float z_origin;
    float minimum_y;
    const uint8_t* cell_records; /* height byte at +0x1a, stride 0x28 */
    void* context;
    void (*vertices_write)(void* context, uint32_t cell,
                           const float vertices[12]); /* X,Z,height; 00,10,11,01 */
} TerrainVertexBuildGrid;
void terrain_grid_vertex_build(TerrainVertexBuildGrid* grid); /* FUN_0044e4d0 */
/* Terrain view rendering (FUN_0045dbc0/FUN_0045e9e0): per-cell style is
 * resolved first, then the view builder places the tile in world space. */
typedef int (*TerrainTileStyleHook)(void* context, uint32_t x, uint32_t y,
                                    uint32_t style, float rgb[3]);
typedef void (*TerrainViewTileEmitHook)(void* context, uint32_t x, uint32_t y,
                                        const float origin[3],
                                        const float size[2],
                                        const float rgb[3]);
typedef struct TerrainViewTileGrid {
    uint32_t width;
    uint32_t height;
    uint32_t style;
    float origin[3];
    float tile_size[2];
    void* context;
    TerrainTileStyleHook style_resolve;
    TerrainViewTileEmitHook emit;
} TerrainViewTileGrid;
/* FUN_0045e9e0 reads the live cell and selects one of the retail palette
 * triplets.  The output triplet is written at byte offset 0x10. */
int terrain_tile_style_resolve(void* output, int x, int y);
void terrain_view_tile_geometry_build(TerrainViewTileGrid* grid);
typedef struct NestedPointerRecord {
    void* first_handle;
    void* child_buffer;
    void* second_handle;
} NestedPointerRecord;
typedef void (*NestedPointerReleaseHook)(void* pointer);
void nested_pointer_record_release_hooks_set(NestedPointerReleaseHook handle_hook,
                                             NestedPointerReleaseHook memory_hook);
void nested_pointer_record_destroy(NestedPointerRecord* record); /* FUN_0044e670 */
int terrain_cell_surface_vertices_get(TerrainSurfaceGrid* grid, uint32_t x,
                                      uint32_t y, float out[12]); /* 0x004500d0 */
void terrain_cell_surface_orientation_set(TerrainSurfaceGrid* grid,
                                         uint32_t x, uint32_t y,
                                         int flipped); /* 0x0044edf0 */
void terrain_cell_surface_triangle_vertices_select(
    TerrainSurfaceGrid* grid, float x, float z, uint32_t out_vertices[3]);
int terrain_cell_ray_hit_test(TerrainSurfaceGrid* grid, int x,
                              int y, const float origin[3],
                              const float direction[3], float hit[3]);
int terrain_surface_vertex_span_validate(TerrainSurfaceGrid* grid,
                                         const int* cell_xy,
                                         uint32_t cell_count,
                                         float maximum_height_span,
                                         float maximum_edge_delta); /* 0x0044e8c0 */
typedef struct StagedCellCandidateRecord {
    void* record;
    void* predecessor; /* adapter metadata; retail merges use first match */
    uint32_t pair_count;
    const float* cell_pairs; /* pair_count consecutive (x,y) float pairs */
} StagedCellCandidateRecord;
typedef struct StagedCellCandidateLinks {
    void** previous_records;
    void** current_records;
    uint32_t count;
    uint32_t capacity;
} StagedCellCandidateLinks;
void staged_cell_candidate_collect_state_install(
    const StagedCellCandidateRecord* records, uint32_t record_count,
    StagedCellCandidateLinks* links);
void* staged_cell_candidate_collect(uint32_t row, uint32_t first_cell,
                                    uint32_t end_cell);
typedef struct StagedCellCandidateSelectionRecord {
    uint32_t item_count;
    const float* candidate_xy; /* item_count * four (x,y) alternatives */
    uint32_t selected_count;
    uint32_t selected_capacity;
    uint32_t* selected_items;
    uint32_t* selected_alternatives;
} StagedCellCandidateSelectionRecord;
void staged_cell_candidate_selection_state_install(
    StagedCellCandidateSelectionRecord* records, uint32_t record_count);
void staged_cell_candidate_select(void* unused, struct LevelState* level);
typedef void (*TerrainCellQuadRunFlushHook)(void* context, uint32_t row,
                                           uint32_t first_cell,
                                           uint32_t end_cell);
typedef void (*TerrainCellQuadRunFinishHook)(void* context);
typedef int (*TerrainCellTypeReadHook)(void* context, struct LevelState* level,
                                       uint32_t x, uint32_t y,
                                       uint32_t* cell_type);
void terrain_cell_quad_run_stage(
    struct LevelState* level, void* context,
    TerrainCellTypeReadHook read_cell_type,
    TerrainCellQuadRunFlushHook flush_run,
    TerrainCellQuadRunFinishHook finish);
void terrain_cell_quad_run_stage_level(void* scene,
                                       struct LevelState* level); /* 0x0046db90 */
void terrain_cell_quad_run_stage_level_release(struct LevelState* level);
void world_spatial_records_install(struct LevelState* level, void* records);
void terrain_world_spatial_edge_midpoints_update(struct LevelState* level);
typedef struct TerrainCellQuadRunRecord {
    uint32_t item_count;
    const float* cell_xy; /* item_count consecutive (x,y) pairs */
    float height;
    uint32_t flags;
    int mesh;
    uint32_t slot_capacity;
    uint32_t selected_count;
    uint32_t* slot_source_items; /* selected slot -> source run item */
    uint8_t* slot_active;
    float* slot_offsets; /* slot_capacity consecutive (x,y) pairs */
    float* slot_heights;
    uint32_t* slot_orientations; /* selects one of four corner pairs */
} TerrainCellQuadRunRecord;
typedef struct TerrainCellQuadRunEmitHooks {
    void* context;
    int (*cell_type_get)(void* context, struct LevelState* level,
                         int x, int y, uint32_t* cell_type);
    int (*cell_vertices_get)(void* context, float x, float y,
                             float vertices[4][3]);
    int (*mesh_create)(void* context, int kind);
    void (*mesh_visibility_set)(void* context, int mesh, int visible);
    void (*mesh_texture_scale_set)(void* context, int mesh,
                                   float scale_x, float scale_y,
                                   float scale_z);
    int (*quad_vertices_emit)(void* context, int mesh, uint32_t item,
                              const float vertices[4][9]);
} TerrainCellQuadRunEmitHooks;
void terrain_cell_quad_run_emit(
    struct LevelState* level, TerrainCellQuadRunRecord* records,
    uint32_t record_count, const TerrainCellQuadRunEmitHooks* hooks);
typedef int (*TerrainCellTriangleCornerPairHook)(void* context, uint32_t x,
                                                 uint32_t y,
                                                 uint32_t corners[2]);
typedef struct TerrainCellQuadSlotMutationHooks {
    void* context;
    void (*mesh_visibility_set)(void* context, int mesh, int visible);
    TerrainCellTriangleCornerPairHook height_corner_pair_get;
} TerrainCellQuadSlotMutationHooks;
int terrain_cell_surface_quad_height_fill(
    TerrainSurfaceGrid* surface, TerrainCellQuadRunRecord* records,
    uint32_t record_count, uint32_t x, uint32_t y,
    const TerrainCellQuadSlotMutationHooks* hooks);
void terrain_cell_surface_quad_height_fill_current(struct LevelState* level,
                                                    int x, int y); /* 0x0046e220 */
void terrain_cell_surface_quad_reset_level(struct LevelState* level, int x,
                                           int y);
void terrain_cell_surface_quad_reset_current(int x, int y); /* 0x0046e330 */
int terrain_cell_quad_run_slot_state_get(struct LevelState* level, int x,
                                         int y, float* height_out,
                                         uint32_t* active_out);
typedef struct BuildingFootprintPreviewHooks {
    void* context;
    /* FUN_00433090 && !FUN_00433030 for one cardinal neighbor. */
    int (*adjacent_occupied_unflagged)(void* context, int x, int y);
    /* FUN_00431b20 is called once for the footprint cell. The final two
     * arguments are its param_3/param_4 mode flags. */
    int (*neighbor_cell_accept)(void* context, int x, int y,
                                uint32_t repeated_cell, int route_limit_hit);
    /* FUN_00428780; nonzero selects the blocked preview style. */
    int (*terrain_command_predicate)(void* context, const int offset[2]);
    void (*preview_color_set)(void* context, uint32_t preview_index,
                              float red, float green, float blue,
                              float alpha);
    void (*preview_overlay_color_set)(void* context, uint32_t preview_index,
                                      float red, float green, float blue);
    void (*cell_preview_emit)(void* context, uint32_t preview_index,
                              int x, int y, int in_bounds,
                              int surface_valid, int neighbor_valid);
} BuildingFootprintPreviewHooks;
/* Update the twenty raw (X,Z,height) positions; preserve six metadata words
 * per vertex. Input corners may overlap the output, and NULL inputs refuse. */
int building_footprint_vertices_update(float vertices[20][9],
                                       const float terrain_corners[4][3],
                                       float extrusion);
/* Returns the rotated scratch cells when placement is valid, NULL otherwise.
 * Rendering callbacks use compact group indices, one per distinct source
 * cell; adjacent duplicate pairs retain their separate validation semantics. */
const int* building_footprint_render_and_validate(
    TerrainSurfaceGrid* surface, const int offset[2], const int* source_xy,
    uint32_t count, int rotation, float maximum_height_span,
    float maximum_edge_delta, uint32_t minimum_routes,
    const BuildingFootprintPreviewHooks* hooks);
void terrain_cell_surface_height_clamp(TerrainSurfaceGrid* grid,
                                       const int* cell_xy,
                                       uint32_t cell_count,
                                       float maximum_offset); /* 0x0044ead0 */
int terrain_surface_point_project_offset(TerrainSurfaceGrid* grid,
                                         float x, float z,
                                         int orientation, float offset,
                                         float* out_x, float* out_z); /* 0x0044fb90 */
void terrain_surface_grid_values_get(TerrainSurfaceGrid* grid, uint32_t x,
                                     uint32_t y, float out[12]);
/* Read live mesh XYZ positions from the owning level's SceneRender cache. */
int grid_cell_read_surface_values_level(struct LevelState* level, uint32_t x,
                                       uint32_t y, float out[12]);
int grid_cell_read_surface_vector(TerrainSurfaceGrid* grid, uint32_t x,
                                  uint32_t y, float out[3]); /* 0x00450060 */
int grid_cell_read_surface_vector_level(struct LevelState* level,
                                        uint32_t x, uint32_t y,
                                        float out[3]);
void terrain_cell_surface_vector_write(TerrainSurfaceGrid* grid,
                                       uint32_t x, uint32_t y,
                                       uint32_t corner,
                                       const float vector[3]); /* 0x00451230 */
void terrain_cell_surface_vectors_initialize(TerrainSurfaceGrid* grid,
                                             uint32_t x, uint32_t y); /* 0x00450f30 */
void terrain_cell_surface_vector_offset_add(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            uint32_t corner,
                                            const float offset[3]); /* 0x00451450 */
void terrain_cell_surface_voxel_neighbors_mutate(
    TerrainSurfaceGrid* grid, uint32_t x, uint32_t y,
    float height_delta); /* 0x0044f380 */
void terrain_cell_surface_vectors_rebuild(TerrainSurfaceGrid* grid,
                                          uint32_t x, uint32_t y); /* 0x004514f0 */
int terrain_cell_edge_vector_query(TerrainSurfaceGrid* grid, uint32_t x,
                                   uint32_t y, float out[3]); /* 0x004507d0 */
void grid_surface_write_at_xy(TerrainSurfaceGrid* grid, int x, int y,
                              uint8_t value); /* FUN_00450700 */
typedef void (*TerrainGridCircleCellCallback)(int x, int y);
void terrain_surface_clear_circle(TerrainSurfaceGrid* grid, int center_x,
                                  int center_y, int radius,
                                  TerrainGridCircleCellCallback callback);
                                                        /* FUN_00450600 */
float terrain_context_scale_get(const void* context); /* FUN_00450890 */
void terrain_cell_change_queue_append(TerrainSurfaceGrid* grid, int x,
                                      int y); /* FUN_00450730 */
void terrain_cell_change_queue_flush(TerrainSurfaceGrid* grid); /* FUN_00450780 */

/* MapCellRecordGridInitialize/MapCellStateBufferRebuild (FUN_0040cdc0,
 * FUN_0040ce80): own the map's width*height array of 0x14-byte cell records. */
void map_cell_record_grid_initialize(void* level);
void map_cell_state_buffer_rebuild(void* level);
void map_cell_record_grid_activate(void* level);
void map_cell_record_grid_release(void* level);
void map_cell_record_grid_release_current(void);
void* map_cell_record_grid_get(void);
void* map_cell_record_grid_level_get(void);
/* Retail current-level global DAT_005560d4, mirrored by level activation. */
extern struct LevelState* g_active_level_retail;
int terrain_cell_object_record_present(int x, int y);
int terrain_cell_object_record_present_for_level(const struct LevelState* level,
                                                 int x, int y);
typedef int (*TerrainCellInBoundsHook)(void* level, int x, int y,
                                       void* context);
typedef void (*TerrainCellStateUpdateHook)(void* level, int x, int y,
                                           void* context);
void terrain_cell_neighbor_update_hooks_install(
    TerrainCellInBoundsHook in_bounds, TerrainCellStateUpdateHook update,
    void* context);
void terrain_cell_and_neighbors_update(int x, int y); /* FUN_0040cea0 */
typedef int (*GridCellObjectCoordinatesHook)(void* object, int* x, int* y,
                                             void* context);
typedef void (*GridCellObjectFlagHook)(void* object, void* context);
void grid_cell_object_hooks_install(GridCellObjectCoordinatesHook coordinates,
                                    GridCellObjectFlagHook set_registered,
                                    void* context);
void* grid_cell_object_link_insert(void* object, int x, int y); /* FUN_0040cf80 */
void register_object_at_grid_position(void* object); /* FUN_0040cf50 */
void terrain_cell_entity_record_unlink(void* record, int x, int y); /* FUN_0040d100 */
void terrain_entity_cell_record_remove(void* object); /* FUN_0040d0a0 */
typedef int (*TerrainCellObjectAvailableHook)(int x, int y, void* context);
typedef void* (*TerrainCellObjectCreateHook)(int x, int y, void* context);
typedef void (*TerrainCellObjectDeleteHook)(void* object, void* context);
void terrain_cell_object_hooks_install(TerrainCellObjectAvailableHook available,
                                      TerrainCellObjectCreateHook create,
                                      TerrainCellObjectDeleteHook destroy,
                                      void* context);
int create_cell_object_if_available(int x, int y); /* FUN_0040d170 */
int delete_cell_object(int x, int y); /* FUN_0040d1f0 */
typedef void* (*TerrainCellSceneObjectCreateHook)(
    struct LevelState* level, uint32_t x, uint32_t y,
    float world_x, float world_y, void* context);
typedef void (*TerrainCellSceneObjectReleaseHook)(
    struct LevelState* level, void* object, void* context);
void terrain_cell_scene_object_hooks_install(
    TerrainCellSceneObjectCreateHook create,
    TerrainCellSceneObjectReleaseHook release, void* context);
void terrain_cell_scene_object_update(struct LevelState* level,
                                      uint32_t x, uint32_t y,
                                      int should_exist); /* FUN_0040e350 */
typedef void (*TerrainActivationPrepareHook)(void* context);
typedef int (*TerrainActivationResolveHook)(void* object, int* x, int* y,
                                            uint32_t* entity_flags,
                                            uint32_t* state_flags,
                                            void* context);
typedef int (*TerrainActivationSpecialCellHook)(int x, int y, void* context);
typedef void (*TerrainActivationFloodHook)(int x, int y, int mode,
                                           float strength, void* context);
void terrain_activation_hooks_install(TerrainActivationPrepareHook prepare,
                                     TerrainActivationResolveHook resolve,
                                     TerrainActivationSpecialCellHook special,
                                     TerrainActivationFloodHook flood,
                                     void* context);
int terrain_cell_activation_try(void* object, int x, int y); /* FUN_0040d4f0 */
/* Flood-state query overrides for the active level. Without an override the
 * implementation uses TerrainCellSpecialStatePredicate directly: mode 0
 * tests primary bit 0x400; mode 1 also requires secondary bit 0x100. Edge
 * creation remains renderer-owned. */
typedef int (*TerrainFloodStateHook)(int x, int y, int mode, void* context);
typedef void (*TerrainFloodEffectHook)(int from_x, int from_y, int to_x,
                                       int to_y, void* context);
void terrain_neighbor_flood_hooks_install(TerrainFloodStateHook state,
                                          TerrainFloodEffectHook effect,
                                          void* context);
void terrain_cell_neighbor_flood_walk(int x, int y, int force,
                                      float strength); /* FUN_0040d850 */
typedef int (*TerrainCellWorldPositionHook)(void* level, int x, int y,
                                            float position[3], void* context);
void terrain_cell_world_position_hook_install(
    TerrainCellWorldPositionHook hook, void* context);
int terrain_cell_world_position_get(int x, int y, float position[3]);
void compute_world_position_delta(int x1, int y1, int x2, int y2,
                                  float delta[3]); /* FUN_0040dd90 */
int cell_pair_edge_effect_apply(int x1, int y1, int x2, int y2); /* FUN_0040dc20 */
typedef int (*FenceSparkCellCenterHook)(int x, int y, float position[3],
                                        void* context);
typedef int (*FenceSparkHeightHook)(float x, float y, float* height,
                                    void* context);
typedef float (*FenceSparkEntityRadiusHook)(void* entity, void* context);
typedef void (*FenceSparkFinalizeHook)(void* entity, float dx, float dy,
                                       void* context);
void fence_spark_terrain_hooks_install(FenceSparkCellCenterHook center,
                                      FenceSparkHeightHook height,
                                      FenceSparkEntityRadiusHook radius,
                                      FenceSparkFinalizeHook finalize,
                                      void* context);
void fence_spark_launch_register(void* entity, const float position_xy[2],
                                 int cell_x, int cell_y); /* FUN_0040e0c0 */
typedef int (*TerrainFenceSparkEntityPositionHook)(void* entity,
                                                   float position_xy[2],
                                                   void* context);
typedef int (*TerrainFenceSparkCellLifetimeHook)(int x, int y,
                                                 float* remaining,
                                                 void* context);
typedef int (*TerrainFenceSparkEligibilityHook)(int x, int y,
                                                void* context);
typedef float (*TerrainFenceSparkDurationHook)(void* entity, void* context);
void terrain_fence_spark_hooks_install(
    TerrainFenceSparkEntityPositionHook entity_position,
    TerrainFenceSparkCellLifetimeHook lifetime,
    TerrainFenceSparkEligibilityHook eligible,
    TerrainFenceSparkDurationHook duration, void* context);
int entity_fence_spark_terrain_interaction(void* entity); /* FUN_0040de40 */
void terrain_record_pool_chunk_allocate(void); /* FUN_0040d030 */

/* TerrainCellVariantUpdateQueue (FUN_0043abf0) uses native entity/level APIs
 * for position, random scatter, cell gates, work clearing, and CellInitPass.
 * The marker is a DDi scene object, and the trailing LAB_0043ac90 body is an
 * entity-specific visitor not represented by the CPU renderer; keep only
 * those operations behind typed owner hooks. */
typedef void (*TerrainVariantSceneMarkerHook)(void* entity, void* context);
typedef int (*TerrainVariantEntityVisitHook)(void* owner_entity,
                                             struct EntitySlot* candidate,
                                             void* context);
void terrain_variant_scene_hooks_set(TerrainVariantSceneMarkerHook marker,
                                     TerrainVariantEntityVisitHook visit,
                                     void* context);
void terrain_cell_variant_update_queue(void* entity); /* FUN_0043abf0 */

/* TerrainGridPathSearch (FUN_00440c70): cardinal breadth-first search over
 * the active LevelState with alternating frontiers and a reusable per-cell
 * depth map, passing an optional actor to the route predicate and an
 * optional stop callback. The typed grid/route hooks are host overrides;
 * normal gameplay resolves dimensions and cell state natively. */
typedef int (*TerrainPathGridSizeHook)(void* level, uint32_t* width,
                                       uint32_t* height, void* context);
typedef int (*TerrainPathRouteHook)(void* level, uint32_t x, uint32_t y,
                                    int direction, void* context);
/* The neighbor search checks one cell beyond a candidate in the same
 * direction; a positive result makes that route edge cost 0.5 instead of 1. */
typedef int (*TerrainPathHalfCostHook)(void* level, uint32_t x, uint32_t y,
                                       int direction, void* context);
typedef int (*TerrainPathStopHook)(void* level, const uint32_t xy[2],
                                   void* user);
void terrain_grid_path_hooks_set(TerrainPathGridSizeHook grid_size,
                                 TerrainPathRouteHook route,
                                 void* context);
void terrain_neighbor_path_half_cost_hook_set(TerrainPathHalfCostHook hook);
int terrain_grid_path_search(void* actor, uint32_t start_x, uint32_t start_y,
                             uint32_t goal_x, uint32_t goal_y,
                             int** out_x, int** out_y, uint32_t* out_count,
                             TerrainPathStopHook stop, void* user);
/* Both route searches return allocated cell arrays INCLUDING start and goal.
 * A distinct adjacent destination therefore requires two entries. */
int terrain_neighbor_path_check(void* actor, uint32_t start_x,
                                uint32_t start_y, uint32_t goal_x,
                                uint32_t goal_y, int** out_x, int** out_y,
                                uint32_t* out_count, TerrainPathStopHook stop,
                                void* user);
int terrain_neighbor_path_check_current(void* actor, uint32_t start_x,
    uint32_t start_y, uint32_t goal_x, uint32_t goal_y, int** out_x,
    int** out_y, float* out_count, TerrainPathStopHook stop, void* user);
                                                        /* 0x004410f0 */
/* Release the reusable DAT_004dee00 cost field during entity-pool teardown. */
void terrain_neighbor_path_cost_release(void);

#ifdef __cplusplus
}

namespace unported_ledger_stubs {
uint32_t terrain_placement_blocked_adjacent_cell_scan(
    int x, int y, int mode, int duplicate_pair); /* 0x00431b20 */
}
#endif

#endif
