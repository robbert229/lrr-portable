// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "terrain_grid.hpp"
#include "level.hpp"
#include "game.hpp"
#include "entity.hpp"
#include "../crt/crt.hpp"
#include "../gods98/heap.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/view.hpp"

#include <stdlib.h>

static void* s_map_cell_record_grid;
static void* s_map_cell_record_level;
static TerrainCellInBoundsHook s_terrain_cell_in_bounds_hook;
static TerrainCellStateUpdateHook s_terrain_cell_state_update_hook;
static void* s_terrain_cell_neighbor_context;
typedef struct GridCellObjectNode {
    void* object;
    int x;
    int y;
    struct GridCellObjectNode* next_cell;
    struct GridCellObjectNode* next_free;
} GridCellObjectNode;
typedef struct GridCellObjectChunk {
    struct GridCellObjectChunk* next;
    GridCellObjectNode nodes[32];
} GridCellObjectChunk;
static void** s_grid_cell_object_heads;
static GridCellObjectChunk* s_grid_cell_object_chunks;
static GridCellObjectNode* s_grid_cell_object_free;
static GridCellObjectCoordinatesHook s_grid_object_coordinates_hook;
static GridCellObjectFlagHook s_grid_object_flag_hook;
static void* s_grid_cell_object_context;
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
static FenceSparkCellCenterHook s_fence_spark_center_hook;
static FenceSparkHeightHook s_fence_spark_height_hook;
static FenceSparkEntityRadiusHook s_fence_spark_radius_hook;
static FenceSparkFinalizeHook s_fence_spark_finalize_hook;
static void* s_fence_spark_context;
static TerrainVariantCellResolveHook s_terrain_variant_resolve_hook;
static TerrainVariantRandomHook s_terrain_variant_random_hook;
static TerrainVariantCellStateHook s_terrain_variant_cell_state_hook;
static TerrainVariantOperationHook s_terrain_variant_operation_hook;
static void* s_terrain_variant_context;
static TerrainPathGridSizeHook s_terrain_path_grid_size_hook;
static TerrainPathRouteHook s_terrain_path_route_hook;
static TerrainPathHalfCostHook s_terrain_path_half_cost_hook;
static void* s_terrain_path_context;
static TerrainFenceSparkEntityPositionHook s_fence_spark_entity_position_hook;
static TerrainFenceSparkCellLifetimeHook s_fence_spark_lifetime_hook;
static TerrainFenceSparkEligibilityHook s_fence_spark_eligibility_hook;
static TerrainFenceSparkDurationHook s_fence_spark_duration_hook;
static void* s_fence_spark_interaction_context;

/* MapCellRecordGridInitialize (FUN_0040cdc0): allocate the fixed five-word
 * cell record array using the level dimensions at +0x14/+0x18. */
// FUNCTION: LEGORR 0x0040CDC0
void map_cell_record_grid_initialize(void* level)
{
    GridCellObjectChunk* chunk;
    uint32_t width;
    uint32_t height;
    uint32_t count;
    size_t bytes;
    if (s_map_cell_record_grid != NULL) {
        free(s_map_cell_record_grid);
        s_map_cell_record_grid = NULL;
    }
    if (s_grid_cell_object_heads != NULL) {
        free(s_grid_cell_object_heads);
        s_grid_cell_object_heads = NULL;
    }
    s_grid_cell_object_free = NULL;
    for (chunk = s_grid_cell_object_chunks; chunk != NULL; chunk = chunk->next) {
        int node_index;
        for (node_index = 0; node_index < 32; ++node_index) {
            memset(&chunk->nodes[node_index], 0,
                   sizeof(chunk->nodes[node_index]));
            chunk->nodes[node_index].next_free = s_grid_cell_object_free;
            s_grid_cell_object_free = &chunk->nodes[node_index];
        }
    }
    s_map_cell_record_level = level;
    if (level == NULL) {
        return;
    }
    width = *(const uint32_t*)((const unsigned char*)level + 0x14);
    height = *(const uint32_t*)((const unsigned char*)level + 0x18);
    if (height != 0u && width > 0x7fffffffu / height) {
        return;
    }
    count = width * height;
    if (count > 0x7fffffffu / 0x14u) {
        return;
    }
    bytes = (size_t)count * 0x14u;
    s_map_cell_record_grid = malloc(bytes);
    if (s_map_cell_record_grid != NULL) {
        memset(s_map_cell_record_grid, 0, bytes);
        s_grid_cell_object_heads = (void**)calloc(count, sizeof(void*));
    }
}

/* MapCellStateBufferRebuild (FUN_0040ce80): release prior marker resources,
 * then rebuild the level's per-cell state records. */
// FUNCTION: LEGORR 0x0040CE80
void map_cell_state_buffer_rebuild(void* level)
{
    selection_marker_resources_release();
    map_cell_record_grid_initialize(level);
}

void* map_cell_record_grid_get(void)
{
    return s_map_cell_record_grid;
}

