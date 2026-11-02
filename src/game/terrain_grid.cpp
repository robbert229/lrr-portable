// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

#include "terrain_grid.hpp"
#include "../gods98/block.hpp"
#include "level.hpp"
#include "game.hpp"
#include "entity.hpp"
#include "weapon.hpp"
#include "util.hpp"
#include "scene_render.hpp"
#include "../crt/crt.hpp"
#include "../gods98/heap.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/record.hpp"
#include "../gods98/input.hpp"
#include "../gods98/message.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/surface.hpp"
#include "../gods98/view.hpp"
#include "../gods98/engine.hpp"

#include <stdlib.h>

static BlockFile* s_texture_set_config_context;
static TerrainCellInBoundsHook s_terrain_cell_in_bounds_hook;
static TerrainCellStateUpdateHook s_terrain_cell_state_update_hook;
static void* s_terrain_cell_neighbor_context;
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
static LevelCellKindGrid s_empty_cell_kind_grid;
static LevelCellKindGrid* s_active_cell_kind_grid =
    &s_empty_cell_kind_grid;
#define s_map_cell_record_grid (s_active_cell_kind_grid->retail_records)
#define s_map_cell_record_level (s_active_cell_kind_grid->owner)
// GLOBAL: LEGORR 0x005560d4
struct LevelState* g_active_level_retail;
#define s_grid_cell_object_heads (s_active_cell_kind_grid->object_heads)
#define s_grid_cell_object_chunks (s_active_cell_kind_grid->chunks)
#define s_grid_cell_object_free (s_active_cell_kind_grid->free_nodes)
#define s_grid_cell_object_chunk_count (s_active_cell_kind_grid->chunk_count)
static GridCellObjectCoordinatesHook s_grid_object_coordinates_hook;
static GridCellObjectFlagHook s_grid_object_flag_hook;
static void* s_grid_cell_object_context;
static void* terrain_cell_kind14_entity_create(struct LevelState* level,
    uint32_t x, uint32_t y, float world_x, float world_y, void* context);
static void terrain_cell_kind14_entity_release(struct LevelState* level,
    void* object, void* context);
static TerrainCellSceneObjectCreateHook s_cell_scene_object_create_hook =
    terrain_cell_kind14_entity_create;
static TerrainCellSceneObjectReleaseHook s_cell_scene_object_release_hook =
    terrain_cell_kind14_entity_release;
static void* s_cell_scene_object_context;

static int terrain_config_key_equal(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return 0;
    while (*a != '\0' && *b != '\0') {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static void terrain_tile_mesh_visual_lookup_release(TerrainTileMeshGrid* grid)
{
    if (grid == NULL) return;
    free(grid->visual_lookup_entries);
    free(grid->visual_lookup_table);
    grid->visual_lookup_entries = NULL;
    grid->visual_lookup_table = NULL;
}
static TerrainCellObjectAvailableHook s_cell_object_available_hook;
static TerrainCellObjectCreateHook s_cell_object_create_hook;
static TerrainCellObjectDeleteHook s_cell_object_delete_hook;
static void* s_cell_object_context;
static TerrainActivationPrepareHook s_terrain_activation_prepare_hook;
static TerrainActivationResolveHook s_terrain_activation_resolve_hook;
static TerrainActivationSpecialCellHook s_terrain_activation_special_hook;
static TerrainActivationFloodHook s_terrain_activation_flood_hook;
static void* s_terrain_activation_context;
static TerrainFloodStateHook s_terrain_flood_state_hook;
static TerrainFloodEffectHook s_terrain_flood_effect_hook;
static void* s_terrain_flood_context;
static TerrainCellWorldPositionHook s_cell_world_position_hook;
static void* s_cell_world_position_context;
static int terrain_flood_special_query(int x, int y, int mode);
static FenceSparkCellCenterHook s_fence_spark_center_hook;
static FenceSparkHeightHook s_fence_spark_height_hook;
static FenceSparkEntityRadiusHook s_fence_spark_radius_hook;
static FenceSparkFinalizeHook s_fence_spark_finalize_hook;
static void* s_fence_spark_context;
static TerrainVariantSceneMarkerHook s_terrain_variant_marker_hook;
static TerrainVariantEntityVisitHook s_terrain_variant_visit_hook;
static void* s_terrain_variant_scene_context;
static TerrainPathGridSizeHook s_terrain_path_grid_size_hook;
static TerrainPathRouteHook s_terrain_path_route_hook;
static TerrainPathHalfCostHook s_terrain_path_half_cost_hook;
static void* s_terrain_path_context;
/* DAT_004dee04 / DAT_004dee08: the retail path-search functions share and
 * reuse one zeroed dword per map cell. */
// GLOBAL: LEGORR 0x004dee04
static uint32_t* s_terrain_path_depth;
// GLOBAL: LEGORR 0x004dee08
static uint32_t s_terrain_path_depth_count;
// GLOBAL: LEGORR 0x004dee00
static float* s_terrain_neighbor_path_cost;
static uint32_t s_terrain_neighbor_path_cost_count;
static TerrainFenceSparkEntityPositionHook s_fence_spark_entity_position_hook;
static TerrainFenceSparkCellLifetimeHook s_fence_spark_lifetime_hook;
static TerrainFenceSparkEligibilityHook s_fence_spark_eligibility_hook;
static TerrainFenceSparkDurationHook s_fence_spark_duration_hook;
static void* s_fence_spark_interaction_context;

static void level_cell_kind_grid_storage_release(LevelCellKindGrid* grid)
{
    GridCellObjectChunk* chunk;
    size_t count, i;
    if (grid == NULL) return;
    count = (size_t)grid->width * (size_t)grid->height;
    if (grid->scene_objects != NULL) {
        for (i = 0u; i < count; ++i) {
            if (grid->scene_objects[i] != NULL) {
                TerrainCellSceneObjectReleaseHook release =
                    grid->scene_releases != NULL &&
                    grid->scene_releases[i] != NULL
                    ? grid->scene_releases[i]
                    : s_cell_scene_object_release_hook;
                void* context = grid->scene_release_contexts != NULL &&
                    grid->scene_releases != NULL &&
                    grid->scene_releases[i] != NULL
                    ? grid->scene_release_contexts[i]
                    : s_cell_scene_object_context;
                if (release != NULL)
                    release(grid->owner, grid->scene_objects[i], context);
            }
        }
    }
    chunk = grid->chunks;
    while (chunk != NULL) {
        GridCellObjectChunk* next = chunk->next;
        free(chunk->nodes);
        free(chunk);
        chunk = next;
    }
    free(grid->retail_records);
    free(grid->object_heads);
    free(grid->scene_objects);
    free(grid->auxiliary);
    free(grid->scene_releases);
    free(grid->scene_release_contexts);
    grid->retail_records = NULL;
    grid->object_heads = NULL;
    grid->scene_objects = NULL;
    grid->auxiliary = NULL;
    grid->scene_releases = NULL;
    grid->scene_release_contexts = NULL;
    grid->chunks = NULL;
    grid->free_nodes = NULL;
    grid->chunk_count = 0u;
}

void map_cell_record_grid_release(void* level)
{
    LevelCellKindGrid* grid;
    if (level == NULL) return;
    grid = (LevelCellKindGrid*)
        ((struct LevelState*)level)->ptr_cell_kind_grid;
    if (grid == NULL || grid->owner != level) {
        if (g_active_level_retail == level)
            g_active_level_retail = NULL;
        return;
    }
    level_cell_kind_grid_storage_release(grid);
    if (s_active_cell_kind_grid == grid) {
        s_active_cell_kind_grid = &s_empty_cell_kind_grid;
        g_active_level_retail = NULL;
    }
    ((struct LevelState*)level)->ptr_cell_kind_grid = NULL;
    free(grid);
}

/* SelectionMarkerResourcesRelease (FUN_0040ce30): release the active
 * registry's pool and cell table. The portable owner also releases scene
 * records through the renderer callback before detaching its LevelState. */
void map_cell_record_grid_release_current(void)
{
    LevelCellKindGrid* grid = s_active_cell_kind_grid;
    if (grid == NULL || grid == &s_empty_cell_kind_grid) {
        g_active_level_retail = NULL;
        return;
    }
    if (grid->owner != NULL &&
        grid->owner->ptr_cell_kind_grid == grid)
        grid->owner->ptr_cell_kind_grid = NULL;
    level_cell_kind_grid_storage_release(grid);
    free(grid);
    s_active_cell_kind_grid = &s_empty_cell_kind_grid;
    g_active_level_retail = NULL;
}

void map_cell_record_grid_activate(void* level)
{
    LevelCellKindGrid* grid = level != NULL
        ? (LevelCellKindGrid*)((struct LevelState*)level)->ptr_cell_kind_grid
        : NULL;
    s_active_cell_kind_grid = grid != NULL && grid->owner == level
        ? grid : &s_empty_cell_kind_grid;
    g_active_level_retail = (struct LevelState*)level;
}

/* MapCellRecordGridInitialize (FUN_0040cdc0): allocate the target's
 * width*height*0x14-byte table and widened per-level pointer views. */
// FUNCTION: LEGORR 0x0040CDC0
void map_cell_record_grid_initialize(void* level)
{
    LevelCellKindGrid* grid;
    uint32_t width, height, count;
    size_t bytes;
    if (level == NULL) {
        s_active_cell_kind_grid = &s_empty_cell_kind_grid;
        g_active_level_retail = NULL;
        return;
    }
    map_cell_record_grid_release(level);
    g_active_level_retail = (struct LevelState*)level;
    width = *(const uint32_t*)((const unsigned char*)level + 0x14);
    height = *(const uint32_t*)((const unsigned char*)level + 0x18);
    if (height != 0u && width > 0x7fffffffu / height) return;
    count = width * height;
    if (count > 0x7fffffffu / 0x14u) return;
    grid = (LevelCellKindGrid*)calloc(1u, sizeof(*grid));
    if (grid == NULL) return;
    grid->owner = (struct LevelState*)level;
    grid->width = width;
    grid->height = height;
    bytes = (size_t)count * 0x14u;
    grid->retail_records = calloc(1u, bytes);
    grid->object_heads = (void**)calloc(count, sizeof(void*));
    grid->scene_objects = (void**)calloc(count, sizeof(void*));
    grid->auxiliary = (uint32_t*)calloc(count, sizeof(uint32_t));
    grid->scene_releases = (TerrainCellSceneObjectReleaseHook*)calloc(
        count, sizeof(TerrainCellSceneObjectReleaseHook));
    grid->scene_release_contexts = (void**)calloc(count, sizeof(void*));
    if (grid->retail_records == NULL || grid->object_heads == NULL ||
        grid->scene_objects == NULL || grid->auxiliary == NULL ||
        grid->scene_releases == NULL ||
        grid->scene_release_contexts == NULL) {
        level_cell_kind_grid_storage_release(grid);
        free(grid);
        return;
    }
    ((struct LevelState*)level)->ptr_cell_kind_grid = grid;
    s_active_cell_kind_grid = grid;
}

/* MapCellStateBufferRebuild (FUN_0040ce80): release prior marker resources,
 * then rebuild the level's per-cell state records. */
// FUNCTION: LEGORR 0x0040CE80
void map_cell_state_buffer_rebuild(void* level)
{
    /* FUN_0040ce80 always releases the active marker registry before
     * allocating and clearing the replacement cell table. */
    if (s_active_cell_kind_grid != NULL &&
        s_active_cell_kind_grid != &s_empty_cell_kind_grid)
        map_cell_record_grid_release_current();
    map_cell_record_grid_initialize(level);
}

void* map_cell_record_grid_get(void)
{
    return s_map_cell_record_grid;
}

void* map_cell_record_grid_level_get(void)
{
    return g_active_level_retail;
}

void terrain_cell_neighbor_update_hooks_install(
    TerrainCellInBoundsHook in_bounds, TerrainCellStateUpdateHook update,
    void* context)
{
    s_terrain_cell_in_bounds_hook = in_bounds;
    s_terrain_cell_state_update_hook = update;
    s_terrain_cell_neighbor_context = context;
}

/* TerrainCellAndNeighborsUpdate (FUN_0040cea0): update a cell, then each
 * cardinal neighbor that passes the installed terrain bounds predicate. */
// FUNCTION: LEGORR 0x0040CEA0
void terrain_cell_and_neighbors_update(int x, int y)
{
    static const int offsets[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 },
                                      { -1, 0 } };
    int index;
    LevelState* level = (LevelState*)s_map_cell_record_level;
    if (level == NULL) {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL) level = game->level;
    }
    /* The original registry is global and implicitly names the current
     * level. Keep that implicit state synchronized with the explicit owner
     * used by the portable sidecar before refreshing this cell ring. */
    map_cell_record_grid_activate(level);
    if (s_terrain_cell_state_update_hook == NULL) {
        if (level == NULL) return;
        level_cell_init_pass(level, x, y);
        for (index = 0; index < 4; ++index) {
            int nx = x + offsets[index][0];
            int ny = y + offsets[index][1];
            if (level_voxel(level, nx, ny) == NULL)
                continue;
            level_cell_init_pass(level, nx, ny);
        }
        return;
    }
    s_terrain_cell_state_update_hook(s_map_cell_record_level, x, y,
                                     s_terrain_cell_neighbor_context);
    for (index = 0; index < 4; ++index) {
        int nx = x + offsets[index][0];
        int ny = y + offsets[index][1];
        int in_bounds = s_terrain_cell_in_bounds_hook != NULL
            ? s_terrain_cell_in_bounds_hook(s_map_cell_record_level, nx, ny,
                    s_terrain_cell_neighbor_context)
            : level != NULL && level_voxel(level, nx, ny) != NULL;
        if (in_bounds != 0) {
            s_terrain_cell_state_update_hook(s_map_cell_record_level, nx, ny,
                                             s_terrain_cell_neighbor_context);
        }
    }
}

void grid_cell_object_hooks_install(GridCellObjectCoordinatesHook coordinates,
                                    GridCellObjectFlagHook set_registered,
                                    void* context)
{
    s_grid_object_coordinates_hook = coordinates;
    s_grid_object_flag_hook = set_registered;
    s_grid_cell_object_context = context;
}

static int grid_cell_object_coordinates_get(void* object, int* x, int* y)
{
    EntitySlot* entity = (EntitySlot*)object;
    GameContext* game;
    LevelState* level;
    float position[2];
    if (object == NULL || x == NULL || y == NULL) return 0;
    if (s_grid_object_coordinates_hook != NULL) {
        int result = s_grid_object_coordinates_hook(
            object, x, y, s_grid_cell_object_context);
        if (result != 0) {
            game = entity->owner_ctx != NULL ? entity->owner_ctx :
                entity_pool_callback_context_get();
            if (game != NULL)
                map_cell_record_grid_activate(game->level);
            return 1;
        }
    }
    game = entity->owner_ctx != NULL ? entity->owner_ctx :
        entity_pool_callback_context_get();
    level = game != NULL ? game->level :
        (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    map_cell_record_grid_activate(level);
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                  position[1], x, y) ||
        !level_coord_pair_within_bounds(level, *x, *y)) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x0040D030
void terrain_record_pool_chunk_allocate(void)
{
    GridCellObjectChunk* chunk;
    size_t node_count;
    size_t node_bytes;
    size_t index;
    if (s_grid_cell_object_chunk_count >= 32u
        || s_grid_cell_object_chunk_count >= sizeof(size_t) * CHAR_BIT) {
        return;
    }
    node_count = (size_t)1u << s_grid_cell_object_chunk_count;
    if (node_count > ((size_t)-1) / sizeof(GridCellObjectNode)) {
        return;
    }
    node_bytes = node_count * sizeof(GridCellObjectNode);
    chunk = (GridCellObjectChunk*)malloc(sizeof(GridCellObjectChunk));
    if (chunk == NULL) {
        return;
    }
    chunk->nodes = (GridCellObjectNode*)malloc(node_bytes);
    if (chunk->nodes == NULL) {
        free(chunk);
        return;
    }
    chunk->node_count = node_count;
    chunk->next = s_grid_cell_object_chunks;
    s_grid_cell_object_chunks = chunk;
    ++s_grid_cell_object_chunk_count;
    for (index = 0; index < node_count; ++index) {
        memset(&chunk->nodes[index], 0, sizeof(chunk->nodes[index]));
        chunk->nodes[index].next = index + 1u < node_count
            ? &chunk->nodes[index + 1u] : s_grid_cell_object_free;
    }
    /* The retail pool points at the first record in the new contiguous
     * chunk; its last record already links to the prior free-list head. */
    s_grid_cell_object_free = &chunk->nodes[0];
}

typedef struct TerrainVariantVisitContext {
    void* owner_entity;
} TerrainVariantVisitContext;

static int terrain_variant_entity_visit_adapter(EntitySlot* candidate,
                                                 void* context)
{
    TerrainVariantVisitContext* visit = (TerrainVariantVisitContext*)context;
    if (visit == NULL || s_terrain_variant_visit_hook == NULL) return 0;
    return s_terrain_variant_visit_hook(visit->owner_entity, candidate,
                                        s_terrain_variant_scene_context);
}

void terrain_variant_scene_hooks_set(TerrainVariantSceneMarkerHook marker,
                                     TerrainVariantEntityVisitHook visit,
                                     void* context)
{
    s_terrain_variant_marker_hook = marker;
    s_terrain_variant_visit_hook = visit;
    s_terrain_variant_scene_context = context;
}

/* TerrainCellVariantUpdateQueue (FUN_0043abf0). CPU-visible behavior is
 * owned here; only the DDi marker and unavailable local entity visitor are
 * renderer/entity-owner hooks. */
// FUNCTION: LEGORR 0x0043ABF0
void terrain_cell_variant_update_queue(void* entity)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    int cell_xy[2] = { 0, 0 };
    TerrainVariantVisitContext visit;
    if (level == NULL) level = (LevelState*)map_cell_record_grid_level_get();
    if (entity == NULL || level == NULL) return;

    map_cell_record_grid_activate(level);
    entity_position_to_terrain_cell((const EntitySlot*)entity, level,
                                    &cell_xy[0], &cell_xy[1]);
    if (level_random_draw15() % 5 == 0) {
        random_variant_cell_scatter(cell_xy[0], cell_xy[1], 3, 1);
    }
    if (level_cell_occupied_at(level, cell_xy[0], cell_xy[1]) != 0) {
        if (s_terrain_variant_marker_hook != NULL) {
            s_terrain_variant_marker_hook(
                entity, s_terrain_variant_scene_context);
        }
        terrain_cell_work_flag_clear(level, cell_xy, 0x3fu);
        level_cell_init_pass(level, cell_xy[0], cell_xy[1]);
    }

    /* FUN_0043ac90's per-entity callback is renderer/entity-owner behavior
     * not represented by the terrain module. Preserve the pool walk when a
     * live GameContext exists; the level-only path above remains useful to
     * callers that own the active terrain registry without that context. */
    if (game != NULL) {
        visit.owner_entity = entity;
        (void)entity_pool_find_by_callback_filtered_current(
            terrain_variant_entity_visit_adapter, &visit);
    }
}

void terrain_grid_path_hooks_set(TerrainPathGridSizeHook grid_size,
                                 TerrainPathRouteHook route,
                                 void* context)
{
    s_terrain_path_grid_size_hook = grid_size;
    s_terrain_path_route_hook = route;
    s_terrain_path_context = context;
}

void terrain_neighbor_path_half_cost_hook_set(TerrainPathHalfCostHook hook)
{
    s_terrain_path_half_cost_hook = hook;
}

/* The retail search's first argument is an optional actor passed through to
 * TerrainCellRouteAndOccupancyCheck. Grid dimensions always come from the
 * active level. Older portable callers also pass LevelState or TerrainState
 * here, so recognize those owners without interpreting their leading words
 * as an EntitySlot. */
static LevelState* terrain_path_level_resolve(void* argument,
                                              EntitySlot** actor_out)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level :
        (LevelState*)map_cell_record_grid_level_get();
    int type;
    if (actor_out != NULL) *actor_out = NULL;
    if (level == NULL) return NULL;
    if (argument == NULL || argument == level ||
        argument == level->ptr_terrain) return level;
    /* Validate actor identity against the active typed tables before reading
     * any EntitySlot fields. This also makes the current-level APIs safe for
     * their historical LevelState/TerrainState arguments. */
    if (game != NULL) {
        for (type = 1; type <= 19; ++type) {
            int index;
            const int count = entity_count(game, type);
            for (index = 0; index < count; ++index) {
                EntitySlot* candidate =
                    (EntitySlot*)entity_at(game, type, index);
                if (candidate == argument) {
                    if (actor_out != NULL) *actor_out = candidate;
                    return level;
                }
            }
        }
        return level;
    }
    /* Without the entity pools an unknown pointer cannot safely be treated
     * as an EntitySlot; NULL and explicit level/terrain arguments still use
     * the active registry's live level. */
    return NULL;
}

static int terrain_path_grid_size_get(void* entity, uint32_t* width,
                                      uint32_t* height)
{
    LevelState* level;
    if (s_terrain_path_grid_size_hook != NULL)
        return s_terrain_path_grid_size_hook(entity, width, height,
                                             s_terrain_path_context);
    level = terrain_path_level_resolve(entity, NULL);
    if (level == NULL || width == NULL ||
        height == NULL) return 0;
    *width = (uint32_t)level_width(level);
    *height = (uint32_t)level_height(level);
    return *width != 0u && *height != 0u;
}

static int terrain_path_route_accept(void* entity, uint32_t x, uint32_t y,
                                     int direction)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    LevelState* level;
    EntitySlot* actor;
    if (s_terrain_path_route_hook != NULL)
        return s_terrain_path_route_hook(entity, x, y, direction,
                                         s_terrain_path_context);
    level = terrain_path_level_resolve(entity, &actor);
    if (level == NULL || direction < 0 ||
        direction >= 4) return 0;
    return terrain_cell_route_and_occupancy_check(
        actor, level, (int)x, (int)y,
        (int)x + dx[direction], (int)y + dy[direction], 0) != 0;
}

static int terrain_path_cell_accept(void* entity, uint32_t x, uint32_t y)
{
    LevelState* level;
    EntitySlot* actor;
    level = terrain_path_level_resolve(entity, &actor);
    if (level != NULL) {
        return terrain_cell_route_and_occupancy_check(
            actor, level, (int)x, (int)y,
            (int)x, (int)y, 0) != 0;
    }
    /* The legacy route hook cannot express the target's same-cell
     * `(x,y)->(x,y)` predicate, so use it only when no live level exists. */
    return s_terrain_path_route_hook != NULL
        ? s_terrain_path_route_hook(entity, x, y, 0,
                                    s_terrain_path_context)
        : 0;
}

static int terrain_path_half_cost_accept(void* entity, uint32_t x,
                                         uint32_t y, int direction)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    LevelState* level;
    int probe_x;
    int probe_y;
    if (s_terrain_path_half_cost_hook != NULL)
        return s_terrain_path_half_cost_hook(entity, x, y, direction,
                                             s_terrain_path_context);
    level = terrain_path_level_resolve(entity, NULL);
    if (level == NULL || direction < 0 ||
        direction >= 4) return 0;
    probe_x = (int)x + dx[direction];
    probe_y = (int)y + dy[direction];
    return level_cell_occupied_at(level, probe_x, probe_y) != 0;
}

