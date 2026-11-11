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
#include "terrain_grid_shared.hpp"

static BlockFile* s_texture_set_config_context;
TerrainCellInBoundsHook s_terrain_cell_in_bounds_hook;
TerrainCellStateUpdateHook s_terrain_cell_state_update_hook;
void* s_terrain_cell_neighbor_context;
LevelCellKindGrid s_empty_cell_kind_grid;
LevelCellKindGrid* s_active_cell_kind_grid =
    &s_empty_cell_kind_grid;
// GLOBAL: LEGORR 0x005560d4
struct LevelState* g_active_level_retail;
GridCellObjectCoordinatesHook s_grid_object_coordinates_hook;
GridCellObjectFlagHook s_grid_object_flag_hook;
void* s_grid_cell_object_context;
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
TerrainCellObjectAvailableHook s_cell_object_available_hook;
TerrainCellObjectCreateHook s_cell_object_create_hook;
TerrainCellObjectDeleteHook s_cell_object_delete_hook;
void* s_cell_object_context;
TerrainActivationPrepareHook s_terrain_activation_prepare_hook;
TerrainActivationResolveHook s_terrain_activation_resolve_hook;
TerrainActivationSpecialCellHook s_terrain_activation_special_hook;
TerrainActivationFloodHook s_terrain_activation_flood_hook;
void* s_terrain_activation_context;
TerrainFloodStateHook s_terrain_flood_state_hook;
TerrainFloodEffectHook s_terrain_flood_effect_hook;
void* s_terrain_flood_context;
static TerrainCellWorldPositionHook s_cell_world_position_hook;
static void* s_cell_world_position_context;
FenceSparkCellCenterHook s_fence_spark_center_hook;
FenceSparkHeightHook s_fence_spark_height_hook;
FenceSparkEntityRadiusHook s_fence_spark_radius_hook;
FenceSparkFinalizeHook s_fence_spark_finalize_hook;
void* s_fence_spark_context;
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
TerrainFenceSparkEntityPositionHook s_fence_spark_entity_position_hook;
TerrainFenceSparkCellLifetimeHook s_fence_spark_lifetime_hook;
TerrainFenceSparkEligibilityHook s_fence_spark_eligibility_hook;
TerrainFenceSparkDurationHook s_fence_spark_duration_hook;
void* s_fence_spark_interaction_context;

void level_cell_kind_grid_storage_release(LevelCellKindGrid* grid)
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