void* map_cell_record_grid_level_get(void)
{
    return s_map_cell_record_level;
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
    if (s_terrain_cell_state_update_hook == NULL) {
        return;
    }
    s_terrain_cell_state_update_hook(s_map_cell_record_level, x, y,
                                     s_terrain_cell_neighbor_context);
    for (index = 0; index < 4; ++index) {
        int nx = x + offsets[index][0];
        int ny = y + offsets[index][1];
        if (s_terrain_cell_in_bounds_hook != NULL
            && s_terrain_cell_in_bounds_hook(s_map_cell_record_level, nx, ny,
                    s_terrain_cell_neighbor_context) != 0) {
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

// FUNCTION: LEGORR 0x0040D030
void terrain_record_pool_chunk_allocate(void)
{
    GridCellObjectChunk* chunk;
    int index;
    chunk = (GridCellObjectChunk*)malloc(sizeof(GridCellObjectChunk));
    if (chunk == NULL) {
        return;
    }
    chunk->next = s_grid_cell_object_chunks;
    s_grid_cell_object_chunks = chunk;
    for (index = 0; index < 32; ++index) {
        chunk->nodes[index].next_free = s_grid_cell_object_free;
        s_grid_cell_object_free = &chunk->nodes[index];
    }
}

void terrain_variant_update_hooks_set(TerrainVariantCellResolveHook resolve,
                                      TerrainVariantRandomHook random_value,
                                      TerrainVariantCellStateHook cell_state,
                                      TerrainVariantOperationHook operation,
                                      void* context)
{
    s_terrain_variant_resolve_hook = resolve;
    s_terrain_variant_random_hook = random_value;
    s_terrain_variant_cell_state_hook = cell_state;
    s_terrain_variant_operation_hook = operation;
    s_terrain_variant_context = context;
}

/* TerrainCellVariantUpdateQueue (FUN_0043abf0). Operation ids: 1 is the
 * 1-in-5 cell variant adjustment, 2 creates the cell marker, 3 applies
 * surface type 0x3f, 4 submits the terrain update, and 5 dispatches the
 * trailing deferred callback. */
// FUNCTION: LEGORR 0x0043ABF0
void terrain_cell_variant_update_queue(void* entity)
{
    int x = 0, y = 0;
    int have_cell;
    if (entity == NULL || s_terrain_variant_operation_hook == NULL) return;
    have_cell = s_terrain_variant_resolve_hook != NULL
        && s_terrain_variant_resolve_hook(entity, &x, &y,
                                          s_terrain_variant_context);
    if (have_cell && s_terrain_variant_random_hook != NULL
        && s_terrain_variant_random_hook(s_terrain_variant_context) % 5 == 0) {
        s_terrain_variant_operation_hook(1, entity, x, y, 3, 1,
                                         s_terrain_variant_context);
    }
    if (have_cell && s_terrain_variant_cell_state_hook != NULL
        && s_terrain_variant_cell_state_hook(x, y,
                                             s_terrain_variant_context)) {
        s_terrain_variant_operation_hook(2, entity, x, y, 0, 0,
                                         s_terrain_variant_context);
        s_terrain_variant_operation_hook(3, entity, x, y, 0x3f, 0,
                                         s_terrain_variant_context);
        s_terrain_variant_operation_hook(4, entity, x, y, 0, 0,
                                         s_terrain_variant_context);
    }
    s_terrain_variant_operation_hook(5, entity, x, y, 0, 0,
                                     s_terrain_variant_context);
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

/* The reference path routines use the live map dimensions and call the
 * terrain route predicate for every candidate. Keep optional test/adapter
 * overrides, but make the gameplay path work from the active game context. */
static GameContext* terrain_path_game_context_get(void* entity)
{
    if (entity != NULL && ((EntitySlot*)entity)->owner_ctx != NULL)
        return ((EntitySlot*)entity)->owner_ctx;
    return entity_pool_callback_context_get();
}

static int terrain_path_grid_size_get(void* entity, uint32_t* width,
                                      uint32_t* height)
{
    GameContext* game;
    if (s_terrain_path_grid_size_hook != NULL)
        return s_terrain_path_grid_size_hook(entity, width, height,
                                             s_terrain_path_context);
    game = terrain_path_game_context_get(entity);
    if (game == NULL || game->level == NULL || width == NULL ||
        height == NULL) return 0;
    *width = (uint32_t)level_width(game->level);
    *height = (uint32_t)level_height(game->level);
    return *width != 0u && *height != 0u;
}

static int terrain_path_route_accept(void* entity, uint32_t x, uint32_t y,
                                     int direction)
{
    static const int dx[4] = { -1, 0, 1, 0 };
    static const int dy[4] = { 0, 1, 0, -1 };
    GameContext* game;
    if (s_terrain_path_route_hook != NULL)
        return s_terrain_path_route_hook(entity, x, y, direction,
                                         s_terrain_path_context);
    game = terrain_path_game_context_get(entity);
    if (game == NULL || game->level == NULL || direction < 0 ||
        direction >= 4) return 0;
    return terrain_cell_route_and_occupancy_check(
        (const EntitySlot*)entity, game->level, (int)x, (int)y,
        (int)x + dx[direction], (int)y + dy[direction], 0) != 0;
}

static int terrain_path_cell_accept(void* entity, uint32_t x, uint32_t y)
{
    GameContext* game;
    if (s_terrain_path_route_hook != NULL)
        return s_terrain_path_route_hook(entity, x, y, 0,
                                         s_terrain_path_context);
    game = terrain_path_game_context_get(entity);
    if (game == NULL || game->level == NULL) return 0;
    return terrain_cell_route_and_occupancy_check(
        (const EntitySlot*)entity, game->level, (int)x, (int)y,
        (int)x, (int)y, 0) != 0;
}

static int terrain_path_half_cost_accept(void* entity, uint32_t x,
                                         uint32_t y, int direction)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    GameContext* game;
    int probe_x;
    int probe_y;
    if (s_terrain_path_half_cost_hook != NULL)
        return s_terrain_path_half_cost_hook(entity, x, y, direction,
                                             s_terrain_path_context);
    game = terrain_path_game_context_get(entity);
    if (game == NULL || game->level == NULL || direction < 0 ||
        direction >= 4) return 0;
    probe_x = (int)x + dx[direction];
    probe_y = (int)y + dy[direction];
    return level_cell_occupied_at(game->level, probe_x, probe_y) != 0;
}

// FUNCTION: LEGORR 0x00440C70
int terrain_grid_path_search(void* level, uint32_t start_x,
                             uint32_t start_y, uint32_t goal_x,
                             uint32_t goal_y, int** out_x, int** out_y,
                             uint32_t* out_count, TerrainPathStopHook stop,
                             void* user)
{
    static const int dx[4] = { -1, 0, 1, 0 };
    static const int dy[4] = { 0, 1, 0, -1 };
    uint32_t width, height, cell_count, start, goal;
    uint32_t* depth;
    uint32_t* queue;
    uint32_t read_index, write_index;
    uint32_t found = 0;
    int* route_x;
    int* route_y;
    uint32_t length, cell, step;
    if (out_x == NULL || out_y == NULL || out_count == NULL
        || !terrain_path_grid_size_get(level, &width, &height)
        || width == 0 || height == 0 || width > 65535u || height > 65535u
        || width > 16777216u / height
        || start_x >= width || start_y >= height) return 0;
    cell_count = width * height;
    if (stop == NULL) {
        if (goal_x >= width || goal_y >= height
            || !terrain_path_cell_accept(level, goal_x, goal_y)) return 0;
    }
    depth = (uint32_t*)calloc((size_t)cell_count, sizeof(uint32_t));
    queue = (uint32_t*)malloc((size_t)cell_count * sizeof(uint32_t));
    if (depth == NULL || queue == NULL) {
        free(depth);
        free(queue);
        return 0;
    }
    start = start_y * width + start_x;
    goal = 0;
    depth[start] = 1;
    queue[0] = start;
    read_index = 0;
    write_index = 1;
    while (read_index < write_index && found == 0) {
        uint32_t current = queue[read_index++];
        uint32_t x = current % width;
        uint32_t y = current / width;
        int direction;
        for (direction = 0; direction < 4; ++direction) {
            int nx = (int)x + dx[direction];
            int ny = (int)y + dy[direction];
            uint32_t next;
            uint32_t candidate[2];
            if (nx < 0 || ny < 0 || (uint32_t)nx >= width
                || (uint32_t)ny >= height) continue;
            next = (uint32_t)ny * width + (uint32_t)nx;
            if (depth[next] != 0
                || !terrain_path_route_accept(level, (uint32_t)nx,
                                              (uint32_t)ny, direction)) continue;
            depth[next] = depth[current] + 1;
            queue[write_index++] = next;
            candidate[0] = (uint32_t)nx;
            candidate[1] = (uint32_t)ny;
            if (stop != NULL) {
                if (stop(level, candidate, user)) {
                    goal = next;
                    found = 1;
                    break;
                }
            }
            else if ((uint32_t)nx == goal_x && (uint32_t)ny == goal_y) {
                goal = next;
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        free(depth);
        free(queue);
        return 0;
    }
    length = depth[goal];
    route_x = (int*)malloc((size_t)length * sizeof(int));
    route_y = (int*)malloc((size_t)length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(depth);
        free(queue);
        return 0;
    }
    cell = goal;
    for (step = length; step > 0; --step) {
        uint32_t x = cell % width;
        uint32_t y = cell / width;
        int direction;
        route_x[step - 1] = (int)x;
        route_y[step - 1] = (int)y;
        if (step == 1) break;
        for (direction = 0; direction < 4; ++direction) {
            int px = (int)x + dx[direction];
            int py = (int)y + dy[direction];
            if (px >= 0 && py >= 0 && (uint32_t)px < width
                && (uint32_t)py < height
                && depth[(uint32_t)py * width + (uint32_t)px]
                    == depth[cell] - 1) {
                cell = (uint32_t)py * width + (uint32_t)px;
                break;
            }
        }
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = length;
    free(depth);
    free(queue);
    return 1;
}

/* TerrainNeighborPathCheck (FUN_004410f0): relax cardinal routes in FIFO
 * order. A qualifying cell one step beyond a neighbor makes that edge cost
 * 0.5; otherwise it costs 1.0. */
// FUNCTION: LEGORR 0x004410F0
int terrain_neighbor_path_check(void* level, uint32_t start_x,
                                uint32_t start_y, uint32_t goal_x,
                                uint32_t goal_y, int** out_x, int** out_y,
                                uint32_t* out_count, TerrainPathStopHook stop,
                                void* user)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    uint32_t width, height, cell_count, start, goal;
    float* cost;
    uint32_t* queue;
    uint8_t* queued;
    uint32_t read_index = 0, write_index = 0;
    uint32_t queue_count = 0;
    uint32_t i;
    int found = 0;
    int target_reached = 0;
    int* reverse_x;
    int* reverse_y;
    int* route_x;
    int* route_y;
    uint32_t length = 0;
    uint32_t cell;
    if (out_x == NULL || out_y == NULL || out_count == NULL ||
        !terrain_path_grid_size_get(level, &width, &height) ||
        width == 0 || height == 0 || width > 65535u || height > 65535u ||
        width > 16777216u / height || start_x >= width || start_y >= height)
        return 0;
    if (stop == NULL && (goal_x >= width || goal_y >= height ||
        !terrain_path_cell_accept(level, goal_x, goal_y))) return 0;
    cell_count = width * height;
    cost = (float*)malloc((size_t)cell_count * sizeof(float));
    queue = (uint32_t*)malloc((size_t)cell_count * sizeof(uint32_t));
    queued = (uint8_t*)calloc((size_t)cell_count, sizeof(uint8_t));
    if (cost == NULL || queue == NULL || queued == NULL) {
        free(cost);
        free(queue);
        free(queued);
        return 0;
    }
    for (i = 0; i < cell_count; ++i) cost[i] = 3.402823466e+38F;
    start = start_y * width + start_x;
    goal = 0;
    cost[start] = 0.0f;
    queue[write_index] = start;
    write_index = (write_index + 1u) % cell_count;
    queue_count = 1;
    queued[start] = 1;
    while (queue_count != 0u && !found) {
        uint32_t current;
        uint32_t x, y;
        int direction;
        current = queue[read_index];
        read_index = (read_index + 1u) % cell_count;
        --queue_count;
        queued[current] = 0;
        x = current % width;
        y = current / width;
        for (direction = 0; direction < 4; ++direction) {
            int nx = (int)x + dx[direction];
            int ny = (int)y + dy[direction];
            uint32_t next;
            uint32_t candidate[2];
            float edge_cost = 1.0f;
            if (nx < 0 || ny < 0 || (uint32_t)nx >= width ||
                (uint32_t)ny >= height ||
                !terrain_path_route_accept(level, (uint32_t)nx,
                                           (uint32_t)ny, direction)) continue;
            if (terrain_path_half_cost_accept(level, (uint32_t)nx,
                                               (uint32_t)ny, direction))
                edge_cost = 0.5f;
            next = (uint32_t)ny * width + (uint32_t)nx;
            if (cost[next] != 0.0f &&
                cost[current] + edge_cost < cost[next]) {
                cost[next] = cost[current] + edge_cost;
                candidate[0] = (uint32_t)nx;
                candidate[1] = (uint32_t)ny;
                if (stop != NULL && stop(level, candidate, user)) {
                    goal = next;
                    found = 1;
                }
                else if (stop == NULL && (uint32_t)nx == goal_x &&
                         (uint32_t)ny == goal_y) {
                    goal = next;
                    target_reached = 1;
                }
                if (!queued[next]) {
                    queue[write_index] = next;
                    write_index = (write_index + 1u) % cell_count;
                    ++queue_count;
                    queued[next] = 1;
                }
            }
        }
    }
    if (stop == NULL) found = target_reached;
    if (!found) {
        free(cost);
        free(queue);
        free(queued);
        return 0;
    }
    reverse_x = (int*)malloc((size_t)cell_count * sizeof(int));
    reverse_y = (int*)malloc((size_t)cell_count * sizeof(int));
    if (reverse_x == NULL || reverse_y == NULL) {
        free(reverse_x);
        free(reverse_y);
        free(cost);
        free(queue);
        free(queued);
        return 0;
    }
    cell = goal;
    for (;;) {
        uint32_t x = cell % width;
        uint32_t y = cell / width;
        int predecessor = 0;
        float best_difference = 2.0f;
        uint32_t best_cell = cell;
        if (length >= cell_count) break;
        reverse_x[length] = (int)x;
        reverse_y[length] = (int)y;
        ++length;
        if (cell == start) break;
        for (i = 0; i < 4u; ++i) {
            int px = (int)x + dx[i];
            int py = (int)y + dy[i];
            uint32_t previous;
            float difference;
            if (px < 0 || py < 0 || (uint32_t)px >= width ||
                (uint32_t)py >= height) continue;
            previous = (uint32_t)py * width + (uint32_t)px;
            difference = cost[cell] - cost[previous];
            if (difference > 0.0f && difference <= 1.0f &&
                difference < best_difference) {
                best_difference = difference;
                best_cell = previous;
                predecessor = 1;
            }
        }
        if (!predecessor) break;
        cell = best_cell;
    }
    if (length == 0u || (uint32_t)reverse_y[length - 1] * width +
                        (uint32_t)reverse_x[length - 1] != start) {
        free(reverse_x);
        free(reverse_y);
        free(cost);
        free(queue);
        free(queued);
        return 0;
    }
    route_x = (int*)malloc((size_t)length * sizeof(int));
    route_y = (int*)malloc((size_t)length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(reverse_x);
        free(reverse_y);
        free(cost);
        free(queue);
        free(queued);
        return 0;
    }
    for (i = 0; i < length; ++i) {
        route_x[i] = reverse_x[length - i - 1u];
        route_y[i] = reverse_y[length - i - 1u];
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = length;
    free(reverse_x);
    free(reverse_y);
    free(cost);
    free(queue);
    free(queued);
    return 1;
}

/* LoadTileMeshes (FUN_0042b330): the level owns a row-major pointer table.
 * The editor path leaves it zeroed; normal play registers one paired bitmap
 * resource for every cell. */
// FUNCTION: LEGORR 0x0042b330
int terrain_tile_mesh_grid_load(TerrainTileMeshGrid* grid,
                                const char* bitmap_prefix)
{
    size_t count;
    uint32_t x, y;
    if (grid == NULL || grid->width == 0 || grid->height == 0
        || grid->width > 65535u || grid->height > 65535u
        || (size_t)grid->width > ((size_t)-1) / (size_t)grid->height
        || (size_t)grid->width * (size_t)grid->height
            > ((size_t)-1) / sizeof(void*)) return 0;
    grid->bitmap_prefix = bitmap_prefix;
    terrain_tile_mesh_grid_release(grid);
    count = (size_t)grid->width * (size_t)grid->height;
    grid->meshes = (void**)calloc(count, sizeof(void*));
    if (grid->meshes == NULL) return 0;
    if (grid->editor_mode) return 1;
    if (grid->bitmap_prefix == NULL || grid->register_mesh == NULL) {
        terrain_tile_mesh_grid_release(grid);
        return 0;
    }
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
        }
    }
    return 1;
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
    free(grid->meshes);
    grid->meshes = NULL;
}

/* TerrainGridEntryResourcesReleaseAll (FUN_00434530): release every loaded
 * per-cell palette resource. The terrain owner retains its table and grid
 * dimensions; clear released handles so its separate teardown cannot release
 * a stale texture a second time. */
// FUNCTION: LEGORR 0x00434530
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
            texture_release((TextureSurface*)grid->handles[i]);
            grid->handles[i] = NULL;
        }
    }
}

/* LoadTileVariants (FUN_0040b310): allocate a 12-byte width/height/handle
 * owner, then load `%s%0.1x%0.1x.bmp` for every row-major cell. The original
 * renderer returns a COM-backed texture wrapper; the portable resource path
 * retains its decoded TextureSurface until the matching release call. */
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
            grid->handles[(size_t)y * width + x] = texture_load(path);
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
                texture_release((TextureSurface*)grid->handles[i]);
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
        s_grid_cell_object_free = node->next_free;
        memset(node, 0, sizeof(*node));
        return node;
    }
}