// FUNCTION: LEGORR 0x00440C70
int terrain_grid_path_search(void* actor, uint32_t start_x,
                             uint32_t start_y, uint32_t goal_x,
                             uint32_t goal_y, int** out_x, int** out_y,
                             uint32_t* out_count, TerrainPathStopHook stop,
                             void* user)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    enum {
        TERRAIN_PATH_FRONTIER_CAPACITY = 100,
        TERRAIN_PATH_FRONTIER_STRIDE = 5
    };
    /* Retail's local_fc0 allocates 500 dwords per side. Each queue record
     * advances five dwords while its first two carry x/y. */
    uint32_t frontier[2][TERRAIN_PATH_FRONTIER_CAPACITY]
                     [TERRAIN_PATH_FRONTIER_STRIDE];
    uint32_t frontier_count[2] = { 0u, 0u };
    uint32_t width, height, cell_count, start, goal = 0u;
    uint32_t side = 0u, depth_value = 1u;
    uint32_t found = 0u;
    int* route_x;
    int* route_y;
    uint32_t length;
    uint32_t current_x, current_y, current_depth;
    int direction_cursor = 0;
    if (out_x == NULL || out_y == NULL || out_count == NULL
        || !terrain_path_grid_size_get(actor, &width, &height)
        || width == 0 || height == 0 || width > 65535u || height > 65535u
        || width > 16777216u / height
        || start_x >= width || start_y >= height) return 0;
    cell_count = width * height;
    if (stop == NULL) {
        if (goal_x >= width || goal_y >= height
            || !terrain_path_cell_accept(actor, goal_x, goal_y)) return 0;
    }
    if (s_terrain_path_depth_count != cell_count) {
        free(s_terrain_path_depth);
        s_terrain_path_depth = NULL;
        s_terrain_path_depth_count = cell_count;
        s_terrain_path_depth = (uint32_t*)malloc(
            (size_t)cell_count * sizeof(uint32_t));
    }
    if (s_terrain_path_depth == NULL) {
        s_terrain_path_depth_count = 0u;
        return 0;
    }
    memset(s_terrain_path_depth, 0,
           (size_t)cell_count * sizeof(uint32_t));
    start = start_y * width + start_x;
    s_terrain_path_depth[start] = 1u;
    frontier[0][0][0] = start_x;
    frontier[0][0][1] = start_y;
    frontier_count[0] = 1u;
    for (;;) {
        uint32_t read_index;
        uint32_t next_side = side ^ 1u;
        uint32_t next_count = 0u;
        ++depth_value;
        for (read_index = 0u; read_index < frontier_count[side];
             ++read_index) {
            uint32_t x = frontier[side][read_index][0];
            uint32_t y = frontier[side][read_index][1];
            int direction;
            for (direction = 0; direction < 4; ++direction) {
                int nx = (int)x + dx[direction];
                int ny = (int)y + dy[direction];
                uint32_t next;
                uint32_t candidate[2];
                if (nx < 0 || ny < 0 || (uint32_t)nx >= width ||
                    (uint32_t)ny >= height) continue;
                next = (uint32_t)ny * width + (uint32_t)nx;
                if (s_terrain_path_depth[next] != 0u ||
                    !terrain_path_route_accept(actor, (uint32_t)nx,
                                               (uint32_t)ny, direction))
                    continue;
                /* Retail's five-dword record window holds 100 coordinates
                 * per side. Do not reproduce its unchecked stack overwrite
                 * if a malformed grid exceeds that frontier size. */
                if (next_count >= TERRAIN_PATH_FRONTIER_CAPACITY) continue;
                frontier[next_side][next_count][0] = (uint32_t)nx;
                frontier[next_side][next_count][1] = (uint32_t)ny;
                ++next_count;
                s_terrain_path_depth[next] = depth_value;
                candidate[0] = (uint32_t)nx;
                candidate[1] = (uint32_t)ny;
                if (stop != NULL) {
                    if (stop(actor, candidate, user)) {
                        goal = next;
                        found = 1u;
                    }
                }
                else if ((uint32_t)nx == goal_x &&
                         (uint32_t)ny == goal_y) {
                    goal = next;
                    found = 1u;
                }
            }
        }
        frontier_count[side] = 0u;
        if (found != 0u) break;
        side = next_side;
        frontier_count[side] = next_count;
        if (frontier_count[side] == 0u) return 0;
    }
    length = depth_value;
    route_x = (int*)malloc((size_t)length * sizeof(int));
    route_y = (int*)malloc((size_t)length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        return 0;
    }
    current_x = goal % width;
    current_y = goal / width;
    route_x[length - 1u] = (int)current_x;
    route_y[length - 1u] = (int)current_y;
    route_x[0] = (int)start_x;
    route_y[0] = (int)start_y;
    current_depth = length - 1u;
    while (current_depth > 1u) {
        int tries;
        for (tries = 0; tries < 4; ++tries) {
            int previous_x = (int)current_x + dx[direction_cursor];
            int previous_y = (int)current_y + dy[direction_cursor];
            if (previous_x >= 0 && previous_y >= 0 &&
                (uint32_t)previous_x < width &&
                (uint32_t)previous_y < height &&
                s_terrain_path_depth[(uint32_t)previous_y * width +
                                     (uint32_t)previous_x] == current_depth) {
                current_x = (uint32_t)previous_x;
                current_y = (uint32_t)previous_y;
                --current_depth;
                route_x[current_depth] = previous_x;
                route_y[current_depth] = previous_y;
                break;
            }
            direction_cursor = (direction_cursor + 1) & 3;
        }
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = length;
    return 1;
}

/* TerrainNeighborPathCheck (FUN_004410f0) uses two alternating fixed-size
 * FIFO frontiers and a reusable float cost map. Each accepted relaxation is
 * appended to the opposite frontier, even if that coordinate is already
 * present there; lower costs cause another visit on the next pass. */
// FUNCTION: LEGORR 0x004410F0
int terrain_neighbor_path_check_current(void* actor, uint32_t start_x,
    uint32_t start_y, uint32_t goal_x, uint32_t goal_y, int** out_x,
    int** out_y, float* out_count, TerrainPathStopHook stop, void* user)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    enum { FRONTIER_CAPACITY = 100, FRONTIER_STRIDE = 5 };
    uint32_t frontier[2][FRONTIER_CAPACITY][FRONTIER_STRIDE];
    uint32_t frontier_count[2] = { 1u, 0u };
    LevelState* route_level = terrain_path_level_resolve(actor, NULL);
    uint32_t width, height, cell_count;
    uint32_t side = 0u;
    uint32_t goal_index = 0u;
    uint32_t result_goal_x = goal_x;
    uint32_t result_goal_y = goal_y;
    int status = 0;
    int target_found = 0;
    /* Retail starts with the smallest positive float, then truncates to int
     * and adds one before processing the first frontier. */
    float iteration_count = 0.0f;
    float result_count = 0.0f;
    int* route_x;
    int* route_y;
    uint32_t route_length;
    uint32_t current_x, current_y;
    if (out_x == NULL || out_y == NULL || out_count == NULL ||
        !terrain_path_grid_size_get(actor, &width, &height) || width == 0u ||
        height == 0u || width > 65535u || height > 65535u ||
        width > 16777216u / height || start_x >= width || start_y >= height)
        return 0;
    if (stop == NULL && (goal_x >= width || goal_y >= height ||
        !terrain_path_cell_accept(actor, goal_x, goal_y))) return 0;
    cell_count = width * height;
    if (s_terrain_neighbor_path_cost_count != cell_count) {
        free(s_terrain_neighbor_path_cost);
        s_terrain_neighbor_path_cost = NULL;
        s_terrain_neighbor_path_cost_count = cell_count;
    }
    if (s_terrain_neighbor_path_cost == NULL) {
        s_terrain_neighbor_path_cost = (float*)malloc(
            (size_t)cell_count * sizeof(float));
    }
    if (s_terrain_neighbor_path_cost == NULL) {
        s_terrain_neighbor_path_cost_count = 0u;
        return 0;
    }
    memset(s_terrain_neighbor_path_cost, 0,
           (size_t)cell_count * sizeof(float));
    s_terrain_neighbor_path_cost[start_y * width + start_x] = 1.0f;
    frontier[0][0][0] = start_x;
    frontier[0][0][1] = start_y;

    /* Retail has no queue-membership bit: a coordinate is enqueued again
     * whenever a later relaxation lowers its current cost. */
    while (status == 0) {
        uint32_t read_index;
        uint32_t next_side = side ^ 1u;
        uint32_t next_count = 0u;
        iteration_count = (float)((int)iteration_count + 1);
        for (read_index = 0u; read_index < frontier_count[side];
             ++read_index) {
            uint32_t x = frontier[side][read_index][0];
            uint32_t y = frontier[side][read_index][1];
            float base_cost = s_terrain_neighbor_path_cost[y * width + x];
            int direction;
            for (direction = 0; direction < 4; ++direction) {
                int nx = (int)x + dx[direction];
                int ny = (int)y + dy[direction];
                uint32_t next;
                float edge_cost = 1.0f;
                float candidate_cost;
                if (nx < 0 || ny < 0 || (uint32_t)nx >= width ||
                    (uint32_t)ny >= height) continue;
                if (terrain_path_half_cost_accept(
                        actor, (uint32_t)nx, (uint32_t)ny, direction))
                    edge_cost = 0.5f;
                if (!terrain_path_route_accept(actor, (uint32_t)nx,
                                               (uint32_t)ny, direction))
                    continue;
                next = (uint32_t)ny * width + (uint32_t)nx;
                candidate_cost = edge_cost + base_cost;
                if (s_terrain_neighbor_path_cost[next] != 0.0f &&
                    candidate_cost >= s_terrain_neighbor_path_cost[next])
                    continue;
                s_terrain_neighbor_path_cost[next] = candidate_cost;
                /* Retail writes unbounded records into a 100-entry side.
                 * Preserve state/stop effects while dropping only an
                 * unrepresentable queue write in the portable build. */
                if (next_count < FRONTIER_CAPACITY) {
                    frontier[next_side][next_count][0] = (uint32_t)nx;
                    frontier[next_side][next_count][1] = (uint32_t)ny;
                    ++next_count;
                }
                if (stop != NULL) {
                    uint32_t candidate[2];
                    candidate[0] = (uint32_t)nx;
                    candidate[1] = (uint32_t)ny;
                    if (stop(actor, candidate, user)) {
                        status = 1;
                        goal_index = next;
                        result_goal_x = (uint32_t)nx;
                        result_goal_y = (uint32_t)ny;
                        result_count = iteration_count;
                    }
                }
                else if ((uint32_t)nx == goal_x &&
                         (uint32_t)ny == goal_y) {
                    target_found = 1;
                    goal_index = next;
                    result_count = iteration_count;
                }
            }
        }
        frontier_count[side] = 0u;
        side = next_side;
        frontier_count[side] = next_count;
        if (frontier_count[side] == 0u) status = -1;
    }
    if (stop == NULL && target_found) status = 1;
    if (status != 1) return 0;

    route_length = (uint32_t)result_count;
    if (route_length == 0u) return 0;
    route_x = (int*)malloc((size_t)route_length * sizeof(int));
    route_y = (int*)malloc((size_t)route_length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        return 0;
    }
    current_x = goal_index % width;
    current_y = goal_index / width;
    route_x[route_length - 1u] = (int)current_x;
    route_y[route_length - 1u] = (int)current_y;
    route_x[0] = (int)start_x;
    route_y[0] = (int)start_y;

    /* Select the smallest positive cost difference below ten. An exact
     * difference of ten is accepted only for an occupied predecessor. */
    if (route_length > 2u) {
        uint32_t index = route_length - 2u;
        for (;;) {
            int found_previous = 0;
            int direction;
            uint32_t current_index = current_y * width + current_x;
            float current_cost = s_terrain_neighbor_path_cost[current_index];
            float best_difference = 10.0f;
            int best_x = 0;
            int best_y = 0;
            for (direction = 0; direction < 4; ++direction) {
                int previous_x = (int)current_x + dx[direction];
                int previous_y = (int)current_y + dy[direction];
                int previous_xy[2];
                uint32_t previous_index;
                float difference;
                if (previous_x < 0 || previous_y < 0 ||
                    (uint32_t)previous_x >= width ||
                    (uint32_t)previous_y >= height) continue;
                previous_index = (uint32_t)previous_y * width +
                                 (uint32_t)previous_x;
                previous_xy[0] = previous_x;
                previous_xy[1] = previous_y;
                difference = current_cost -
                    s_terrain_neighbor_path_cost[previous_index];
                if ((difference > 0.0f && difference < best_difference) ||
                    (difference == best_difference &&
                     (route_level != NULL
                        ? level_cell_occupied_at(route_level, previous_x,
                                                 previous_y)
                        : level_cell_occupied_at_current(previous_xy))
                        != 0)) {
                    best_difference = difference;
                    best_x = previous_x;
                    best_y = previous_y;
                    found_previous = 1;
                }
            }
            if (!found_previous) break;
            current_x = (uint32_t)best_x;
            current_y = (uint32_t)best_y;
            route_x[index] = best_x;
            route_y[index] = best_y;
            if (index == 1u) break;
            --index;
        }
    }

    if (route_length > 1u &&
        !((route_x[0] == route_x[1] &&
           abs(route_y[0] - route_y[1]) == 1) ||
          (route_y[0] == route_y[1] &&
           abs(route_x[0] - route_x[1]) == 1))) {
        uint32_t fallback_count = 0u;
        free(route_x);
        free(route_y);
        if (!terrain_grid_path_search(actor, start_x, start_y,
                result_goal_x, result_goal_y, out_x, out_y,
                &fallback_count, stop, user))
            return 0;
        *out_count = (float)fallback_count;
        return 1;
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = result_count;
    return 1;
}

/* Integer-count source adapter for the older portable callers. */
int terrain_neighbor_path_check(void* actor, uint32_t start_x,
                                uint32_t start_y, uint32_t goal_x,
                                uint32_t goal_y, int** out_x, int** out_y,
                                uint32_t* out_count, TerrainPathStopHook stop,
                                void* user)
{
    float count = 0.0f;
    int found = terrain_neighbor_path_check_current(
        actor, start_x, start_y, goal_x, goal_y, out_x, out_y, &count,
        stop, user);
    if (found && out_count != NULL) *out_count = (uint32_t)count;
    return found;
}

/* EntityPoolRelease (FUN_00437050) frees DAT_004dee00 as part of the global
 * terrain search teardown. Retail leaves the pointer and size stale after
 * freeing; clear both in the portable owner so a later path query can safely
 * allocate again and repeated teardown cannot double-free the buffer. */
void terrain_neighbor_path_cost_release(void)
{
    free(s_terrain_neighbor_path_cost);
    s_terrain_neighbor_path_cost = NULL;
    s_terrain_neighbor_path_cost_count = 0u;
}

/* The helper works on the portable visual-grid sidecar; the target entry
 * below keeps the retail level-record ABI. */
int terrain_tile_mesh_grid_load(TerrainTileMeshGrid* grid,
                                const char* bitmap_prefix)
{
    size_t count;
    uint32_t x, y;
    if (grid != NULL && grid->owner_level != NULL)
        return terrain_tile_mesh_grid_load_level(grid->owner_level,
                                                 bitmap_prefix);
    if (grid == NULL || grid->width == 0 || grid->height == 0
        || grid->width > 65535u || grid->height > 65535u
        || (size_t)grid->width > ((size_t)-1) / (size_t)grid->height
        || (size_t)grid->width * (size_t)grid->height
            > ((size_t)-1) / sizeof(void*)) return 0;
    grid->bitmap_prefix = bitmap_prefix;
    terrain_tile_mesh_grid_release(grid);
    count = (size_t)grid->width * (size_t)grid->height;
    grid->meshes = (SceneCellVisualRelayRecord**)calloc(
        count, sizeof(SceneCellVisualRelayRecord*));
    if (grid->meshes == NULL) return 0;
    if (grid->editor_mode) return 1;
    if (grid->bitmap_prefix == NULL) {
        terrain_tile_mesh_grid_release(grid);
        return 0;
    }
    /* Retail keeps the allocated row-major table and returns success even if
     * an individual 0x40b5f0 registration yields NULL. With no native scene
     * registrar installed, preserve that empty-slot state instead of dropping
     * the table and aborting the remaining texture-set load. */
    if (grid->register_mesh == NULL) return 1;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            char bitmap_a[256];
            char bitmap_b[256];
            size_t prefix_len = strlen(grid->bitmap_prefix);
            size_t index = (size_t)y * (size_t)grid->width + (size_t)x;
            if (prefix_len + 16u >= sizeof(bitmap_a)) {
                terrain_tile_mesh_grid_release(grid);
                return 0;
            }
            strcpy(bitmap_a, grid->bitmap_prefix);
            strcpy(bitmap_b, grid->bitmap_prefix);
            sprintf(bitmap_a + prefix_len, "%x%xa", (unsigned int)x,
                    (unsigned int)y);
            sprintf(bitmap_b + prefix_len, "%x%xb", (unsigned int)x,
                    (unsigned int)y);
            grid->meshes[index] = grid->register_mesh(
                grid->scene, bitmap_a, bitmap_b, grid->material,
                grid->options, grid->context);
            if (grid->meshes[index] != NULL) {
                SceneCellVisualRelayRecord* visual = grid->meshes[index];
                /* TileModelEntityRegister returns this 0x340 record. Its
                 * final field borrows the level's loaded texture-variant
                 * grid, and +0x328 is the level's projection scale. */
                visual->lookup_table =
                    (SceneCellVisualLookupTable*)grid->visual_lookup_table;
                visual->projection_scale = grid->cell_projection_scale;
            }
        }
    }
    return 1;
}

/* LoadTileMeshes (FUN_0042b330): the level owns a row-major pointer table of
 * TileModelEntityRegister records. The editor path leaves it zeroed; normal
 * play registers one paired visual record for every surface-text cell. */
// FUNCTION: LEGORR 0x0042b330
int terrain_tile_mesh_grid_load_level(LevelState* level,
                                      const char* bitmap_prefix)
{
    TerrainTileMeshGrid* grid;
    size_t count;
    uint32_t y;
    if (level == NULL) return 0;
    grid = level->ptr_tile_mesh_grid;
    if (grid == NULL) return 0;
    grid->owner_level = level;
    if (grid->width == 0u || grid->height == 0u ||
        grid->width > 65535u || grid->height > 65535u ||
        (size_t)grid->width > ((size_t)-1) / (size_t)grid->height ||
        (size_t)grid->width * (size_t)grid->height >
            ((size_t)-1) / sizeof(void*)) return 0;
    grid->bitmap_prefix = bitmap_prefix;
    terrain_tile_mesh_grid_release(grid);
    count = (size_t)grid->width * (size_t)grid->height;
    grid->meshes = (SceneCellVisualRelayRecord**)calloc(
        count, sizeof(SceneCellVisualRelayRecord*));
    if (grid->meshes == NULL) return 0;
    if (grid->editor_mode) return 1;
    if (grid->bitmap_prefix == NULL) {
        terrain_tile_mesh_grid_release(grid);
        return 0;
    }
    /* The retail loop indexes the pointer table by y * width + x. Its
     * registration failure is nonfatal: a failed scene resource leaves a
     * null slot while the rest of the cell grid is still initialized. */
    if (grid->register_mesh == NULL) return 1;
    for (y = 0u; y < grid->height; ++y) {
        uint32_t x;
        for (x = 0u; x < grid->width; ++x) {
            char bitmap_a[256];
            char bitmap_b[256];
            size_t prefix_len = strlen(grid->bitmap_prefix);
            size_t index = (size_t)y * (size_t)grid->width + x;
            if (prefix_len + 16u >= sizeof(bitmap_a)) {
                terrain_tile_mesh_grid_release(grid);
                return 0;
            }
            strcpy(bitmap_a, grid->bitmap_prefix);
            strcpy(bitmap_b, grid->bitmap_prefix);
            sprintf(bitmap_a + prefix_len, "%x%xa", (unsigned int)x,
                    (unsigned int)y);
            sprintf(bitmap_b + prefix_len, "%x%xb", (unsigned int)x,
                    (unsigned int)y);
            grid->meshes[index] = grid->register_mesh(
                grid->scene, bitmap_a, bitmap_b, grid->material,
                grid->options, grid->context);
            if (grid->meshes[index] != NULL) {
                SceneCellVisualRelayRecord* visual = grid->meshes[index];
                visual->lookup_table =
                    (SceneCellVisualLookupTable*)grid->visual_lookup_table;
                visual->projection_scale = grid->cell_projection_scale;
            }
        }
    }
    return 1;
}

struct SceneCellVisualRelayRecord* terrain_tile_mesh_visual_channels_get(
    TerrainTileMeshGrid* grid, uint8_t packed_surface)
{
    uint32_t row, column;
    size_t index;
    if (grid == NULL || grid->meshes == NULL || grid->width == 0u ||
        grid->height == 0u) return NULL;
    row = packed_surface & 0x0fu;
    column = packed_surface >> 4;
    if (row >= grid->height || column >= grid->width) return NULL;
    index = (size_t)row * grid->width + column;
    return grid->meshes[index];
}

static void terrain_cell_visual_record_dispose(
    SceneCellVisualRelayRecord* visual)
{
    uint32_t channel, index;
    if (visual == NULL) return;
    visual->channel_cursor = 0u;
    for (channel = 0u; channel < 2u; ++channel) {
        for (index = 0u; index < 100u; ++index) {
            if (visual->channel_resources[channel][index] != NULL) {
                ddiscene_object_destroy(
                    visual->channel_resources[channel][index]);
                visual->channel_resources[channel][index] = NULL;
            }
        }
    }
    for (channel = 0u; channel < 2u; ++channel) {
        if (visual->resource_roots[channel] != NULL) {
            ddiscene_object_destroy(visual->resource_roots[channel]);
            visual->resource_roots[channel] = NULL;
        }
    }
    engine_heap_free(visual);
}

void terrain_tile_mesh_grid_release(TerrainTileMeshGrid* grid)
{
    size_t i, count;
    if (grid == NULL || grid->meshes == NULL) return;
    count = (size_t)grid->width * (size_t)grid->height;
    if (grid->release_mesh != NULL) {
        for (i = 0; i < count; ++i) {
            if (grid->meshes[i] != NULL) {
                grid->release_mesh(grid->meshes[i], grid->context);
            }
        }
    }
    else {
        for (i = 0; i < count; ++i) {
            if (grid->meshes[i] != NULL) {
                terrain_cell_visual_record_dispose(grid->meshes[i]);
            }
        }
    }
    free(grid->meshes);
    grid->meshes = NULL;
}

/* TextureSetConfigLoad (FUN_0042bb60): surface-text dimensions are stored
 * separately from the map dimensions used by LoadTileMeshes. */
void terrain_texture_set_config_context_bind(BlockFile* config)
{
    s_texture_set_config_context = config;
}

int terrain_texture_set_config_load_current(TerrainTileMeshGrid* grid,
                                            const char* group_name)
{
    return terrain_texture_set_config_load(grid, group_name);
}

static int terrain_texture_set_config_load_grid(TerrainTileMeshGrid* grid,
                                                const char* group_name)
{
    BlockFile* config = s_texture_set_config_context;
    BlockRec* group;
    BlockRec* entry;
    const char* width_text = NULL;
    const char* height_text = NULL;
    const char* mesh_name = NULL;
    const char* texture_name = NULL;
    const char* roof_name = NULL;
    int width;
    int height;
    if (grid == NULL || config == NULL || group_name == NULL) return 0;
    group = block_subgroup(config, group_name);
    if (group == NULL) {
        char path[512];
        const char* root = util_root_name(config);
        if (root == NULL) root = "Lego";
        if (strlen(root) + strlen(group_name) + 2u < sizeof(path)) {
            strcpy(path, root);
            strcat(path, " ");
            strcat(path, group_name);
            group = block_subgroup(config, path);
        }
    }
    if (group == NULL) return 0;
    for (entry = block_first_child(group); entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        if (name == NULL || value == NULL) continue;
        if (terrain_config_key_equal(name, "SurfTextWidth"))
            width_text = value;
        else if (terrain_config_key_equal(name, "SurfTextHeight"))
            height_text = value;
        else if (terrain_config_key_equal(name, "MeshBaseName"))
            mesh_name = value;
        else if (terrain_config_key_equal(name, "TextureBaseName"))
            texture_name = value;
        else if (terrain_config_key_equal(name, "RoofTexture"))
            roof_name = value;
    }
    if (width_text == NULL || height_text == NULL) return 0;
    width = atoi(width_text);
    height = atoi(height_text);
    if (width <= 0 || height <= 0) return 0;

    /* Level offsets +0x74/+0x78 are the packed cell-visual table dimensions;
     * LoadTileMeshes and CellSurfaceUpdateDispatch share this exact stride. */
    /* Replacing a texture-set group releases records before their borrowed
     * lookup table and texture handles, then detaches the old window view. */
    terrain_texture_set_release(grid);
    grid->surface_text_width = (uint32_t)width;
    grid->surface_text_height = (uint32_t)height;
    grid->width = (uint32_t)width;
    grid->height = (uint32_t)height;
    if (grid->owner_level != NULL) {
        /* TextureSetConfigLoad writes these dimensions into the level block
         * at +0x74/+0x78; the port grid keeps the same values in its sidecar
         * for the visual-resource helpers. */
        grid->owner_level->word[0x74u / 4u] = (uint32_t)width;
        grid->owner_level->word[0x78u / 4u] = (uint32_t)height;
    }
    grid->mesh_basename = mesh_name;
    grid->texture_basename = texture_name;

    if (grid->owner_level != NULL && grid->owner_level->ptr_terrain != NULL) {
        const void* variant_window = terrain_cell_variant_window_get(
            grid->owner_level->ptr_terrain);
        grid->surface_text_map = (void*)variant_window;
        grid->surface_text_map_insert = terrain_cell_variant_table_insert;
        if (grid->surface_grid != NULL)
            terrain_surface_grid_terrain_window_set(
                grid->surface_grid, variant_window);
    }

    if (grid->surface_text_map != NULL &&
        grid->surface_text_map_insert != NULL) {
        surface_text_character_map_initialize(
            grid->surface_text_map, grid->surface_text_map_insert);
    }
    terrain_grid_cells_initialize(grid->surface_grid);

    if (mesh_name == NULL || texture_name == NULL) return 0;
    terrain_tile_mesh_visual_lookup_release(grid);
    if (grid->textures != NULL) terrain_cell_resource_grid_dispose(grid->textures);
    grid->textures = terrain_cell_resource_grid_load(texture_name,
        (uint32_t)width, (uint32_t)height);
    if (grid->textures == NULL) return 0;
    if (grid->owner_level != NULL) {
        /* TextureSetConfigLoad stores this descriptor at level +0x7c; keep
         * that corpus slot and the native-width owner in sync. */
        level_store_ptr(grid->owner_level, LEVEL_OFF_CELL_RESOURCES,
                        grid->textures);
    }
    {
        SceneCellVisualLookupTable* lookup;
        size_t count = (size_t)width * (size_t)height;
        size_t i;
        if (count > ((size_t)-1) / sizeof(uint32_t)) return 0;
        grid->visual_lookup_entries = (uint32_t*)malloc(
            count * sizeof(uint32_t));
        lookup = (SceneCellVisualLookupTable*)malloc(sizeof(*lookup));
        if (grid->visual_lookup_entries == NULL || lookup == NULL) {
            free(lookup);
            terrain_tile_mesh_visual_lookup_release(grid);
            return 0;
        }
        for (i = 0; i < count; ++i) {
            grid->visual_lookup_entries[i] = (uint32_t)(unsigned long)
                grid->textures->handles[i];
        }
        lookup->stride = (uint32_t)width;
        lookup->reserved = (uint32_t)height;
        lookup->entries = grid->visual_lookup_entries;
        grid->visual_lookup_table = lookup;
    }
    if (grid->surface_texture_grid_attach != NULL) {
        grid->surface_texture_grid_attach(grid->scene, grid->textures,
                                          grid->context);
    }
    /* MapLoadPipeline attaches the cell texture grid before it registers
     * the paired A/B tile meshes through LoadTileMeshes. */
    if (grid->owner_level != NULL) {
        if (!terrain_tile_mesh_grid_load_level(grid->owner_level, mesh_name))
            return 0;
    }
    else if (!terrain_tile_mesh_grid_load(grid, mesh_name))
        return 0;
    if (roof_name == NULL) return 0;
    /* FUN_00462e60 replaces the globally-owned animated indexed texture and
     * propagates it through that resource owner's slots. It is not a
     * per-level TextureSurface handle. */
    return animated_texture_replace(roof_name);
}

