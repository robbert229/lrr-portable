// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006 decision 4): the port's own electric-fence model.
 *
 * GAME-013 (the original's ElectricFence.c) is rebuilt in original shape in
 * terrain_cell_objects.c (issue #556). The port's model is a different
 * contract: a calloc'd LevelCellKindGrid per level, explicit level
 * arguments and the scene-object hooks the native tests pin. These are the
 * port's implementations of the unit's functions under the port's names,
 * unannotated, so the native callers and tests keep their behaviour
 * (finding 566). Callers move to terrain_cell_objects.c as their own units
 * are converted. */
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
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "string.hpp"
#include "crt/crt_for_terrain_cell_objects.hpp"
#include "ai.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "effects.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/sound.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_for_terrain_cell_objects.hpp"
#include "gods98/media.hpp"
#include "gods98/simtick.hpp"
#include "gods98/camera.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_terrain_cell_objects.hpp"
#include "game/render.hpp"
#include "simtick_for_terrain_cell_objects.hpp"
#include "terrain_grid_for_terrain_cell_objects.hpp"
#include "crt/crt_for_terrain_cell_objects.hpp"
#include "entity_for_terrain_cell_objects.hpp"
#include "level_for_terrain_cell_objects.hpp"
#include "gods98/simtick_for_terrain_cell_objects.hpp"

#define s_grid_cell_object_chunks (s_active_cell_kind_grid->chunks)
#define s_grid_cell_object_free (s_active_cell_kind_grid->free_nodes)
#define s_grid_cell_object_chunk_count (s_active_cell_kind_grid->chunk_count)

/* Port state at 0x004c7df8 (GAME-013): electricFenceGlobs in
 * terrain_cell_objects.c holds the annotation. */
void*    s_marker_descriptor;

/* Port state at 0x004c7dfc (GAME-013): electricFenceGlobs in
 * terrain_cell_objects.c holds the annotation. */
static const uint32_t* s_grid_record_manager;

/* --- the selection marker table (see crt.hpp) ------------------------------
 * DAT_004c7e00..e7c is 32 pointer slots plus the count word at e80; the
 * descriptor sits at DAT_004c7df8. All unowned: zero-init statics here. */
/* Port state at 0x004c7e00 (GAME-013): electricFenceGlobs in
 * terrain_cell_objects.c holds the annotation. */
void*    s_marker_table[32];
/* Port state at 0x004c7e80 (GAME-013): electricFenceGlobs in
 * terrain_cell_objects.c holds the annotation. */
uint32_t s_marker_count;

/* The active table and its dimension owner are mirrored from the cell-kind
 * registry. The Reccmp GLOBAL alias for this address is already attached to
 * the shared descriptor view in crt.cpp; do not annotate a second mirror. */
static uint32_t* s_grid_present_table;

static int terrain_flood_special_query(int x, int y, int mode);

/* TerrainCellActivationTry (FUN_0040d4f0): prepare activation state, validate
 * the target cell or entity flags, then dispatch a one-step flood walk. */
static LevelState* terrain_flood_current_level_get(void);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int grid_cell_object_coordinates_get(void* object, int* x, int* y);
static void grid_cell_object_head_image_set(uint32_t index, const GridCellObjectNode* node);
static GridCellObjectNode* grid_cell_object_node_acquire(void);
static void post_cluster_active_records(void);
static int post_cluster_entity_first(EntitySlot* entity, void* payload);
static int post_cluster_entity_second(EntitySlot* entity, void* payload);
static int terrain_cell_object_flood_gate_get(int x, int y, float* gate);
static uint32_t* terrain_cell_state_flags_at(int x, int y);
static int terrain_flood_coordinate_in_bounds(int x, int y);
static int terrain_cell_object_flood_gate_set(int x, int y, float gate);
static int terrain_flood_special_state(int x, int y, int mode);

/* MapCellRecordGridInitialize (FUN_0040cdc0): allocate the target's
 * width*height*0x14-byte table and widened per-level pointer views. */
/* Port model of 0x0040cdc0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* Port model of 0x0040ce30 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
void selection_marker_resources_release(void)
{
    /* Corpus walk: an unsigned add-4 pointer stride bounded by the table
     * end, entered unconditionally and bottom-tested (body, add, cmp, jb —
     * no pre-loop check). Deviation: each freed slot is cleared in place
     * (the corpus dangles, which a second release could double-free), so
     * the window stays idempotent; pinned by test_crt.cpp's "the freed
     * slots clear out" / double-release checks. */
    void** p;
    void*  desc;

    p = s_marker_table;
    do {
        void* slot = *p;
        if (slot != NULL) {
            engine_heap_free(slot);
            *p = NULL;
        }
        ++p;
    } while (p < s_marker_table + 32);

    desc = s_marker_descriptor; /* the corpus loads it once... */
    s_marker_count = 0u;        /* ...before the count resets */
    if (desc != NULL) {
        engine_heap_free(desc); /* the loaded word rides the push */
        s_marker_descriptor = NULL;
    }
}

/* MapCellStateBufferRebuild (FUN_0040ce80): release prior marker resources,
 * then rebuild the level's per-cell state records. */
/* Port model of 0x0040ce80 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
void map_cell_state_buffer_rebuild(void* level)
{
    /* FUN_0040ce80 always releases the active marker registry before
     * allocating and clearing the replacement cell table. */
    if (s_active_cell_kind_grid != NULL &&
        s_active_cell_kind_grid != &s_empty_cell_kind_grid)
        map_cell_record_grid_release_current();
    map_cell_record_grid_initialize(level);
}