/* GridCellObjectLinkInsert (FUN_0040cf80): acquire a five-word node and
 * prepend it to the per-cell object chain. */
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
    node->next_cell = (GridCellObjectNode*)s_grid_cell_object_heads[index];
    s_grid_cell_object_heads[index] = node;
    if (s_grid_object_flag_hook != NULL) {
        s_grid_object_flag_hook(object, s_grid_cell_object_context);
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
    if (object == NULL || s_grid_object_coordinates_hook == NULL
        || s_grid_object_coordinates_hook(object, &x, &y,
                                         s_grid_cell_object_context) == 0) {
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
    (void)record;
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
    node = (GridCellObjectNode*)s_grid_cell_object_heads[index];
    s_grid_cell_object_heads[index] = NULL;
    while (node != NULL) {
        GridCellObjectNode* next = node->next_cell;
        node->next_free = s_grid_cell_object_free;
        s_grid_cell_object_free = node;
        node = next;
    }
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
    if (object == NULL || s_grid_object_coordinates_hook == NULL
        || s_grid_object_coordinates_hook(object, &x, &y,
                                          s_grid_cell_object_context) == 0
        || s_map_cell_record_level == NULL || s_grid_cell_object_heads == NULL) {
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

/* CreateCellObjectIfAvailable (FUN_0040d170): defer eligibility and game
 * object construction to explicit seams, then register the created object. */
// FUNCTION: LEGORR 0x0040D170
int create_cell_object_if_available(int x, int y)
{
    void* object;
    if (s_cell_object_available_hook == NULL
        || s_cell_object_create_hook == NULL
        || s_cell_object_available_hook(x, y, s_cell_object_context) == 0) {
        return 0;
    }
    object = s_cell_object_create_hook(x, y, s_cell_object_context);
    if (object == NULL
        || grid_cell_object_link_insert(object, x, y) == NULL) {
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
    terrain_entity_cell_record_remove(node->object);
    if (s_cell_object_delete_hook != NULL) {
        s_cell_object_delete_hook(node->object, s_cell_object_context);
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

/* TerrainCellActivationTry (FUN_0040d4f0): prepare activation state, validate
 * the target cell or entity flags, then dispatch a one-step flood walk. */
// FUNCTION: LEGORR 0x0040D4F0
int terrain_cell_activation_try(void* object, int x, int y)
{
    if (s_terrain_activation_prepare_hook != NULL) {
        s_terrain_activation_prepare_hook(s_terrain_activation_context);
    }
    if (object == NULL) {
        if (!terrain_cell_object_record_present(x, y)
            && (s_terrain_activation_special_hook == NULL
                || s_terrain_activation_special_hook(x, y,
                    s_terrain_activation_context) == 0)) {
            return 0;
        }
    } else {
        uint32_t entity_flags = 0u;
        uint32_t state_flags = 0u;
        if (s_terrain_activation_resolve_hook == NULL
            || s_terrain_activation_resolve_hook(object, &x, &y,
                    &entity_flags, &state_flags,
                    s_terrain_activation_context) == 0
            || ((entity_flags & 0x400u) == 0u
                && (state_flags & 0x20000000u) == 0u)) {
            return 0;
        }
    }
    if (s_terrain_activation_flood_hook != NULL) {
        s_terrain_activation_flood_hook(x, y, 1, 0.0f,
                                        s_terrain_activation_context);
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

static uint32_t* terrain_cell_state_flags_at(int x, int y)
{
    uint32_t width;
    uint32_t height;
    uint32_t index;
    if (s_map_cell_record_level == NULL || s_map_cell_record_grid == NULL
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
    return (uint32_t*)((unsigned char*)s_map_cell_record_grid
                       + (size_t)index * 0x14u + 8u);
}

static int terrain_flood_special_state(int x, int y, int mode)
{
    const LevelState* level = (const LevelState*)s_map_cell_record_level;
    if (mode > 1) return 0;
    if (level == NULL) return 0;
    if (level_cell_occupied_or_flag(level, x, y, 1) == 0u) return 0;
    return mode == 0 || level_cell_secondary_flag_100(level, x, y) != 0u;
}

static int terrain_flood_coordinate_in_bounds(int x, int y)
{
    const LevelState* level = (const LevelState*)s_map_cell_record_level;
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
        candidate_has_record = terrain_cell_object_record_present(nx, ny);
        /* The original also permits an existing cell record when its
         * float at +0x0c is nonpositive. GridCellObjectNode has no matching
         * field, so the port can only establish the absent-record case. */
        if (!candidate_has_record) {
            const uint32_t direction_flag = 1u << direction;
            if ((*next_flags & direction_flag) == 0u) {
                int emit_effect = force != 0;
                if (!emit_effect && strength != 0.0f) {
                    const float capped = strength > 200.0f ? 200.0f : strength;
                    const int random_value = level_rand_u16();
                    const int divisor = crt_float_to_long(capped);
                    emit_effect = divisor != 0 &&
                        random_value % divisor == 0;
                }
                *flags |= 1u << ((direction - 2) & 3);
                if (emit_effect && s_terrain_flood_effect_hook != NULL) {
                    s_terrain_flood_effect_hook(x, y, nx, ny,
                                                s_terrain_flood_context);
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
    if (position == NULL || s_cell_world_position_hook == NULL) return 0;
    return s_cell_world_position_hook(s_map_cell_record_level, x, y,
        position, s_cell_world_position_context) != 0;
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
    if (s_cell_world_position_hook == NULL
        || s_cell_world_position_hook(s_map_cell_record_level, x1, y1, first,
                                      s_cell_world_position_context) == 0
        || s_cell_world_position_hook(s_map_cell_record_level, x2, y2, second,
                                      s_cell_world_position_context) == 0) {
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
    if (s_cell_world_position_hook == NULL) {
        return 0;
    }
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
    if (side < 0
        || s_cell_world_position_hook(s_map_cell_record_level, x1, y1,
                position, s_cell_world_position_context) == 0) {
        return 0;
    }
    compute_world_position_delta(x1, y1, x2, y2, delta);
    return transient_effect_render_slot_configure(side, position[0],
        position[1], position[2], delta[0], delta[1], delta[2]);
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
    float center[3];
    float source_height;
    float dx;
    float dy;
    float radius;
    if (entity == NULL || position_xy == NULL
        || s_fence_spark_center_hook == NULL
        || s_fence_spark_height_hook == NULL
        || s_fence_spark_center_hook(cell_x, cell_y, center,
                                     s_fence_spark_context) == 0
        || s_fence_spark_height_hook(position_xy[0], position_xy[1],
                &source_height, s_fence_spark_context) == 0) {
        return;
    }
    dx = position_xy[0] - center[0];
    dy = position_xy[1] - center[1];
    radius = s_fence_spark_radius_hook != NULL
        ? s_fence_spark_radius_hook(entity, s_fence_spark_context) : 0.0f;
    transient_effect_render_slot_configure(0, center[0], center[1], center[2],
        dx, dy, source_height - radius * 0.5f - center[2]);
    if (s_fence_spark_finalize_hook != NULL) {
        s_fence_spark_finalize_hook(entity, dx, dy, s_fence_spark_context);
    }
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
    if (entity == NULL || s_grid_object_coordinates_hook == NULL
        || s_fence_spark_entity_position_hook == NULL
        || s_fence_spark_lifetime_hook == NULL
        || s_fence_spark_eligibility_hook == NULL
        || s_cell_world_position_hook == NULL
        || s_grid_object_coordinates_hook(entity, &cell_x, &cell_y,
                s_grid_cell_object_context) == 0
        || s_fence_spark_entity_position_hook(entity, entity_xy,
                s_fence_spark_interaction_context) == 0
        || s_fence_spark_eligibility_hook(cell_x, cell_y,
                s_fence_spark_interaction_context) == 0) {
        return 0;
    }
    if (s_fence_spark_lifetime_hook(cell_x, cell_y, &remaining,
            s_fence_spark_interaction_context) != 0 && remaining <= 0.0f) {
        target_x = cell_x;
        target_y = cell_y;
        have_target = 1;
    } else {
        for (index = 0; index < 4; ++index) {
            int nx = cell_x + offsets[index][0];
            int ny = cell_y + offsets[index][1];
            float candidate_remaining;
            float position[3];
            float dx;
            float dy;
            float distance;
            if (s_fence_spark_lifetime_hook(nx, ny, &candidate_remaining,
                    s_fence_spark_interaction_context) == 0
                || candidate_remaining > 0.0f
                || s_cell_world_position_hook(s_map_cell_record_level, nx, ny,
                    position, s_cell_world_position_context) == 0) {
                continue;
            }
            dx = entity_xy[0] - position[0];
            dy = entity_xy[1] - position[1];
            distance = dx * dx + dy * dy;
            if (!have_target || distance < target_distance) {
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
    if (s_fence_spark_duration_hook != NULL
        && s_fence_spark_lifetime_hook != NULL) {
        remaining = s_fence_spark_duration_hook(entity,
                                                s_fence_spark_interaction_context);
        s_fence_spark_lifetime_hook(target_x, target_y, &remaining,
                                    s_fence_spark_interaction_context);
    }
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
    grid->width = width;
    grid->height = height;
    grid->surface_values = surface_values;
    grid->surface_vectors = surface_vectors;
    grid->triangulation_flags = triangulation_flags;
    grid->selected_payload_index = 0;
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
    if (grid == NULL || grid->initialize_cell == NULL) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x)
            grid->initialize_cell(grid->initialize_context, x, y);
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

// FUNCTION: LEGORR 0x00429160
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

// FUNCTION: LEGORR 0x0044e4d0
void terrain_grid_vertex_build(TerrainVertexBuildGrid* grid)
{
    static const float corners[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 0.0f, 1.0f }, { 1.0f, 1.0f }
    };
    uint32_t x, y;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->vertices_write == NULL || grid->map_width == 0) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float vertices[12];
            unsigned int corner;
            for (corner = 0; corner < 4; ++corner) {
                int sample_x = (int)x + (int)(corners[corner][0] * grid->spacing + 0.5f);
                int sample_y = (int)y + (int)(corners[corner][1] * grid->spacing + 0.5f);
                unsigned long sample_index;
                float height;
                if (sample_x < 0) sample_x = 0;
                if (sample_y < 0) sample_y = 0;
                if (grid->map_width != 0 && (uint32_t)sample_x >= grid->map_width)
                    sample_x = (int)grid->map_width - 1;
                if (grid->map_height != 0 && (uint32_t)sample_y >= grid->map_height)
                    sample_y = (int)grid->map_height - 1;
                sample_index = ((unsigned long)sample_y * grid->map_width +
                                (unsigned long)sample_x) * 0x28ul;
                height = -(float)grid->cell_records[sample_index + 0x1aul] *
                         grid->vertical_scale;
                vertices[corner * 3] = ((float)x + corners[corner][0]) *
                                       grid->spacing + grid->x_origin;
                vertices[corner * 3 + 1] = height;
                vertices[corner * 3 + 2] = grid->z_origin -
                    ((float)y + corners[corner][1]) * grid->spacing;
                if (height < grid->minimum_y) grid->minimum_y = height;
            }
            grid->vertices_write(grid->context, y * grid->width + x, vertices);
        }
    }
}

/* TerrainTileStyleResolve (FUN_0045e9e0): the retail routine selects an RGB
 * triplet from the cell's state and a global palette. Those global reads are
 * represented by the resolver; preserve the output record's leading fields. */
int terrain_tile_style_resolve(void* output, uint32_t x, uint32_t y,
                               uint32_t style, void* context,
                               TerrainTileStyleHook resolver)
{
    float rgb[3];
    if (output == NULL || resolver == NULL) return 0;
    if (!resolver(context, x, y, style, rgb)) return 0;
    memcpy((unsigned char*)output + 0x10, rgb, sizeof(rgb));
    return 1;
}

/* TerrainViewTileGeometryBuild (FUN_0045dbc0) consumes the active render-view
 * stack, terrain cell table, and global palette through FUN_0045e9e0. Those
 * owners are not currently connected to LevelState/TerrainState in the port.
 * This entry therefore runs the supported row-major transform and callback
 * dispatch over explicit dimensions/style data; it is not the missing global
 * view/mesh owner itself. */
void terrain_view_tile_geometry_build(TerrainViewTileGrid* grid)
{
    uint32_t x, y;
    if (grid == NULL || grid->style_resolve == NULL || grid->emit == NULL)
        return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float rgb[3];
            float position[3];
            if (!grid->style_resolve(grid->context, x, y, grid->style, rgb))
                continue;
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

// FUNCTION: LEGORR 0x0044f230
void terrain_cell_material_set(TerrainSurfaceGrid* grid, uint32_t x,
                               uint32_t y, uint8_t material,
                               uint8_t rotation)
{
    uint32_t index;
    uint32_t handle = material;
    uint8_t flags;
    uint32_t i;
    static const uint8_t corner_order[4] = { 1, 3, 2, 0 };
    if (!terrain_grid_cell_valid(grid, x, y) || grid->cell_records == NULL)
        return;
    index = terrain_grid_cell_index(grid, x, y);
    flags = grid->cell_records[index * 0x28u + 0x19u];
    if (grid->material_handle_resolve != NULL)
        handle = grid->material_handle_resolve(grid->material_context, material);
    grid->cell_records[index * 0x28u + 0x18u] = material;
    if ((flags & 1u) != 0) rotation = (uint8_t)(rotation + 1u);
    if (grid->surface_palette_apply != NULL)
        grid->surface_palette_apply(grid->material_context, index, handle);
    if ((flags & 0x10u) == 0 && grid->texture_corner_values != NULL) {
        uint8_t input[4];
        uint8_t* output = grid->texture_corner_values + index * 4u;
        for (i = 0; i < 4; ++i) input[i] = output[corner_order[i]];
        for (i = 0; i < 4; ++i)
            output[i] = input[(uint8_t)(i - rotation) & 3u];
    }
}

static int terrain_grid_cell_valid(const TerrainSurfaceGrid* grid,
                                   uint32_t x, uint32_t y)
{
    return grid != NULL && x < grid->width && y < grid->height;
}

static uint32_t terrain_grid_cell_index(const TerrainSurfaceGrid* grid,
                                        uint32_t x, uint32_t y)
{
    return y * grid->width + x;
}

// FUNCTION: LEGORR 0x0044e860
void grid_cell_set_payload_index(TerrainSurfaceGrid* grid, uint32_t x,
                                 uint32_t y)
{
    if (terrain_grid_cell_valid(grid, x, y))
        grid->selected_payload_index = terrain_grid_cell_index(grid, x, y);
}

// FUNCTION: LEGORR 0x004500d0
int terrain_cell_surface_vertices_get(TerrainSurfaceGrid* grid, uint32_t x,
                                      uint32_t y, float out[12])
{
    uint32_t index;
    int flip;
    static const uint8_t flipped_order[4] = { 1, 2, 3, 0 };
    uint32_t i;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        out == NULL) return 0;
    index = terrain_grid_cell_index(grid, x, y);
    flip = grid->triangulation_flags != NULL &&
           (grid->triangulation_flags[index] & 1u) != 0;
    for (i = 0; i < 4; ++i) {
        uint32_t src_corner = flip ? flipped_order[i] : i;
        memcpy(out + i * 3, grid->surface_vectors + (index * 4 + src_corner) * 3,
               sizeof(float) * 3);
    }
    return flip;
}

/* TerrainCellSurfaceTriangleVerticesSelect (FUN_0044fab0). Select between
 * the two stored triplets using the point's signed distance from the cell
 * edge. The reference compares against the terrain grid's +0x10 cell size;
 * this grid stores unit map cells, so that threshold is one map unit. */
// FUNCTION: LEGORR 0x0044FAB0
void terrain_cell_surface_triangle_vertices_select(
    TerrainSurfaceGrid* grid, float x, float z, uint32_t out_vertices[3])
{
    uint32_t cell_x;
    uint32_t cell_z;
    uint32_t index;
    uint32_t corner_offset;
    const uint32_t* triangle;
    float vertices[12];
    int flipped;
    uint32_t i;
    if (grid == NULL || out_vertices == NULL || x < 0.0f || z < 0.0f)
        return;
    cell_x = (uint32_t)x;
    cell_z = (uint32_t)z;
    if (!terrain_grid_cell_valid(grid, cell_x, cell_z) ||
        grid->surface_vectors == NULL || grid->cell_records == NULL)
        return;
    index = terrain_grid_cell_index(grid, cell_x, cell_z);
    flipped = terrain_cell_surface_vertices_get(grid, cell_x, cell_z,
                                                vertices);
    if (!flipped) {
        if ((vertices[1] - z) + (x - vertices[0]) < 1.0f)
            corner_offset = 0u;
        else
            corner_offset = 3u;
    }
    else if (1.0f <= (vertices[4] - z) + (vertices[3] - x)) {
        corner_offset = 0u;
    }
    else {
        corner_offset = 3u;
    }
    triangle = (const uint32_t*)(grid->cell_records + index * 0x28u +
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
    float first_hit[3], second_hit[3];
    int first, second;
    int i;
    if (origin == NULL || direction == NULL || hit == NULL || grid == NULL ||
        grid->surface_vectors == NULL || x < 0 || y < 0 ||
        !terrain_grid_cell_valid(grid, (uint32_t)x, (uint32_t)y))
        return 0;
    terrain_cell_surface_vertices_get(grid, (uint32_t)x, (uint32_t)y,
                                      vertices);
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
 * from the previous row, append each predecessor/current pointer pair, and
 * return the last record encountered for this row. */
// FUNCTION: LEGORR 0x0046E9D0
void* staged_cell_candidate_collect(uint32_t row, uint32_t first_cell,
                                    uint32_t end_cell)
{
    uint32_t cell;
    uint32_t record_index;
    void* last_record = NULL;
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
            if (record->record == NULL || record->predecessor == NULL ||
                record->record == record->predecessor ||
                record->cell_pairs == NULL) continue;
            for (pair = 0; pair < record->pair_count; ++pair) {
                const float* xy = record->cell_pairs + pair * 2u;
                if (xy[0] == (float)cell && xy[1] == (float)(row - 1u)) {
                    if (links->count >= links->capacity) return last_record;
                    links->previous_records[links->count] = record->predecessor;
                    links->current_records[links->count] = record->record;
                    ++links->count;
                    last_record = record->record;
                }
            }
        }
    }
    return last_record;
}

void staged_cell_candidate_selection_state_install(
    StagedCellCandidateSelectionRecord* records, uint32_t record_count)
{
    s_staged_selection_records = records;
    s_staged_selection_record_count = records != NULL ? record_count : 0u;
}

/* StagedCellCandidateSelect (FUN_0046dd10): filter each staged item to its
 * first candidate whose cell is in bounds and is neither type 7 nor type 1. */
// FUNCTION: LEGORR 0x0046DD10
void staged_cell_candidate_select(void* unused, LevelState* level)
{
    uint32_t record_index;
    int width = level_width(level);
    int height = level_height(level);
    StagedCellCandidateSelectionRecord* const records =
        s_staged_selection_records;
    const uint32_t record_count = s_staged_selection_record_count;
    (void)unused;
    for (record_index = 0; records != NULL && record_index < record_count;
         ++record_index) {
        StagedCellCandidateSelectionRecord* record = &records[record_index];
        uint32_t item;
        record->selected_count = 0u;
        if (record->candidate_xy == NULL || record->selected_items == NULL ||
            record->selected_alternatives == NULL || width <= 0 || height <= 0)
            continue;
        for (item = 0; item < record->item_count; ++item) {
            uint32_t alternative;
            const float* candidates = record->candidate_xy + item * 8u;
            for (alternative = 0; alternative < 4u; ++alternative) {
                int x = crt_float_to_long(candidates[alternative * 2u]);
                int y = crt_float_to_long(candidates[alternative * 2u + 1u]);
                uint32_t cell_type;
                if (x < 0 || y < 0 || x >= width || y >= height ||
                    !level_cell_type_get_at(level, (uint32_t)x, (uint32_t)y,
                                            &cell_type) ||
                    cell_type == 7u || cell_type == 1u) continue;
                if (record->selected_count >= record->selected_capacity) break;
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

void terrain_cell_quad_run_emit(
    LevelState* level, TerrainCellQuadRunRecord* records,
    uint32_t record_count, const TerrainCellQuadRunEmitHooks* hooks)
{
    static const float texture_coordinates[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    uint32_t record_index;
    if (records == NULL || hooks == NULL || hooks->cell_vertices_get == NULL ||
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
            if (!hooks->cell_vertices_get(hooks->context, xy[0], xy[1],
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
            if (!hooks->cell_vertices_get(hooks->context, xy[0], xy[1],
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
        if (record->cell_xy == NULL || record->slot_active == NULL ||
            record->slot_offsets == NULL || record->slot_heights == NULL)
            continue;
        for (item = 0; item < record->item_count; ++item) {
            const float* xy = record->cell_xy + item * 2u;
            uint32_t corners[2];
            float vertices[12];
            float height_a, height_b;
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
            height_a = vertices[corners[0] * 3u];
            height_b = vertices[corners[1] * 3u];
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
    if (offset == NULL || (count != 0u && source_xy == NULL) || hooks == NULL ||
        hooks->adjacent_occupied_unflagged == NULL ||
        hooks->neighbor_cell_accept == NULL ||
        hooks->terrain_command_predicate == NULL ||
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
            if (hooks->adjacent_occupied_unflagged != NULL &&
                hooks->adjacent_occupied_unflagged(
                    hooks->context, x + adjacent[direction][0],
                    y + adjacent[direction][1])) {
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
        int neighbor_valid = 0;
        int blocked = 0;
        float red = 0.0f;
        float green = 0.7f;
        float blue = 0.9f;
        if (hooks->neighbor_cell_accept != NULL) {
            neighbor_valid = hooks->neighbor_cell_accept(
                hooks->context, x, y, (uint32_t)repeated_cell,
                route_limit_hit);
        }
        route_result = neighbor_valid;
        blocked = route_result == 0 || !adjacent_condition ||
            (hooks->terrain_command_predicate != NULL &&
             hooks->terrain_command_predicate(hooks->context, offset) != 0);
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
        if (hooks->preview_color_set != NULL)
            hooks->preview_color_set(hooks->context, index, red, green, blue,
                                     0.4f);
        if (hooks->preview_overlay_color_set != NULL)
            hooks->preview_overlay_color_set(hooks->context, index,
                red * 0.5f, green * 0.5f, blue * 0.5f);
        if (hooks->cell_preview_emit != NULL)
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
    static const uint8_t edge_corners[6][2] = {
        { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 }, { 2, 3 }
    };
    uint32_t i;
    int have_flagged_height = 0;
    float minimum_height = 10000.0f;
    float maximum_height = -10000.0f;
    if (grid == NULL || (cell_count != 0 && cell_xy == NULL)) return 0;
    for (i = 0; i < cell_count; ++i) {
        float vertices[12];
        float cell_minimum;
        float cell_maximum;
        uint32_t corner;
        uint32_t edge;
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        uint32_t index;
        if (i != 0 && x == cell_xy[(i - 1u) * 2u] &&
            y == cell_xy[(i - 1u) * 2u + 1u]) continue;
        if (grid->surface_vectors == NULL || x < 0 || y < 0 ||
            (uint32_t)x >= grid->width || (uint32_t)y >= grid->height)
            return 0;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x, (uint32_t)y,
                                           vertices);
        cell_minimum = vertices[0];
        cell_maximum = vertices[0];
        for (corner = 0; corner < 4; ++corner) {
            float height = vertices[corner * 3];
            if (height < cell_minimum) cell_minimum = height;
            if (height > cell_maximum) cell_maximum = height;
        }
        for (edge = 0; edge < 6; ++edge) {
            float first = vertices[edge_corners[edge][0] * 3];
            float second = vertices[edge_corners[edge][1] * 3];
            float delta = first - second;
            if (delta < 0.0f) delta = -delta;
            if (delta > maximum_edge_delta) return 0;
        }
        index = terrain_grid_cell_index(grid, (uint32_t)x, (uint32_t)y);
        if (grid->cell_records != NULL &&
            (grid->cell_records[index * 0x28u + 0x19u] & 8u) != 0) {
            if (cell_minimum < minimum_height) minimum_height = cell_minimum;
            if (cell_maximum > maximum_height) maximum_height = cell_maximum;
            have_flagged_height = 1;
        }
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
    record = grid->cell_records + index * 0x28u;
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
    if (grid == NULL || grid->surface_vectors == NULL ||
        grid->cell_records == NULL || cell_xy == NULL ||
        maximum_offset < 0.0f) return;
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
        vertices = staged_vertices + staged_count * 12u;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x,
                                          (uint32_t)y, vertices);
        for (corner = 0; corner < 4; ++corner) {
            float height = vertices[corner * 3u];
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            uint32_t neighbor_index;
            uint8_t flags;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= grid->width ||
                (uint32_t)neighbor_y >= grid->height) continue;
            neighbor_index = terrain_grid_cell_index(
                grid, (uint32_t)neighbor_x, (uint32_t)neighbor_y);
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
        vertices = staged_vertices + staged_count * 12u;
        for (corner = 0; corner < 4; ++corner) {
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            uint32_t neighbor_index;
            uint8_t* flags;
            float difference = vertices[corner * 3u] - average;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= grid->width ||
                (uint32_t)neighbor_y >= grid->height) continue;
            neighbor_index = terrain_grid_cell_index(
                grid, (uint32_t)neighbor_x, (uint32_t)neighbor_y);
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
    unsigned int cell_x;
    unsigned int cell_z;
    unsigned int index;
    unsigned int triangle_offset;
    float fraction_x;
    float fraction_z;
    float normal_x;
    float normal_z;
    float length;
    int flipped;
    const float* normal;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->triangulation_flags == NULL || out_x == NULL || out_z == NULL ||
        x < 0.0f || z < 0.0f) return 0;
    cell_x = (unsigned int)x;
    cell_z = (unsigned int)z;
    if (cell_x >= grid->width || cell_z >= grid->height) return 0;
    index = cell_z * grid->width + cell_x;
    fraction_x = x - (float)cell_x;
    fraction_z = z - (float)cell_z;
    flipped = (grid->triangulation_flags[index] & 1u) != 0;
    if (!flipped)
        triangle_offset = fraction_x + fraction_z < 1.0f ? 0u : 0x0cu;
    else
        triangle_offset = fraction_z <= fraction_x ? 0x0cu : 0u;
    normal = (const float*)(grid->cell_records + index * 0x28u +
                            triangle_offset);
    normal_x = normal[0];
    normal_z = normal[1];
    if (orientation != 0) {
        normal_x = -normal_x;
        normal_z = -normal_z;
    }
    length = (float)sqrt((double)(normal_x * normal_x + normal_z * normal_z));
    if (length == 0.0f) return 0;
    *out_x = x - (normal_x / length) * offset;
    *out_z = z - (normal_z / length) * offset;
    return 1;
}

// FUNCTION: LEGORR 0x00450220
void grid_cell_read_surface_values(TerrainSurfaceGrid* grid, uint32_t x,
                                   uint32_t y, float out[12])
{
    const float* values = grid->surface_vectors +
                          ((size_t)y * grid->width + x) * 12u;
    /* The reference forwards (row * width + column) to the terrain
     * surface's four-corner property getter and copies its twelve-word
     * result to the caller. The portable surface stores that payload inline. */
    out[0] = values[0];
    out[1] = values[1];
    out[2] = values[2];
    out[3] = values[3];
    out[4] = values[4];
    out[5] = values[5];
    out[6] = values[6];
    out[7] = values[7];
    out[8] = values[8];
    out[9] = values[9];
    out[10] = values[10];
    out[11] = values[11];
}

// FUNCTION: LEGORR 0x00450060
int grid_cell_read_surface_vector(TerrainSurfaceGrid* grid, uint32_t x,
                                  uint32_t y, float out[3])
{
    uint32_t index;
    uint32_t corner;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        out == NULL) return 0;
    index = terrain_grid_cell_index(grid, x, y);
    corner = grid->triangulation_flags != NULL &&
             (grid->triangulation_flags[index] & 1u) != 0 ? 1u : 0u;
    memcpy(out, grid->surface_vectors + (index * 4 + corner) * 3,
           sizeof(float) * 3);
    return 1;
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
    if (grid->triangulation_flags != NULL &&
        (grid->triangulation_flags[index] & 1u) != 0) corner = (corner + 1u) & 3u;
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
    if (grid->triangulation_flags != NULL &&
        (grid->triangulation_flags[index] & 1u) != 0) corner = (corner + 1u) & 3u;
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
    if (grid->triangulation_flags != NULL &&
        (grid->triangulation_flags[index] & 1u) != 0) {
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
    uint32_t index = (uint32_t)y * grid->width + (uint32_t)x;
    /* FUN_00450700 delegates this exact index to the surface-value setter. */
    grid->surface_values[index] = value;
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
    if (grid == NULL || grid->changed_count >= TERRAIN_GRID_CHANGE_CAPACITY)
        return;
    grid_surface_write_at_xy(grid, x, y, 0);
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
