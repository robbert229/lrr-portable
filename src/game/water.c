// SPDX-License-Identifier: LGPL-3.0-or-later
/* Water.c, original translation unit GAME-049 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 614): text 0046db90-0046eb80, the
 * literal pool 0049e7f4-0049e81c and the common block 00549520-0054bf0c.
 *
 * The level's water pools. Water_Initialise gathers each row's runs of
 * lake blocks (terrain 7) into pools, joining runs that touch a pool of the
 * row above, and sorts the pools largest first. Each pool then gets its
 * entries, the edges where a neighbouring block is neither lake nor solid
 * rock, and a mesh of one quad per block at its start level. When a wall
 * beside an entry is destroyed the entry opens and the pool fills to the
 * open wall's height; when the block is sealed again it drains back.
 * Water_Update moves the levels and the flow distances, and marks the
 * blocks the flow front and tail reach.
 *
 * The port's implementation stays in the port tail cell_flood_passes_port.cpp,
 * unannotated. In the matching build water_link_names.h and Lego.c's
 * lego_unit_imports.h give this unit's functions the names Lego.c calls;
 * natively the port serves those names and nothing calls these. */
#include "water_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "water.h"
#include "water_imports.h"
/* lego_globs.h (finding 653): unused here; lego_unit.h, included above,
 * pays for its symbols, so this unit's symbol counter does not move
 * (finding 538). Deviation (scoring only). */
#include "lego_globs.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#include "water_maths.h"

// GLOBAL: LEGORR 0x00549520
Water_Globs waterGlobs;

// FUNCTION: LEGORR 0x0046db90
void water_initialise(void* contRoot, Lego_Level* level)
{
    unsigned int by, bx;
    unsigned int bxStart, bxEnd;
    unsigned int i;
    WaterPool* pool;

    waterGlobs.poolCount = 0;
    waterGlobs.joinCount = 0;
    waterGlobs.digDepth = level->digDepth;

    /* Each row's runs of lake blocks (Lego_Block terrain, f_02, 7). */
    for (by = 0; by < level->height; by++) {
        bxStart = bxEnd = 0;
        for (bx = 0; bx < level->width; bx++) {
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by * level->width) + bx].f_02 == 7) {
                if (bxStart == bxEnd) bxStart = bx;
                bxEnd = bx + 1;
            } else {
                if (bxStart != bxEnd) {
                    if ((pool = water_find_joining_pool(by, bxStart, bxEnd)) != NULL) {
                        water_add_pool_points(pool, by, bxStart, bxEnd);
                    } else {
                        water_create_pool(by, bxStart, bxEnd);
                    }
                }
                bxStart = bxEnd = 0;
            }
        }
    }

    for (i = 0; i < waterGlobs.joinCount; i++) {
        WaterPool* first = waterGlobs.joinList[i].first;
        WaterPool* second = waterGlobs.joinList[i].second;
        if (second->pointCount) {
            memcpy(&first->points[first->pointCount], second->points,
                   sizeof(TerrainMapPoint2) * second->pointCount);
            first->pointCount += second->pointCount;
            second->pointCount = 0;
        }
    }

    qsort(waterGlobs.poolList, waterGlobs.poolCount, sizeof(WaterPool),
          water_compare_pools);

    for (i = 0; i < waterGlobs.poolCount; i++) {
        if (waterGlobs.poolList[i].pointCount == 0) {
            waterGlobs.poolCount = i;
        }
    }

    water_init_entries(contRoot, level);
    water_init_vertices(contRoot, level);
}