/* TerrainCellAndNeighborsUpdate (FUN_0040cea0): update a cell, then each
 * cardinal neighbor that passes the installed terrain bounds predicate. */
/* Port model of 0x0040cea0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* RegisterObjectAtGridPosition (FUN_0040cf50): query an object's grid cell,
 * then insert it in that cell's linked record chain. */
/* Port model of 0x0040cf50 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
void register_object_at_grid_position(void* object)
{
    int x;
    int y;
    if (!grid_cell_object_coordinates_get(object, &x, &y)) {
        return;
    }
    grid_cell_object_link_insert(object, x, y);
}
/* GridCellObjectLinkInsert (FUN_0040cf80): acquire a five-word node, mark
 * its link as the retail self-linked sentinel, and install it in the cell. */
/* Port model of 0x0040cf80 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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
/* Port model of 0x0040d030 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* TerrainEntityCellRecordRemove (FUN_0040d0a0): locate the object's cell and
 * clear its cell record chain when the object is registered there. */
/* Port model of 0x0040d0a0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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
/* TerrainCellEntityRecordUnlink (FUN_0040d100): return the cell's linked
 * records to the pool, clear its head, and refresh the changed cell ring. */
/* Port model of 0x0040d100 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* CreateCellObjectIfAvailable (FUN_0040d170): apply the decompiled cell
 * predicate, place a type-0xb entity at the cell center, and register it.
 * Explicit callbacks remain overrides for renderer-owned hosts. */
/* Port model of 0x0040d170 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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
/* Port model of 0x0040d1f0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* TerrainCellHasOccupiedOrFlaggedNeighbor (FUN_0040d240): preserve the
 * corpus's center rejection and adjacent/distance-two checks per axis. The
 * per-cell DAT_004c7df8 side table is threaded as a query callback. */