int terrain_texture_set_config_load(TerrainTileMeshGrid* grid,
                                    const char* group_name)
{
    return terrain_texture_set_config_load_grid(grid, group_name);
}

/* Retail TextureSetConfigLoad receives the level record and resolves its
 * +0x74/+0x78 grid and +0x04 terrain owner before loading resources. */
// FUNCTION: LEGORR 0x0042BB60
int terrain_texture_set_config_load_level(LevelState* level,
                                          const char* group_name)
{
    if (level == NULL || level->ptr_tile_mesh_grid == NULL) return 0;
    level->ptr_tile_mesh_grid->owner_level = level;
    return terrain_texture_set_config_load_grid(
        level->ptr_tile_mesh_grid, group_name);
}

void terrain_texture_set_release(TerrainTileMeshGrid* grid)
{
    const void* borrowed_window;
    if (grid == NULL) return;
    borrowed_window = grid->surface_text_map;
    if (borrowed_window == NULL && grid->owner_level != NULL &&
        grid->owner_level->ptr_terrain != NULL) {
        borrowed_window = terrain_cell_variant_window_get(
            grid->owner_level->ptr_terrain);
    }
    if (grid->surface_grid != NULL &&
        (grid->surface_grid->terrain_window == borrowed_window ||
         grid->owner_level == NULL)) {
        terrain_surface_grid_terrain_window_set(grid->surface_grid, NULL);
    }
    grid->surface_text_map = NULL;
    grid->surface_text_map_insert = NULL;
    /* Visual records borrow the texture-variant lookup grid at their final
     * field, so release them before disposing that table. */
    terrain_tile_mesh_grid_release(grid);
    if (grid->textures != NULL) {
        if (grid->owner_level != NULL &&
            grid->owner_level->ptr_cell_resources == grid->textures) {
            level_store_ptr(grid->owner_level, LEVEL_OFF_CELL_RESOURCES,
                            NULL);
        }
        terrain_cell_resource_grid_dispose(grid->textures);
        grid->textures = NULL;
    }
    terrain_tile_mesh_visual_lookup_release(grid);
}

/* LoadTileVariants stores TextureSurfaceCreate records. Their reference
 * release is the paired TextureRelease operation, not TextureSurface's CPU
 * pixel-cache release. */
void terrain_grid_entry_resources_release_all(TerrainCellResourceGrid* grid)
{
    size_t i;
    size_t count;
    if (grid == NULL || grid->handles == NULL) return;
    if (grid->height != 0u && (size_t)grid->width >
        ((size_t)-1) / (size_t)grid->height) return;
    count = (size_t)grid->width * (size_t)grid->height;
    for (i = 0; i < count; ++i) {
        if (grid->handles[i] != NULL) {
            texture_release_record((TextureReleaseRecord*)grid->handles[i]);
            grid->handles[i] = NULL;
        }
    }
}

/* TerrainGridEntryResourcesReleaseAll (FUN_00434530): the retail level's
 * +0x80 pointer array contains visual-channel records. Clear their two
 * 100-entry resource channels and reset +0x32c; retain the records and roots. */
// FUNCTION: LEGORR 0x00434530
void terrain_grid_entry_resources_release_all_current(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    TerrainTileMeshGrid* grid;
    size_t count;
    size_t i;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return;
    grid = level->ptr_tile_mesh_grid;
    if (grid == NULL || grid->meshes == NULL) return;
    if (grid->height != 0u && (size_t)grid->width >
        ((size_t)-1) / (size_t)grid->height) return;
    count = (size_t)grid->width * (size_t)grid->height;
    for (i = 0u; i < count; ++i) {
        SceneCellVisualRelayRecord* visual = grid->meshes[i];
        uint32_t channel;
        uint32_t item;
        if (visual == NULL) continue;
        visual->channel_cursor = 0u;
        for (channel = 0u; channel < 2u; ++channel) {
            for (item = 0u; item < 100u; ++item) {
                void* resource = visual->channel_resources[channel][item];
                if (resource != NULL) ddiscene_object_destroy(resource);
                visual->channel_resources[channel][item] = NULL;
            }
        }
    }
}

/* LoadTileVariants (FUN_0040b310): allocate a 12-byte width/height/handle
 * owner, then call TextureSurfaceCreate(mode 1, null output, mode data 0)
 * with `%s%0.1x%0.1x.bmp` for every row-major cell. The result is the
 * two-interface TextureRelease record (or its portable host-record form). */
// FUNCTION: LEGORR 0x0040B310
TerrainCellResourceGrid* terrain_cell_resource_grid_load(
    const char* bitmap_prefix, uint32_t width, uint32_t height)
{
    TerrainCellResourceGrid* grid;
    size_t count;
    uint32_t y;
    if (bitmap_prefix == NULL ||
        (width != 0u && (size_t)height > ((size_t)-1) / (size_t)width))
        return NULL;
    grid = (TerrainCellResourceGrid*)engine_heap_alloc(
        (long)sizeof(TerrainCellResourceGrid));
    if (grid == NULL) return NULL;
    memset(grid, 0, sizeof(*grid));
    count = (size_t)width * (size_t)height;
    if (count > ((size_t)-1) / sizeof(void*)) {
        engine_heap_free(grid);
        return NULL;
    }
    if (count * sizeof(void*) > (size_t)0x7fffffffu) {
        engine_heap_free(grid);
        return NULL;
    }
    grid->handles = (void**)engine_heap_alloc(
        (long)(count * sizeof(void*)));
    if (grid->handles == NULL) {
        engine_heap_free(grid);
        return NULL;
    }
    memset(grid->handles, 0, count * sizeof(void*));
    grid->width = width;
    grid->height = height;
    for (y = 0u; y < height; ++y) {
        uint32_t x;
        for (x = 0u; x < width; ++x) {
            char path[1024];
            size_t prefix_length = strlen(bitmap_prefix);
            int length;
            if (prefix_length + 21u > sizeof(path)) continue;
            length = sprintf(path + prefix_length, "%x%x.bmp",
                             (unsigned int)x, (unsigned int)y);
            if (length < 0) continue;
            memcpy(path, bitmap_prefix, prefix_length);
            grid->handles[(size_t)y * width + x] =
                texture_surface_create(path, 1u, NULL, 0u);
        }
    }
    return grid;
}

/* TerrainCellResourceGridDispose (FUN_0040b410): release each cell's
 * resource handle, then release the handle table and grid owner. */
// FUNCTION: LEGORR 0x0040B410
void terrain_cell_resource_grid_dispose(TerrainCellResourceGrid* grid)
{
    size_t i, count;
    if (grid == NULL) return;
    if (grid->height != 0u
        && (size_t)grid->width > ((size_t)-1) / (size_t)grid->height) {
        engine_heap_free(grid->handles);
        engine_heap_free(grid);
        return;
    }
    count = (size_t)grid->width * (size_t)grid->height;
    if (grid->handles != NULL) {
        for (i = 0; i < count; ++i) {
            if (grid->handles[i] != NULL) {
                texture_release_record(
                    (TextureReleaseRecord*)grid->handles[i]);
            }
        }
    }
    engine_heap_free(grid->handles);
    engine_heap_free(grid);
}

static GridCellObjectNode* grid_cell_object_node_acquire(void)
{
    if (s_grid_cell_object_free == NULL) {
        terrain_record_pool_chunk_allocate();
        if (s_grid_cell_object_free == NULL) {
            return NULL;
        }
    }
    {
        GridCellObjectNode* node = s_grid_cell_object_free;
        s_grid_cell_object_free = node->next;
        memset(node, 0, sizeof(*node));
        return node;
    }
}

/* The retail cell table allocates five dwords per cell but indexes each
 * record with a three-dword (0x0c) stride. Keep that head word synchronized
 * with the native-pointer sidecar used by object_heads. */
static void grid_cell_object_head_image_set(uint32_t index,
                                            const GridCellObjectNode* node)
{
    uint32_t* records = (uint32_t*)s_map_cell_record_grid;
    if (records != NULL) {
        uint32_t image = (uint32_t)(uintptr_t)node;
        if (node != NULL && image == 0u) image = 1u;
        records[(size_t)index * 3u] = image;
        records[(size_t)index * 3u + 2u] = 0u;
    }
}

/* GridCellObjectLinkInsert (FUN_0040cf80): acquire a five-word node, mark
 * its link as the retail self-linked sentinel, and install it in the cell. */
// FUNCTION: LEGORR 0x0040CF80
void* grid_cell_object_link_insert(void* object, int x, int y)
{
    uint32_t width;
    uint32_t height;
    uint32_t index;
    GridCellObjectNode* node;
    if (object == NULL || s_map_cell_record_level == NULL
        || s_grid_cell_object_heads == NULL) {
        return NULL;
    }
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height) {
        return NULL;
    }
    index = (uint32_t)y * width + (uint32_t)x;
    node = grid_cell_object_node_acquire();
    if (node == NULL) {
        return NULL;
    }
    node->object = object;
    node->x = x;
    node->y = y;
    node->next = node;
    s_grid_cell_object_heads[index] = node;
    grid_cell_object_head_image_set(index, node);
    s_active_cell_kind_grid->auxiliary[index] = NULL;
    if (s_grid_object_flag_hook != NULL) {
        s_grid_object_flag_hook(object, s_grid_cell_object_context);
    }
    else {
        /* FUN_0040cf80 sets entity +0x3e4 bit 0x02000000 directly. This
         * state mutation does not depend on a renderer or entity adapter. */
        uint32_t* flags = (uint32_t*)((unsigned char*)object + 0x3e4);
        *flags |= 0x02000000u;
    }
    terrain_cell_and_neighbors_update(x, y);
    return node;
}

/* RegisterObjectAtGridPosition (FUN_0040cf50): query an object's grid cell,
 * then insert it in that cell's linked record chain. */
// FUNCTION: LEGORR 0x0040CF50
void register_object_at_grid_position(void* object)
{
    int x;
    int y;
    if (!grid_cell_object_coordinates_get(object, &x, &y)) {
        return;
    }
    grid_cell_object_link_insert(object, x, y);
}

/* TerrainCellEntityRecordUnlink (FUN_0040d100): return the cell's linked
 * records to the pool, clear its head, and refresh the changed cell ring. */
// FUNCTION: LEGORR 0x0040D100
void terrain_cell_entity_record_unlink(void* record, int x, int y)
{
    uint32_t width;
    uint32_t height;
    uint32_t index;
    GridCellObjectNode* node;
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL) {
        return;
    }
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height) {
        return;
    }
    index = (uint32_t)y * width + (uint32_t)x;
    node = (GridCellObjectNode*)record;
    if (node == NULL) {
        return;
    }
    node->next = s_grid_cell_object_free;
    s_grid_cell_object_free = node;
    s_grid_cell_object_heads[index] = NULL;
    grid_cell_object_head_image_set(index, NULL);
    s_active_cell_kind_grid->auxiliary[index] = NULL;
    terrain_cell_and_neighbors_update(x, y);
}

/* TerrainEntityCellRecordRemove (FUN_0040d0a0): locate the object's cell and
 * clear its cell record chain when the object is registered there. */
// FUNCTION: LEGORR 0x0040D0A0
void terrain_entity_cell_record_remove(void* object)
{
    int x;
    int y;
    uint32_t width;
    uint32_t height;
    uint32_t index;
    GridCellObjectNode* node;
    if (object == NULL
        || (((*(const uint32_t*)((const unsigned char*)object + 0x3e4))
             & 0x02000000u) == 0u)
        || s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL) {
        return;
    }
    if (!grid_cell_object_coordinates_get(object, &x, &y)) return;
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height) {
        return;
    }
    index = (uint32_t)y * width + (uint32_t)x;
    node = (GridCellObjectNode*)s_grid_cell_object_heads[index];
    if (node != NULL) {
        terrain_cell_entity_record_unlink(node, x, y);
    }
}

void terrain_cell_object_hooks_install(TerrainCellObjectAvailableHook available,
                                      TerrainCellObjectCreateHook create,
                                      TerrainCellObjectDeleteHook destroy,
                                      void* context)
{
    s_cell_object_available_hook = available;
    s_cell_object_create_hook = create;
    s_cell_object_delete_hook = destroy;
    s_cell_object_context = context;
}

void terrain_cell_scene_object_hooks_install(
    TerrainCellSceneObjectCreateHook create,
    TerrainCellSceneObjectReleaseHook release, void* context)
{
    /* The portable entity pool is the native kind-14 owner. A platform may
     * replace it with a paired DDi scene factory/teardown implementation. */
    if (create != NULL && release != NULL) {
        s_cell_scene_object_create_hook = create;
        s_cell_scene_object_release_hook = release;
        s_cell_scene_object_context = context;
    }
    else {
        s_cell_scene_object_create_hook = terrain_cell_kind14_entity_create;
        s_cell_scene_object_release_hook = terrain_cell_kind14_entity_release;
        s_cell_scene_object_context = NULL;
    }
}

typedef struct CellSceneObjectContext {
    struct LevelState* level;
    LevelCellKindGrid* grid;
    uint32_t x;
    uint32_t y;
    size_t index;
} CellSceneObjectContext;

static void* terrain_cell_kind14_entity_create(struct LevelState* level,
    uint32_t x, uint32_t y, float world_x, float world_y, void* context)
{
    GameContext* game = entity_pool_callback_context_get();
    (void)x;
    (void)y;
    (void)context;
    if (game == NULL || level == NULL || game->level != level) return NULL;
    return entity_create_with_position(game, 14, NULL, 0,
                                       world_x, world_y, 0.0f);
}

static void terrain_cell_kind14_entity_release(struct LevelState* level,
    void* object, void* context)
{
    EntitySlot* entity = (EntitySlot*)object;
    (void)level;
    (void)context;
    if (entity != NULL) (void)entity_teardown(entity);
}

static int cell_scene_object_grid_to_world(void* context, uint32_t x,
                                           uint32_t y, float* world_x,
                                           float* world_y)
{
    CellSceneObjectContext* state = (CellSceneObjectContext*)context;
    float position[2];
    if (state == NULL || state->level == NULL || world_x == NULL ||
        world_y == NULL || state->level->ptr_terrain == NULL ||
        !terrain_cell_world_center_xy_get(state->level->ptr_terrain,
                                          (int)x, (int)y, position))
        return 0;
    *world_x = position[0];
    *world_y = position[1];
    return 1;
}

static void* cell_scene_object_kind14_create(void* context,
                                            float world_x, float world_y)
{
    CellSceneObjectContext* state = (CellSceneObjectContext*)context;
    if (state == NULL || s_cell_scene_object_create_hook == NULL ||
        s_cell_scene_object_release_hook == NULL) return NULL;
    return s_cell_scene_object_create_hook(state->level, state->x, state->y,
        world_x, world_y, s_cell_scene_object_context);
}

static void cell_scene_object_release(void* context, void* object)
{
    CellSceneObjectContext* state = (CellSceneObjectContext*)context;
    if (state != NULL) {
        TerrainCellSceneObjectReleaseHook release =
            state->grid->scene_releases[state->index] != NULL
            ? state->grid->scene_releases[state->index]
            : s_cell_scene_object_release_hook;
        void* release_context = state->grid->scene_releases[state->index]
            != NULL ? state->grid->scene_release_contexts[state->index]
            : s_cell_scene_object_context;
        if (release != NULL)
            release(state->level, object, release_context);
        state->grid->scene_releases[state->index] = NULL;
        state->grid->scene_release_contexts[state->index] = NULL;
    }
}

void terrain_cell_scene_object_update(struct LevelState* level,
                                      uint32_t x, uint32_t y,
                                      int should_exist)
{
    LevelCellKindGrid* grid;
    CellSceneObjectContext context;
    CellEntityKindRegistry registry;
    if (level == NULL || level->ptr_cell_kind_grid == NULL) return;
    grid = (LevelCellKindGrid*)level->ptr_cell_kind_grid;
    if (x >= grid->width || y >= grid->height) return;
    context.level = level;
    context.grid = grid;
    context.x = x;
    context.y = y;
    context.index = (size_t)y * grid->width + x;
    /* Retail clears the auxiliary word only when a live cell-kind object is
     * removed. It remains untouched when there was no object to remove. */
    if (!should_exist) {
        size_t index = (size_t)y * grid->width + x;
        if (grid->scene_objects[index] != NULL &&
            grid->scene_releases[index] == NULL &&
            s_cell_scene_object_release_hook == NULL)
            return;
        if (grid->scene_objects[index] != NULL)
            grid->auxiliary[index] = NULL;
    }
    registry.width = grid->width;
    registry.height = grid->height;
    registry.records = grid->scene_objects;
    registry.context = &context;
    registry.grid_to_world = cell_scene_object_grid_to_world;
    registry.create_kind14 = cell_scene_object_kind14_create;
    registry.release_entity = cell_scene_object_release;
    cell_entity_kind_create_or_remove(&registry, x, y, should_exist);
    if (should_exist && grid->scene_objects[context.index] != NULL &&
        grid->scene_releases[context.index] == NULL) {
        grid->scene_releases[context.index] =
            s_cell_scene_object_release_hook;
        grid->scene_release_contexts[context.index] =
            s_cell_scene_object_context;
    }
}

/* CreateCellObjectIfAvailable (FUN_0040d170): apply the decompiled cell
 * predicate, place a type-0xb entity at the cell center, and register it.
 * Explicit callbacks remain overrides for renderer-owned hosts. */
// FUNCTION: LEGORR 0x0040D170
int create_cell_object_if_available(int x, int y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level :
        (LevelState*)map_cell_record_grid_level_get();
    float position[3];
    void* object;
    int created_here = 0;
    if (s_cell_object_available_hook != NULL) {
        if (s_cell_object_available_hook(x, y, s_cell_object_context) == 0)
            return 0;
    }
    else if (level == NULL ||
        !terrain_cell_has_occupied_or_flagged_neighbor(
            level, x, y, terrain_cell_object_record_present_for_level)) {
        return 0;
    }
    if (s_cell_object_create_hook != NULL) {
        object = s_cell_object_create_hook(x, y, s_cell_object_context);
    }
    else {
        if (game == NULL || level == NULL) return 0;
        map_cell_record_grid_activate(level);
        if (!terrain_cell_world_position_get(x, y, position)) return 0;
        object = entity_create_with_position(game, 0xb, NULL, 0,
                                             position[0], position[1], 0.0f);
        created_here = 1;
    }
    if (object == NULL) return 0;
    if (grid_cell_object_link_insert(object, x, y) == NULL) {
        if (created_here) entity_teardown((EntitySlot*)object);
        return 0;
    }
    return 1;
}

/* DeleteCellObject (FUN_0040d1f0): remove and destroy the cell's head object
 * when present. */
// FUNCTION: LEGORR 0x0040D1F0
int delete_cell_object(int x, int y)
{
    uint32_t width;
    uint32_t height;
    uint32_t index;
    GridCellObjectNode* node;
    void* object;
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL) {
        return 0;
    }
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height) {
        return 0;
    }
    index = (uint32_t)y * width + (uint32_t)x;
    node = (GridCellObjectNode*)s_grid_cell_object_heads[index];
    if (node == NULL) {
        return 0;
    }
    /* The remove path returns the record to a pool before refreshing the
     * neighboring cells; retain the entity pointer across that mutation,
     * matching FUN_0040d1f0's local copy. */
    object = node->object;
    terrain_entity_cell_record_remove(object);
    if (s_cell_object_delete_hook != NULL) {
        s_cell_object_delete_hook(object, s_cell_object_context);
    }
    else {
        entity_teardown((EntitySlot*)object);
    }
    return 1;
}

void terrain_activation_hooks_install(TerrainActivationPrepareHook prepare,
                                     TerrainActivationResolveHook resolve,
                                     TerrainActivationSpecialCellHook special,
                                     TerrainActivationFloodHook flood,
                                     void* context)
{
    s_terrain_activation_prepare_hook = prepare;
    s_terrain_activation_resolve_hook = resolve;
    s_terrain_activation_special_hook = special;
    s_terrain_activation_flood_hook = flood;
    s_terrain_activation_context = context;
}

int terrain_cell_object_record_present(int x, int y)
{
    uint32_t width;
    uint32_t height;
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL
        || x < 0 || y < 0) {
        return 0;
    }
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if ((uint32_t)x >= width || (uint32_t)y >= height) {
        return 0;
    }
    return s_grid_cell_object_heads[(uint32_t)y * width + (uint32_t)x] != NULL;
}

static int terrain_cell_object_flood_gate_get(int x, int y, float* gate)
{
    uint32_t width;
    uint32_t height;
    GridCellObjectNode* node;
    if (gate != NULL) *gate = 0.0f;
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL ||
        x < 0 || y < 0) return 0;
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if ((uint32_t)x >= width || (uint32_t)y >= height) return 0;
    node = (GridCellObjectNode*)s_grid_cell_object_heads[
        (uint32_t)y * width + (uint32_t)x];
    if (node == NULL) return 0;
    if (gate != NULL) *gate = node->flood_gate;
    return 1;
}

static int terrain_cell_object_flood_gate_set(int x, int y, float gate)
{
    uint32_t width;
    uint32_t height;
    GridCellObjectNode* node;
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL ||
        x < 0 || y < 0) return 0;
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if ((uint32_t)x >= width || (uint32_t)y >= height) return 0;
    node = (GridCellObjectNode*)s_grid_cell_object_heads[
        (uint32_t)y * width + (uint32_t)x];
    if (node == NULL) return 0;
    node->flood_gate = gate;
    return 1;
}

int terrain_cell_object_record_present_for_level(const LevelState* level,
                                                 int x, int y)
{
    const LevelCellKindGrid* grid;
    size_t index;
    if (level == NULL || level->ptr_cell_kind_grid == NULL || x < 0 || y < 0)
        return 0;
    grid = (const LevelCellKindGrid*)level->ptr_cell_kind_grid;
    if (grid->owner != level || (uint32_t)x >= grid->width ||
        (uint32_t)y >= grid->height || grid->object_heads == NULL) return 0;
    index = (size_t)(uint32_t)y * grid->width + (uint32_t)x;
    return grid->object_heads[index] != NULL;
}

/* TerrainCellActivationTry (FUN_0040d4f0): prepare activation state, validate
 * the target cell or entity flags, then dispatch a one-step flood walk. */
static LevelState* terrain_flood_current_level_get(void);
// FUNCTION: LEGORR 0x0040D4F0
int terrain_cell_activation_try(void* object, int x, int y)
{
    LevelState* level = terrain_flood_current_level_get();
    if (s_terrain_activation_prepare_hook != NULL) {
        s_terrain_activation_prepare_hook(s_terrain_activation_context);
    }
    if (object == NULL) {
        if (!terrain_cell_object_record_present(x, y)
            && (s_terrain_activation_special_hook != NULL
                ? s_terrain_activation_special_hook(x, y,
                    s_terrain_activation_context)
                : terrain_flood_special_query(x, y, 0)) == 0) {
            return 0;
        }
    } else {
        uint32_t entity_flags = 0u;
        uint32_t state_flags = 0u;
        int resolved;
        if (s_terrain_activation_resolve_hook != NULL) {
            resolved = s_terrain_activation_resolve_hook(object, &x, &y,
                &entity_flags, &state_flags, s_terrain_activation_context);
        }
        else if (level != NULL && level->ptr_terrain != NULL) {
            EntitySlot* entity = (EntitySlot*)object;
            float position[2];
            entity_transform_position_get(entity, position);
            resolved = terrain_world_xy_to_cell(level->ptr_terrain,
                position[0], position[1], &x, &y);
            entity_flags = entity_state_flags_get(entity);
            state_flags = (uint32_t)entity->flags_3e8;
        }
        else {
            resolved = 0;
        }
        if (resolved == 0 || ((entity_flags & 0x400u) == 0u &&
                              (state_flags & 0x20000000u) == 0u)) {
            return 0;
        }
    }
    if (s_terrain_activation_flood_hook != NULL) {
        s_terrain_activation_flood_hook(x, y, 1, 0.0f,
                                        s_terrain_activation_context);
    }
    else {
        /* The flood walker is the direct implementation of the callee used
         * here; keep activation's cell-state propagation live without an
         * external dispatcher binding. */
        terrain_cell_neighbor_flood_walk(x, y, 1, 0.0f);
    }
    return 1;
}