// FUNCTION: LEGORR 0x0046dd10
void water_init_entries(void* contRoot, Lego_Level* level)
{
    TerrainMapPoint2 offsets[4] = {
        { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }
    };
    unsigned int i, j, d;
    int x, y;
    unsigned char terrain;

    for (i = 0; i < waterGlobs.poolCount; i++) {
        WaterPool* pool = &waterGlobs.poolList[i];
        pool->entryCount = 0;
        for (j = 0; j < pool->pointCount; j++) {
            for (d = 0; d < 4; d++) {
                x = (int)(offsets[d].x + pool->points[j].x);
                y = (int)(pool->points[j].y + offsets[d].y);
                if (x >= 0 && x < (int)level->width && y >= 0 && y < (int)level->height) {
                    terrain = LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(y * (int)level->width) + x].f_02;
                    if (terrain != 7 && terrain != 1) {
                        pool->entries[pool->entryCount].pointIndex = j;
                        pool->entries[pool->entryCount].direction = d;
                        pool->entryCount++;
                        break;
                    }
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0046de80
void water_init_vertices(void* contRoot, Lego_Level* level)
{
    TerrainMapPoint2 offsets[4] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    unsigned int faceData[6] = { 0, 1, 3, 1, 2, 3 };
    TerrainMapVector3 normal = { 0.0f, 0.0f, -1.0f };
    float tu[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
    float tv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    TerrainMapVector3 vertPos[4];
    TerrainMapVertex vertices[4];
    unsigned int i, j, k;
    int group;

    for (i = 0; i < waterGlobs.poolCount; i++) {
        WaterPool* pool = &waterGlobs.poolList[i];
        pool->startLevel = -10000.0f;
        for (j = 0; j < pool->pointCount; j++) {
            terrain_map_block_vertices_get(
                (TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
                (int)pool->points[j].x, (int)pool->points[j].y, vertPos);
            /* The four blocks around the vertex: predug (f_00) 0 is a wall. */
            for (k = 0; k < 4; k++) {
                int bx = (int)(pool->points[j].x + offsets[k].x);
                int by = (int)(pool->points[j].y + offsets[k].y);
                if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by * level->width) + bx].f_00 == 0) {
                    if (vertPos[k].z > pool->startLevel) pool->startLevel = vertPos[k].z;
                } else {
                    pool->flags |= WATER_FLAG_VISIBLE;
                }
            }
        }
        pool->level = pool->startLevel;
        pool->mesh = container_make_mesh2(contRoot, 3);
        scene_render_lock_toggle(pool->mesh, !(pool->flags & WATER_FLAG_VISIBLE));

        for (j = 0; j < pool->pointCount; j++) {
            group = mesh_record_allocate_or_append(pool->mesh, 4, 2, 3,
                                                   (const unsigned short*)faceData);
            scene_mesh_color_and_alpha_set(pool->mesh, group, 0.0f, 0.3f, 0.8f, 0.6f);
            terrain_map_block_vertices_get(
                (TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
                (int)pool->points[j].x, (int)pool->points[j].y, vertPos);
            for (k = 0; k < 4; k++) {
                vertices[k].position.x = vertPos[k].x;
                vertices[k].position.y = vertPos[k].y;
                vertices[k].position.z = pool->startLevel;
                vertices[k].normal = normal;
                vertices[k].colour = 0;
                vertices[k].tu = tu[k];
                vertices[k].tv = tv[k];
            }
            container_mesh_set_vertices(pool->mesh, j, 0, 4, vertices);
        }
    }
}

// FUNCTION: LEGORR 0x0046e1c0
void water_change_view_mode(int on)
{
    unsigned int i;

    for (i = 0; i < waterGlobs.poolCount; i++) {
        if (on) water_set_emissive(waterGlobs.poolList[i].mesh, 0.0f, 0.3f * 0.4f, 0.32f);
        else water_set_emissive(waterGlobs.poolList[i].mesh, 0.0f, 0.0f, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0046e220
void water_destroy_wall_complete(Lego_Level* level, int bx, int by)
{
    unsigned int pairs[4][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 } };
    TerrainMapVector3 vertPos[4];
    unsigned int entryIndex;
    WaterPool* pool;
    WaterEntry* entry;
    TerrainMapPoint2* point;
    float a, b;

    if ((pool = water_find_pool(bx, by, &entryIndex)) != NULL) {
        pool->flags = (pool->flags & ~WATER_FLAG_FILLED) | WATER_FLAG_VISIBLE;
        scene_render_lock_toggle(pool->mesh, 0);
        entry = &pool->entries[entryIndex];
        point = &pool->points[entry->pointIndex];
        entry->active = 1;
        entry->spread = 0.0f;
        entry->recede = 0.0f;
        terrain_map_block_vertices_get(
            (TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
            (int)point->x, (int)point->y, vertPos);
        a = -vertPos[pairs[entry->direction][0]].z;
        b = -vertPos[pairs[entry->direction][1]].z;
        entry->level = -((a < b) ? a : b);
    }
}

// FUNCTION: LEGORR 0x0046e330
void water_seal_block(int bx, int by)
{
    unsigned int entryIndex;
    WaterPool* pool;

    if ((pool = water_find_pool(bx, by, &entryIndex)) != NULL) {
        pool->flags = (pool->flags & ~WATER_FLAG_DRAINED) | WATER_FLAG_VISIBLE;
        scene_render_lock_toggle(pool->mesh, 0);
        pool->entries[entryIndex].active = 0;
    }
}

// FUNCTION: LEGORR 0x0046e390
void water_update(Lego_Level* level, float elapsedGame)
{
    unsigned int i, j, k;
    unsigned int activeCount = 0;
    float targetLevel, ratio;
    int filling, draining, receding;
    TerrainMapVertex vertices[4];

    for (i = 0; i < waterGlobs.poolCount; i++) {
        WaterPool* pool = &waterGlobs.poolList[i];
        filling = 0;
        targetLevel = pool->startLevel;
        draining = 1;
        if (pool->flags & WATER_FLAG_DRAINED) draining = 0;
        for (j = 0; j < pool->entryCount; j++) {
            WaterEntry* entry = &pool->entries[j];
            receding = 0;
            if (!entry->active || pool->level > entry->level) {
                receding = 1;
            } else {
                if (targetLevel < entry->level) targetLevel = entry->level;
                draining = 0;
                activeCount++;
                filling = 1;
                if (pool->level == entry->level) receding = 1;
            }
            entry->spread = entry->spread + (elapsedGame * 1.5f);
            if (receding) entry->recede = entry->recede + (elapsedGame * 0.5f);
            if (entry->spread > 240.0f) entry->spread = 240.0f;
            if (entry->recede > 240.0f) entry->recede = 240.0f;
        }
        if (pool->flags & WATER_FLAG_FILLED) filling = 0;

        if (filling || draining) {
            ratio = (pool->level - pool->startLevel) / level->digDepth;
            if (filling) {
                pool->flags &= ~WATER_FLAG_DRAINED;
                pool->level += (activeCount * elapsedGame * 0.3f) / (ratio + 1.0f);
                if (pool->level >= targetLevel) {
                    pool->level = targetLevel;
                    pool->flags |= WATER_FLAG_FILLED;
                }
            } else {
                pool->flags &= ~WATER_FLAG_FILLED;
                pool->level -= elapsedGame * 0.05f;
                if (pool->level <= targetLevel) {
                    pool->level = targetLevel;
                    pool->flags |= WATER_FLAG_DRAINED;
                }
            }
            render_object_set_packed_rgba_from_float_channels(
                pool->mesh, 0.0f, 0.3f, 0.8f, 0.6f - (ratio * (0.6f - 0.2f)));
            for (j = 0; j < pool->pointCount; j++) {
                lwo_layer_polygon_vertex_data_read(pool->mesh, j, 0, 4, vertices);
                for (k = 0; k < 4; k++) vertices[k].position.z = pool->level;
                container_mesh_set_vertices(pool->mesh, j, 0, 4, vertices);
            }
        }
    }

    water_update_flow_blocks(level, elapsedGame);
}

// FUNCTION: LEGORR 0x0046e610
void water_update_flow_blocks(Lego_Level* level, float elapsedGame)
{
    TerrainMapVector3 dirs[4] = {
        { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }
    };
    unsigned int pairs[4][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 } };
    TerrainMapVector3 vertPos[4];
    unsigned int i, j;
    TerrainMapVector3 pos;
    TerrainMapVector3* p;
    int bx, by;

    for (i = 0; i < waterGlobs.poolCount; i++) {
        WaterPool* pool = &waterGlobs.poolList[i];
        for (j = 0; j < pool->entryCount; j++) {
            WaterEntry* entry = &pool->entries[j];
            terrain_map_block_vertices_get(
                (TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
                (int)pool->points[entry->pointIndex].x,
                (int)pool->points[entry->pointIndex].y, vertPos);
            p = water_flow_point(&pos, &vertPos[pairs[entry->direction][0]],
                             &vertPos[pairs[entry->direction][1]],
                             &dirs[entry->direction], entry->spread);
            terrain_map_world_to_block((TerrainMap*)LEGO_LEVEL_PTR(void, level->map), p->x, p->y, &bx, &by);
            lego_terrain_cell_edge_flag_set_and_update(level, bx, by, 1);

            terrain_map_block_vertices_get(
                (TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
                (int)pool->points[entry->pointIndex].x,
                (int)pool->points[entry->pointIndex].y, vertPos);
            p = water_flow_point(&pos, &vertPos[pairs[entry->direction][0]],
                             &vertPos[pairs[entry->direction][1]],
                             &dirs[entry->direction], entry->recede);
            terrain_map_world_to_block((TerrainMap*)LEGO_LEVEL_PTR(void, level->map), p->x, p->y, &bx, &by);
            lego_terrain_cell_edge_flag_set_and_update(level, bx, by, 0);
        }
    }
}

// FUNCTION: LEGORR 0x0046e8a0
WaterPool* water_find_pool(int bx, int by, unsigned int* outEntry)
{
    TerrainMapPoint2 offsets[4] = {
        { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }
    };
    unsigned int i, j;

    for (i = 0; i < waterGlobs.poolCount; i++) {
        WaterPool* pool = &waterGlobs.poolList[i];
        for (j = 0; j < pool->entryCount; j++) {
            WaterEntry* entry = &pool->entries[j];
            TerrainMapPoint2 pos;
            TerrainMapPoint2* offset = &offsets[entry->direction];
            TerrainMapPoint2* point = &pool->points[entry->pointIndex];
            pos.x = offset->x + point->x;
            pos.y = offset->y + point->y;
            if (bx == (int)pos.x && by == (int)pos.y) {
                if (outEntry) *outEntry = j;
                return pool;
            }
        }
    }
    return NULL;
}

/* The qsort callback of 0046db90: the pool with more points first. */
// FUNCTION: LEGORR 0x0046e9a0
int water_compare_pools(const void* a, const void* b)
{
    if (((const WaterPool*)a)->pointCount > ((const WaterPool*)b)->pointCount) return -1;
    return ((const WaterPool*)a)->pointCount < ((const WaterPool*)b)->pointCount;
}

// FUNCTION: LEGORR 0x0046e9d0
WaterPool* water_find_joining_pool(unsigned int by, unsigned int bxStart,
                                   unsigned int bxEnd)
{
    WaterPool* found = NULL;
    unsigned int bx, i, j;

    if (by > 0) {
        by--; /* the row above */
        for (bx = bxStart; bx < bxEnd; bx++) {
            for (i = 0; i < waterGlobs.poolCount; i++) {
                WaterPool* pool = &waterGlobs.poolList[i];
                for (j = 0; j < pool->pointCount; j++) {
                    if (pool->points[j].x == (float)bx && pool->points[j].y == (float)by) {
                        if (found == NULL) {
                            found = pool;
                        } else if (found != pool) {
                            waterGlobs.joinList[waterGlobs.joinCount].first = found;
                            waterGlobs.joinList[waterGlobs.joinCount].second = pool;
                            waterGlobs.joinCount++;
                        }
                    }
                }
            }
        }
    }
    return found;
}

// FUNCTION: LEGORR 0x0046ead0
void water_add_pool_points(WaterPool* pool, unsigned int by,
                           unsigned int bxStart, unsigned int bxEnd)
{
    unsigned int bx;

    for (bx = bxStart; bx < bxEnd; bx++) {
        pool->points[pool->pointCount].x = (float)bx;
        pool->points[pool->pointCount].y = (float)by;
        pool->pointCount++;
    }
}

// FUNCTION: LEGORR 0x0046eb30
void water_create_pool(unsigned int by, unsigned int bxStart,
                       unsigned int bxEnd)
{
    WaterPool* pool = &waterGlobs.poolList[waterGlobs.poolCount];
    pool->pointCount = 0;
    water_add_pool_points(pool, by, bxStart, bxEnd);
    waterGlobs.poolCount++;
}

#ifdef __cplusplus
}
#endif