/* Port model of 0x0040d240 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
int terrain_cell_has_occupied_or_flagged_neighbor(
    const LevelState* s, int cx, int cy,
    TerrainCellSidecarPresenceQuery sidecar_present)
{
    const uint8_t* center = level_voxel(s, cx, cy);
    if (center == NULL || (center[8] & 8u) == 0u
        || center[2] == 6u || center[2] == 7u) {
        return 0;
    }
    if ((sidecar_present != NULL && sidecar_present(s, cx, cy) != 0)
        || terrain_cell_special_state_predicate(s, cx, cy, 0) != 0) {
        return 0;
    }

    static const int8_t offsets[2][4][2] = {
        { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } },
        { { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } },
    };
    for (int direction = 0; direction < 4; ++direction) {
        int distance;
        /* FUN_0040d240 tests the adjacent cell, then the distance-two cell
         * on that same cardinal axis before advancing to the next axis. */
        for (distance = 0; distance < 2; ++distance) {
            const int x = cx + offsets[distance][direction][0];
            const int y = cy + offsets[distance][direction][1];
            if (level_coord_pair_within_bounds(s, x, y) == 0) {
                continue;
            }
            if ((sidecar_present != NULL &&
                 sidecar_present(s, x, y) != 0) ||
                terrain_cell_special_state_predicate(s, x, y, 0) != 0) {
                return 1;
            }
        }
    }
    return 0;
}
/* Port model of 0x0040d3f0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
int terrain_cell_special_state_predicate(const LevelState* s, int cx,
                                         int cy, int secondary_gate)
{
    if (level_cell_occupied_or_flag(s, cx, cy, 1) == 0u) {
        return 0;
    }
    if (secondary_gate == 0) {
        return 1;
    }
    return level_cell_secondary_flag_100(s, cx, cy) != 0u ? 1 : 0;
}

/* Port model of 0x0040d450 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
void simtick_post_cluster(EngineContext* c)
{
    (void)c;
    post_cluster_active_records();
    entity_pool_find_by_callback_filtered_current(
        post_cluster_entity_first, NULL);
    entity_pool_find_by_callback_filtered_current(
        post_cluster_entity_second, NULL);
    s_post_steps += 3;
}

/* Port model of 0x0040d4f0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* TerrainCellNeighborFloodWalk (FUN_0040d850): mark visited cells, inspect
 * the corpus's cardinal/extended candidate offsets, draw the exact shared
 * random gate, emit edge effects, and recurse into eligible cells. The edge
 * effect itself remains renderer-owned. */
/* Port model of 0x0040d850 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* CellPairEdgeEffectApply (FUN_0040dc20): accept the four corpus-adjacent
 * offset pairs, compute their world direction, and configure the matching
 * transient effect group. */
/* Port model of 0x0040dc20 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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
/* ComputeWorldPositionDelta (FUN_0040dd90): query the two cell centers and
 * subtract the first world position from the second. */
/* Port model of 0x0040dd90 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

/* EntityFenceSparkTerrainInteraction (FUN_0040de40): check the source cell,
 * choose the closest adjacent cell whose spark timer has expired, register
 * the launch/activation, and refresh that cell's timer. */
/* Port model of 0x0040de40 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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
/* FenceSparkLaunchRegister (FUN_0040e0c0): place the transient spark at the
 * target cell center, derive its vertical separation from the source height
 * and entity radius, then attach/register the spark through the caller hook. */
/* Port model of 0x0040e0c0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
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

namespace unported_ledger_stubs {
// CellCornerOccupancyScan examines the center and ordered cardinal neighbors
// in the map-record sidecar and the level's special-state predicate.
/* Port model of 0x0040e1e0 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
uint32_t ledger_stub_0040e1e0_CellCornerOccupancyScan(
    LevelState* level, int cell_x, int cell_y)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { 1, 0, -1, 0 };
    uint32_t i;
    if (level == NULL ||
        terrain_cell_object_record_present_for_level(level, cell_x, cell_y) ||
        terrain_cell_special_state_predicate(level, cell_x, cell_y, 0))
        return 0u;
    for (i = 0u; i < 4u; ++i) {
        int nx = cell_x + dx[i];
        int ny = cell_y + dy[i];
        int opposite = (int)((i - 2u) & 3u);
        int ox = cell_x + dx[opposite];
        int oy = cell_y + dy[opposite];
        int neighbor_record = terrain_cell_object_record_present_for_level(
            level, nx, ny);
        int neighbor_special = 0;
        if (!neighbor_record) {
            neighbor_special = terrain_cell_special_state_predicate(
                level, nx, ny, 0);
            if (!neighbor_special) continue;
        }
        if (terrain_cell_object_record_present_for_level(level, ox, oy))
            return 1u;
        if (terrain_cell_special_state_predicate(level, ox, oy, 0) &&
            !terrain_cell_special_state_predicate(level, nx, ny, 0))
            return 1u;
    }
    return 0u;
}
} // namespace unported_ledger_stubs

/* Port model of 0x0040e350 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
void cell_entity_kind_create_or_remove(CellEntityKindRegistry* registry,
                                       uint32_t x, uint32_t y,
                                       int create)
{
    uint32_t index;
    void* current;
    if (registry == NULL || registry->records == NULL ||
        x >= registry->width || y >= registry->height) return;
    index = y * registry->width + x;
    current = registry->records[index];
    if (!create) {
        if (current != NULL && registry->release_entity != NULL)
            registry->release_entity(registry->context, current);
        registry->records[index] = NULL;
        return;
    }
    if (current != NULL || registry->grid_to_world == NULL ||
        registry->create_kind14 == NULL) return;
    {
        float world_x, world_y;
        if (registry->grid_to_world(registry->context, x, y,
                                    &world_x, &world_y))
            registry->records[index] = registry->create_kind14(
                registry->context, world_x, world_y);
    }
}

/* Port model of 0x0040e460 (GAME-013); the original-shape
 * function is in terrain_cell_objects.c. */