void terrain_neighbor_flood_hooks_install(TerrainFloodStateHook state,
                                          TerrainFloodEffectHook effect,
                                          void* context)
{
    s_terrain_flood_state_hook = state;
    s_terrain_flood_effect_hook = effect;
    s_terrain_flood_context = context;
}

static LevelState* terrain_flood_current_level_get(void)
{
    LevelState* level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL) level = game->level;
        if (level != NULL) map_cell_record_grid_activate(level);
    }
    return level;
}

static uint32_t* terrain_cell_state_flags_at(int x, int y)
{
    uint32_t width;
    uint32_t height;
    uint32_t index;
    (void)terrain_flood_current_level_get();
    if (s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL
        || x < 0 || y < 0) {
        return NULL;
    }
    width = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                               + 0x14);
    height = *(const uint32_t*)((const unsigned char*)s_map_cell_record_level
                                + 0x18);
    if ((uint32_t)x >= width || (uint32_t)y >= height) {
        return NULL;
    }
    index = (uint32_t)y * width + (uint32_t)x;
    return &s_active_cell_kind_grid->auxiliary[index];
}

static int terrain_flood_special_state(int x, int y, int mode)
{
    const LevelState* level;
    level = terrain_flood_current_level_get();
    if (level == NULL) return 0;
    /* FUN_0040d3f0 tests mode == 0; every nonzero mode asks for the
     * secondary 0x100 mark after the shared occupancy-or-flag gate. */
    if (level_cell_occupied_or_flag(level, x, y, 1) == 0u) return 0;
    return mode == 0 || level_cell_secondary_flag_100(level, x, y) != 0u;
}

static int terrain_flood_coordinate_in_bounds(int x, int y)
{
    const LevelState* level = terrain_flood_current_level_get();
    if (level == NULL || x < 0 || y < 0) return 0;
    return (uint32_t)x < (uint32_t)level_int(level, LEVEL_OFF_WIDTH) &&
           (uint32_t)y < (uint32_t)level_int(level, LEVEL_OFF_HEIGHT);
}

static int terrain_flood_special_query(int x, int y, int mode)
{
    if (s_terrain_flood_state_hook != NULL) {
        return s_terrain_flood_state_hook(x, y, mode,
                                         s_terrain_flood_context);
    }
    return terrain_flood_special_state(x, y, mode);
}

/* TerrainCellNeighborFloodWalk (FUN_0040d850): mark visited cells, inspect
 * the corpus's cardinal/extended candidate offsets, draw the exact shared
 * random gate, emit edge effects, and recurse into eligible cells. The edge
 * effect itself remains renderer-owned. */
// FUNCTION: LEGORR 0x0040D850
void terrain_cell_neighbor_flood_walk(int x, int y, int force, float strength)
{
    static const int adjacent[4][2] = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 }
    };
    static const int extended[4][2] = {
        { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 }
    };
    uint32_t* flags = terrain_cell_state_flags_at(x, y);
    int source_special;
    int direction;
    if (flags == NULL || (*flags & 0x100u) != 0u) return;
    *flags |= 0x100u;
    source_special = terrain_flood_special_query(x, y, 0);
    for (direction = 0; direction < 4; ++direction) {
        int nx = x + adjacent[direction][0];
        int ny = y + adjacent[direction][1];
        int intermediate = 0;
        int candidate = 0;
        int fallback_x = x + extended[direction][0];
        int fallback_y = y + extended[direction][1];
        uint32_t* next_flags;
        int candidate_has_record;
        float candidate_flood_gate = 0.0f;
        if (terrain_flood_coordinate_in_bounds(nx, ny) &&
            (terrain_cell_object_record_present(nx, ny) ||
             (terrain_flood_special_query(nx, ny, 1) && !source_special))) {
            candidate = 1;
        }
        else if (terrain_flood_coordinate_in_bounds(fallback_x, fallback_y) &&
                 (!terrain_flood_special_query(nx, ny, 0) ||
                  !source_special) &&
                 (terrain_cell_object_record_present(fallback_x, fallback_y) ||
                  (terrain_flood_special_query(fallback_x, fallback_y, 1) &&
                   !source_special))) {
            candidate = 1;
            intermediate = 1;
            nx = fallback_x;
            ny = fallback_y;
        }
        if (!candidate) continue;
        next_flags = terrain_cell_state_flags_at(nx, ny);
        if (next_flags == NULL) continue;
        if (intermediate) {
            uint32_t* intermediate_flags = terrain_cell_state_flags_at(
                x + adjacent[direction][0], y + adjacent[direction][1]);
            if (intermediate_flags != NULL) *intermediate_flags |= 0x100u;
        }
        candidate_has_record = terrain_cell_object_flood_gate_get(
            nx, ny, &candidate_flood_gate);
        /* Retail continues through an absent record or a record whose
         * +0x0c flood gate is nonpositive. */
        if (!candidate_has_record || candidate_flood_gate <= 0.0f) {
            const uint32_t direction_flag = 1u << direction;
            if ((*next_flags & direction_flag) == 0u) {
                /* The source-cell back-edge is committed before the random
                 * effect gate in the retail walk. Keep it visible to the
                 * edge-effect callback even when the strength rounds to a
                 * zero divisor and no effect is emitted. */
                *flags |= 1u << ((direction - 2) & 3);
                int emit_effect = force != 0;
                if (!emit_effect && strength != 0.0f) {
                    const float capped = strength > 200.0f ? 200.0f : strength;
                    const int random_value = level_random_draw15();
                    const int divisor = crt_float_to_long(capped);
                    emit_effect = divisor != 0 &&
                        random_value % divisor == 0;
                }
                if (emit_effect) {
                    if (s_terrain_flood_effect_hook != NULL) {
                        s_terrain_flood_effect_hook(x, y, nx, ny,
                                                    s_terrain_flood_context);
                    }
                    else {
                        /* FUN_0040d850 calls FUN_0040dc20 directly. This
                         * helper reaches the native two-group resource-slot
                         * owner; actual scene mutation remains its typed
                         * renderer callback. */
                        (void)cell_pair_edge_effect_apply(x, y, nx, ny);
                    }
                }
            }
            if (!terrain_flood_special_query(nx, ny, 0) &&
                (*next_flags & 0x100u) == 0u) {
                terrain_cell_neighbor_flood_walk(nx, ny, force, strength);
            }
        }
    }
}

void terrain_cell_world_position_hook_install(
    TerrainCellWorldPositionHook hook, void* context)
{
    s_cell_world_position_hook = hook;
    s_cell_world_position_context = context;
}

int terrain_cell_world_position_get(int x, int y, float position[3])
{
    LevelState* level = (LevelState*)s_map_cell_record_level;
    GameContext* game;
    if (position == NULL) return 0;
    if (s_cell_world_position_hook != NULL)
        return s_cell_world_position_hook(s_map_cell_record_level, x, y,
            position, s_cell_world_position_context) != 0;
    if (level == NULL) {
        game = entity_pool_callback_context_get();
        if (game != NULL) level = game->level;
    }
    /* MapCellToWorldCenter supplies exact cell-center X/Z. FUN_0040dd90
     * obtains Y through TerrainSurfaceHeightInterpolate (0x44f940), so keep
     * this query on the terrain's live quad data instead of the renderer's
     * separate draw cache or the integer SurfaceMap sample. */
    if (level == NULL || level->ptr_terrain == NULL ||
        !terrain_cell_world_center_xy_get(level->ptr_terrain, x, y,
                                          position)) return 0;
    position[2] = terrain_surface_height_interpolate(level->ptr_terrain,
                                                       position[0],
                                                       position[1]);
    return 1;
}

/* ComputeWorldPositionDelta (FUN_0040dd90): query the two cell centers and
 * subtract the first world position from the second. */
// FUNCTION: LEGORR 0x0040DD90
void compute_world_position_delta(int x1, int y1, int x2, int y2,
                                  float delta[3])
{
    float first[3];
    float second[3];
    int index;
    if (delta == NULL) {
        return;
    }
    if (terrain_cell_world_position_get(x1, y1, first) == 0 ||
        terrain_cell_world_position_get(x2, y2, second) == 0) {
        delta[0] = 0.0f;
        delta[1] = 0.0f;
        delta[2] = 0.0f;
        return;
    }
    for (index = 0; index < 3; ++index) {
        delta[index] = second[index] - first[index];
    }
}

/* CellPairEdgeEffectApply (FUN_0040dc20): accept the four corpus-adjacent
 * offset pairs, compute their world direction, and configure the matching
 * transient effect group. */
// FUNCTION: LEGORR 0x0040DC20
int cell_pair_edge_effect_apply(int x1, int y1, int x2, int y2)
{
    static const int offsets[8][2] = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 },
        { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 }
    };
    int side = -1;
    int index;
    float position[3];
    float delta[3];
    for (index = 0; index < 4; ++index) {
        if (x2 == x1 + offsets[index][0]
            && y2 == y1 + offsets[index][1]) {
            side = 0;
            break;
        }
        if (x2 == x1 + offsets[index + 4][0]
            && y2 == y1 + offsets[index + 4][1]) {
            side = 1;
            break;
        }
    }
    if (side < 0) return 0;
    /* FUN_0040dc20 returns success as soon as the coordinate pair matches
     * one of its accepted offsets; it does not propagate the render-slot
     * allocation result. The active retail terrain always supplies these
     * positions. If the portable terrain adapter is absent, skip the effect
     * setup but retain that coordinate-based return value. */
    if (terrain_cell_world_position_get(x1, y1, position) != 0) {
        compute_world_position_delta(x1, y1, x2, y2, delta);
        (void)transient_effect_render_slot_configure(side, position[0],
            position[1], position[2], delta[0], delta[1], delta[2]);
    }
    return 1;
}

void fence_spark_terrain_hooks_install(FenceSparkCellCenterHook center,
                                      FenceSparkHeightHook height,
                                      FenceSparkEntityRadiusHook radius,
                                      FenceSparkFinalizeHook finalize,
                                      void* context)
{
    s_fence_spark_center_hook = center;
    s_fence_spark_height_hook = height;
    s_fence_spark_radius_hook = radius;
    s_fence_spark_finalize_hook = finalize;
    s_fence_spark_context = context;
}

/* FenceSparkLaunchRegister (FUN_0040e0c0): place the transient spark at the
 * target cell center, derive its vertical separation from the source height
 * and entity radius, then attach/register the spark through the caller hook. */
// FUNCTION: LEGORR 0x0040E0C0
void fence_spark_launch_register(void* entity, const float position_xy[2],
                                 int cell_x, int cell_y)
{
    EntitySlot* slot = (EntitySlot*)entity;
    LevelState* level = terrain_flood_current_level_get();
    float center[3];
    float source_height;
    float dx;
    float dy;
    float radius;
    int center_ok;
    int height_ok;
    if (entity == NULL || position_xy == NULL) return;
    center_ok = s_fence_spark_center_hook != NULL
        ? s_fence_spark_center_hook(cell_x, cell_y, center,
                                    s_fence_spark_context)
        : terrain_cell_world_position_get(cell_x, cell_y, center);
    if (s_fence_spark_height_hook != NULL) {
        height_ok = s_fence_spark_height_hook(position_xy[0], position_xy[1],
                &source_height, s_fence_spark_context);
    }
    else if (level != NULL && level->ptr_terrain != NULL) {
        source_height = terrain_surface_height_interpolate(
            level->ptr_terrain, position_xy[0], position_xy[1]);
        height_ok = 1;
    }
    else {
        height_ok = 0;
    }
    if (!center_ok || !height_ok) return;
    dx = position_xy[0] - center[0];
    dy = position_xy[1] - center[1];
    radius = s_fence_spark_radius_hook != NULL
        ? s_fence_spark_radius_hook(entity, s_fence_spark_context)
        : entity_stats_float_at74(slot);
    transient_effect_render_slot_configure(0, center[0], center[1], center[2],
        dx, dy, source_height - radius * 0.5f - center[2]);
    if (s_fence_spark_finalize_hook != NULL) {
        s_fence_spark_finalize_hook(entity, dx, dy, s_fence_spark_context);
    }
    /* FUN_0040e0c0 always follows TypeMotionScaleApplyDispatch with the
     * one-argument EntityActionRouteStateInitialize call. The type-motion
     * call remains adapter-owned because its retail callsite supplies only
     * two arguments to a five-argument routine; these route-state writes do
     * not depend on those unresolved stack values. */
    entity_action_route_state_initialize(slot);
}

void terrain_fence_spark_hooks_install(
    TerrainFenceSparkEntityPositionHook entity_position,
    TerrainFenceSparkCellLifetimeHook lifetime,
    TerrainFenceSparkEligibilityHook eligible,
    TerrainFenceSparkDurationHook duration, void* context)
{
    s_fence_spark_entity_position_hook = entity_position;
    s_fence_spark_lifetime_hook = lifetime;
    s_fence_spark_eligibility_hook = eligible;
    s_fence_spark_duration_hook = duration;
    s_fence_spark_interaction_context = context;
}

/* EntityFenceSparkTerrainInteraction (FUN_0040de40): check the source cell,
 * choose the closest adjacent cell whose spark timer has expired, register
 * the launch/activation, and refresh that cell's timer. */
// FUNCTION: LEGORR 0x0040DE40
int entity_fence_spark_terrain_interaction(void* entity)
{
    static const int offsets[4][2] = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 }
    };
    int cell_x;
    int cell_y;
    int target_x;
    int target_y;
    int have_target = 0;
    float entity_xy[2];
    float target_distance = 0.0f;
    float remaining;
    int index;
    EntitySlot* slot = (EntitySlot*)entity;
    GameContext* game;
    LevelState* level;
    uint32_t* source_state;
    int has_source_record;
    int source_eligible;
    int position_ok;
    int cell_ok;
    if (entity == NULL) return 0;
    game = slot->owner_ctx != NULL ? slot->owner_ctx :
        entity_pool_callback_context_get();
    level = game != NULL ? game->level :
        (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    map_cell_record_grid_activate(level);
    if (s_fence_spark_entity_position_hook != NULL) {
        position_ok = s_fence_spark_entity_position_hook(entity, entity_xy,
            s_fence_spark_interaction_context);
    }
    else {
        entity_transform_position_get(slot, entity_xy);
        position_ok = 1;
    }
    if (!position_ok) return 0;
    if (s_grid_object_coordinates_hook != NULL) {
        cell_ok = s_grid_object_coordinates_hook(entity, &cell_x, &cell_y,
                                                 s_grid_cell_object_context);
    }
    else {
        cell_ok = terrain_world_xy_to_cell(level->ptr_terrain,
            entity_xy[0], entity_xy[1], &cell_x, &cell_y);
    }
    if (!cell_ok || !level_coord_pair_within_bounds(level, cell_x, cell_y))
        return 0;
    source_state = terrain_cell_state_flags_at(cell_x, cell_y);
    source_eligible = s_fence_spark_eligibility_hook != NULL
        ? s_fence_spark_eligibility_hook(cell_x, cell_y,
            s_fence_spark_interaction_context)
        : source_state != NULL && (*source_state & 0x100u) != 0u &&
          terrain_flood_special_query(cell_x, cell_y, 0) == 0;
    if (!source_eligible) return 0;
    has_source_record = s_fence_spark_lifetime_hook != NULL
        ? s_fence_spark_lifetime_hook(cell_x, cell_y, &remaining,
            s_fence_spark_interaction_context)
        : terrain_cell_object_flood_gate_get(cell_x, cell_y, &remaining);
    if (has_source_record && remaining <= 0.0f) {
        target_x = cell_x;
        target_y = cell_y;
        have_target = 1;
    } else if (!has_source_record) {
        for (index = 0; index < 4; ++index) {
            int nx = cell_x + offsets[index][0];
            int ny = cell_y + offsets[index][1];
            float candidate_remaining;
            float position[3];
            float dx;
            float dy;
            float distance;
            int has_candidate = s_fence_spark_lifetime_hook != NULL
                ? s_fence_spark_lifetime_hook(nx, ny, &candidate_remaining,
                    s_fence_spark_interaction_context)
                : terrain_cell_object_flood_gate_get(nx, ny,
                                                     &candidate_remaining);
            if (!has_candidate
                || candidate_remaining > 0.0f
                || terrain_cell_world_position_get(nx, ny, position) == 0) {
                continue;
            }
            dx = entity_xy[0] - position[0];
            dy = entity_xy[1] - position[1];
            distance = dx * dx + dy * dy;
            if (distance < target_distance || target_distance == 0.0f) {
                target_distance = distance;
                target_x = nx;
                target_y = ny;
                have_target = 1;
            }
        }
    }
    if (!have_target) {
        return 0;
    }
    fence_spark_launch_register(entity, entity_xy, cell_x, cell_y);
    terrain_cell_activation_try(NULL, target_x, target_y);
    if (s_fence_spark_duration_hook != NULL) {
        remaining = s_fence_spark_duration_hook(entity,
            s_fence_spark_interaction_context);
    }
    else {
        uint32_t resource_type = lego::object_type_name_index_find("FenceSpark");
        remaining = lego::weapon_type_recharge_time_get((int)resource_type);
    }
    if (s_fence_spark_lifetime_hook != NULL) {
        s_fence_spark_lifetime_hook(target_x, target_y, &remaining,
                                    s_fence_spark_interaction_context);
    }
    else terrain_cell_object_flood_gate_set(target_x, target_y, remaining);
    return 1;
}

static int terrain_grid_cell_valid(const TerrainSurfaceGrid* grid,
                                   uint32_t x, uint32_t y);
static uint32_t terrain_grid_cell_index(const TerrainSurfaceGrid* grid,
                                        uint32_t x, uint32_t y);
static NestedPointerReleaseHook s_nested_handle_release_hook;
static NestedPointerReleaseHook s_nested_memory_release_hook;

void nested_pointer_record_release_hooks_set(NestedPointerReleaseHook handle_hook,
                                             NestedPointerReleaseHook memory_hook)
{
    s_nested_handle_release_hook = handle_hook;
    s_nested_memory_release_hook = memory_hook;
}

// FUNCTION: LEGORR 0x0044e670
void nested_pointer_record_destroy(NestedPointerRecord* record)
{
    if (record == NULL) return;
    if (s_nested_handle_release_hook != NULL) {
        s_nested_handle_release_hook(record->first_handle);
        s_nested_handle_release_hook(record->second_handle);
    }
    if (s_nested_memory_release_hook != NULL) {
        s_nested_memory_release_hook(record->child_buffer);
        s_nested_memory_release_hook(record);
    }
}

void terrain_surface_grid_initialize(TerrainSurfaceGrid* grid,
                                     uint32_t width, uint32_t height,
                                     uint8_t* surface_values,
                                     float* surface_vectors,
                                     uint8_t* triangulation_flags)
{
    if (grid == NULL) return;
    grid->owner_level = NULL;
    grid->width = width;
    grid->height = height;
    grid->record_width = width;
    grid->surface_values = surface_values;
    grid->surface_vectors = surface_vectors;
    grid->triangulation_flags = triangulation_flags;
    grid->cell_records = NULL;
    grid->texture_corner_values = NULL;
    grid->material_context = NULL;
    grid->material_handle_resolve = NULL;
    grid->surface_palette_apply = NULL;
    grid->terrain_window = NULL;
    grid->changed_count = 0;
    grid->initialize_context = NULL;
    grid->initialize_cell = NULL;
    memset(grid->changed_xy, 0, sizeof(grid->changed_xy));
}

void terrain_grid_cell_initializer_set(TerrainSurfaceGrid* grid, void* context,
                                       void (*initialize_cell)(void*, uint32_t,
                                                               uint32_t))
{
    if (grid == NULL) return;
    grid->initialize_context = context;
    grid->initialize_cell = initialize_cell;
}

// FUNCTION: LEGORR 0x0044e880
void terrain_grid_cells_initialize(TerrainSurfaceGrid* grid)
{
    uint32_t x;
    uint32_t y;
    if (grid == NULL) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            if (grid->initialize_cell != NULL)
                grid->initialize_cell(grid->initialize_context, x, y);
            else
                terrain_cell_surface_vectors_initialize(grid, x, y);
        }
    }
}

// FUNCTION: LEGORR 0x0044f490
void terrain_grid_operation_apply_all(TerrainSurfaceGrid* grid,
                                     uint32_t operation, void* context,
                                     TerrainGridOperationHook hook)
{
    uint32_t x;
    uint32_t y;
    if (grid != NULL && hook != NULL) {
        for (y = 0; y < grid->height; ++y) {
            for (x = 0; x < grid->width; ++x)
                hook(context, y * grid->width + x, operation);
        }
    }
}

void terrain_cell_palette_resource_at_grid_position(
    const TerrainPaletteGrid* grid, uint32_t x, uint32_t y)
{
    uint32_t cell_index;
    uint32_t packed;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->palette_resource_select == NULL || x >= grid->width ||
        y >= grid->height) return;
    cell_index = y * grid->width + x;
    packed = (cell_index * 9u) & 0xffffff00u;
    packed |= grid->cell_records[cell_index * 0x48u + 1u];
    grid->palette_resource_select(grid->context, packed);
}

/* TerrainCellPaletteResourceAtGridPosition returns the cell's palette
 * texture handle from the current level's +0x7c resource grid. */
// FUNCTION: LEGORR 0x00429160
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void* terrain_cell_palette_resource_at_grid_position_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov edx, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [ecx + 7ch]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        mov al, byte ptr [edx + eax*8 + 1]
        push eax
        push ecx
        call packed_grid_cell_value_get
        add esp, 8
        pop esi
        ret
    }
}
#else
void* terrain_cell_palette_resource_at_grid_position_current(
    const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (level == NULL || cell_xy == NULL) return NULL;
#if defined(_WIN32) && !defined(_WIN64)
    /* The retail target reads the active level's +0x14 width, +0xb8 cell
     * table, and +0x7c LoadTileVariants descriptor directly. Its callers
     * supply in-bounds coordinates; preserve the same row-major selector and
     * let PackedGridCellValueGet decode the table coordinates. */
    {
        uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
        uint32_t index = width * (uint32_t)cell_xy[1] +
                         (uint32_t)cell_xy[0];
        uint32_t packed = index * 9u;
        const uint8_t* voxels = (const uint8_t*)
            level->word[LEVEL_OFF_VOXELS / 4];
        const void* resources = (const void*)
            level->word[LEVEL_OFF_CELL_RESOURCES / 4];
        packed = (packed & 0xffffff00u) |
            voxels[index * LEVEL_VOXEL_STRIDE + 1u];
        return (void*)packed_grid_cell_value_get(resources, packed);
    }
#else
    {
        TerrainCellResourceGrid* resources;
        const uint8_t* cell;
        uint32_t x, y, width, height, index, packed, row, column;
        uint32_t resource_index;
        width = (uint32_t)level_width(level);
        height = (uint32_t)level_height(level);
        if (cell_xy[0] < 0 || cell_xy[1] < 0 ||
            (uint32_t)cell_xy[0] >= width ||
            (uint32_t)cell_xy[1] >= height) return NULL;
        x = (uint32_t)cell_xy[0];
        y = (uint32_t)cell_xy[1];
        cell = level_voxel(level, (int)x, (int)y);
        resources = level->ptr_cell_resources;
        if (cell == NULL || resources == NULL || resources->handles == NULL)
            return NULL;
        index = y * width + x;
        packed = (index * 9u & 0xffffff00u) | cell[1];
        /* FUN_0040b850 indexes low-nibble * width + high-nibble. */
        row = packed & 0x0fu;
        column = (packed & 0xffu) >> 4;
        if (row >= resources->height || column >= resources->width)
            return NULL;
        resource_index = row * resources->width + column;
        if (resource_index >= resources->width * resources->height)
            return NULL;
        return resources->handles[resource_index];
    }
#endif
}
#endif

// FUNCTION: LEGORR 0x0044e4d0
void terrain_grid_vertex_build(TerrainVertexBuildGrid* grid)
{
    static const float corners[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    static const uint8_t sample_x_offset[4] = { 0u, 1u, 1u, 0u };
    static const uint8_t sample_y_offset[4] = { 0u, 0u, 1u, 1u };
    uint32_t x, y;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->vertices_write == NULL || grid->map_width == 0 ||
        grid->map_height == 0 || grid->width >= grid->map_width ||
        grid->height >= grid->map_height) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float vertices[12];
            unsigned int corner;
            for (corner = 0; corner < 4; ++corner) {
                int sample_x = (int)x + (int)sample_x_offset[corner];
                int sample_y = (int)y + (int)sample_y_offset[corner];
                unsigned long sample_index;
                float height;
                sample_index = ((unsigned long)sample_y * grid->map_width +
                                (unsigned long)sample_x) * 0x28ul;
                height = -(float)grid->cell_records[sample_index + 0x1aul] *
                         grid->vertical_scale;
                vertices[corner * 3] = ((float)x + corners[corner][0]) *
                                       grid->spacing + grid->x_origin;
                vertices[corner * 3 + 1] = grid->z_origin -
                    ((float)y + corners[corner][1]) * grid->spacing;
                vertices[corner * 3 + 2] = height;
                if (height < grid->minimum_y) grid->minimum_y = height;
            }
            grid->vertices_write(grid->context, y * grid->width + x, vertices);
        }
    }
}

