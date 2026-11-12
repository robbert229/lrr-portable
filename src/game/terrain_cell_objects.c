// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit GAME-013 (text 0040cdc0-0040e490, .rdata
 * 0049e114-0049e11c, literals 004a1e8c, .bss 004c7df8-004c7e88; finding
 * 505): the electric fences (the original's ElectricFence.c), rebuilt in
 * original shape (ADR 0005, finding 566). terrain_cell_objects.h describes
 * the model. */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "terrain_cell_objects.h"
#include "building_model.h"
#include "entity_api.h"
#include "lego_blocks.h"
#include "legoobject.h"
#include "placement_footprint.h"
#include "rock_fall_styles.h"
#include "stats.h"
#include "weapons.h"
#include "gods98/maths.h"
#include "crt/vc5names.h"

typedef struct ElectricFenceGlobs {
    ElectricFenceBlock* fenceGrid;      /* 004c7df8 one record per block */
    ElectricFenceLevel* level;          /* 004c7dfc */
    ElectricFenceNode* listSet[32];     /* 004c7e00 list i holds 1 << i nodes */
    ElectricFenceNode* freeList;        /* 004c7e80 */
    unsigned int listCount;             /* 004c7e84 */
} ElectricFenceGlobs;                   /* 0x90 bytes */

// GLOBAL: LEGORR 0x004c7df8
static ElectricFenceGlobs electricFenceGlobs;

// FUNCTION: LEGORR 0x0040cdc0
void electric_fence_initialise(ElectricFenceLevel* level)
{
    electricFenceGlobs.fenceGrid = (ElectricFenceBlock*)malloc(
        level->width * level->height * sizeof(ElectricFenceNode));
    if (electricFenceGlobs.fenceGrid != NULL)
        memset(electricFenceGlobs.fenceGrid, 0,
               level->width * level->height * sizeof(ElectricFenceNode));
    electricFenceGlobs.level = level;
    memset(electricFenceGlobs.listSet, 0, sizeof(electricFenceGlobs.listSet));
    electricFenceGlobs.freeList = NULL;
    electricFenceGlobs.listCount = 0;
}

// FUNCTION: LEGORR 0x0040ce30
void electric_fence_shutdown(void)
{
    unsigned int list;

    for (list = 0; list < 32; list++) {
        if (electricFenceGlobs.listSet[list] != NULL)
            free(electricFenceGlobs.listSet[list]);
    }
    electricFenceGlobs.freeList = NULL;
    if (electricFenceGlobs.fenceGrid != NULL) {
        free(electricFenceGlobs.fenceGrid);
        electricFenceGlobs.fenceGrid = NULL;
    }
}

// FUNCTION: LEGORR 0x0040ce80
void electric_fence_restart(ElectricFenceLevel* level)
{
    electric_fence_shutdown();
    electric_fence_initialise(level);
}

// FUNCTION: LEGORR 0x0040cea0
void electric_fence_block_surfaces_update(int bx, int by)
{
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    unsigned int i;
    int x, y;

    level_block_surface_update((struct LevelState*)electricFenceGlobs.level,
                               bx, by, 0);
    for (i = 0; i < 4; i++) {
        x = bx + DIRECTIONS[i].x;
        y = by + DIRECTIONS[i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)electricFenceGlobs.level->map, x, y))
            level_block_surface_update(
                (struct LevelState*)electricFenceGlobs.level, x, y, 0);
    }
}

// FUNCTION: LEGORR 0x0040cf50
void electric_fence_add(ElectricFenceObject* object)
{
    int bx, by;

    legoobject_block_position_get((struct EntitySlot*)object, &bx, &by);
    electric_fence_create(object, bx, by);
}

// FUNCTION: LEGORR 0x0040cf80
ElectricFenceNode* electric_fence_create(ElectricFenceObject* object, int bx,
                                         int by)
{
    ElectricFenceNode* node;

    if (electricFenceGlobs.freeList == NULL)
        electric_fence_add_list();
    node = electricFenceGlobs.freeList;
    electricFenceGlobs.freeList = node->nextFree;
    memset(node, 0, sizeof(*node));
    node->nextFree = node;

    node->object = object;
    node->blockX = bx;
    node->blockY = by;
    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags = 0;
    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence = node;
    object->flags3 |= 0x2000000;
    electric_fence_block_surfaces_update(bx, by);
    return node;
}