int grid_cell_record_present_check(int x, int y)
{
    size_t index;
    s_grid_present_table = (uint32_t*)map_cell_record_grid_get();
    s_grid_record_manager =
        (const uint32_t*)map_cell_record_grid_level_get();
    if (s_grid_present_table == NULL || s_grid_record_manager == NULL ||
        x < 0 || y < 0) return 0;
    index = (size_t)s_grid_record_manager[5] * (size_t)y + (size_t)x;
    /* MapCellRecordGridInitialize allocates 0x14 bytes per cell, but the
     * retail accessors address the first 0x0c-byte record by this stride. */
    return s_grid_present_table[index * 3u] != 0u;
}

static int terrain_flood_special_query(int x, int y, int mode)
{
    if (s_terrain_flood_state_hook != NULL) {
        return s_terrain_flood_state_hook(x, y, mode,
                                         s_terrain_flood_context);
    }
    return terrain_flood_special_state(x, y, mode);
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

static void post_cluster_active_records(void)
{
    uint32_t table_index;
    const size_t record_stride = 0x10u + sizeof(void*);
    for (table_index = 0; table_index < s_post_record_table_count;
         ++table_index) {
        void* table = s_post_record_tables != NULL
            ? s_post_record_tables[table_index] : NULL;
        uint32_t count;
        uint32_t i;
        if (table == NULL) continue;
        count = (uint32_t)1u << (table_index & 31u);
        for (i = 0; i < count; ++i) {
            uint8_t* record = (uint8_t*)table + i * record_stride;
            void* self;
            memcpy(&self, record + 0x10, sizeof(self));
            if (self == (void*)record && s_post_active_hook != NULL)
                s_post_active_hook(record, 1, s_post_cluster_user);
        }
    }
}

static int post_cluster_entity_first(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_post_entity_first_hook != NULL)
        s_post_entity_first_hook(entity, 0, s_post_cluster_user);
    return 0;
}

static int post_cluster_entity_second(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_post_entity_second_hook != NULL)
        s_post_entity_second_hook(entity, 1, s_post_cluster_user);
    return 0;
}

/* Adapters with the original signatures for terrain_cell_objects.c's
 * callees whose port entries take a different contract (ADR 0005 step 6).
 * Their owners (lego.cpp, legoobject.cpp) are converted separately. */
// PORT-ONLY: Level_BlockUpdateSurface; the port's entry takes a float column.
extern "C" void level_block_surface_update(LevelState* level, int bx, int by,
                                           int reserved)
{
    (void)reserved;
    cell_init_pass(level, (float)bx, (uint32_t)by);
}

// PORT-ONLY: LegoObject_GetBlockPos; the port's entry also takes the level.
extern "C" void legoobject_block_position_get(EntitySlot* entity, int* bx,
                                              int* by)
{
    entity_position_to_terrain_cell(entity, g_active_level_retail, bx, by);
}