/* The palette globals read by FUN_0045e9e0 are adjacent float triplets in
 * the retail data section.  The game's ScreenCoordNormalizeInit divides this
 * block by 255 before the style resolver uses it; retain the original values
 * here and apply that same normalization when copying a selected triplet. */
typedef struct TerrainTilePaletteWords {
    uint32_t word[3];
} TerrainTilePaletteWords;

static const TerrainTilePaletteWords s_terrain_tile_palette[] = {
    { { 0x42340000u, 0x00000000u, 0x42960000u } }, /* 0x4a8f34 */
    { { 0x42240000u, 0x42c00000u, 0x437f0000u } }, /* 0x4a8f40 */
    { { 0x437f0000u, 0x42b40000u, 0x00000000u } }, /* 0x4a8f4c */
    { { 0x43240000u, 0x43630000u, 0x43150000u } }, /* 0x4a8f58 */
    { { 0x42c20000u, 0x434b0000u, 0x42ac0000u } }, /* 0x4a8f64 */
    { { 0x00000000u, 0x432f0000u, 0x41400000u } }, /* 0x4a8f70 */
    { { 0x00000000u, 0x42ba0000u, 0x00000000u } }, /* 0x4a8f7c */
    { { 0x00000000u, 0x423c0000u, 0x00000000u } }, /* 0x4a8f88 */
    { { 0x43470000u, 0x42f60000u, 0x437f0000u } }, /* 0x4a8f94 */
    { { 0x432e0000u, 0x42b20000u, 0x43690000u } }, /* 0x4a8fa0 */
    { { 0x43100000u, 0x42700000u, 0x43480000u } }, /* 0x4a8fac */
    { { 0x42e60000u, 0x41f00000u, 0x43280000u } }, /* 0x4a8fb8 */
    { { 0x42ac0000u, 0x40000000u, 0x43080000u } }, /* 0x4a8fc4 */
    { { 0x437f0000u, 0x42f60000u, 0x437b0000u } }, /* 0x4a8ff4 */
    { { 0x43690000u, 0x42b20000u, 0x43620000u } }, /* 0x4a9000 */
    { { 0x43480000u, 0x42700000u, 0x43400000u } }, /* 0x4a900c */
    { { 0x43280000u, 0x41f00000u, 0x431e0000u } }, /* 0x4a9018 */
    { { 0x43080000u, 0x40000000u, 0x42fa0000u } }, /* 0x4a9024 */
    { { 0x437f0000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9030 */
    { { 0x431d0000u, 0x431b0000u, 0x00000000u } }, /* 0x4a903c */
    { { 0x431b0000u, 0x42820000u, 0x41600000u } }, /* 0x4a9048 */
    { { 0x43340000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9054 */
    { { 0x437f0000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9060 */
    { { 0x00000000u, 0x423c0000u, 0x43350000u } }, /* 0x4a906c */
    { { 0x433f0000u, 0x433f0000u, 0x433f0000u } }, /* 0x4a9078 */
    { { 0x42d80000u, 0x00000000u, 0x00000000u } }  /* 0x4a9084 */
};

enum TerrainTilePaletteIndex {
    TERRAIN_TILE_PALETTE_TYPE1_PRE = 7,
    TERRAIN_TILE_PALETTE_TYPE2_PRE = 6,
    TERRAIN_TILE_PALETTE_TYPE3_PRE = 5,
    TERRAIN_TILE_PALETTE_TYPE4_PRE = 4,
    TERRAIN_TILE_PALETTE_TYPE5_PRE = 3,
    TERRAIN_TILE_PALETTE_TYPE6 = 2,
    TERRAIN_TILE_PALETTE_TYPE7 = 1,
    TERRAIN_TILE_PALETTE_TYPE8_SHARED = 20,
    TERRAIN_TILE_PALETTE_TYPE9 = 23,
    TERRAIN_TILE_PALETTE_TYPE10 = 21,
    TERRAIN_TILE_PALETTE_TYPE11 = 22,
    TERRAIN_TILE_PALETTE_DUG_TYPE1 = 12,
    TERRAIN_TILE_PALETTE_DUG_TYPE2 = 11,
    TERRAIN_TILE_PALETTE_DUG_TYPE3 = 10,
    TERRAIN_TILE_PALETTE_DUG_TYPE4 = 9,
    TERRAIN_TILE_PALETTE_DUG_TYPE5 = 8,
    TERRAIN_TILE_PALETTE_CELL_OCCUPIED = 18,
    TERRAIN_TILE_PALETTE_CELL_UNOCCUPIED = 19,
    TERRAIN_TILE_PALETTE_SPECIAL_DUG = 25,
    TERRAIN_TILE_PALETTE_FLAGS_100000 = 24
};

static uint32_t terrain_tile_palette_index(uint32_t material, int walkable,
                                           int wave)
{
    if (walkable) {
        if (wave) {
            if (material >= 1u && material <= 5u)
                return 18u - material;
            /* The wave branch shares the same palette records for terrain
             * types 8, 10, and 11.  The reference switches on these cases
             * after the five wave-specific type colors. */
            if (material == 8u) return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
            if (material == 10u) return TERRAIN_TILE_PALETTE_TYPE10;
            if (material == 11u) return TERRAIN_TILE_PALETTE_TYPE11;
            return 0xffffffffu;
        }
        switch (material) {
        case 1: return TERRAIN_TILE_PALETTE_DUG_TYPE1;
        case 2: return TERRAIN_TILE_PALETTE_DUG_TYPE2;
        case 3: return TERRAIN_TILE_PALETTE_DUG_TYPE3;
        case 4: return TERRAIN_TILE_PALETTE_DUG_TYPE4;
        case 5: return TERRAIN_TILE_PALETTE_DUG_TYPE5;
        case 8: return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
        case 10: return TERRAIN_TILE_PALETTE_TYPE10;
        case 11: return TERRAIN_TILE_PALETTE_TYPE11;
        default: return 0xffffffffu;
        }
    }
    switch (material) {
    case 1: return TERRAIN_TILE_PALETTE_TYPE1_PRE;
    case 2: return TERRAIN_TILE_PALETTE_TYPE2_PRE;
    case 3: return TERRAIN_TILE_PALETTE_TYPE3_PRE;
    case 4: return TERRAIN_TILE_PALETTE_TYPE4_PRE;
    case 5: return TERRAIN_TILE_PALETTE_TYPE5_PRE;
    case 6: return TERRAIN_TILE_PALETTE_TYPE6;
    case 7: return TERRAIN_TILE_PALETTE_TYPE7;
    case 8: return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
    case 9: return TERRAIN_TILE_PALETTE_TYPE9;
    case 10: return TERRAIN_TILE_PALETTE_TYPE10;
    case 11: return TERRAIN_TILE_PALETTE_TYPE11;
    default: return 0xffffffffu;
    }
}

static void terrain_tile_palette_copy(void* output, uint32_t palette)
{
    if (palette < sizeof(s_terrain_tile_palette) /
                  sizeof(s_terrain_tile_palette[0])) {
        float color[3];
        uint32_t component;
        for (component = 0; component < 3u; ++component) {
            memcpy(&color[component],
                   &s_terrain_tile_palette[palette].word[component],
                   sizeof(color[component]));
            color[component] /= 255.0f;
        }
        memcpy((unsigned char*)output + 0x10, color, sizeof(color));
    }
}

/* The target reads the current-level material and cell flags, then writes a
 * palette triplet only on the branches where the retail body selects one.
 * Recognized palette words are recorded verbatim from the retail data at
 * 0x4a8f34..0x4a908c. */
// FUNCTION: LEGORR 0x0045e9e0
int terrain_tile_style_resolve(void* output, int x, int y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level;
    const uint8_t* voxel;
    uint32_t flags = 0u;
    uint32_t palette;
    int material;
    int walkable;
    if (output == NULL) return 0;
    level = game != NULL ? game->level : NULL;
    if (level == NULL) level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0;
    voxel = level_voxel(level, x, y);
    if (voxel == NULL) return 0;
    memcpy(&flags, voxel + 0x08, sizeof(flags));
    material = level_tile_type(level, x, y);
    walkable = level_cell_walkable_flag(level, x, y);
    if (walkable) {
        palette = terrain_tile_palette_index((uint32_t)material, 1,
            level_cell_flag_wave(level, x, y) != 0u);
        if (palette != 0xffffffffu) terrain_tile_palette_copy(output, palette);
        return 1;
    }

    if (level_cell_mask18_clear(level, x, y) &&
        level_cell_flag_4(level, x, y) != 0u) {
        if ((flags & 0x20000u) != 0u) {
            terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_SPECIAL_DUG);
            return 1;
        }
        palette = terrain_tile_palette_index((uint32_t)material, 0, 0);
        if (palette != 0xffffffffu) {
            terrain_tile_palette_copy(output, palette);
            return 1;
        }
        return 1;
    }

    if (level_cell_flag_8_bounded(level, x, y) == 0u) return 0;
    if (material == 6) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE6);
        return 1;
    }
    if (material == 7) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE7);
        return 1;
    }
    if (material == 9) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE9);
        return 1;
    }
    if ((flags & 0x100000u) != 0u) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_FLAGS_100000);
        return 1;
    }
    if (level_cell_occupied_at(level, x, y) != 0) {
        if (level_cell_secondary_flag_100(level, x, y) != 0u) {
            terrain_tile_palette_copy(output,
                TERRAIN_TILE_PALETTE_CELL_OCCUPIED);
        }
        else {
            terrain_tile_palette_copy(output,
                TERRAIN_TILE_PALETTE_CELL_UNOCCUPIED);
        }
        return 1;
    }
    terrain_tile_palette_copy(output, 0u);
    return 1;
}

/* TerrainViewTileGeometryBuild (FUN_0045dbc0) remains an explicit-grid
 * adapter for the retail global render-view/mesh owner.  When no renderer
 * override is installed, it now takes its per-cell color from the direct
 * current-level TerrainTileStyleResolve target above. */
void terrain_view_tile_geometry_build(TerrainViewTileGrid* grid)
{
    uint32_t x, y;
    if (grid == NULL || grid->emit == NULL)
        return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float rgb[3];
            float position[3];
            if (grid->style_resolve != NULL) {
                if (!grid->style_resolve(grid->context, x, y, grid->style, rgb))
                    continue;
            }
            else {
                unsigned char output[0x1c] = { 0 };
                if (!terrain_tile_style_resolve(output, (int)x, (int)y))
                    continue;
                memcpy(rgb, output + 0x10, sizeof(rgb));
            }
            position[0] = grid->origin[0] + (float)x * grid->tile_size[0];
            position[1] = grid->origin[1];
            position[2] = grid->origin[2] - (float)y * grid->tile_size[1];
            grid->emit(grid->context, x, y, position, grid->tile_size, rgb);
        }
    }
}

void terrain_surface_grid_material_hooks_set(
    TerrainSurfaceGrid* grid, uint8_t* cell_records,
    uint8_t* texture_corner_values, void* context,
    uint32_t (*resolve)(void*, uint8_t),
    void (*apply)(void*, uint32_t, uint32_t))
{
    if (grid == NULL) return;
    grid->cell_records = cell_records;
    grid->texture_corner_values = texture_corner_values;
    grid->material_context = context;
    grid->material_handle_resolve = resolve;
    grid->surface_palette_apply = apply;
}

void terrain_surface_grid_terrain_window_set(TerrainSurfaceGrid* grid,
                                             const void* terrain_window)
{
    if (grid == NULL) return;
    grid->terrain_window = terrain_window;
}

/* Adapt the terrain window's shared 0x34/0x4034 table to the same layout as
 * InputKeyMappingAddBidirectional (FUN_0044f1a0). */
void terrain_cell_variant_table_insert(void* terrain_window, uint8_t key,
                                      uint8_t value)
{
    input_key_mapping_add_bidirectional(
        (InputKeyMappingTable*)terrain_window, key, value);
}

// FUNCTION: LEGORR 0x0044f230
void terrain_cell_material_set(TerrainSurfaceGrid* grid, uint32_t x,
                               uint32_t y, uint8_t material,
                               uint8_t rotation)
{
    uint32_t index;
    uint32_t record_width;
    uint32_t handle = material;
    uint8_t flags;
    uint32_t i;
    static const uint8_t corner_order[4] = { 1, 3, 2, 0 };
    if (!terrain_grid_cell_valid(grid, x, y) || grid->cell_records == NULL)
        return;
    index = terrain_grid_cell_index(grid, x, y);
    record_width = grid->record_width != 0u
        ? grid->record_width : grid->width;
    flags = grid->cell_records[((size_t)y * record_width + x) *
                               0x28u + 0x19u];
    if (grid->material_handle_resolve != NULL)
        handle = grid->material_handle_resolve(grid->material_context, material);
    grid->cell_records[((size_t)y * record_width + x) * 0x28u +
                       0x18u] = material;
    if ((flags & 1u) != 0) rotation = (uint8_t)(rotation + 1u);
    if (grid->surface_palette_apply != NULL)
        grid->surface_palette_apply(grid->material_context, index, handle);
    if ((flags & 0x10u) == 0 && grid->texture_corner_values != NULL) {
        uint8_t* output = grid->texture_corner_values + index * 4u;
        for (i = 0; i < 4; ++i)
            output[i] = corner_order[(uint8_t)(i - rotation) & 3u];
    }
}

static int terrain_grid_cell_valid(const TerrainSurfaceGrid* grid,
                                   uint32_t x, uint32_t y)
{
    return grid != NULL && x < grid->width && y < grid->height;
}

static uint32_t terrain_surface_record_width(const TerrainSurfaceGrid* grid)
{
    return grid->record_width != 0u ? grid->record_width : grid->width;
}

/* The surface dimensions describe quads; the record window may include the
 * extra row and column at their far edges. */
static int terrain_surface_record_bounds_get(const TerrainSurfaceGrid* grid,
                                             uint32_t* width,
                                             uint32_t* height)
{
    if (grid == NULL || width == NULL || height == NULL) return 0;
    *width = terrain_surface_record_width(grid);
    *height = grid->height;
    if (grid->terrain_window != NULL) {
        uint32_t window_height;
        memcpy(&window_height, (const uint8_t*)grid->terrain_window + 4,
               sizeof(window_height));
        if (window_height != 0u) *height = window_height;
    }
    return *width != 0u && *height != 0u;
}

static uint32_t terrain_grid_cell_index(const TerrainSurfaceGrid* grid,
                                        uint32_t x, uint32_t y)
{
    return y * grid->width + x;
}

static int terrain_surface_grid_cell_orientation_get(
    const TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, uint32_t index);

// FUNCTION: LEGORR 0x004500d0
int terrain_cell_surface_vertices_get(TerrainSurfaceGrid* grid, uint32_t x,
                                      uint32_t y, float out[12])
{
    uint32_t index;
    int flip;
    static const uint8_t flipped_order[4] = { 1, 2, 3, 0 };
    uint32_t i;
    if (grid == NULL || out == NULL) return 0;
    if (x >= grid->width || y >= grid->height) return (int)grid->width;
    index = terrain_grid_cell_index(grid, x, y);
    flip = terrain_surface_grid_cell_orientation_get(grid, x, y, index);
    if (grid->surface_vectors == NULL) {
        float vertices[4][3];
        uint32_t corner;
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level, x, y,
                                                     vertices)) return 0;
        /* SceneRender stores conventional (X,height,Z) triples; the retail
         * indexed surface read returns (X,Z,height). Its corner-height
         * cache already reflects the cell orientation, so preserve that
         * order instead of rotating it a second time. */
        for (corner = 0u; corner < 4u; ++corner) {
            out[corner * 3u] = vertices[corner][0];
            out[corner * 3u + 1u] = vertices[corner][2];
            out[corner * 3u + 2u] = vertices[corner][1];
        }
        return flip;
    }
    for (i = 0; i < 4; ++i) {
        uint32_t src_corner = flip ? flipped_order[i] : i;
        memcpy(out + i * 3, grid->surface_vectors + (index * 4 + src_corner) * 3,
               sizeof(float) * 3);
    }
    return flip;
}

/* TerrainCellOrientationToggleAndRebuild (FUN_0044edf0): switch the quad's
 * diagonal by cyclically rotating its four stored vertex records. Retail
 * writes the corresponding bit at cell +0x19, rebuilds the two triangle
 * normals, and marks the cell and four shared-corner records dirty. */
// FUNCTION: LEGORR 0x0044edf0
void terrain_cell_surface_orientation_set(TerrainSurfaceGrid* grid,
                                         uint32_t x, uint32_t y,
                                         int flipped)
{
    uint32_t index;
    uint32_t corner;
    uint32_t record_width, record_height;
    int old_flip;
    float rotated[12];
    float* vertices;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL)
        return;
    index = terrain_grid_cell_index(grid, x, y);
    old_flip = terrain_surface_grid_cell_orientation_get(grid, x, y, index);
    flipped = flipped != 0;
    if (old_flip == flipped) return;

    vertices = grid->surface_vectors + index * 12u;
    for (corner = 0u; corner < 4u; ++corner) {
        uint32_t source = old_flip ? ((corner + 1u) & 3u)
                                   : ((corner + 3u) & 3u);
        memcpy(rotated + corner * 3u, vertices + source * 3u,
               3u * sizeof(float));
    }
    memcpy(vertices, rotated, sizeof(rotated));
    if (grid->cell_records != NULL) {
        uint8_t* flags = grid->cell_records +
            ((size_t)y * terrain_surface_record_width(grid) + x) * 0x28u +
            0x19u;
        *flags = (uint8_t)((*flags & (uint8_t)~1u) | (flipped ? 1u : 0u));
    }
    if (grid->triangulation_flags != NULL) {
        grid->triangulation_flags[index] = (uint8_t)(
            (grid->triangulation_flags[index] & (uint8_t)~1u) |
            (flipped ? 1u : 0u));
    }
    terrain_cell_surface_vectors_rebuild(grid, x, y);
    if (grid->cell_records != NULL) {
        static const int dirty_offsets[4][2] = {
            { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
        };
        record_width = terrain_surface_record_width(grid);
        record_height = grid->height;
        if (grid->terrain_window != NULL) {
            uint32_t window_height;
            memcpy(&window_height,
                   (const uint8_t*)grid->terrain_window + 4,
                   sizeof(window_height));
            if (window_height != 0u)
                record_height = window_height;
        }
        for (corner = 0u; corner < 4u; ++corner) {
            int dirty_x = (int)x + dirty_offsets[corner][0];
            int dirty_y = (int)y + dirty_offsets[corner][1];
            if (dirty_x >= 0 && dirty_y >= 0 &&
                (uint32_t)dirty_x < record_width &&
                (uint32_t)dirty_y < record_height) {
                size_t dirty_index = (size_t)dirty_y * record_width +
                                     (uint32_t)dirty_x;
                grid->cell_records[dirty_index * 0x28u + 0x19u] |= 4u;
            }
            if (grid->owner_level != NULL &&
                grid->owner_level->ptr_terrain != NULL) {
                terrain_cell_changed_flag_set(
                    grid->owner_level->ptr_terrain, dirty_x, dirty_y);
            }
        }
    }
    else if (grid->owner_level != NULL &&
             grid->owner_level->ptr_terrain != NULL) {
        static const int dirty_offsets[4][2] = {
            { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
        };
        for (corner = 0u; corner < 4u; ++corner) {
            terrain_cell_changed_flag_set(
                grid->owner_level->ptr_terrain,
                (int)x + dirty_offsets[corner][0],
                (int)y + dirty_offsets[corner][1]);
        }
    }
}

/* TerrainCellSurfaceTriangleVerticesSelect (FUN_0044fab0). The renderer
 * grid's point coordinates are world-space; use its owning terrain transform
 * to select the cell and its configured block size for the edge test. */
// FUNCTION: LEGORR 0x0044FAB0
void terrain_cell_surface_triangle_vertices_select(
    TerrainSurfaceGrid* grid, float x, float z, uint32_t out_vertices[3])
{
    int mapped_x, mapped_z;
    uint32_t cell_x;
    uint32_t cell_z;
    uint32_t index;
    uint32_t corner_offset;
    const uint32_t* triangle;
    float vertices[12];
    const uint8_t* cell_records;
    uint32_t record_width;
    size_t record_index;
    int flipped;
    uint32_t i;
    float cell_size;
    if (grid == NULL || out_vertices == NULL || grid->owner_level == NULL ||
        grid->owner_level->ptr_terrain == NULL)
        return;
    if (!terrain_world_xy_to_cell(grid->owner_level->ptr_terrain, x, z,
                                  &mapped_x, &mapped_z)) return;
    if (mapped_x < 0 || mapped_z < 0) return;
    cell_x = (uint32_t)mapped_x;
    cell_z = (uint32_t)mapped_z;
    if (!terrain_grid_cell_valid(grid, cell_x, cell_z) ||
        grid->surface_vectors == NULL)
        return;
    cell_size = terrain_blocksize_get(grid->owner_level->ptr_terrain);
    if (cell_size == 0.0f) return;
    index = terrain_grid_cell_index(grid, cell_x, cell_z);
    flipped = terrain_cell_surface_vertices_get(grid, cell_x, cell_z,
                                                vertices);
    if (!flipped) {
        if ((vertices[1] - z) + (x - vertices[0]) < cell_size)
            corner_offset = 0u;
        else
            corner_offset = 3u;
    }
    else if (cell_size <= (vertices[4] - z) + (vertices[3] - x)) {
        corner_offset = 0u;
    }
    else {
        corner_offset = 3u;
    }
    cell_records = grid->cell_records;
    record_width = grid->record_width != 0u
        ? grid->record_width : grid->width;
    if (cell_records == NULL && grid->terrain_window != NULL) {
        uint32_t window_width, window_height, window_stride;
        memcpy(&window_width, grid->terrain_window, sizeof(window_width));
        memcpy(&window_height,
               (const uint8_t*)grid->terrain_window + 4u,
               sizeof(window_height));
        memcpy(&window_stride,
               (const uint8_t*)grid->terrain_window + 8u,
               sizeof(window_stride));
        memcpy(&cell_records,
               (const uint8_t*)grid->terrain_window + 0x28u,
               sizeof(cell_records));
        if (cell_records == NULL || cell_x >= window_width ||
            cell_z >= window_height) return;
        record_width = window_stride;
    }
    if (cell_records == NULL || record_width == 0u || cell_x >= record_width)
        return;
    record_index = (size_t)cell_z * record_width + cell_x;
    triangle = (const uint32_t*)(cell_records + record_index * 0x28u +
                                  (corner_offset == 0u ? 0u : 0x0cu));
    for (i = 0u; i < 3u; ++i) out_vertices[i] = triangle[i];
}

static int terrain_ray_triangle_hit(const float origin[3],
                                    const float direction[3],
                                    const float a[3], const float b[3],
                                    const float c[3], float hit[3])
{
    float normal[3];
    float point[3];
    float v0[3], v1[3], v2[3];
    float d00, d01, d11, d20, d21, denominator, u, v;
    int i;
    surface_triangle_normal_calculate(normal, a, b, c);
    if (!view_ray_intersection_test(point, origin, direction, a, normal))
        return 0;
    for (i = 0; i < 3; ++i) {
        v0[i] = c[i] - a[i];
        v1[i] = b[i] - a[i];
        v2[i] = point[i] - a[i];
    }
    d00 = v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2];
    d01 = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
    d11 = v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2];
    d20 = v2[0]*v0[0] + v2[1]*v0[1] + v2[2]*v0[2];
    d21 = v2[0]*v1[0] + v2[1]*v1[1] + v2[2]*v1[2];
    denominator = d00 * d11 - d01 * d01;
    if (denominator == 0.0f) return 0;
    u = (d11 * d20 - d01 * d21) / denominator;
    v = (d00 * d21 - d01 * d20) / denominator;
    if (u < 0.0f || v < 0.0f || u + v > 1.0f) return 0;
    for (i = 0; i < 3; ++i) hit[i] = point[i];
    return 1;
}

// FUNCTION: LEGORR 0x004515E0
int terrain_cell_ray_hit_test(TerrainSurfaceGrid* grid, int x,
                              int y, const float origin[3],
                              const float direction[3], float hit[3])
{
    float vertices[12];
    float rendered_vertices[4][3];
    float first_hit[3], second_hit[3];
    int first, second;
    int i;
    if (origin == NULL || direction == NULL || hit == NULL || grid == NULL ||
        x < 0 || y < 0 ||
        !terrain_grid_cell_valid(grid, (uint32_t)x, (uint32_t)y))
        return 0;
    if (grid->surface_vectors != NULL) {
        (void)terrain_cell_surface_vertices_get(grid, (uint32_t)x,
                                                (uint32_t)y, vertices);
    }
    else {
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level,
                (uint32_t)x, (uint32_t)y, rendered_vertices)) return 0;
        memcpy(vertices, rendered_vertices, sizeof(rendered_vertices));
    }
    first = terrain_ray_triangle_hit(origin, direction, vertices,
                                     vertices + 3, vertices + 6, first_hit);
    second = terrain_ray_triangle_hit(origin, direction, vertices + 6,
                                      vertices + 9, vertices, second_hit);
    if (!first && !second) return 0;
    if (first && second) {
        float dx0 = first_hit[0] - origin[0];
        float dy0 = first_hit[1] - origin[1];
        float dz0 = first_hit[2] - origin[2];
        float dx1 = second_hit[0] - origin[0];
        float dy1 = second_hit[1] - origin[1];
        float dz1 = second_hit[2] - origin[2];
        if (dx1*dx1 + dy1*dy1 + dz1*dz1 <
            dx0*dx0 + dy0*dy0 + dz0*dz0) {
            for (i = 0; i < 3; ++i) hit[i] = second_hit[i];
            return 1;
        }
    }
    for (i = 0; i < 3; ++i) hit[i] = first ? first_hit[i] : second_hit[i];
    return 1;
}