// FUNCTION: LEGORR 0x0040d030
void electric_fence_add_list(void)
{
    ElectricFenceNode* list;
    unsigned int count, loop;

    count = 1 << electricFenceGlobs.listCount;
    electricFenceGlobs.listSet[electricFenceGlobs.listCount] =
        (ElectricFenceNode*)malloc(sizeof(ElectricFenceNode) * count);
    if ((list = electricFenceGlobs.listSet[electricFenceGlobs.listCount]) != NULL) {
        electricFenceGlobs.listCount++;
        for (loop = 1; loop < count; loop++)
            list[loop - 1].nextFree = &list[loop];
        list[count - 1].nextFree = electricFenceGlobs.freeList;
        electricFenceGlobs.freeList = list;
    }
}

// FUNCTION: LEGORR 0x0040d0a0
void electric_fence_remove(ElectricFenceObject* object)
{
    int bx, by;
    ElectricFenceNode* node;

    if (object->flags3 & 0x2000000) {
        legoobject_block_position_get((struct EntitySlot*)object, &bx, &by);
        node = electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence;
        if (node != NULL)
            electric_fence_remove_node(node, bx, by);
    }
}

// FUNCTION: LEGORR 0x0040d100
void electric_fence_remove_node(ElectricFenceNode* node, int bx, int by)
{
    node->nextFree = electricFenceGlobs.freeList;
    electricFenceGlobs.freeList = node;
    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags = 0;
    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence = NULL;
    electric_fence_block_surfaces_update(bx, by);
}

// FUNCTION: LEGORR 0x0040d170
int electric_fence_debug_place(int bx, int by)
{
    TerrainMapPoint2 wPos;
    ElectricFenceObject* object;

    if (electric_fence_can_place_at_block(bx, by)) {
        terrain_map_block_to_world(electricFenceGlobs.level->map, bx, by, &wPos.x, &wPos.y);
        object = (ElectricFenceObject*)entity_create_with_position(
            (struct GameContext*)legoGlobs_contElectricFence, 0xb, NULL, 0, wPos.x, wPos.y, 0.0f);
        electric_fence_create(object, bx, by);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d1f0
int electric_fence_debug_remove(int bx, int by)
{
    ElectricFenceNode* node;
    ElectricFenceObject* object;

    node = electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence;
    if (node != NULL) {
        object = node->object;
        electric_fence_remove(object);
        entity_teardown((struct EntitySlot*)object);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d240
int electric_fence_can_place_at_block(int bx, int by)
{
    TerrainMapPoint2I DIRECTIONS_FAR[4] = { { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } };
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    unsigned int i;
    int x, y, idx;
    ElectricFenceLevelBlock* block;

    idx = electricFenceGlobs.level->width * by + bx;
    block = &electricFenceGlobs.level->blocks[idx];
    if (!(block->flags1 & 0x8) || block->predug == 6 || block->predug == 7 ||
        electricFenceGlobs.fenceGrid[idx].fence != NULL ||
        electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0))
        return 0;

    for (i = 0; i < 4; i++) {
        x = bx + DIRECTIONS[i].x;
        y = by + DIRECTIONS[i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)electricFenceGlobs.level->map, x, y) &&
            (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].fence != NULL ||
             electric_fence_block_is_building(electricFenceGlobs.level, x, y, 0)))
            return 1;

        x = bx + DIRECTIONS_FAR[i].x;
        y = by + DIRECTIONS_FAR[i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)electricFenceGlobs.level->map, x, y)) {
            if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].fence != NULL)
                return 1;
            if (electric_fence_block_is_building(electricFenceGlobs.level, x, y, 0))
                return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d3f0