void grid_cell_object_hooks_install(GridCellObjectCoordinatesHook coordinates,
                                    GridCellObjectFlagHook set_registered,
                                    void* context)
{
    s_grid_object_coordinates_hook = coordinates;
    s_grid_object_flag_hook = set_registered;
    s_grid_cell_object_context = context;
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
    /* Count route ENTRIES, including the start, not just frontier edges.
     * 0x4410f0 writes start at [0] and goal at [count-1]; its 0x440c70
     * fallback likewise seeds depth 1 and returns 2 for adjacent cells.
     * The decompile's bit-pattern-1 float seed is ambiguous, but zero here
     * made an adjacent goal overwrite itself with the start. */
    float iteration_count = 1.0f;
    float result_count = 0.0f;
    int* route_x;
    int* route_y;
    uint32_t route_length;
    uint32_t current_x, current_y;
    int route_valid = 1;
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
                /* Unvisited zero-cost cells are not predecessors. */
                if (s_terrain_neighbor_path_cost[previous_index] == 0.0f)
                    continue;
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
            if (!found_previous) {
                route_valid = 0;
                break;
            }
            current_x = (uint32_t)best_x;
            current_y = (uint32_t)best_y;
            route_x[index] = best_x;
            route_y[index] = best_y;
            if (index == 1u) break;
            --index;
        }
    }

    /* Weighted relaxation can change the number of edges in the best
     * cost trace. Never return holes, a missing endpoint, or a broken edge.
     * Retain the reference's existing 0x440c70 fallback for such traces. */
    if (route_valid) {
        uint32_t index;
        for (index = 1u; index < route_length; ++index) {
            int direction;
            int edge_x = route_x[index] - route_x[index - 1u];
            int edge_y = route_y[index] - route_y[index - 1u];
            if (edge_x == 0 && edge_y == -1) direction = 0;
            else if (edge_x == 1 && edge_y == 0) direction = 1;
            else if (edge_x == 0 && edge_y == 1) direction = 2;
            else if (edge_x == -1 && edge_y == 0) direction = 3;
            else { route_valid = 0; break; }
            if (!terrain_path_route_accept(actor, (uint32_t)route_x[index],
                    (uint32_t)route_y[index], direction)) {
                route_valid = 0;
                break;
            }
        }
    }
    if (!route_valid) {
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
    /* The group name is a config value such as "Textures::Lava", resolved
     * under the root. A bare one-segment lookup would reach the file's
     * trailing empty record on a miss (Config_FindItem reads its NULL key). */
    {
        char path[512];
        const char* root = util_root_name(config);
        if (root == NULL) root = "Lego";
        if (strlen(root) + strlen(group_name) + 3u >= sizeof(path)) return 0;
        strcpy(path, root);
        strcat(path, "::");
        strcat(path, group_name);
        group = block_subgroup(config, path);
    }
    if (group == NULL) return 0;
    for (entry = group; entry != NULL;
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

void terrain_neighbor_flood_hooks_install(TerrainFloodStateHook state,
                                          TerrainFloodEffectHook effect,
                                          void* context)
{
    s_terrain_flood_state_hook = state;
    s_terrain_flood_effect_hook = effect;
    s_terrain_flood_context = context;
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

NestedPointerReleaseHook s_nested_handle_release_hook;
NestedPointerReleaseHook s_nested_memory_release_hook;

void nested_pointer_record_release_hooks_set(NestedPointerReleaseHook handle_hook,
                                             NestedPointerReleaseHook memory_hook)
{
    s_nested_handle_release_hook = handle_hook;
    s_nested_memory_release_hook = memory_hook;
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

const StagedCellCandidateRecord* s_staged_candidate_records;
uint32_t s_staged_candidate_record_count;
StagedCellCandidateLinks* s_staged_candidate_links;
StagedCellCandidateSelectionRecord* s_staged_selection_records;
uint32_t s_staged_selection_record_count;

void terrain_world_spatial_record_release(
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
                /* Cardinal edge indices use physical 00/10/11/01 order,
                 * independent of the tile triangulation. */
                if (!scene_render_terrain_cell_vertices_canonical_get(
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

void staged_cell_candidate_collect_state_install(
    const StagedCellCandidateRecord* records, uint32_t record_count,
    StagedCellCandidateLinks* links)
{
    s_staged_candidate_records = records;
    s_staged_candidate_record_count = records != NULL ? record_count : 0u;
    s_staged_candidate_links = links;
    if (links != NULL) links->count = 0u;
}

void staged_cell_candidate_selection_state_install(
    StagedCellCandidateSelectionRecord* records, uint32_t record_count)
{
    s_staged_selection_records = records;
    s_staged_selection_record_count = records != NULL ? record_count : 0u;
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

TerrainWorldSpatialRecord* terrain_world_spatial_record_find_at(
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

/* 4645c3..464664: preserve vertex metadata and update only the position
 * triplets, using the terrain's raw (X,Z,height) coordinate convention. */
int building_footprint_vertices_update(float vertices[20][9],
                                       const float terrain_corners[4][3],
                                       float extrusion)
{
    static const unsigned char mapping[20] = {
        0, 1, 2, 3, 0, 4, 5, 1, 1, 5,
        6, 2, 2, 6, 7, 3, 3, 7, 4, 0
    };
    float corners[8][3];
    int i, j;
    if (vertices == NULL || terrain_corners == NULL) return 0;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 3; ++j)
            corners[i][j] = corners[i + 4][j] = terrain_corners[i][j];
    for (i = 0; i < 4; ++i) corners[i][2] -= extrusion;
    for (i = 0; i < 20; ++i)
        for (j = 0; j < 3; ++j)
            vertices[i][j] = corners[mapping[i]][j];
    return 1;
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
    int placement_invalid = 0;
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
        if (hooks != NULL && hooks->neighbor_cell_accept != NULL) {
            placement_cell_valid = hooks->neighbor_cell_accept(
                hooks->context, x, y, repeated_cell, route_limit_hit) != 0;
        }
        else {
            placement_cell_valid =
                unported_ledger_stubs::
                    terrain_placement_blocked_adjacent_cell_scan(
                    x, y, repeated_cell, route_limit_hit) != 0u;
        }
        neighbor_valid = placement_cell_valid;
        route_result = neighbor_valid;
        blocked = !placement_cell_valid || route_result == 0 ||
            !adjacent_condition ||
            ((hooks != NULL && hooks->terrain_command_predicate != NULL)
                ? hooks->terrain_command_predicate(hooks->context, offset) != 0
                : building_footprint_command_predicate_current(offset) != 0);
        if (blocked) {
            placement_invalid = 1;
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
            placement_invalid = 1;
            red = 0.7f;
            green = 0.0f;
            blue = 0.7f;
        }
        else {
            red = 0.0f;
            green = 0.7f;
            blue = 0.1f;
        }
        /* 464542 passes binary32 0x3e4ccccd as the preview alpha. */
        if (hooks != NULL && hooks->preview_color_set != NULL)
            hooks->preview_color_set(hooks->context, render_count,
                                     red, green, blue, 0.2f);
        if (hooks != NULL && hooks->preview_overlay_color_set != NULL)
            hooks->preview_overlay_color_set(hooks->context, render_count,
                red * 0.5f, green * 0.5f, blue * 0.5f);
        if (hooks != NULL && hooks->cell_preview_emit != NULL)
            hooks->cell_preview_emit(hooks->context, render_count,
                                     x, y, in_bounds,
                                     surface_valid, neighbor_valid);
        ++render_count;
        if (repeated_cell) {
            ++index;
        }
    }
    /* 464694 advances one group per distinct cell; repeated source pairs
     * consume a second pair, not another preview group. 4646f7..464706
     * returns the rotated cells only while the validity flag remains set. */
    return placement_invalid ? NULL : cells;
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