static const StagedCellCandidateRecord* s_staged_candidate_records;
static uint32_t s_staged_candidate_record_count;
static StagedCellCandidateLinks* s_staged_candidate_links;
static StagedCellCandidateSelectionRecord* s_staged_selection_records;
static uint32_t s_staged_selection_record_count;
typedef struct TerrainWorldSpatialRecord {
    TerrainCellQuadRunRecord run;
    float* candidate_xy; /* four candidate coordinates per run item */
} TerrainWorldSpatialRecord;

static void terrain_world_spatial_candidates_select(
    TerrainWorldSpatialRecord* record, LevelState* level);

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

static void terrain_world_spatial_record_release(
    TerrainWorldSpatialRecord* record)
{
    if (record == NULL) return;
    free((void*)record->run.cell_xy);
    free(record->run.slot_source_items);
    free(record->run.slot_active);
    free(record->run.slot_offsets);
    free(record->run.slot_heights);
    free((void*)record->run.slot_orientations);
    free(record->candidate_xy);
    memset(record, 0, sizeof(*record));
}

void terrain_cell_quad_run_stage_level_release(LevelState* level)
{
    TerrainWorldSpatialRecords* owner;
    uint32_t i;
    if (level == NULL || level->ptr_terrain_quad_runs == NULL) return;
    owner = (TerrainWorldSpatialRecords*)level->ptr_terrain_quad_runs;
    if (owner->owner != level) return;
    for (i = 0u; i < owner->count; ++i)
        terrain_world_spatial_record_release(&owner->records[i]);
    free(owner->records);
    free(owner->merges);
    free(owner);
    level->ptr_terrain_quad_runs = NULL;
}

static int terrain_world_spatial_merge_append(
    TerrainWorldSpatialRecords* owner, uint32_t destination, uint32_t source)
{
    TerrainWorldSpatialMerge* merges;
    uint32_t capacity;
    if (owner->merge_count >= owner->merge_capacity) {
        capacity = owner->merge_capacity == 0u
            ? 8u : owner->merge_capacity * 2u;
        if (capacity < owner->merge_capacity ||
            (size_t)capacity > ((size_t)-1) / sizeof(*merges)) return 0;
        merges = (TerrainWorldSpatialMerge*)realloc(
            owner->merges, (size_t)capacity * sizeof(*merges));
        if (merges == NULL) return 0;
        owner->merges = merges;
        owner->merge_capacity = capacity;
    }
    owner->merges[owner->merge_count].destination = destination;
    owner->merges[owner->merge_count].source = source;
    ++owner->merge_count;
    return 1;
}

void world_spatial_records_install(LevelState* level, void* records)
{
    TerrainWorldSpatialRecords* owner =
        (TerrainWorldSpatialRecords*)records;
    if (level == NULL || (owner != NULL && owner->owner != level)) return;
    if (level->ptr_terrain_quad_runs != NULL &&
        level->ptr_terrain_quad_runs != records)
        terrain_cell_quad_run_stage_level_release(level);
    level->ptr_terrain_quad_runs = records;
}

/* TerrainEdgeFlagMidpointUpdate's CPU half (FUN_0046e610): for each staged
 * cell item, sample the selected edge's world-space endpoints and map the
 * two offset midpoints back to terrain cells. The renderer's mesh updates
 * remain with the level-owned typed callback. */
void terrain_world_spatial_edge_midpoints_update(LevelState* level)
{
    static const uint8_t edge_corners[8] = {
        0u, 1u, 1u, 2u, 2u, 3u, 3u, 0u
    };
    static const float direction_x[4] = { 0.0f, 1.0f, 0.0f, -1.0f };
    static const float direction_z[4] = { 1.0f, 0.0f, -1.0f, 0.0f };
    TerrainWorldSpatialRecords* owner;
    uint32_t record_index;
    if (level == NULL || level->ptr_terrain == NULL ||
        level->ptr_terrain_quad_runs == NULL) return;
    owner = (TerrainWorldSpatialRecords*)level->ptr_terrain_quad_runs;
    if (owner->owner != level) return;
    for (record_index = 0u; record_index < owner->count; ++record_index) {
        TerrainCellQuadRunRecord* run = &owner->records[record_index].run;
        uint32_t item;
        if (run->cell_xy == NULL || run->slot_source_items == NULL ||
            run->slot_orientations == NULL ||
            run->slot_offsets == NULL) continue;
        for (item = 0u; item < run->selected_count; ++item) {
            uint32_t source_item = run->slot_source_items[item];
            int x;
            int y;
            uint32_t orientation = run->slot_orientations[item];
            uint32_t endpoint;
            if (source_item >= run->item_count) continue;
            x = crt_float_to_long(run->cell_xy[source_item * 2u]);
            y = crt_float_to_long(run->cell_xy[source_item * 2u + 1u]);
            if (orientation > 3u || x < 0 || y < 0) continue;
            for (endpoint = 0u; endpoint < 2u; ++endpoint) {
                float vertices[4][3];
                uint32_t corner_a =
                    edge_corners[orientation * 2u];
                uint32_t corner_b =
                    edge_corners[orientation * 2u + 1u];
                float offset = run->slot_offsets[item * 2u + endpoint];
                float world_x, world_z;
                int cell_x, cell_y;
                if (!scene_render_terrain_cell_vertices_get(
                        level, (uint32_t)x, (uint32_t)y, vertices)) continue;
                world_x = (vertices[corner_a][0] +
                           vertices[corner_b][0]) * 0.5f +
                          direction_x[orientation] * offset;
                world_z = (vertices[corner_a][2] +
                           vertices[corner_b][2]) * 0.5f +
                          direction_z[orientation] * offset;
                if (terrain_world_xy_to_cell(level->ptr_terrain,
                        world_x, world_z, &cell_x, &cell_y)) {
                    terrain_cell_edge_flag_set_and_update(
                        level, cell_x, cell_y, endpoint == 0u);
                }
            }
        }
    }
}

static int terrain_world_spatial_records_reserve(
    TerrainWorldSpatialRecords* owner, uint32_t minimum)
{
    TerrainWorldSpatialRecord* records;
    uint32_t capacity;
    if (owner->capacity >= minimum) return 1;
    capacity = owner->capacity == 0u ? 8u : owner->capacity;
    while (capacity < minimum) {
        if (capacity > 0x7fffffffu) return 0;
        capacity *= 2u;
    }
    records = (TerrainWorldSpatialRecord*)realloc(
        owner->records, (size_t)capacity * sizeof(*records));
    if (records == NULL) return 0;
    memset(records + owner->capacity, 0,
           (size_t)(capacity - owner->capacity) * sizeof(*records));
    owner->records = records;
    owner->capacity = capacity;
    return 1;
}

static int terrain_world_spatial_record_reserve_items(
    TerrainWorldSpatialRecord* record, uint32_t minimum)
{
    TerrainCellQuadRunRecord* run = &record->run;
    uint32_t capacity = run->slot_capacity;
    float* cells;
    float* candidates;
    float* offsets;
    float* heights;
    uint8_t* active;
    uint32_t* source_items;
    uint32_t* orientations;
    if (capacity >= minimum) return 1;
    capacity = capacity == 0u ? 8u : capacity;
    while (capacity < minimum) {
        if (capacity > 0x7fffffffu) return 0;
        capacity *= 2u;
    }
    cells = (float*)realloc((void*)run->cell_xy,
                            (size_t)capacity * 2u * sizeof(float));
    if (cells == NULL) return 0;
    run->cell_xy = cells;
    candidates = (float*)realloc(record->candidate_xy,
        (size_t)capacity * 8u * sizeof(float));
    if (candidates == NULL) return 0;
    record->candidate_xy = candidates;
    offsets = (float*)realloc(run->slot_offsets,
                              (size_t)capacity * 2u * sizeof(float));
    if (offsets == NULL) return 0;
    run->slot_offsets = offsets;
    heights = (float*)realloc(run->slot_heights,
                              (size_t)capacity * sizeof(float));
    if (heights == NULL) return 0;
    run->slot_heights = heights;
    active = (uint8_t*)realloc(run->slot_active,
                               (size_t)capacity * sizeof(uint8_t));
    if (active == NULL) return 0;
    run->slot_active = active;
    source_items = (uint32_t*)realloc(run->slot_source_items,
                                      (size_t)capacity * sizeof(uint32_t));
    if (source_items == NULL) return 0;
    run->slot_source_items = source_items;
    orientations = (uint32_t*)realloc(
        (void*)run->slot_orientations,
        (size_t)capacity * sizeof(uint32_t));
    if (orientations == NULL) return 0;
    run->slot_orientations = orientations;
    memset(cells + (size_t)run->slot_capacity * 2u, 0,
           (size_t)(capacity - run->slot_capacity) * 2u * sizeof(float));
    memset(candidates + (size_t)run->slot_capacity * 8u, 0,
           (size_t)(capacity - run->slot_capacity) * 8u * sizeof(float));
    memset(offsets + (size_t)run->slot_capacity * 2u, 0,
           (size_t)(capacity - run->slot_capacity) * 2u * sizeof(float));
    memset(heights + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(float));
    memset(active + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint8_t));
    memset(source_items + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint32_t));
    memset(orientations + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint32_t));
    run->slot_capacity = capacity;
    return 1;
}

static int terrain_world_spatial_record_append(
    TerrainWorldSpatialRecord* record, uint32_t x0, uint32_t x1,
    uint32_t y)
{
    uint32_t x;
    TerrainCellQuadRunRecord* run = &record->run;
    for (x = x0; x < x1; ++x) {
        uint32_t item = run->item_count;
        float* cells;
        float* candidates;
        static const int candidate_dx[4] = { 0, 1, 0, -1 };
        static const int candidate_dy[4] = { -1, 0, 1, 0 };
        uint32_t alternative;
        if (!terrain_world_spatial_record_reserve_items(record, item + 1u))
            return 0;
        cells = (float*)run->cell_xy;
        candidates = record->candidate_xy;
        cells[item * 2u] = (float)x;
        cells[item * 2u + 1u] = (float)y;
        for (alternative = 0u; alternative < 4u; ++alternative) {
            candidates[item * 8u + alternative * 2u] =
                (float)((int)x + candidate_dx[alternative]);
            candidates[item * 8u + alternative * 2u + 1u] =
                (float)((int)y + candidate_dy[alternative]);
        }
        ++run->item_count;
    }
    return 1;
}

void staged_cell_candidate_collect_state_install(
    const StagedCellCandidateRecord* records, uint32_t record_count,
    StagedCellCandidateLinks* links)
{
    s_staged_candidate_records = records;
    s_staged_candidate_record_count = records != NULL ? record_count : 0u;
    s_staged_candidate_links = links;
    if (links != NULL) links->count = 0u;
}

/* StagedCellCandidateCollect (FUN_0046e9d0): walk the installed staged-cell
 * records for every cell in [first_cell,end_cell), join matching records
 * from the previous row, append pairs from the first matching record to each
 * distinct later match, and return the first matching record. This matches
 * the retail accumulator: after its first match, later matches preserve that
 * first record as the merge destination. */
// FUNCTION: LEGORR 0x0046E9D0
void* staged_cell_candidate_collect(uint32_t row, uint32_t first_cell,
                                    uint32_t end_cell)
{
    uint32_t cell;
    uint32_t record_index;
    void* first_record = NULL;
    StagedCellCandidateLinks* const links = s_staged_candidate_links;
    const StagedCellCandidateRecord* const records = s_staged_candidate_records;
    const uint32_t record_count = s_staged_candidate_record_count;
    if (row == 0u || records == NULL || links == NULL ||
        links->previous_records == NULL || links->current_records == NULL)
        return NULL;
    for (cell = first_cell; cell < end_cell; ++cell) {
        for (record_index = 0; record_index < record_count; ++record_index) {
            const StagedCellCandidateRecord* record = &records[record_index];
            uint32_t pair;
            if (record->record == NULL || record->cell_pairs == NULL)
                continue;
            for (pair = 0; pair < record->pair_count; ++pair) {
                const float* xy = record->cell_pairs + pair * 2u;
                if (xy[0] == (float)cell && xy[1] == (float)(row - 1u)) {
                    if (first_record == NULL) {
                        first_record = record->record;
                    }
                    else if (first_record != record->record) {
                        if (links->count >= links->capacity)
                            return first_record;
                        links->previous_records[links->count] = first_record;
                        links->current_records[links->count] = record->record;
                        ++links->count;
                    }
                }
            }
        }
    }
    return first_record;
}

void staged_cell_candidate_selection_state_install(
    StagedCellCandidateSelectionRecord* records, uint32_t record_count)
{
    s_staged_selection_records = records;
    s_staged_selection_record_count = records != NULL ? record_count : 0u;
}

/* StagedCellCandidateSelect (FUN_0046dd10): filter each staged item to its
 * first candidate whose cell is in bounds and is neither type 7 nor type 1. */
// FUNCTION: LEGORR 0x0046dd10
void staged_cell_candidate_select(void* unused, LevelState* level)
{
    uint32_t record_index;
    int width;
    int height;
    const uint8_t* voxels;
    StagedCellCandidateSelectionRecord* const records =
        s_staged_selection_records;
    const uint32_t record_count = s_staged_selection_record_count;
    (void)unused;
    if (level != NULL && level->ptr_terrain_quad_runs != NULL) {
        TerrainWorldSpatialRecords* owner =
            (TerrainWorldSpatialRecords*)level->ptr_terrain_quad_runs;
        if (owner->owner == level) {
            for (record_index = 0u; record_index < owner->count;
                 ++record_index)
                terrain_world_spatial_candidates_select(
                    &owner->records[record_index], level);
            return;
        }
    }
    if (level == NULL) return;
    width = level_width(level);
    height = level_height(level);
    voxels = (const uint8_t*)level->ptr_voxels;
    for (record_index = 0; record_index < record_count;
         ++record_index) {
        StagedCellCandidateSelectionRecord* record = &records[record_index];
        uint32_t item;
        record->selected_count = 0u;
        for (item = 0; item < record->item_count; ++item) {
            uint32_t alternative;
            const float* candidates = record->candidate_xy + item * 8u;
            for (alternative = 0; alternative < 4u; ++alternative) {
                int x = crt_float_to_long(candidates[alternative * 2u]);
                int y = crt_float_to_long(candidates[alternative * 2u + 1u]);
                uint32_t index;
                uint8_t cell_type;
                if (x < 0 || x >= width || y < 0 || y >= height) continue;
                index = ((uint32_t)y * (uint32_t)width + (uint32_t)x) *
                    LEVEL_VOXEL_STRIDE;
                cell_type = voxels[index + 2u];
                if (cell_type == 7u || cell_type == 1u) continue;
                record->selected_items[record->selected_count] = item;
                record->selected_alternatives[record->selected_count] =
                    alternative;
                ++record->selected_count;
                break;
            }
        }
    }
}

void terrain_cell_quad_run_stage(
    LevelState* level, void* context, TerrainCellTypeReadHook read_cell_type,
    TerrainCellQuadRunFlushHook flush_run,
    TerrainCellQuadRunFinishHook finish)
{
    int width;
    int height;
    uint32_t y;
    if (level == NULL || flush_run == NULL) return;
    width = level_width(level);
    height = level_height(level);
    for (y = 0; width > 0 && height > 0 && y < (uint32_t)height; ++y) {
        uint32_t x;
        uint32_t run_start = 0u;
        int run_active = 0;
        for (x = 0; x < (uint32_t)width; ++x) {
            uint32_t cell_type = 0xffffffffu;
            int readable = read_cell_type != NULL
                ? read_cell_type(context, level, x, y, &cell_type)
                : level_cell_type_get_at(level, x, y, &cell_type);
            if (readable && cell_type == 7u) {
                if (!run_active) {
                    run_start = x;
                    run_active = 1;
                }
            }
            else if (run_active) {
                flush_run(context, y, run_start, x);
                run_active = 0;
            }
        }
        /* The reference has no trailing-run flush at the row boundary. */
    }
    if (finish != NULL) finish(context);
}

typedef struct TerrainWorldSpatialStageContext {
    TerrainWorldSpatialRecords* owner;
    int failed;
} TerrainWorldSpatialStageContext;

static void terrain_world_spatial_run_flush(void* context, uint32_t row,
                                           uint32_t first_cell,
                                           uint32_t end_cell)
{
    TerrainWorldSpatialStageContext* stage =
        (TerrainWorldSpatialStageContext*)context;
    TerrainWorldSpatialRecord* selected = NULL;
    uint32_t selected_index = 0u;
    uint32_t x, record_index;
    if (stage == NULL || stage->owner == NULL || stage->failed) return;
    /* StagedCellCandidateCollect searches the prior row for matching x
     * coordinates and returns the last record found. */
    if (row != 0u) {
        for (x = first_cell; x < end_cell; ++x) {
            for (record_index = 0u;
                 record_index < stage->owner->count; ++record_index) {
                TerrainWorldSpatialRecord* record =
                    &stage->owner->records[record_index];
                uint32_t item;
                for (item = 0u; item < record->run.item_count; ++item) {
                    const float* xy = record->run.cell_xy + item * 2u;
                    if (crt_float_to_long(xy[0]) == (int)x &&
                        crt_float_to_long(xy[1]) == (int)(row - 1u)) {
                        if (selected == NULL) {
                            selected = record;
                            selected_index = record_index;
                        }
                        else if (selected != record &&
                            !terrain_world_spatial_merge_append(
                                stage->owner, selected_index, record_index)) {
                            stage->failed = 1;
                            return;
                        }
                    }
                }
            }
        }
    }
    if (selected == NULL) {
        if (!terrain_world_spatial_records_reserve(stage->owner,
                                                   stage->owner->count + 1u)) {
            stage->failed = 1;
            return;
        }
        selected_index = stage->owner->count++;
        selected = &stage->owner->records[selected_index];
        memset(selected, 0, sizeof(*selected));
        selected->run.mesh = -1;
    }
    if (!terrain_world_spatial_record_append(selected, first_cell, end_cell,
                                              row))
        stage->failed = 1;
}

static void terrain_world_spatial_candidates_select(
    TerrainWorldSpatialRecord* record, LevelState* level)
{
    uint32_t item;
    int width = level_width(level);
    int height = level_height(level);
    const uint8_t* voxels = (const uint8_t*)level->ptr_voxels;
    if (record == NULL || voxels == NULL) return;
    record->run.selected_count = 0u;
    for (item = 0u; item < record->run.item_count; ++item) {
        uint32_t alternative;
        const float* xy = record->candidate_xy + item * 8u;
        for (alternative = 0u; alternative < 4u; ++alternative) {
            int x = crt_float_to_long(xy[alternative * 2u]);
            int y = crt_float_to_long(xy[alternative * 2u + 1u]);
            uint8_t type;
            if (x < 0 || y < 0 || x >= width || y >= height) continue;
            type = voxels[((size_t)(uint32_t)y * (size_t)width +
                           (uint32_t)x) * LEVEL_VOXEL_STRIDE + 2u];
            if (type == 7u || type == 1u) continue;
            {
                uint32_t slot = record->run.selected_count;
                record->run.slot_source_items[slot] = item;
                record->run.slot_orientations[slot] = alternative;
                record->run.slot_active[slot] = 0u;
                record->run.slot_offsets[slot * 2u] = 0.0f;
                record->run.slot_offsets[slot * 2u + 1u] = 0.0f;
                record->run.slot_heights[slot] = 0.0f;
                ++record->run.selected_count;
            }
            break;
        }
    }
}

static int terrain_world_spatial_records_merge(
    TerrainWorldSpatialRecords* owner)
{
    uint32_t merge_index;
    uint32_t read_index;
    uint32_t write_index = 0u;
    for (merge_index = 0u; merge_index < owner->merge_count; ++merge_index) {
        TerrainWorldSpatialMerge* merge = &owner->merges[merge_index];
        TerrainWorldSpatialRecord* destination;
        TerrainWorldSpatialRecord* source;
        uint32_t item;
        if (merge->destination >= owner->count ||
            merge->source >= owner->count ||
            merge->destination == merge->source) continue;
        destination = &owner->records[merge->destination];
        source = &owner->records[merge->source];
        for (item = 0u; item < source->run.item_count; ++item) {
            uint32_t target_item = destination->run.item_count;
            if (!terrain_world_spatial_record_reserve_items(
                    destination, target_item + 1u)) return 0;
            ((float*)destination->run.cell_xy)[target_item * 2u] =
                source->run.cell_xy[item * 2u];
            ((float*)destination->run.cell_xy)[target_item * 2u + 1u] =
                source->run.cell_xy[item * 2u + 1u];
            memcpy(destination->candidate_xy + target_item * 8u,
                   source->candidate_xy + item * 8u,
                   8u * sizeof(float));
            destination->run.slot_active[target_item] =
                source->run.slot_active[item];
            destination->run.slot_source_items[target_item] =
                source->run.slot_source_items[item];
            destination->run.slot_offsets[target_item * 2u] =
                source->run.slot_offsets[item * 2u];
            destination->run.slot_offsets[target_item * 2u + 1u] =
                source->run.slot_offsets[item * 2u + 1u];
            destination->run.slot_heights[target_item] =
                source->run.slot_heights[item];
            ((uint32_t*)destination->run.slot_orientations)[target_item] =
                source->run.slot_orientations[item];
            ++destination->run.item_count;
        }
        source->run.item_count = 0u;
    }
    for (read_index = 0u; read_index < owner->count; ++read_index) {
        if (owner->records[read_index].run.item_count == 0u) {
            terrain_world_spatial_record_release(&owner->records[read_index]);
            continue;
        }
        if (write_index != read_index) {
            owner->records[write_index] = owner->records[read_index];
            memset(&owner->records[read_index], 0,
                   sizeof(owner->records[read_index]));
        }
        ++write_index;
    }
    owner->count = write_index;
    owner->merge_count = 0u;
    return 1;
}

/* Comparator at LAB_0046e9a0 reads the dword at record + 0x320, the
 * accumulated run item count. Its return value orders larger counts first. */
static void terrain_world_spatial_records_sort(
    TerrainWorldSpatialRecords* owner)
{
    uint32_t index;
    for (index = 1u; index < owner->count; ++index) {
        TerrainWorldSpatialRecord value = owner->records[index];
        uint32_t position = index;
        while (position > 0u &&
               owner->records[position - 1u].run.item_count <
                   value.run.item_count) {
            owner->records[position] = owner->records[position - 1u];
            --position;
        }
        owner->records[position] = value;
    }
}

// FUNCTION: LEGORR 0x0046db90
void terrain_cell_quad_run_stage_level(void* scene, LevelState* level)
{
    TerrainWorldSpatialRecords* owner;
    TerrainWorldSpatialStageContext context;
    uint32_t record_index;
    if (level == NULL) return;
    (void)scene;
    terrain_cell_quad_run_stage_level_release(level);
    owner = (TerrainWorldSpatialRecords*)calloc(1u, sizeof(*owner));
    if (owner == NULL) return;
    owner->owner = level;
    world_spatial_records_install(level, owner);
    context.owner = owner;
    context.failed = 0;
    /* This is the production CellQuadRunStage producer called at the end of
     * MapLoadPipeline after the voxel and terrain maps are initialized. */
    terrain_cell_quad_run_stage(level, &context, NULL,
                                terrain_world_spatial_run_flush, NULL);
    if (context.failed) {
        terrain_cell_quad_run_stage_level_release(level);
        return;
    }
    if (!terrain_world_spatial_records_merge(owner)) {
        terrain_cell_quad_run_stage_level_release(level);
        return;
    }
    terrain_world_spatial_records_sort(owner);
    staged_cell_candidate_select(NULL, level);
    /* Prime SceneRender's row-major CPU quad entries for the staged cells.
     * Native mesh creation/visibility remains with the DirectDraw owner. */
    for (record_index = 0u; record_index < owner->count; ++record_index) {
        TerrainCellQuadRunRecord* run = &owner->records[record_index].run;
        uint32_t item;
        for (item = 0u; item < run->item_count; ++item) {
            const float* xy = run->cell_xy + item * 2u;
            int cell_x = crt_float_to_long(xy[0]);
            int cell_y = crt_float_to_long(xy[1]);
            if (cell_x >= 0 && cell_y >= 0)
                scene_render_terrain_cell_overlay_height_fill(
                    NULL, level, (uint32_t)cell_x, (uint32_t)cell_y);
        }
    }
}