int electric_fence_block_is_building(ElectricFenceLevel* level, int bx,
                                     int by, int powered)
{
    TerrainMapPoint2I pos;

    pos.x = bx;
    pos.y = by;
    if (terrain_cell_occupied_or_flag_current(bx, by, 1)) {
        if (!powered)
            return 1;
        if (terrain_cell_secondary_flag_100_current(&pos.x))
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d450
void electric_fence_update(float elapsed)
{
    electric_fence_run_through_lists(
        (ElectricFenceNodeCallback)electric_fence_node_update_callback,
        &elapsed);
    entity_pool_find_by_callback_filtered_current(
        (int (*)(struct EntitySlot*, void*))electric_fence_building_clear_callback,
        NULL);
    entity_pool_find_by_callback_filtered_current(
        (int (*)(struct EntitySlot*, void*))electric_fence_building_spark_callback,
        &elapsed);
}

// FUNCTION: LEGORR 0x0040d490
void electric_fence_run_through_lists(ElectricFenceNodeCallback callback,
                                      void* data)
{
    unsigned int list, count, loop;
    ElectricFenceNode* node;

    for (list = 0; list < electricFenceGlobs.listCount; list++) {
        if (electricFenceGlobs.listSet[list] != NULL) {
            count = 1 << list;
            for (loop = 0; loop < count; loop++) {
                node = &electricFenceGlobs.listSet[list][loop];
                if (node != NULL && node->nextFree == node)
                    callback(node, data);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040d4f0
int electric_fence_spark_from(ElectricFenceObject* object, int bx, int by)
{
    TerrainMapPoint2 wPos;

    electric_fence_run_through_lists(
        (ElectricFenceNodeCallback)electric_fence_node_update_callback, NULL);
    entity_pool_find_by_callback_filtered_current(
        (int (*)(struct EntitySlot*, void*))electric_fence_building_clear_callback,
        NULL);
    if (object != NULL) {
        entity_transform_position_get_outputs((struct EntitySlot*)object, &wPos.x, &wPos.y);
        terrain_map_world_to_block(electricFenceGlobs.level->map, wPos.x, wPos.y, &bx, &by);
        if (!(entity_state_flags_get((struct EntitySlot*)object) & 0x400) &&
            !(object->flags4 & 0x20000000))
            return 0;
    } else {
        if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence == NULL &&
            !electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0))
            return 0;
    }
    electric_fence_spark(bx, by, 1, 0.0f);
    return 1;
}

// FUNCTION: LEGORR 0x0040d5e0
int electric_fence_building_spark_callback(ElectricFenceObject* object,
                                           float* elapsed)
{
    float elapsedGame = *elapsed;
    TerrainMapPoint2 dir;
    TerrainMapPoint2I blockPos;
    unsigned int direction, count, i;
    TerrainMapPoint2I* shape;

    if (object->type == 4) {
        legoobject_block_position_get((struct EntitySlot*)object, &blockPos.x, &blockPos.y);
        if (electric_fence_block_is_building(electricFenceGlobs.level, blockPos.x, blockPos.y, 1)) {
            entity_surface_vector_normalize(object, &dir.x);
            if (fabs(dir.x) > fabs(dir.y)) {
                if (dir.x > 0.0f)
                    direction = 1;
                else
                    direction = 3;
            } else {
                if (dir.y > 0.0f)
                    direction = 0;
                else
                    direction = 2;
            }
            shape = (TerrainMapPoint2I*)(uintptr_t)task_field84_get_and_field88_out(
                object->stats, &count);
            shape = (TerrainMapPoint2I*)rotated_pairs_scratch_fill(
                &blockPos.x, (const int*)shape, count, direction);
            for (i = 0; i < count; i++) {
                if (i != count - 1 && shape[i].x == shape[i + 1].x &&
                    shape[i].y == shape[i + 1].y)
                    i++;
                else
                    electric_fence_spark(shape[i].x, shape[i].y, 0, elapsedGame);
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d720
int electric_fence_building_clear_callback(ElectricFenceObject* object,
                                           void* data)
{
    TerrainMapPoint2I blockPos;

    if (object->type == 4) {
        legoobject_block_position_get((struct EntitySlot*)object, &blockPos.x, &blockPos.y);
        electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * blockPos.y + blockPos.x].flags = 0;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040d770
void electric_fence_node_update_callback(ElectricFenceNode* node,
                                         float* elapsed)
{
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    unsigned int i;
    int x, y;

    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * node->blockY + node->blockX].flags = 0;
    for (i = 0; i < 4; i++) {
        x = node->blockX + DIRECTIONS[i].x;
        y = node->blockY + DIRECTIONS[i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)electricFenceGlobs.level->map, x, y))
            electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].flags = 0;
    }
    if (elapsed != NULL && node->timer > 0.0f)
        node->timer -= *elapsed;
}

// FUNCTION: LEGORR 0x0040d850
void electric_fence_spark(int bx, int by, int force, float elapsed)
{
    TerrainMapPoint2I DIRECTIONS[2][4] = {
        { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } },
        { { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } },
    };
    unsigned int i;
    int x, y, x2, y2, tx, ty, adjacent, found;
    float rate;
    ElectricFenceBlock* target;

    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags |= 0x100;
    for (i = 0; i < 4; i++) {
        found = 0;
        x = bx + DIRECTIONS[0][i].x;
        y = by + DIRECTIONS[0][i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)electricFenceGlobs.level->map, x, y) &&
            (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].fence != NULL ||
             (electric_fence_block_is_building(electricFenceGlobs.level, x, y, 1) &&
              !electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0)))) {
            found = 1;
            adjacent = 1;
            tx = x;
            ty = y;
        }
        if (!found) {
            x2 = bx + DIRECTIONS[1][i].x;
            y2 = by + DIRECTIONS[1][i].y;
            if (level_coord_pair_within_bounds_target(
                    (const unsigned int*)electricFenceGlobs.level->map, x2, y2) &&
                (!electric_fence_block_is_building(electricFenceGlobs.level, x, y, 0) ||
                 !electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0)) &&
                (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y2 + x2].fence != NULL ||
                 (electric_fence_block_is_building(electricFenceGlobs.level, x2, y2, 1) &&
                  !electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0)))) {
                found = 1;
                adjacent = 0;
                tx = x2;
                ty = y2;
            }
        }
        if (found) {
            if (!adjacent)
                electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].flags |= 0x100;
            target = &electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * ty + tx];
            if (target->fence == NULL || target->fence->timer <= 0.0f) {
                if (!(target->flags & (1 << i))) {
                    electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags |=
                        1 << ((i - 2) & 3);
                    rate = elapsed;
                    if (elapsed > 200.0f)
                        rate = 200.0f;
                    if (force ||
                        (rate != 0.0f &&
                         rng_thunk_draw15() % (unsigned int)(200.0f / rate) == 0))
                        electric_fence_beam_create(bx, by, tx, ty);
                }
                if (!electric_fence_block_is_building(electricFenceGlobs.level, tx, ty, 0) &&
                    !(electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * ty + tx].flags & 0x100))
                    electric_fence_spark(tx, ty, force, elapsed);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040dc20