static int terrain_cell_quad_vertices_get(
    LevelState* level, const TerrainCellQuadRunEmitHooks* hooks,
    float x, float y, float vertices[4][3])
{
    int cell_x = crt_float_to_long(x);
    int cell_y = crt_float_to_long(y);
    float values[12];
    if (cell_x < 0 || cell_y < 0 || vertices == NULL) return 0;
    if (grid_cell_read_surface_values_level(level, (uint32_t)cell_x,
                                            (uint32_t)cell_y, values)) {
        memcpy(vertices, values, sizeof(values));
        return 1;
    }
    if (hooks->cell_vertices_get != NULL)
        return hooks->cell_vertices_get(hooks->context, x, y, vertices);
    return scene_render_terrain_cell_vertices_get(
        level, (uint32_t)cell_x, (uint32_t)cell_y, vertices);
}

void terrain_cell_quad_run_emit(
    LevelState* level, TerrainCellQuadRunRecord* records,
    uint32_t record_count, const TerrainCellQuadRunEmitHooks* hooks)
{
    static const float texture_coordinates[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    uint32_t record_index;
    if (records == NULL || hooks == NULL ||
        hooks->mesh_create == NULL || hooks->quad_vertices_emit == NULL)
        return;
    for (record_index = 0; record_index < record_count; ++record_index) {
        TerrainCellQuadRunRecord* record = &records[record_index];
        uint32_t item;
        record->height = -10000.0f;
        record->mesh = -1;
        if (record->item_count != 0u && record->cell_xy == NULL) continue;
        for (item = 0; item < record->item_count; ++item) {
            const float* xy;
            float vertices[4][3];
            uint32_t corner;
            xy = record->cell_xy + item * 2u;
            if (!terrain_cell_quad_vertices_get(level, hooks, xy[0], xy[1],
                                                vertices)) continue;
            for (corner = 0; corner < 4u; ++corner) {
                uint32_t cell_type = 0xffffffffu;
                int cell_x = crt_float_to_long(vertices[corner][0]);
                int cell_y = crt_float_to_long(vertices[corner][1]);
                int readable = hooks->cell_type_get != NULL
                    ? hooks->cell_type_get(hooks->context, level, cell_x,
                                           cell_y, &cell_type)
                    : level != NULL && cell_x >= 0 && cell_y >= 0 &&
                      level_cell_type_get_at(level, (uint32_t)cell_x,
                                             (uint32_t)cell_y, &cell_type);
                if (readable && cell_type == 0u) {
                    if (record->height < vertices[corner][2])
                        record->height = vertices[corner][2];
                }
                else {
                    record->flags |= 1u;
                }
            }
        }
        record->mesh = hooks->mesh_create(hooks->context, 3);
        if (record->mesh == -1) continue;
        if (hooks->mesh_visibility_set != NULL)
            hooks->mesh_visibility_set(hooks->context, record->mesh,
                                       (record->flags & 1u) == 0u);
        if (hooks->mesh_texture_scale_set != NULL)
            hooks->mesh_texture_scale_set(hooks->context, record->mesh,
                                         0.3f, 0.8f, 0.6f);
        for (item = 0; item < record->item_count; ++item) {
            const float* xy = record->cell_xy + item * 2u;
            float vertices[4][3];
            float quad_vertices[4][9];
            uint32_t corner;
            if (!terrain_cell_quad_vertices_get(level, hooks, xy[0], xy[1],
                                                vertices)) continue;
            for (corner = 0; corner < 4u; ++corner) {
                quad_vertices[corner][0] = vertices[corner][0];
                quad_vertices[corner][1] = vertices[corner][1];
                quad_vertices[corner][2] = record->height;
                quad_vertices[corner][3] = 0.0f;
                quad_vertices[corner][4] = 0.0f;
                quad_vertices[corner][5] = -1.0f;
                quad_vertices[corner][6] = texture_coordinates[corner][0];
                quad_vertices[corner][7] = texture_coordinates[corner][1];
                quad_vertices[corner][8] = 0.0f;
            }
            hooks->quad_vertices_emit(hooks->context, record->mesh, item,
                                      quad_vertices);
        }
    }
}

int terrain_cell_surface_quad_height_fill(
    TerrainSurfaceGrid* surface, TerrainCellQuadRunRecord* records,
    uint32_t record_count, uint32_t x, uint32_t y,
    const TerrainCellQuadSlotMutationHooks* hooks)
{
    static const uint32_t corner_pair_indices[8] = {
        0u, 1u, 1u, 2u, 2u, 3u, 3u, 0u
    };
    uint32_t record_index;
    for (record_index = 0; records != NULL && record_index < record_count;
         ++record_index) {
        TerrainCellQuadRunRecord* record = &records[record_index];
        uint32_t item;
        if (record->cell_xy == NULL || record->slot_source_items == NULL ||
            record->slot_active == NULL || record->slot_offsets == NULL ||
            record->slot_heights == NULL)
            continue;
        for (item = 0; item < record->selected_count; ++item) {
            uint32_t source_item = record->slot_source_items[item];
            const float* xy;
            uint32_t corners[2];
            float vertices[12];
            float height_a, height_b;
            if (source_item >= record->item_count) continue;
            xy = record->cell_xy + source_item * 2u;
            if (crt_float_to_long(xy[0]) != (int)x ||
                crt_float_to_long(xy[1]) != (int)y) continue;
            if (item >= record->slot_capacity) return 0;
            if (record->slot_orientations != NULL) {
                uint32_t orientation = record->slot_orientations[item];
                uint32_t pair_index;
                if (orientation > 3u) return 0;
                pair_index = orientation * 2u;
                corners[0] = corner_pair_indices[pair_index];
                corners[1] = corner_pair_indices[pair_index + 1u];
            }
            else if (hooks != NULL && hooks->height_corner_pair_get != NULL) {
                if (!hooks->height_corner_pair_get(hooks->context, x, y,
                                                   corners)) return 0;
            }
            else {
                return 0;
            }
            if (corners[0] >= 4u || corners[1] >= 4u) return 0;
            record->flags = (record->flags & ~4u) | 1u;
            if (hooks != NULL && hooks->mesh_visibility_set != NULL)
                hooks->mesh_visibility_set(hooks->context, record->mesh, 0);
            record->slot_active[item] = 1u;
            record->slot_offsets[item * 2u] = 0.0f;
            record->slot_offsets[item * 2u + 1u] = 0.0f;
            if (surface == NULL || surface->surface_vectors == NULL ||
                x >= surface->width || y >= surface->height) return 0;
            terrain_cell_surface_vertices_get(surface, x, y, vertices);
            height_a = vertices[corners[0] * 3u + 2u];
            height_b = vertices[corners[1] * 3u + 2u];
            /* FUN_0046e220 negates both corner heights, selects the larger
             * negated value, then negates it again: the stored value is the
             * higher of the selected corners. */
            record->slot_heights[item] = height_a > height_b
                ? height_a : height_b;
            return 1;
        }
    }
    return 0;
}

static TerrainWorldSpatialRecord* terrain_world_spatial_record_find_at(
    LevelState* level, int x, int y, uint32_t* item_index)
{
    TerrainWorldSpatialRecords* owner;
    uint32_t record_index;
    if (level == NULL || level->ptr_terrain_quad_runs == NULL) return NULL;
    owner = (TerrainWorldSpatialRecords*)level->ptr_terrain_quad_runs;
    if (owner->owner != level) return NULL;
    for (record_index = 0u; record_index < owner->count; ++record_index) {
        TerrainWorldSpatialRecord* record = &owner->records[record_index];
        uint32_t item;
        if (record->candidate_xy == NULL ||
            record->run.slot_source_items == NULL ||
            record->run.slot_orientations == NULL) continue;
        for (item = 0u; item < record->run.selected_count; ++item) {
            uint32_t source_item = record->run.slot_source_items[item];
            uint32_t alternative = record->run.slot_orientations[item];
            const float* xy;
            if (source_item >= record->run.item_count) continue;
            if (alternative > 3u) continue;
            /* The retail lookup applies the selected candidate's cardinal
             * offset to the source run coordinate before comparing it. */
            xy = record->candidate_xy + source_item * 8u + alternative * 2u;
            if (crt_float_to_long(xy[0]) == x &&
                crt_float_to_long(xy[1]) == y) {
                if (item_index != NULL) *item_index = item;
                return record;
            }
        }
    }
    return NULL;
}

/* CellSurfaceQuadHeightFill (FUN_0046e220): find the staged per-level item,
 * activate its CPU slot, reset its offsets, and sample the selected edge.
 * The original scene object's visibility/lock operation remains renderer
 * owned; the record and selector now have a LevelState owner. */
// FUNCTION: LEGORR 0x0046e220
void terrain_cell_surface_quad_height_fill_current(LevelState* level,
                                                    int x, int y)
{
    TerrainWorldSpatialRecord* record;
    TerrainSurfaceGrid* surface;
    uint32_t item = 0u;
    uint32_t orientation;
    uint32_t corner_a, corner_b;
    float vertices[12];
    float first_height, second_height;
    static const uint8_t corner_pairs[8] = {
        0u, 1u, 1u, 2u, 2u, 3u, 3u, 0u
    };
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    record = terrain_world_spatial_record_find_at(level, x, y, &item);
    surface = path_record_surface_grid_get();
    if (record == NULL || item >= record->run.selected_count ||
        item >= record->run.slot_capacity) return;
    record->run.flags = (record->run.flags & ~4u) | 1u;
    record->run.slot_active[item] = 1u;
    record->run.slot_offsets[item * 2u] = 0.0f;
    record->run.slot_offsets[item * 2u + 1u] = 0.0f;
    if (surface != NULL && surface->owner_level != NULL &&
        surface->owner_level != level) surface = NULL;
    if (surface != NULL && surface->surface_vectors != NULL && x >= 0 &&
        y >= 0 && (uint32_t)x < surface->width &&
        (uint32_t)y < surface->height) {
        (void)terrain_cell_surface_vertices_get(surface, (uint32_t)x,
                                                (uint32_t)y, vertices);
    }
    else {
        float world_vertices[4][3];
        uint32_t corner;
        if (!scene_render_terrain_cell_vertices_get(
                level, (uint32_t)x, (uint32_t)y, world_vertices)) return;
        /* SceneRender exposes conventional (X,height,Z), while the raw
         * terrain surface payload consumed by this retail helper is
         * (X,Z,height). */
        for (corner = 0u; corner < 4u; ++corner)
            vertices[corner * 3u + 2u] = world_vertices[corner][1];
    }
    orientation = record->run.slot_orientations[item];
    if (orientation > 3u) return;
    corner_a = corner_pairs[orientation * 2u];
    corner_b = corner_pairs[orientation * 2u + 1u];
    first_height = vertices[corner_a * 3u + 2u];
    second_height = vertices[corner_b * 3u + 2u];
    record->run.slot_heights[item] = second_height > first_height
        ? second_height : first_height;
    /* Publish the active scalar into the scene-side CPU overlay cache. */
    scene_render_terrain_cell_overlay_height_fill(
        NULL, level, (uint32_t)x, (uint32_t)y);
}

/* GridSurfaceEntryReset (FUN_0046e330): clear the indexed item's active
 * field and update the owning staged-record flags. */
void terrain_cell_surface_quad_reset_level(LevelState* level, int x, int y)
{
    TerrainWorldSpatialRecord* record;
    uint32_t item = 0u;
    if (level == NULL) return;
    record = terrain_world_spatial_record_find_at(level, x, y, &item);
    if (record == NULL || item >= record->run.selected_count ||
        item >= record->run.slot_capacity) return;
    record->run.flags = (record->run.flags & ~2u) | 1u;
    record->run.slot_active[item] = 0u;
    scene_render_terrain_cell_overlay_slot_reset(
        level, (uint32_t)x, (uint32_t)y);
}

void terrain_cell_surface_quad_reset_current(int x, int y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    TerrainWorldSpatialRecord* record;
    uint32_t item = 0u;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return;
    record = terrain_world_spatial_record_find_at(level, x, y, &item);
    if (record == NULL || item >= record->run.selected_count ||
        item >= record->run.slot_capacity) return;
    record->run.flags = (record->run.flags & ~2u) | 1u;
    record->run.slot_active[item] = 0u;
    scene_render_terrain_cell_overlay_slot_reset(
        level, (uint32_t)x, (uint32_t)y);
}

int terrain_cell_quad_run_slot_state_get(LevelState* level, int x, int y,
                                         float* height_out,
                                         uint32_t* active_out)
{
    TerrainWorldSpatialRecord* record;
    uint32_t item = 0u;
    if (active_out != NULL) *active_out = 0u;
    record = terrain_world_spatial_record_find_at(level, x, y, &item);
    if (record == NULL || item >= record->run.selected_count) return 0;
    if (height_out != NULL) *height_out = record->run.slot_heights[item];
    if (active_out != NULL)
        *active_out = record->run.slot_active[item] != 0u ? 1u : 0u;
    return 1;
}

static int building_footprint_command_predicate_current(const int cell_xy[2])
{
    LevelState* level;
    uint32_t flags;
    const uint8_t* cell;
    if (cell_xy == NULL) return 0;
    flags = system_state_flags_get();
    if (flags != 0u) {
        flags = system_state_flags_get();
        if ((flags & 8u) == 0u) {
            flags = system_state_flags_get();
            if ((flags & 4u) == 0u) return 1;
        }
    }
    flags = system_state_flags_get();
    if ((flags & 2u) == 0u) return 0;
    level = level_current_owner_get();
    cell = level_voxel(level, cell_xy[0], cell_xy[1]);
    return cell != NULL && cell[4] == 0u;
}

const int* building_footprint_render_and_validate(
    TerrainSurfaceGrid* surface, const int offset[2], const int* source_xy,
    uint32_t count, int rotation, float maximum_height_span,
    float maximum_edge_delta, uint32_t minimum_routes,
    const BuildingFootprintPreviewHooks* hooks)
{
    static const int adjacent[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    const int* cells;
    uint32_t index;
    uint32_t render_count = 0u;
    int adjacent_condition = 0;
    int repeated_adjacent_condition = 0;
    int saw_repeated_cell = 0;
    int route_result = 0;
    int return_cells = 0;
    int surface_valid;
    if (offset == NULL || (count != 0u && source_xy == NULL) ||
        count > 0x1fffffffu)
        return NULL;
    cells = (const int*)rotated_pairs_scratch_fill(offset, source_xy,
                                                   (int)count, rotation);
    if (cells == NULL) return NULL;
    surface_valid = terrain_surface_vertex_span_validate(
        surface, cells, count, maximum_height_span, maximum_edge_delta);
    for (index = 0u; index < count; ++index) {
        uint32_t direction;
        int x = cells[index * 2u];
        int y = cells[index * 2u + 1u];
        int edge_condition = 0;
        int repeated_cell = index + 1u < count &&
            x == cells[(index + 1u) * 2u] &&
            y == cells[(index + 1u) * 2u + 1u];
        for (direction = 0u; direction < 4u; ++direction) {
            const int neighbor_xy[2] = {
                x + adjacent[direction][0], y + adjacent[direction][1]
            };
            int occupied_unflagged;
            if (hooks != NULL &&
                hooks->adjacent_occupied_unflagged != NULL) {
                occupied_unflagged =
                    hooks->adjacent_occupied_unflagged(
                        hooks->context, neighbor_xy[0], neighbor_xy[1]);
            } else {
                occupied_unflagged =
                    level_cell_occupied_at_current(neighbor_xy) != 0u &&
                    terrain_cell_flag_8000_current(neighbor_xy) == 0u;
            }
            if (occupied_unflagged) {
                edge_condition = 1;
            }
        }
        if (edge_condition) adjacent_condition = 1;
        if (repeated_cell) {
            saw_repeated_cell = 1;
            if (edge_condition) repeated_adjacent_condition = 1;
            ++index;
        }
    }
    if (saw_repeated_cell) adjacent_condition = repeated_adjacent_condition;
    for (index = 0; index < count; ++index) {
        int x = cells[index * 2u];
        int y = cells[index * 2u + 1u];
        int in_bounds = surface != NULL && x >= 0 && y >= 0 &&
            (uint32_t)x < surface->width && (uint32_t)y < surface->height;
        int repeated_cell = index + 1u < count &&
            x == cells[(index + 1u) * 2u] &&
            y == cells[(index + 1u) * 2u + 1u];
        int route_limit_hit = repeated_cell &&
            minimum_routes + 1u > render_count;
        int placement_cell_valid;
        int neighbor_valid = 0;
        int blocked = 0;
        float red = 0.0f;
        float green = 0.7f;
        float blue = 0.9f;
        /* FUN_004641c0 invokes PlacementBlockedAdjacentCellScan for every
         * rotated footprint pair. Its final two arguments are the repeated
         * pair marker and the route-count gate computed above. */
        placement_cell_valid =
            unported_ledger_stubs::
                terrain_placement_blocked_adjacent_cell_scan(
                x, y, repeated_cell, route_limit_hit) != 0u;
        neighbor_valid = placement_cell_valid;
        route_result = neighbor_valid;
        blocked = !placement_cell_valid || route_result == 0 ||
            !adjacent_condition ||
            ((hooks != NULL && hooks->terrain_command_predicate != NULL)
                ? hooks->terrain_command_predicate(hooks->context, offset) != 0
                : building_footprint_command_predicate_current(offset) != 0);
        if (blocked) {
            return_cells = 1;
            red = 0.6f;
            green = 0.0f;
            blue = 0.0f;
        }
        else if (route_limit_hit) {
            red = 0.0f;
            green = 0.7f;
            blue = 0.9f;
        }
        else if (repeated_cell) {
            red = 0.7f;
            green = 0.7f;
            blue = 0.0f;
        }
        else if (!surface_valid) {
            return_cells = 1;
            red = 0.7f;
            green = 0.0f;
            blue = 0.7f;
        }
        else {
            red = 0.0f;
            green = 0.7f;
            blue = 0.1f;
        }
        if (hooks != NULL && hooks->preview_color_set != NULL)
            hooks->preview_color_set(hooks->context, index, red, green, blue,
                                     0.4f);
        if (hooks != NULL && hooks->preview_overlay_color_set != NULL)
            hooks->preview_overlay_color_set(hooks->context, index,
                red * 0.5f, green * 0.5f, blue * 0.5f);
        if (hooks != NULL && hooks->cell_preview_emit != NULL)
            hooks->cell_preview_emit(hooks->context, index, x, y, in_bounds,
                                     surface_valid, neighbor_valid);
        ++render_count;
        if (repeated_cell) {
            ++index;
            ++render_count;
        }
    }
    return return_cells ? cells : NULL;
}

/* SurfaceVertexSpanValidate (FUN_0044e8c0): check every distinct cell in a
 * sampled terrain span. Adjacent corner heights must stay within the edge
 * limit; cells carrying the terrain height-check bit must also stay within
 * the requested total height range. */
// FUNCTION: LEGORR 0x0044E8C0
int terrain_surface_vertex_span_validate(TerrainSurfaceGrid* grid,
                                         const int* cell_xy,
                                         uint32_t cell_count,
                                         float maximum_height_span,
                                         float maximum_edge_delta)
{
    static const uint8_t corner_cell_offsets[4][2] = {
        { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
    };
    static const uint8_t edge_corners[6][2] = {
        { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 }, { 2, 3 }
    };
    uint32_t i;
    int have_flagged_height = 0;
    uint32_t record_width = 0u;
    uint32_t record_height = 0u;
    float minimum_height = 10000.0f;
    float maximum_height = -10000.0f;
    if (grid == NULL || (cell_count != 0 && cell_xy == NULL)) return 0;
    (void)terrain_surface_record_bounds_get(grid, &record_width,
                                            &record_height);
    i = 0u;
    while (i < cell_count) {
        float vertices[12];
        uint32_t corner;
        uint32_t edge;
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        /* The corpus advances the pair cursor by two when adjacent entries
         * match, skipping both entries in that duplicate step. */
        if (i + 1u < cell_count &&
            x == cell_xy[(i + 1u) * 2u] &&
            y == cell_xy[(i + 1u) * 2u + 1u]) {
            i += 2u;
            continue;
        }
        if (grid->surface_vectors == NULL || x < 0 || y < 0 ||
            (uint32_t)x >= grid->width || (uint32_t)y >= grid->height)
            return 0;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x, (uint32_t)y,
                                           vertices);
        for (edge = 0; edge < 6; ++edge) {
            float first = vertices[edge_corners[edge][0] * 3];
            float second = vertices[edge_corners[edge][1] * 3];
            float delta = first - second;
            if (delta < 0.0f) delta = -delta;
            if (delta > maximum_edge_delta) return 0;
        }
        if (grid->cell_records != NULL && record_width != 0u) {
            for (corner = 0u; corner < 4u; ++corner) {
                const int record_x = x + corner_cell_offsets[corner][0];
                const int record_y = y + corner_cell_offsets[corner][1];
                size_t record_index;
                float height;
                if (record_x < 0 || record_y < 0 ||
                    (uint32_t)record_x >= record_width ||
                    (uint32_t)record_y >= record_height) continue;
                record_index = (size_t)(uint32_t)record_y *
                    record_width + (uint32_t)record_x;
                if ((grid->cell_records[record_index * 0x28u + 0x19u] &
                     8u) == 0u) continue;
                height = vertices[corner * 3u];
                if (height < minimum_height) minimum_height = height;
                if (height > maximum_height) maximum_height = height;
                have_flagged_height = 1;
            }
        }
        ++i;
    }
    if (have_flagged_height &&
        maximum_height - minimum_height > maximum_height_span) return 0;
    return 1;
}

/* FUN_004514f0: refresh both stored triangle normals from the cell's four
 * surface vectors. */
// FUNCTION: LEGORR 0x004514f0
void terrain_cell_surface_vectors_rebuild(TerrainSurfaceGrid* grid,
                                          uint32_t x, uint32_t y)
{
    const float* vertices;
    float first[3];
    float second[3];
    uint32_t index;
    uint8_t* record;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->surface_vectors == NULL || x >= grid->width ||
        y >= grid->height) return;
    index = terrain_grid_cell_index(grid, x, y);
    vertices = grid->surface_vectors + index * 12u;
    record = grid->cell_records +
        ((size_t)y * terrain_surface_record_width(grid) + x) * 0x28u;
    surface_triangle_normal_calculate(first, vertices, vertices + 3,
                                      vertices + 9);
    surface_triangle_normal_calculate(second, vertices + 3, vertices + 6,
                                      vertices + 9);
    memcpy(record, first, sizeof(first));
    memcpy(record + 0x0c, second, sizeof(second));
}

/* TerrainVoxelNeighborsMutate (FUN_0044f380): move the selected world-space
 * vertex in each adjacent cell's corner table, then rebuild both triangles
 * for the four cells sharing that vertex. */
// FUNCTION: LEGORR 0x0044f380
void terrain_cell_surface_voxel_neighbors_mutate(TerrainSurfaceGrid* grid,
                                                uint32_t x, uint32_t y,
                                                float height_delta)
{
    float corners[8] = { 0.0f, 0.0f, -1.0f, 0.0f,
                         -1.0f, -1.0f, 0.0f, -1.0f };
    float offset[3] = { height_delta, 0.0f, 0.0f };
    uint32_t corner;
    uint32_t cell_y;
    for (corner = 0; corner < 4; ++corner) {
        corners[corner * 2u] += (float)x;
        corners[corner * 2u + 1u] += (float)y;
    }
    for (corner = 0; corner < 4; ++corner) {
        const int cell_x = (int)corners[corner * 2u];
        const int vertex_y = (int)corners[corner * 2u + 1u];
        terrain_cell_surface_vector_offset_add(
            grid, (uint32_t)cell_x, (uint32_t)vertex_y, corner, offset);
    }
    for (cell_y = y - 1u; cell_y <= y; ++cell_y) {
        uint32_t cell_x;
        for (cell_x = x - 1u; cell_x <= x; ++cell_x) {
            terrain_cell_surface_vectors_rebuild(grid, cell_x, cell_y);
        }
    }
}

/* TerrainCellListSurfaceHeightClampMutate (FUN_0044ead0): use unprocessed
 * cell corners to compute one target height, bounded by already-processed
 * cells, then clamp each new corner to the tolerance band around that target. */
// FUNCTION: LEGORR 0x0044ead0
void terrain_cell_surface_height_clamp(TerrainSurfaceGrid* grid,
                                       const int* cell_xy,
                                       uint32_t cell_count,
                                       float maximum_offset)
{
    /* TerrainCellSurfaceVerticesGet returns corners in TerrainGridVertexBuild
     * order: (0,0), (1,0), (1,1), (0,1). The matching record flags must use
     * that same order in both passes. */
    const int corner_offsets[8] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    float staged_vertices[118];
    uint32_t i;
    uint32_t staged_count = 0;
    float half_offset;
    float average = 0.0f;
    float flagged_minimum = 10000.0f;
    float flagged_maximum = -10000.0f;
    uint32_t unprocessed_count = 0;
    int have_flagged_height = 0;
    uint32_t record_width, record_height;
    if (grid == NULL || grid->surface_vectors == NULL ||
        grid->cell_records == NULL || cell_xy == NULL ||
        maximum_offset < 0.0f) return;
    if (!terrain_surface_record_bounds_get(grid, &record_width,
                                           &record_height)) return;
    half_offset = maximum_offset * 0.5f;

    /* The reference stages unique consecutive coordinate pairs, averages
     * corners of cells not already marked with bit 8, and uses processed
     * cells to bound that shared target height. */
    for (i = 0; i < cell_count; ++i) {
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        float* vertices;
        uint32_t corner;
        if (i != 0u && x == cell_xy[(i - 1u) * 2u] &&
            y == cell_xy[(i - 1u) * 2u + 1u]) continue;
        if (x < 0 || y < 0 || (uint32_t)x >= grid->width ||
            (uint32_t)y >= grid->height) continue;
        /* The retail stack has room for nine complete 12-float vertex
         * records. Reject an invalid oversized list before writing past it. */
        if (staged_count >= 9u) return;
        vertices = staged_vertices + staged_count * 12u;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x,
                                          (uint32_t)y, vertices);
        for (corner = 0; corner < 4; ++corner) {
            float height = vertices[corner * 3u];
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            size_t neighbor_index;
            uint8_t flags;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= record_width ||
                (uint32_t)neighbor_y >= record_height) continue;
            neighbor_index = (size_t)neighbor_y * record_width +
                             (uint32_t)neighbor_x;
            flags = grid->cell_records[neighbor_index * 0x28u + 0x19u];
            if ((flags & 8u) != 0u) {
                if (height < flagged_minimum) flagged_minimum = height;
                if (height > flagged_maximum) flagged_maximum = height;
                have_flagged_height = 1;
            }
            else {
                average += height;
                unprocessed_count++;
            }
        }
        staged_count++;
    }
    if (unprocessed_count == 0u) return;
    average /= (float)unprocessed_count;
    if (have_flagged_height) {
        if (flagged_maximum < average + half_offset)
            average = flagged_maximum - half_offset;
        if (average - half_offset < flagged_minimum)
            average = flagged_minimum + half_offset;
    }

    staged_count = 0;
    for (i = 0; i < cell_count; ++i) {
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        float* vertices;
        uint32_t corner;
        if (i != 0u && x == cell_xy[(i - 1u) * 2u] &&
            y == cell_xy[(i - 1u) * 2u + 1u]) continue;
        if (x < 0 || y < 0 || (uint32_t)x >= grid->width ||
            (uint32_t)y >= grid->height) continue;
        if (staged_count >= 9u) return;
        vertices = staged_vertices + staged_count * 12u;
        for (corner = 0; corner < 4; ++corner) {
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            size_t neighbor_index;
            uint8_t* flags;
            float difference = vertices[corner * 3u] - average;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= record_width ||
                (uint32_t)neighbor_y >= record_height) continue;
            neighbor_index = (size_t)neighbor_y * record_width +
                             (uint32_t)neighbor_x;
            flags = &grid->cell_records[neighbor_index * 0x28u + 0x19u];
            if ((*flags & 8u) != 0u) continue;
            if (difference > half_offset || difference < -half_offset) {
                terrain_cell_surface_voxel_neighbors_mutate(
                    grid, (uint32_t)neighbor_x, (uint32_t)neighbor_y,
                    ((difference / difference) * half_offset + average) -
                        vertices[corner * 3u]);
            }
            *flags |= 8u;
        }
        staged_count++;
    }
}

/* TerrainSurfacePointProjectOffset (FUN_0044fb90): select the triangle
 * containing a point and shift along its stored horizontal normal. */
int terrain_surface_point_project_offset(TerrainSurfaceGrid* grid,
                                         float x, float z,
                                         int orientation, float offset,
                                         float* out_x, float* out_z)
{
    if (grid == NULL || grid->owner_level == NULL ||
        grid->owner_level->ptr_terrain == NULL || out_x == NULL ||
        out_z == NULL) return 0;
    /* The level-owned terrain adapter carries the origin, block size, cell
     * corner data, and triangle records that the renderer context exposes at
     * offsets unavailable in TerrainSurfaceGrid. */
    return terrain_state_surface_point_project_offset(
        grid->owner_level->ptr_terrain, x, z, orientation, offset,
        out_x, out_z);
}

void terrain_surface_grid_values_get(TerrainSurfaceGrid* grid, uint32_t x,
                                     uint32_t y, float out[12])
{
    const float* values;
    if (grid == NULL || out == NULL || grid->surface_vectors == NULL ||
        x >= grid->width || y >= grid->height) return;
    values = grid->surface_vectors +
             ((size_t)y * grid->width + x) * 12u;
    /* FUN_00474cc0's selector-zero/four-value read is an inline 12-word
     * payload in this portable surface. Preserve its raw vertex order. */
    memcpy(out, values, 12u * sizeof(float));
}

int grid_cell_read_surface_values_level(LevelState* level, uint32_t x,
                                        uint32_t y, float out[12])
{
    float vertices[4][3];
    if (level == NULL || level->ptr_terrain == NULL || out == NULL ||
        !scene_render_terrain_cell_vertices_get(level, x, y, vertices))
        return 0;
    /* GridCellReadSurfaceValues reads the cell's four mesh vertices. The
     * TerrainSurfaceGrid::surface_vectors array stores the independently
     * computed corner vectors used by TerrainCellNormalSample; returning
     * those 12 words here confused vectors with positions. SceneRender owns
     * the live X/Y/Z quad positions, including terrain edits and offsets. */
    memcpy(out, vertices, sizeof(vertices));
    return 1;
}

// FUNCTION: LEGORR 0x00450060
int grid_cell_read_surface_vector(TerrainSurfaceGrid* grid, uint32_t x,
                                  uint32_t y, float out[3])
{
    uint32_t index;
    uint32_t corner;
    float vertices[4][3];
    if (!terrain_grid_cell_valid(grid, x, y) || out == NULL) return 0;
    index = terrain_grid_cell_index(grid, x, y);
    /* Retail selects the geometry vector using the live cell-record bit, not
     * the optional triangulation cache. */
    corner = terrain_surface_grid_cell_orientation_get(grid, x, y, index)
        ? 1u : 0u;
    if (grid->surface_vectors == NULL) {
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level, x, y,
                                                     vertices)) return 0;
        out[0] = vertices[corner][0];
        out[1] = vertices[corner][2];
        out[2] = vertices[corner][1];
        return 1;
    }
    memcpy(out, grid->surface_vectors + (index * 4 + corner) * 3,
           sizeof(float) * 3);
    return 1;
}

int grid_cell_read_surface_vector_level(LevelState* level, uint32_t x,
                                        uint32_t y, float out[3])
{
    TerrainSurfaceGrid* grid = NULL;
    if (level != NULL && level->ptr_tile_mesh_grid != NULL)
        grid = level->ptr_tile_mesh_grid->surface_grid;
    if (grid == NULL) {
        grid = path_record_surface_grid_get();
        if (grid != NULL && grid->owner_level != level) return 0;
    }
    if (grid == NULL) return 0;
    return grid_cell_read_surface_vector(grid, x, y, out);
}

static int terrain_surface_grid_cell_orientation_get(
    const TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, uint32_t index)
{
    if (grid->cell_records != NULL) {
        const uint32_t stride = terrain_surface_record_width(grid);
        if (x < stride && y < grid->height)
            return (grid->cell_records[((size_t)y * stride + x) * 0x28u +
                                       0x19u] & 1u) != 0u;
    }
    if (grid->terrain_window != NULL) {
        uint32_t width, height, stride;
        const uint8_t* blocks;
        memcpy(&width, grid->terrain_window, sizeof(width));
        memcpy(&height, (const uint8_t*)grid->terrain_window + 4,
               sizeof(height));
        memcpy(&stride, (const uint8_t*)grid->terrain_window + 8,
               sizeof(stride));
        memcpy(&blocks, (const uint8_t*)grid->terrain_window + 0x28,
               sizeof(blocks));
        if (blocks != NULL && x < width && y < height)
            return (blocks[((size_t)y * stride + x) * 0x28u + 0x19u] & 1u)
                != 0u;
    }
    return grid->triangulation_flags != NULL &&
        (grid->triangulation_flags[index] & 1u) != 0u;
}

// FUNCTION: LEGORR 0x00451230
void terrain_cell_surface_vector_write(TerrainSurfaceGrid* grid,
                                       uint32_t x, uint32_t y,
                                       uint32_t corner,
                                       const float vector[3])
{
    uint32_t index;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        vector == NULL || corner > 3) return;
    index = terrain_grid_cell_index(grid, x, y);
    if (terrain_surface_grid_cell_orientation_get(grid, x, y, index))
        corner = (corner + 1u) & 3u;
    memcpy(grid->surface_vectors + (index * 4 + corner) * 3, vector,
           sizeof(float) * 3);
}

/* FUNCTION: LEGORR 0x00450f30
 * Build the four corner vectors from the surrounding four cells. The input
 * window is the corpus terrain window used by 0x451180/0x4512d0; the output
 * grid is the port's render-vector storage for the same terrain. */
void terrain_cell_surface_vectors_initialize(TerrainSurfaceGrid* grid,
                                             uint32_t x, uint32_t y)
{
    static const int32_t offsets[4][2] = {
        { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, -1 }
    };
    uint32_t width, height, stride;
    const uint8_t* blocks = NULL;
    float samples[4][3];
    uint32_t sample;
    uint32_t corner;

    const void* terrain_window;
    if (grid == NULL) return;
    terrain_window = grid->terrain_window;
    if (terrain_window == NULL) return;
    memcpy(&width, terrain_window, sizeof(width));
    memcpy(&height, (const uint8_t*)terrain_window + 4, sizeof(height));
    memcpy(&stride, (const uint8_t*)terrain_window + 8, sizeof(stride));
    memcpy(&blocks, (const uint8_t*)terrain_window + 0x28, sizeof(blocks));
    if (blocks == NULL || x >= width || y >= height) return;

    for (sample = 0; sample < 4; ++sample) {
        const uint32_t sx = x + (uint32_t)offsets[sample][0];
        const uint32_t sy = y + (uint32_t)offsets[sample][1];
        if (sx < width && sy < height) {
            const uint8_t* cell = blocks + ((size_t)sy * stride + sx) * 0x28u;
            uint32_t source_corner = sample;
            const float* first = (const float*)cell;
            const float* second = (const float*)(cell + 0x0c);
            if ((cell[0x19] & 1u) != 0u)
                source_corner = (source_corner + 1u) & 3u;
            if (source_corner == 0u) {
                samples[sample][0] = first[0];
                samples[sample][1] = first[1];
                samples[sample][2] = first[2];
            } else if (source_corner == 2u) {
                samples[sample][0] = second[0];
                samples[sample][1] = second[1];
                samples[sample][2] = second[2];
            } else {
                float length;
                samples[sample][0] = first[0] + second[0];
                samples[sample][1] = first[1] + second[1];
                samples[sample][2] = first[2] + second[2];
                length = (float)sqrt(samples[sample][0] * samples[sample][0]
                                   + samples[sample][1] * samples[sample][1]
                                   + samples[sample][2] * samples[sample][2]);
                samples[sample][0] /= length;
                samples[sample][1] /= length;
                samples[sample][2] /= length;
            }
        } else {
            samples[sample][0] = 0.0f;
            samples[sample][1] = 0.0f;
            samples[sample][2] = 0.0f;
        }
    }

    for (corner = 0; corner < 4; ++corner) {
        float result[3] = { 0.0f, 0.0f, 0.0f };
        uint32_t sx, sy;
        uint32_t out_x = x + (uint32_t)offsets[corner][0];
        uint32_t out_y = y + (uint32_t)offsets[corner][1];
        for (sample = 0; sample < 4; ++sample) {
            sx = x + (uint32_t)offsets[sample][0];
            sy = y + (uint32_t)offsets[sample][1];
            if (!cell_variant_entry_match_check(terrain_window, out_x, out_y,
                                                sx, sy)) {
                result[0] += samples[sample][0];
                result[1] += samples[sample][1];
                result[2] += samples[sample][2];
            }
        }
        {
            const float inverse_length = 1.0f / (float)sqrt(
                result[0] * result[0] + result[1] * result[1]
                + result[2] * result[2]);
            result[0] *= inverse_length;
            result[1] *= inverse_length;
            result[2] *= inverse_length;
        }
        terrain_cell_surface_vector_write(grid, x, y, corner, result);
    }
}

// FUNCTION: LEGORR 0x00451450
void terrain_cell_surface_vector_offset_add(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            uint32_t corner,
                                            const float offset[3])
{
    uint32_t index;
    float* vector;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        offset == NULL || corner > 3) return;
    index = terrain_grid_cell_index(grid, x, y);
    if (terrain_surface_grid_cell_orientation_get(grid, x, y, index))
        corner = (corner + 1u) & 3u;
    vector = grid->surface_vectors + (index * 4 + corner) * 3;
    vector[0] += offset[0];
    vector[1] += offset[1];
    vector[2] += offset[2];
}

// FUNCTION: LEGORR 0x004507d0
int terrain_cell_edge_vector_query(TerrainSurfaceGrid* grid, uint32_t x,
                                   uint32_t y, float out[3])
{
    uint32_t cell_x = x;
    uint32_t cell_y = y;
    uint32_t corner = 0;
    uint32_t index;
    if (grid == NULL || out == NULL || grid->width == 0 || grid->height == 0 ||
        x > grid->width || y > grid->height) return 0;
    if (x == grid->width) {
        --cell_x;
        corner = 1;
    }
    if (y == grid->height) {
        --cell_y;
        corner = (x == grid->width) ? 2u : 3u;
    }
    if (!terrain_grid_cell_valid(grid, cell_x, cell_y) ||
        grid->surface_vectors == NULL) return 0;
    index = terrain_grid_cell_index(grid, cell_x, cell_y);
    if (terrain_surface_grid_cell_orientation_get(grid, cell_x, cell_y,
                                                  index)) {
        corner = (corner + 1u) & 3u;
    }
    memcpy(out, grid->surface_vectors + (index * 4 + corner) * 3,
           sizeof(float) * 3);
    return 1;
}

// FUNCTION: LEGORR 0x00450700
void grid_surface_write_at_xy(TerrainSurfaceGrid* grid, int x, int y,
                              uint8_t value)
{
    uint32_t index;
    if (grid == NULL || grid->surface_values == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= grid->width || (uint32_t)y >= grid->height)
        return;
    index = (uint32_t)y * grid->width + (uint32_t)x;
    /* FUN_00450700 delegates this exact index to the surface-value setter. */
    grid->surface_values[index] = value;
    /* The retail setter also toggles the same indexed terrain mesh record
     * that TerrainCreate enabled for each quad. */
    if (grid->owner_level != NULL)
        terrain_detail_enable_set(grid->owner_level->ptr_terrain, index,
                                  value != 0u);
}

/* FUN_00450600: clear and queue every in-bounds cell in the inclusive circle.
 * The callback runs after the surface value is cleared but before the
 * coordinate is appended, matching the reference's observable order. */
// FUNCTION: LEGORR 0x00450600
void terrain_surface_clear_circle(TerrainSurfaceGrid* grid, int center_x,
                                  int center_y, int radius,
                                  TerrainGridCircleCellCallback callback)
{
    if (grid == NULL || grid->surface_values == NULL || radius < 0 ||
        radius > 32767 || center_x < INT_MIN + radius ||
        center_x > INT_MAX - radius || center_y < INT_MIN + radius ||
        center_y > INT_MAX - radius)
        return;
    {
        int y_offset;
        const int radius_squared = radius * radius;
        for (y_offset = -radius; y_offset <= radius; ++y_offset) {
            int x_offset;
            for (x_offset = -radius; x_offset <= radius; ++x_offset) {
                const int x = center_x + x_offset;
                const int y = center_y + y_offset;
                if (x_offset * x_offset + y_offset * y_offset >
                    radius_squared || x < 0 || y < 0 ||
                    (uint32_t)x >= grid->width ||
                    (uint32_t)y >= grid->height) continue;

                grid_surface_write_at_xy(grid, x, y, 0);
                if (callback != NULL) callback(x, y);
                if (grid->changed_count < TERRAIN_GRID_CHANGE_CAPACITY) {
                    grid->changed_xy[grid->changed_count][0] =
                        (int16_t)(x_offset + center_x);
                    grid->changed_xy[grid->changed_count][1] =
                        (int16_t)(y_offset + center_y);
                    ++grid->changed_count;
                }
            }
        }
    }
}

/* The terrain context's block-size float is the dword at +0x10. */
// FUNCTION: LEGORR 0x00450890
float terrain_context_scale_get(const void* context)
{
    return *(const float*)((const uint8_t*)context + 0x10);
}

// FUNCTION: LEGORR 0x00450730
void terrain_cell_change_queue_append(TerrainSurfaceGrid* grid, int x, int y)
{
    if (grid == NULL) return;
    grid_surface_write_at_xy(grid, x, y, 0);
    /* Retail writes the surface value before appending the 16-bit pair. */
    if (grid->changed_count >= TERRAIN_GRID_CHANGE_CAPACITY) return;
    grid->changed_xy[grid->changed_count][0] = (int16_t)x;
    grid->changed_xy[grid->changed_count][1] = (int16_t)y;
    ++grid->changed_count;
}

// FUNCTION: LEGORR 0x00450780
void terrain_cell_change_queue_flush(TerrainSurfaceGrid* grid)
{
    uint32_t i;
    if (grid == NULL) return;
    for (i = 0; i < grid->changed_count; ++i) {
        grid_surface_write_at_xy(grid, grid->changed_xy[i][0],
                                 grid->changed_xy[i][1], 1);
    }
    grid->changed_count = 0;
}

namespace unported_ledger_stubs {

// LoadTileMeshes
int ledger_stub_0042b330_LoadTileMeshes(LevelState* level,
                                        const char* bitmap_prefix)
{
    return terrain_tile_mesh_grid_load_level(level, bitmap_prefix);
}

// TextureSetConfigLoad
int ledger_stub_0042bb60_TextureSetConfigLoad(LevelState* level,
                                              const char* group_name)
{
    return terrain_texture_set_config_load_level(level, group_name);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

int ledger_stub_0044fb90_TerrainSurfacePointProjectOffset(
    TerrainSurfaceGrid* grid, float x, float z, int orientation,
    float offset, float* out_x, float* out_z)
{
    return terrain_surface_point_project_offset(grid, x, z, orientation,
                                                offset, out_x, out_z);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TerrainViewTileGeometryBuild
/* Adapter only: TerrainViewTileGeometryBuild (FUN_0045dbc0) consumes global
 * render-view and terrain state. The explicit grid callback seam is not its
 * zero-argument ABI or complete geometry owner. */
void ledger_stub_0045dbc0_TerrainViewTileGeometryBuild(TerrainViewTileGrid* grid)
{
    terrain_view_tile_geometry_build(grid);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

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
int ledger_stub_00463590_TerrainCellSurfaceNormalGet(uint32_t x, uint32_t y,
                                                      float normal[3])
{
    return terrain_cell_surface_normal_get(x, y, normal);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

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


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void ledger_stub_0046db90_CellQuadRunStage(void* scene, LevelState* level)
{
    terrain_cell_quad_run_stage_level(scene, level);
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

void ledger_stub_0046e220_CellSurfaceQuadHeightFill(
    void* level_context, int x, int y)
{
    terrain_cell_surface_quad_height_fill_current(
        (LevelState*)level_context, x, y);
}

void ledger_stub_0046e330_GridSurfaceEntryReset(int x, int y)
{
    terrain_cell_surface_quad_reset_current(x, y);
}

// StagedCellCandidateCollect
void* ledger_stub_0046e9d0_StagedCellCandidateCollect(
    unsigned int row, unsigned int first_cell, unsigned int end_cell)
{
    return staged_cell_candidate_collect(row, first_cell, end_cell);
}


// FUNCTION: LEGORR 0x00431b20
uint32_t terrain_placement_blocked_adjacent_cell_scan(
    int x, int y, int mode, int duplicate_pair)
{
    static const int offset[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint8_t* cell;
    uint32_t flags;
    uint8_t type;
    void* descriptor;
    uint32_t index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0u;
    cell = level_voxel(level, x, y);
    if (cell == NULL || (cell[0x0c] & 0x20u) != 0u) return 0u;
    flags = *(uint32_t*)(cell + 0x08);
    type = cell[0x02];

    descriptor = path_record_last_peek();
    while (descriptor != NULL) {
        const uint32_t* words = (const uint32_t*)descriptor;
        if (words[3] == (uint32_t)x && words[4] == (uint32_t)y)
            return 0u;
        descriptor = *(void**)((uint8_t*)descriptor + 0x24);
    }
    if ((flags & 0x20000000u) != 0u) return 0u;
    if (mode != 0 && duplicate_pair != 0 && type != 9u) return 0u;
    if (mode != 0 && (flags & 0x8000u) != 0u) return 0u;
    if ((duplicate_pair == 0 && type == 9u) || type == 6u) return 0u;
    if ((flags & 8u) == 0u || (flags & 0x200000u) == 0u ||
        (flags & 0x108400u) != 0u) return 0u;
    if (grid_cell_record_present_check(x, y) != 0) return 0u;

    for (index = 0u; index < 4u; ++index) {
        uint8_t* neighbor = level_voxel(
            level, x + offset[index][0], y + offset[index][1]);
        if (neighbor == NULL || neighbor[0x02] == 11u) return 0u;
    }
    return 1u;
}

/* Adapter only: RenderQualityConfigApply (FUN_004334f0) has no explicit
 * arguments; it reads the global config and render device state. This port
 * seam accepts parsed settings and does not provide those global owners. */
int ledger_stub_004334f0_RenderQualityConfigApply(
    const RenderQualitySettings* settings)
{
    return render_quality_config_apply(settings);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity);
}

/* EntityNeighborBlockedCellStepProbe (FUN_00433c10): the retail ABI is an
 * entity, a float cadence divisor, and a mode word. The first branch tests
 * the four adjacent voxel flag words and raises the route-step response on
 * the first cell with bit 0x10 set and bit 8 clear. */
// FUNCTION: LEGORR 0x00433c10
int entity_neighbor_blocked_cell_step_probe(EntitySlot* entity,
                                             float cadence, int mode)
{
    static const int offset[4][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }
    };
    GameContext* game;
    LevelState* level;
    int cell_x = -1;
    int cell_y = -1;
    uint32_t state_flags;
    int i;
    if (entity == NULL) return 0;
    state_flags = entity_state_flags_get(entity);
    game = entity_pool_callback_context_get();
    level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0;

    if ((state_flags & 0x1000u) != 0u &&
        (entity->flags_3e0 & 1u) == 0u &&
        !entity_flags_mask107c33de_or_1c00203_any_test(entity) &&
        cadence != 0.0f) {
        uint32_t divisor = (uint32_t)(int)cadence;
        if (divisor != 0u &&
            (uint32_t)level_random_draw15() % divisor == 0u) {
            entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
            if (!level_coord_pair_within_bounds(level, cell_x, cell_y))
                return 0;
            for (i = 0; i < 4; ++i) {
                int x = cell_x + offset[i][0];
                int y = cell_y + offset[i][1];
                uint8_t* cell;
                uint32_t flags;
                if (x < 0 || y < 0 ||
                    (uint32_t)x >= level->word[5] - 1u ||
                    (uint32_t)y >= level->word[6] - 1u) continue;
                cell = level_voxel(level, x, y);
                if (cell == NULL) continue;
                memcpy(&flags, cell + 8, sizeof(flags));
                if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                    entity->field_340 = -1.0f;
                    unported_ledger_stubs::
                        ledger_stub_00449700_RouteStepCommandIssue(entity);
                    return 1;
                }
            }
        }
    }

    /* The other retail branch calls EntityRandomCellStepRouteIssue after its
     * own cadence gate. Its route search needs the renderer-owned surface
     * sampler and path-marker allocator, neither of which has a live adapter
     * in this module; leave that action inert until those owners are bound. */
    if ((mode != 0 || (state_flags & 0x400u) != 0u) &&
        (entity->flags_3e0 & 1u) == 0u &&
        !entity_flags_mask107c33de_or_1c00203_any_test(entity) &&
        cadence != 0.0f) {
        uint32_t divisor = (uint32_t)(int)cadence;
        if (mode != 0 || (divisor != 0u &&
                          (uint32_t)level_random_draw15() % divisor == 0u)) {
            /* See the dependency note above. */
        }
    }
    return 0;
}

namespace unported_ledger_stubs {

/* Adapter only: TerrainProjectedCellStateRefresh (FUN_00433e80) is a
 * zero-argument routine over global view and terrain state. The explicit
 * state/hooks model below has a different ABI and incomplete render owners. */
uint32_t ledger_stub_00433e80_TerrainProjectedCellStateRefresh(
    const TerrainProjectedCellRefreshState* state,
    const TerrainProjectedCellRefreshHooks* hooks)
{
    return terrain_projected_cell_state_refresh(state, hooks);
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


// ProcessQueuedGridAnimations
void ledger_stub_00437300_ProcessQueuedGridAnimations(
    void* anchor, int mode, float delta)
{
    process_queued_grid_animations(anchor, mode, delta);
}


/* Retain the earlier port-facing mesh-grid adapter for internal callers. */
int ledger_stub_0040b310_LoadTileMeshes(TerrainTileMeshGrid* grid,
                                        int editor_mode)
{
    if (grid == NULL) return 0;
    grid->editor_mode = editor_mode;
    return terrain_tile_mesh_grid_load(grid, grid->bitmap_prefix);
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


} // namespace unported_ledger_stubs