int electric_fence_beam_create(int bx, int by, int tx, int ty)
{
    TerrainMapPoint2I DIRECTIONS_FAR[4] = { { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } };
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    TerrainMapVector3 dir, wPos;
    unsigned int i;
    int found = 0, adjacent;

    for (i = 0; i < 4; i++) {
        if (tx == bx + DIRECTIONS[i].x && ty == by + DIRECTIONS[i].y) {
            found = 1;
            adjacent = 1;
            break;
        }
        if (tx == bx + DIRECTIONS_FAR[i].x && ty == by + DIRECTIONS_FAR[i].y) {
            found = 1;
            adjacent = 0;
            break;
        }
    }
    if (found) {
        electric_fence_block_vector_get(bx, by, tx, ty, &dir);
        terrain_map_block_to_world(electricFenceGlobs.level->map, bx, by, &wPos.x, &wPos.y);
        wPos.z = terrain_map_world_z_get(electricFenceGlobs.level->map, wPos.x, wPos.y);
        if (adjacent) {
            transient_effect_render_slot_configure(0, wPos.x, wPos.y, wPos.z, dir.x, dir.y, dir.z);
            return 1;
        }
        transient_effect_render_slot_configure(1, wPos.x, wPos.y, wPos.z, dir.x, dir.y, dir.z);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040dd90
void electric_fence_block_vector_get(int bx, int by, int tx, int ty,
                                     TerrainMapVector3* out)
{
    TerrainMapVector3 from, to;

    terrain_map_block_to_world(electricFenceGlobs.level->map, bx, by, &from.x, &from.y);
    from.z = terrain_map_world_z_get(electricFenceGlobs.level->map, from.x, from.y);
    terrain_map_block_to_world(electricFenceGlobs.level->map, tx, ty, &to.x, &to.y);
    to.z = terrain_map_world_z_get(electricFenceGlobs.level->map, to.x, to.y);
    out->x = to.x - from.x;
    out->y = to.y - from.y;
    out->z = to.z - from.z;
}

// FUNCTION: LEGORR 0x0040de40
int electric_fence_object_strike(ElectricFenceObject* object)
{
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    TerrainMapPoint2 pos, blockPos;
    float dist, bestDist = 0.0f;
    int found = 0;
    int bx, by, tx, ty, x, y;
    unsigned int i, best;
    ElectricFenceNode* node;

    entity_transform_position_get_outputs((struct EntitySlot*)object, &pos.x, &pos.y);
    terrain_map_world_to_block(electricFenceGlobs.level->map, pos.x, pos.y, &bx, &by);
    tx = bx;
    ty = by;
    if ((electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags & 0x100) &&
        !electric_fence_block_is_building(electricFenceGlobs.level, bx, by, 0)) {
        node = electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence;
        if (node == NULL) {
            for (i = 0; i < 4; i++) {
                x = bx + DIRECTIONS[i].x;
                y = by + DIRECTIONS[i].y;
                node = electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].fence;
                if (node != NULL && node->timer <= 0.0f) {
                    terrain_map_block_to_world(electricFenceGlobs.level->map, x, y,
                                               &blockPos.x, &blockPos.y);
                    dist = (float)sqrt((pos.x - blockPos.x) * (pos.x - blockPos.x) +
                                       (pos.y - blockPos.y) * (pos.y - blockPos.y));
                    if (dist < bestDist || 0.0f == bestDist) {
                        found = 1;
                        bestDist = dist;
                        best = i;
                    }
                }
            }
            if (found) {
                tx += DIRECTIONS[best].x;
                ty += DIRECTIONS[best].y;
            }
        } else if (node->timer <= 0.0f) {
            found = 1;
        }
        if (found) {
            electric_fence_object_beam(object, &pos, bx, by);
            electric_fence_spark_from(NULL, tx, ty);
            electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * ty + tx].fence->timer =
                weapon_type_recharge_time_get_target(
                    // STRING: LEGORR 0x004a1e8c
                    object_type_name_index_find("FenceSpark"));
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040e0c0
void electric_fence_object_beam(ElectricFenceObject* object,
                                const TerrainMapPoint2* position, int bx,
                                int by)
{
    TerrainMapPoint2 dir2D;
    TerrainMapVector3 blockPos, dir, objPos;

    terrain_map_block_to_world(electricFenceGlobs.level->map, bx, by,
                               &blockPos.x, &blockPos.y);
    blockPos.z = terrain_map_world_z_get(electricFenceGlobs.level->map,
                                         blockPos.x, blockPos.y);
    objPos.x = position->x;
    objPos.y = position->y;
    objPos.z = terrain_map_world_z_get(electricFenceGlobs.level->map,
                                       position->x, position->y) -
        entity_stats_float_at74((struct EntitySlot*)object) * 0.5f;
    dir.x = objPos.x - blockPos.x;
    dir.y = objPos.y - blockPos.y;
    dir.z = objPos.z - blockPos.z;
    transient_effect_render_slot_configure(0, blockPos.x, blockPos.y,
                                           blockPos.z, dir.x, dir.y, dir.z);
    dir2D.x = dir.x;
    dir2D.y = dir.y;
    ledger_stub_0046f270_TypeMotionScaleApplyDispatch(
        (struct EntitySlot*)object, object_type_name_index_find("FenceSpark"),
        1, 0.0f, &dir2D);
    entity_action_route_state_initialize((struct EntitySlot*)object);
}

// FUNCTION: LEGORR 0x0040e1e0
int electric_fence_stud_fits(ElectricFenceLevel* level, int bx, int by)
{
    TerrainMapPoint2I DIRECTIONS[4] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    unsigned int i, j;
    int x, y, x2, y2;

    if (electricFenceGlobs.fenceGrid == NULL ||
        electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence != NULL ||
        electric_fence_block_is_building(level, bx, by, 0))
        return 0;

    for (i = 0; i < 4; i++) {
        x = bx + DIRECTIONS[i].x;
        y = by + DIRECTIONS[i].y;
        if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y + x].fence != NULL ||
            electric_fence_block_is_building(level, x, y, 0)) {
            j = (i - 2) & 3;
            x2 = bx + DIRECTIONS[j].x;
            y2 = by + DIRECTIONS[j].y;
            if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * y2 + x2].fence != NULL ||
                (electric_fence_block_is_building(level, x2, y2, 0) &&
                 !electric_fence_block_is_building(level, x, y, 0)))
                return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040e350
void electric_fence_stud_set(ElectricFenceLevel* level, int bx, int by,
                             int on)
{
    TerrainMapPoint2 wPos;

    if (electricFenceGlobs.fenceGrid != NULL) {
        if (on) {
            if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].stud == NULL) {
                terrain_map_block_to_world(level->map, bx, by, &wPos.x, &wPos.y);
                electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].stud =
                    (ElectricFenceObject*)entity_create_with_position(
                        (struct GameContext*)legoGlobs_contElectricFenceStud, 0xe, NULL, 0, wPos.x,
                        wPos.y, 0.0f);
                return;
            }
        } else {
            if (electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].stud != NULL) {
                entity_teardown((struct EntitySlot*)electricFenceGlobs.fenceGrid[
                    electricFenceGlobs.level->width * by + bx].stud);
                electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].stud = NULL;
                electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].flags = 0;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040e460
int electric_fence_block_is_fence(int bx, int by)
{
    return electricFenceGlobs.fenceGrid[electricFenceGlobs.level->width * by + bx].fence != NULL;
}
