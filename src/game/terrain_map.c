// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-026 — original translation unit (text 0044e0c0-00451cd0; finding
 * 505): the 3D terrain map (the original's Map3D.c) and the level-of-detail
 * mesh lists (MeshLOD) at the end of the object.
 *
 * Rebuilt in the original's shape (issue #522, ADR 0005): one file for the
 * unit, its 62 functions in original order and the C-compatible subset, so
 * the matching build compiles it as C. The native port compiles it as C++.
 *
 * The port's terrain model is a different contract, so the port's
 * implementations stay, unannotated, in the port tail
 * (terrain_map_port.cpp), and the native callers keep using them. The
 * functions here that keep a port name are the ones whose callers already
 * use the original's contract (finding 556). */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terrain_map.h"
#include "tile_variants.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/maths.h"
#include "gods98/mesh.h"
#include "gods98/resource.h"
#include "gods98/texture.h"
#include "gods98/view.h"
#include "crt/vc5names.h"

/* The highlight colours (Map3D_SetBlockHighlight's table). */
// GLOBAL: LEGORR 0x004a52c0
static float s_terrain_map_highlight_colours[6][3] = {
    { 1.0f, 1.0f, 1.0f },
    { 0.5f, 0.5f, 0.5f },
    { 1.0f, 0.5f, 0.4f },
    { 0.5f, 1.0f, 0.5f },
    { 0.4f, 0.5f, 1.0f },
    { 0.6f, 0.6f, 0.0f },
};

/* Map3D_FlattenShapeVertices' list of the distinct blocks of a shape. */
// GLOBAL: LEGORR 0x004eaed0
static TerrainMapPoint2I s_terrain_map_flatten_blocks[10];

// FUNCTION: LEGORR 0x0044e0c0
TerrainMap* terrain_map_create(void* root, const char* filename,
                               float blockSize, float roughLevel)
{
    unsigned int faceData[6];
    TerrainMapFile* file;
    TerrainMap* map;
    unsigned int i = 0;
    unsigned int x, y;
    int group;
    TerrainMapPoint2* uv;
    float r;
    float scale;
    int diffA, diffB;
    TerrainMapBlock* block;
    TerrainMapBlock* below;

    if ((file = (TerrainMapFile*)read_whole_file(filename, NULL)) != NULL) {
        if ((map = (TerrainMap*)malloc(sizeof(TerrainMap))) != NULL) {
            map->blockSize = blockSize;
            map->roughLevel = roughLevel;
            terrain_map_file_specs_get(file, &map->blockWidth,
                                       &map->blockHeight);
            map->gridWidth = map->blockWidth - 1;
            map->gridHeight = map->blockHeight - 1;
            map->worldX = -((float)map->gridWidth * blockSize * 0.5f);
            map->textureSet = NULL;
            map->worldY = (float)map->gridHeight * blockSize * 0.5f;
            memset(map->fadePairCount, 0, sizeof(map->fadePairCount));
            map->flags = 0;
            map->visibleCount = 0;
            map->uvWobbleCount = 0;
            map->lowestZ = 0.0f;
            map->material = scene_material_create(1.0f, 1.0f, 1.0f, 0.0f,
                                                  0.0f, 0.0f, 0.0f);
            memset(map->transitions, 0, sizeof(map->transitions));
            memset(map->noFade, 0, sizeof(map->noFade));
            for (i = 0; i < TERRAIN_MAP_MAX_TRANSITIONS; i++) {
                map->transitions[i].groupID = -1;
            }
            map->transMesh = container_make_mesh2(root, 3);
            if ((map->mesh = container_make_mesh2(root, 2)) != NULL) {
                if ((map->blocks = (TerrainMapBlock*)malloc(
                         map->blockWidth * map->blockHeight *
                         sizeof(TerrainMapBlock))) != NULL) {
                    faceData[0] = 0;
                    faceData[1] = 1;
                    faceData[2] = 3;
                    faceData[3] = 1;
                    faceData[4] = 2;
                    faceData[5] = 3;
                    for (i = 0; i < map->gridWidth * map->gridHeight; i++) {
                        group = mesh_record_allocate_or_append(
                            map->mesh, 4, 2, 3,
                            (const unsigned short*)faceData);
                        container_mesh_hide_group(map->mesh, group, 1);
                        animated_texture_frame_bind(map->mesh, group, 3);
                    }
                    for (y = 0; y < map->blockHeight; y++) {
                        for (x = 0; x < map->blockWidth; x++) {
                            map->blocks[y * map->blockWidth + x].flags = 0x60;
                            map->blocks[y * map->blockWidth + x].texture = 0;
                            map->blocks[y * map->blockWidth + x].height =
                                (unsigned char)terrain_map_file_block_value(
                                    file, x, y, map->blockWidth);
                            uv = &map->blocks[y * map->blockWidth + x].uvWobble;
                            uv->x = random_float_range(0.0f, 1.0f);
                            r = random_float_range(0.0f, 1.0f);
                            scale = 1.0f / (float)sqrt(uv->x * uv->x + r * r);
                            uv->x = scale * uv->x;
                            uv->y = scale * r;
                            uv = &map->blocks[y * map->blockWidth + x].uvWobble;
                            uv->x *= 0.3f;
                            uv->y *= 0.3f;
                            map->blocks[y * map->blockWidth + x].highlight = 0;
                        }
                    }
                    terrain_map_vertices_build(map);
                    for (y = 0; y < map->gridHeight; y++) {
                        for (x = 0; x < map->gridWidth; x++) {
                            block = &map->blocks[y * map->blockWidth + x];
                            below = &map->blocks[(y + 1) * map->blockWidth + x];
                            diffA = block[0].height - below[1].height;
                            diffB = block[1].height - below[0].height;
                            if ((unsigned int)abs(diffA) < (unsigned int)abs(diffB)) {
                                terrain_map_block_rotated_set(map, x, y, 1);
                            }
                            else {
                                terrain_map_block_face_normals_update(map, x,
                                                                      y);
                            }
                        }
                    }
                    free(file);
                    return map;
                }
                ledger_stub_00472a40_DDiSceneObjectDestroy(map->mesh);
            }
            free(map);
        }
        free(file);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0044e4d0
void terrain_map_vertices_build(TerrainMap* map)
{
    TerrainMapPoint2 offsets[4];
    TerrainMapVertex vertices[4];
    float roughLevel;
    unsigned int x, y, i;

    offsets[0].x = 0.0f;
    roughLevel = map->roughLevel;
    offsets[0].y = 0.0f;
    offsets[1].x = 1.0f;
    offsets[1].y = 0.0f;
    offsets[2].x = 1.0f;
    offsets[2].y = 1.0f;
    offsets[3].x = 0.0f;
    offsets[3].y = 1.0f;
    for (y = 0; y < map->gridHeight; y++) {
        for (x = 0; x < map->gridWidth; x++) {
            for (i = 0; i < 4; i++) {
                vertices[i].position.x =
                    ((float)x + offsets[i].x) * map->blockSize + map->worldX;
                vertices[i].position.y =
                    map->worldY - (offsets[i].y + (float)y) * map->blockSize;
                vertices[i].position.z =
                    -((float)map->blocks[(y + (int)offsets[i].y) *
                                             map->blockWidth +
                                         x + (int)offsets[i].x]
                          .height *
                      roughLevel);
                if (vertices[i].position.z < map->lowestZ) {
                    map->lowestZ = vertices[i].position.z;
                }
            }
            container_mesh_set_vertices(map->mesh, y * map->gridWidth + x, 0,
                                        4, vertices);
        }
    }
}

// FUNCTION: LEGORR 0x0044e670
void terrain_map_remove(TerrainMap* map)
{
    ledger_stub_00472a40_DDiSceneObjectDestroy(map->mesh);
    ledger_stub_00472a40_DDiSceneObjectDestroy(map->transMesh);
    free(map->blocks);
    free(map);
}

// FUNCTION: LEGORR 0x0044e6b0
void terrain_map_texture_no_fade_set(TerrainMap* map, unsigned char texture)
{
    map->noFade[texture] = 1;
}

// FUNCTION: LEGORR 0x0044e6d0
void terrain_map_block_texture_fade_set(TerrainMap* map, unsigned int bx,
                                        unsigned int by,
                                        unsigned char texture,
                                        unsigned char direction)
{
    unsigned int faceData[6];
    TerrainMapVertex vertices[4];
    TerrainMapTransition* transition;
    unsigned int group = map->gridWidth * by + bx;
    unsigned int i, j;

    faceData[1] = 1;
    faceData[3] = 1;
    faceData[0] = 0;
    faceData[2] = 3;
    faceData[4] = 2;
    faceData[5] = 3;
    if (map->blocks[by * map->blockWidth + bx].texture != texture &&
        !map->noFade[map->blocks[by * map->blockWidth + bx].texture]) {
        for (i = 0; i < TERRAIN_MAP_MAX_TRANSITIONS; i++) {
            transition = &map->transitions[i];
            if (!(transition->flags & 1)) {
                if (transition->groupID == -1) {
                    transition->groupID = mesh_record_allocate_or_append(
                        map->transMesh, 4, 2, 3,
                        (const unsigned short*)faceData);
                }
                else {
                    container_mesh_hide_group(map->transMesh,
                                              transition->groupID, 0);
                }
                lwo_layer_polygon_vertex_data_read(map->mesh, group, 0, 4,
                                                   vertices);
                for (j = 0; j < 4; j++) {
                    transition->uvs[j].x = vertices[j].tu;
                    transition->uvs[j].y = vertices[j].tv;
                }
                animated_texture_frame_set(
                    map->transMesh, transition->groupID,
                    (const void*)(size_t)packed_grid_cell_value_get(
                        map->textureSet,
                        map->blocks[by * map->blockWidth + bx].texture));
                transition->x = bx;
                transition->y = by;
                transition->timer = 0.05f;
                transition->flags |= 1;
                break;
            }
        }
    }
    terrain_map_block_texture_set(map, bx, by, texture, direction);
}

// FUNCTION: LEGORR 0x0044e860
int terrain_map_block_hidden_get(TerrainMap* map, int bx, int by)
{
    return terrain_detail_entry_get_at_index(map->mesh,
                                             map->gridWidth * by + bx);
}

// FUNCTION: LEGORR 0x0044e880
void terrain_map_vertex_normals_update_all(TerrainMap* map)
{
    unsigned int x, y;

    for (y = 0; y < map->blockHeight; y++) {
        for (x = 0; x < map->blockWidth; x++) {
            terrain_map_block_vertex_normals_update(map, x, y);
        }
    }
}

// FUNCTION: LEGORR 0x0044e8c0
int terrain_map_shape_tolerance_check(TerrainMap* map,
                                      const TerrainMapPoint2I* shape,
                                      unsigned int count, float tolerance,
                                      float maxVariation)
{
    TerrainMapPoint2I vertexOffsets[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    unsigned int edges[6][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 },
                                 { 1, 2 }, { 1, 3 }, { 2, 3 } };
    TerrainMapVector3 vertices[4];
    float lowest, highest;
    int found;
    unsigned int i, j;

    found = 0;
    lowest = 10000.0f;
    highest = -10000.0f;
    for (i = 0; i < count; i++) {
        if (i == count - 1 || shape[i].x != shape[i + 1].x ||
            shape[i].y != shape[i + 1].y) {
            terrain_map_block_vertices_get(map, shape[i].x, shape[i].y,
                                           vertices);
            for (j = 0; j < 6; j++) {
                if (fabs(vertices[edges[j][0]].z - vertices[edges[j][1]].z) >
                    maxVariation) {
                    return 0;
                }
            }
            for (j = 0; j < 4; j++) {
                if (map->blocks[(vertexOffsets[j].y + shape[i].y) *
                                    map->blockWidth +
                                vertexOffsets[j].x + shape[i].x]
                        .flags & TERRAIN_MAP_BLOCK_FLATTENED) {
                    if (vertices[j].z < lowest) lowest = vertices[j].z;
                    if (vertices[j].z > highest) highest = vertices[j].z;
                    found = 1;
                }
            }
        }
        else {
            i++;
        }
    }
    if (found && tolerance < highest - lowest) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x0044ead0
void terrain_map_shape_flatten(TerrainMap* map,
                               const TerrainMapPoint2I* shape,
                               unsigned int count, float tolerance)
{
    TerrainMapPoint2I vertexOffsets[4];
    TerrainMapVector3 vertices[10][4];
    float average, lowest, highest, diff;
    unsigned int averageCount, foundCount, blockCount;
    unsigned int i, j;
    TerrainMapPoint2I* point;

    tolerance *= 0.5f;
    vertexOffsets[1].x = 1;
    vertexOffsets[2].x = 1;
    vertexOffsets[2].y = 1;
    vertexOffsets[3].y = 1;
    averageCount = 0;
    foundCount = 0;
    average = 0.0f;
    vertexOffsets[0].x = 0;
    vertexOffsets[0].y = 0;
    vertexOffsets[1].y = 0;
    vertexOffsets[3].x = 0;
    lowest = 10000.0f;
    highest = -10000.0f;
    blockCount = 0;
    for (i = 0; i < count; i++) {
        if (i == count - 1 || shape[i].x != shape[i + 1].x ||
            shape[i].y != shape[i + 1].y) {
            point = &s_terrain_map_flatten_blocks[blockCount];
            *point = shape[i];
            terrain_map_block_vertices_get(map, point->x, point->y,
                                           vertices[blockCount]);
            for (j = 0; j < 4; j++) {
                if (!(map->blocks[(vertexOffsets[j].y + point->y) *
                                      map->blockWidth +
                                  point->x + vertexOffsets[j].x]
                          .flags & TERRAIN_MAP_BLOCK_FLATTENED)) {
                    average += vertices[blockCount][j].z;
                    averageCount++;
                }
                else {
                    if (vertices[blockCount][j].z < lowest) {
                        lowest = vertices[blockCount][j].z;
                    }
                    if (highest < vertices[blockCount][j].z) {
                        highest = vertices[blockCount][j].z;
                    }
                    foundCount++;
                }
            }
            blockCount++;
        }
        else {
            i++;
        }
    }
    average /= (float)averageCount;
    if (foundCount) {
        if (highest < tolerance + average) average = highest - tolerance;
        if (average - tolerance < lowest) average = tolerance + lowest;
    }
    for (i = 0; i < blockCount; i++) {
        point = &s_terrain_map_flatten_blocks[i];
        for (j = 0; j < 4; j++) {
            if (!(map->blocks[(vertexOffsets[j].y + point->y) *
                                  map->blockWidth +
                              point->x + vertexOffsets[j].x]
                      .flags & TERRAIN_MAP_BLOCK_FLATTENED)) {
                diff = vertices[i][j].z - average;
                if (tolerance < fabs(diff)) {
                    terrain_map_vertex_move(
                        map, point->x + vertexOffsets[j].x,
                        point->y + vertexOffsets[j].y,
                        ((diff / (float)fabs(diff)) * tolerance + average) -
                            vertices[i][j].z);
                }
                map->blocks[(point->y + vertexOffsets[j].y) *
                                map->blockWidth +
                            point->x + vertexOffsets[j].x]
                    .flags |= TERRAIN_MAP_BLOCK_FLATTENED;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0044edf0
void terrain_map_block_rotated_set(TerrainMap* map, unsigned int bx,
                                   unsigned int by, int on)
{
    TerrainMapVertex vertices[4];
    TerrainMapVertex temp;
    unsigned int group;
    unsigned char* flags;

    if ((on && !(map->blocks[by * map->blockWidth + bx].flags &
                 TERRAIN_MAP_BLOCK_ROTATED)) ||
        (!on && (map->blocks[by * map->blockWidth + bx].flags &
                 TERRAIN_MAP_BLOCK_ROTATED))) {
        group = map->gridWidth * by + bx;
        lwo_layer_polygon_vertex_data_read(map->mesh, group, 0, 4, vertices);
        flags = &map->blocks[by * map->blockWidth + bx].flags;
        if (*flags & TERRAIN_MAP_BLOCK_ROTATED) {
            *flags &= ~TERRAIN_MAP_BLOCK_ROTATED;
            temp = vertices[0];
            vertices[0] = vertices[1];
            vertices[1] = vertices[2];
            vertices[2] = vertices[3];
            vertices[3] = temp;
        }
        else {
            *flags |= TERRAIN_MAP_BLOCK_ROTATED;
            temp = vertices[3];
            vertices[3] = vertices[2];
            vertices[2] = vertices[1];
            vertices[1] = vertices[0];
            vertices[0] = temp;
        }
        container_mesh_set_vertices(map->mesh, group, 0, 4, vertices);
        terrain_map_block_face_normals_update(map, bx, by);
        terrain_map_block_vertex_modified(map, bx, by);
        terrain_map_block_vertex_modified(map, bx + 1, by);
        terrain_map_block_vertex_modified(map, bx + 1, by + 1);
        terrain_map_block_vertex_modified(map, bx, by + 1);
    }
}

// FUNCTION: LEGORR 0x0044efb0
void terrain_map_block_vertex_modified(TerrainMap* map, unsigned int vx,
                                       unsigned int vy)
{
    map->blocks[map->blockWidth * vy + vx].flags |= TERRAIN_MAP_BLOCK_MODIFIED;
    map->flags |= TERRAIN_MAP_FLAG_MODIFIED;
}

// FUNCTION: LEGORR 0x0044eff0
void terrain_map_update(TerrainMap* map, float elapsed)
{
    unsigned int x, y;

    if (map->flags & TERRAIN_MAP_FLAG_MODIFIED) {
        for (y = 0; y < map->blockHeight; y++) {
            for (x = 0; x < map->blockWidth; x++) {
                if (map->blocks[y * map->blockWidth + x].flags &
                    TERRAIN_MAP_BLOCK_MODIFIED) {
                    terrain_map_block_vertex_normals_update(map, x, y);
                    map->blocks[y * map->blockWidth + x].flags &=
                        ~TERRAIN_MAP_BLOCK_MODIFIED;
                }
            }
        }
        map->flags &= ~TERRAIN_MAP_FLAG_MODIFIED;
    }
    terrain_map_uv_wobble_update(map, elapsed);
    terrain_map_transitions_update(map, elapsed);
}

// FUNCTION: LEGORR 0x0044f090
void terrain_map_transitions_update(TerrainMap* map, float elapsed)
{
    TerrainMapVertex vertices[4];
    TerrainMapTransition* transition;
    unsigned int i, j;

    for (i = 0; i < TERRAIN_MAP_MAX_TRANSITIONS; i++) {
        transition = &map->transitions[i];
        if (transition->flags & 1) {
            lwo_layer_polygon_vertex_data_read(
                map->mesh, map->gridWidth * transition->y + transition->x, 0,
                4, vertices);
            for (j = 0; j < 4; j++) {
                vertices[j].tu = transition->uvs[j].x;
                vertices[j].tv = transition->uvs[j].y;
            }
            container_mesh_set_vertices(map->transMesh, transition->groupID,
                                        0, 4, vertices);
            scene_mesh_color_and_alpha_set(map->transMesh,
                                           transition->groupID, 1.0f, 1.0f,
                                           1.0f, 1.0f - transition->timer);
            transition->timer -= elapsed * -0.04f;
            if (transition->timer > 1.0f) {
                container_mesh_hide_group(map->transMesh,
                                          transition->groupID, 1);
                transition->flags &= ~1;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0044f1a0
void terrain_map_fade_pair_add(TerrainMap* map, unsigned char texA,
                               unsigned char texB)
{
    unsigned int i;

    for (i = 0; i < map->fadePairCount[texA]; i++) {
        if (map->fadePairs[texA][i] == texB) return;
    }
    map->fadePairs[texA][map->fadePairCount[texA]] = texB;
    map->fadePairCount[texA]++;
    map->fadePairs[texB][map->fadePairCount[texB]] = texA;
    map->fadePairCount[texB]++;
}

// FUNCTION: LEGORR 0x0044f220
void terrain_map_texture_set_set(TerrainMap* map, void* textureSet)
{
    map->textureSet = textureSet;
}

// FUNCTION: LEGORR 0x0044f230
void terrain_map_block_texture_set(TerrainMap* map, unsigned int bx,
                                   unsigned int by, unsigned char texture,
                                   unsigned char direction)
{
    unsigned char uvOrder[4];
    TerrainMapVertex vertices[4];
    unsigned int group;
    unsigned int handle;
    unsigned int i;
    unsigned char corner;

    if (bx < map->gridWidth && by < map->gridHeight) {
        group = by * map->gridWidth + bx;
        uvOrder[0] = 1;
        uvOrder[1] = 3;
        uvOrder[2] = 2;
        uvOrder[3] = 0;
        handle = packed_grid_cell_value_get(map->textureSet, texture);
        map->blocks[by * map->blockWidth + bx].texture = texture;
        if (map->blocks[by * map->blockWidth + bx].flags &
            TERRAIN_MAP_BLOCK_ROTATED) {
            direction++;
        }
        animated_texture_frame_set(map->mesh, group, (const void*)(size_t)handle);
        if (!(map->blocks[by * map->blockWidth + bx].flags &
              TERRAIN_MAP_BLOCK_UVWOBBLE)) {
            lwo_layer_polygon_vertex_data_read(map->mesh, group, 0, 4,
                                               vertices);
            for (i = 0; i < 4; i++) {
                corner = uvOrder[(i - direction) & 3];
                vertices[i].tu = (float)(corner >> 1);
                vertices[i].tv = (float)(corner & 1);
            }
            container_mesh_set_vertices(map->mesh, group, 0, 4, vertices);
        }
    }
}

// FUNCTION: LEGORR 0x0044f380
void terrain_map_vertex_move(TerrainMap* map, unsigned int vx,
                             unsigned int vy, float zDist)
{
    TerrainMapPoint2 blockOffsets[4];
    TerrainMapVector3 offset;
    unsigned int i, y;

    blockOffsets[0].x = 0.0f;
    blockOffsets[0].y = 0.0f;
    blockOffsets[1].x = -1.0f;
    blockOffsets[1].y = 0.0f;
    blockOffsets[2].x = -1.0f;
    blockOffsets[2].y = -1.0f;
    blockOffsets[3].x = 0.0f;
    blockOffsets[3].y = -1.0f;
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = zDist;
    for (i = 0; i < 4; i++) {
        blockOffsets[i].x = (float)vx + blockOffsets[i].x;
        blockOffsets[i].y += (float)vy;
    }
    for (i = 0; i < 4; i++) {
        terrain_map_block_vertex_offset(map, (int)blockOffsets[i].x,
                                        (int)blockOffsets[i].y, i, &offset);
    }
    for (y = vy - 1; y <= vy; y++) {
        unsigned int x;
        for (x = vx - 1; x <= vx; x++) {
            terrain_map_block_face_normals_update(map, x, y);
        }
    }
}

// FUNCTION: LEGORR 0x0044f490
void terrain_map_perspective_correction_set(TerrainMap* map, int on)
{
    unsigned int x, y;

    for (y = 0; y < map->gridHeight; y++) {
        for (x = 0; x < map->gridWidth; x++) {
            container_mesh_perspective_correction_set(
                map->mesh, y * map->gridWidth + x, on);
        }
    }
}

// FUNCTION: LEGORR 0x0044f4e0
int terrain_map_block_highlight_set(TerrainMap* map, int bx,
                                              int by, int highlight)
{
    int oldHighlight;
    int index = map->blockWidth * by + bx;

    oldHighlight = map->blocks[index].highlight;
    map->blocks[index].highlight = (unsigned char)highlight;
    terrain_map_block_emissive_set(map, bx, by, 1,
                                   s_terrain_map_highlight_colours[highlight][0],
                                   s_terrain_map_highlight_colours[highlight][1],
                                   s_terrain_map_highlight_colours[highlight][2]);
    return oldHighlight;
}

// FUNCTION: LEGORR 0x0044f540
int terrain_map_block_highlight_get(TerrainMap* map, int bx,
                                              int by)
{
    return map->blocks[map->blockWidth * by + bx].highlight;
}

// FUNCTION: LEGORR 0x0044f570
void terrain_map_block_highlight_clear(TerrainMap* map, int bx, int by)
{
    terrain_map_block_emissive_set(map, bx, by, 0, 1.0f, 1.0f, 1.0f);
    map->blocks[map->blockWidth * by + bx].highlight = 0;
}

// FUNCTION: LEGORR 0x0044f5c0
void terrain_map_block_emissive_set(TerrainMap* map, int bx, int by,
                                    int on, float r, float g, float b)
{
    int group = map->gridWidth * by + bx;

    if (on) {
        scene_mesh_color_and_alpha_set(map->mesh, group, r, g, b, 1.0f);
    }
    else {
        scene_mesh_color_and_alpha_set(map->mesh, group, 1.0f, 1.0f, 1.0f,
                                       1.0f);
        map->blocks[map->blockWidth * by + bx].highlight = 0;
    }
}

// FUNCTION: LEGORR 0x0044f640
int terrain_map_block_to_world(TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut, float* yOut)
{
    if (bx < map->gridWidth && by < map->gridHeight) {
        *xOut = map->worldX + ((float)bx * map->blockSize) +
                (map->blockSize * 0.5f);
        *yOut = map->worldY -
                (((float)by * map->blockSize) + (map->blockSize * 0.5f));
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044f6d0
int terrain_map_world_to_block(TerrainMap* map, float x, float y, int* bx,
                               int* by)
{
    float centre;

    return terrain_map_world_to_block_centre(map, x, y, bx, by, &centre);
}

// FUNCTION: LEGORR 0x0044f700
int terrain_map_world_to_block_centre(TerrainMap* map, float x, float y,
                                      int* bx, int* by, float* centre)
{
    int ix = (int)((x - map->worldX) / map->blockSize);
    int iy = (int)((map->worldY - y) / map->blockSize);
    float cy;

    if (bx) *bx = ix;
    if (by) *by = iy;
    if (ix >= 0 && ix < (int)map->gridWidth && iy >= 0 &&
        iy < (int)map->gridHeight) {
        if (centre) {
            x = (x - ((float)ix * map->blockSize + map->worldX)) /
                (map->blockSize * 0.5f);
            cy = -((y - (map->worldY - (float)iy * map->blockSize)) /
                   (map->blockSize * 0.5f));
            if (x > 1.0f) x = 2.0f - x;
            if (cy > 1.0f) cy = 2.0f - cy;
            if (x < cy) cy = x;
            *centre = cy;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044f810
void terrain_map_world_to_block_clamped(TerrainMap* map, float x, float y,
                                        int* bx, int* by)
{
    int ix = (int)((x - map->worldX) / map->blockSize);
    int iy = (int)((map->worldY - y) / map->blockSize);

    if (ix < 0) ix = 0;
    if (ix >= (int)map->gridWidth) ix = map->gridWidth - 1;
    if (iy < 0) iy = 0;
    if (iy >= (int)map->gridHeight) iy = map->gridHeight - 1;
    if (bx) *bx = ix;
    if (by) *by = iy;
}

// FUNCTION: LEGORR 0x0044f870
int terrain_map_world_to_block_offset(TerrainMap* map,
                                      const TerrainMapPoint2* pos,
                                      TerrainMapPoint2* block,
                                      TerrainMapPoint2* offset)
{
    TerrainMapPoint2 centre;
    unsigned int bx, by;
    float scale;

    if (terrain_map_world_to_block(map, pos->x, pos->y, (int*)&bx, (int*)&by)) {
        if (block) {
            block->x = (float)bx;
            block->y = (float)by;
        }
        if (offset) {
            terrain_map_block_to_world(map, bx, by, &centre.x, &centre.y);
            offset->x = pos->x - centre.x;
            offset->y = pos->y - centre.y;
            scale = 1.0f / map->blockSize;
            offset->x = scale * offset->x - -0.5f;
            offset->y = scale * offset->y - -0.5f;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044f940
float terrain_map_world_z_get(TerrainMap* map, float x, float y)
{
    float z = 0.0f;
    int bx, by;
    TerrainMapVector3 v[4];
    float bottom;

    if (terrain_map_world_to_block(map, x, y, &bx, &by)) {
        if (!terrain_map_block_vertices_get(map, bx, by, v)) {
            if ((v[0].y - y) + (x - v[0].x) < map->blockSize) {
                v[2].z = (v[1].z - v[0].z) + (v[3].z - v[0].z) + v[0].z;
            }
            else {
                v[0].z = (v[1].z - v[2].z) + (v[3].z - v[2].z) + v[2].z;
            }
        }
        else if ((v[1].y - y) + (v[1].x - x) < map->blockSize) {
            v[3].z = (v[0].z - v[1].z) + (v[2].z - v[1].z) + v[1].z;
        }
        else {
            v[1].z = (v[0].z - v[3].z) + (v[2].z - v[3].z) + v[3].z;
        }
        bottom = (v[2].z - v[3].z) * ((x - v[3].x) / (v[2].x - v[3].x)) +
                 v[3].z;
        z = ((y - v[3].y) / (v[0].y - v[3].y)) *
                ((v[1].z - v[0].z) * ((x - v[0].x) / (v[1].x - v[0].x)) +
                 v[0].z - bottom) +
            bottom;
    }
    return z;
}

// FUNCTION: LEGORR 0x0044fab0
void terrain_map_world_normal_get(TerrainMap* map, float x, float y,
                                  TerrainMapVector3* normal)
{
    int bx, by;
    TerrainMapVector3 v[4];

    if (!terrain_map_world_to_block(map, x, y, &bx, &by)) return;
    if (!terrain_map_block_vertices_get(map, bx, by, v)) {
        if ((x - v[0].x) + (v[0].y - y) < map->blockSize) {
            *normal = map->blocks[map->blockWidth * by + bx].normalA;
        }
        else {
            *normal = map->blocks[map->blockWidth * by + bx].normalB;
        }
    }
    else if ((v[1].x - x) + (v[1].y - y) < map->blockSize) {
        *normal = map->blocks[map->blockWidth * by + bx].normalB;
    }
    else {
        *normal = map->blocks[map->blockWidth * by + bx].normalA;
    }
}

// FUNCTION: LEGORR 0x0044fb90
int terrain_map_world_edge_point_get(TerrainMap* map, float x, float y,
                                     int diagonal, float distance,
                                     float* xOut, float* yOut)
{
    int bx, by;
    TerrainMapVector3 v[4];
    TerrainMapVector3 normal;
    TerrainMapPoint2 lineDir;
    TerrainMapPoint2 normal2;
    TerrainMapPoint2 lineStart;
    TerrainMapPoint2 point;
    TerrainMapPoint2 result;
    int rotated;
    int a, b;
    float scale;

    if (terrain_map_world_to_block(map, x, y, &bx, &by)) {
    rotated = terrain_map_block_vertices_get(map, bx, by, v);
    if (!rotated) {
        if ((x - v[0].x) + (v[0].y - y) < map->blockSize) {
            normal = map->blocks[map->blockWidth * by + bx].normalA;
        }
        else {
            normal = map->blocks[map->blockWidth * by + bx].normalB;
        }
    }
    else if ((v[1].x - x) + (v[1].y - y) < map->blockSize) {
        normal = map->blocks[map->blockWidth * by + bx].normalB;
    }
    else {
        normal = map->blocks[map->blockWidth * by + bx].normalA;
    }
    if (diagonal) {
        if (rotated) {
            a = 0;
            b = 2;
        }
        else {
            a = 1;
            b = 3;
        }
    }
    else if (fabs(normal.y) < fabs(normal.x)) {
        if (normal.x < 0.0f) {
            a = 0;
            b = 3;
        }
        else {
            a = 1;
            b = 2;
        }
    }
    else if (normal.y < 0.0f) {
        a = 2;
        b = 3;
    }
    else {
        a = 0;
        b = 1;
    }
    lineStart.x = v[a].x;
    lineStart.y = v[a].y;
    point.x = x;
    lineDir.x = v[b].x - lineStart.x;
    lineDir.y = v[b].y - lineStart.y;
    point.y = y;
    normal2.x = normal.x;
    normal2.y = normal.y;
    line_intersection_2d_compute(&result.x, &lineStart.x, &lineDir.x,
                                 &point.x, &normal2.x);
    distance *= map->blockSize;
    scale = 1.0f / (float)sqrt(normal2.y * normal2.y + normal2.x * normal2.x);
    *xOut = result.x - scale * normal2.x * distance;
    *yOut = result.y - scale * normal2.y * distance;
    return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044fe70
float terrain_map_world_z_unmodified_get(TerrainMap* map, float x, float y)
{
    float z = 0.0f;
    unsigned int bx, by;
    TerrainMapPoint2 offsets[4];
    TerrainMapVector3 v[4];
    float bottom;
    unsigned int i;

    if (terrain_map_world_to_block(map, x, y, (int*)&bx, (int*)&by)) {
        offsets[0].x = 0.0f;
        offsets[0].y = 0.0f;
        offsets[1].x = 1.0f;
        offsets[1].y = 0.0f;
        offsets[2].x = 1.0f;
        offsets[2].y = 1.0f;
        offsets[3].x = 0.0f;
        offsets[3].y = 1.0f;
        for (i = 0; i < 4; i++) {
            v[i].x = (offsets[i].x + (float)bx) * map->blockSize + map->worldX;
            v[i].y = map->worldY - (offsets[i].y + (float)by) * map->blockSize;
            v[i].z = -((float)map->blocks[(by + (int)offsets[i].y) *
                                              map->blockWidth +
                                          bx + (int)offsets[i].x]
                           .height *
                       map->roughLevel);
        }
        if ((v[1].x - x) + (v[1].y - y) < map->blockSize) {
            v[3].z = (v[0].z - v[1].z) + (v[2].z - v[1].z) + v[1].z;
        }
        else {
            v[1].z = (v[0].z - v[3].z) + (v[2].z - v[3].z) + v[3].z;
        }
        bottom = (v[2].z - v[3].z) * ((x - v[3].x) / (v[2].x - v[3].x)) +
                 v[3].z;
        z = ((y - v[3].y) / (v[0].y - v[3].y)) *
                ((v[1].z - v[0].z) * ((x - v[0].x) / (v[1].x - v[0].x)) +
                 v[0].z - bottom) +
            bottom;
    }
    return z;
}

// FUNCTION: LEGORR 0x00450060
void terrain_map_block_first_vertex_get(TerrainMap* map, int bx, int by,
                                        TerrainMapVector3* position)
{
    TerrainMapVertex vertex;

    lwo_layer_polygon_vertex_data_read(
        map->mesh, by * map->gridWidth + bx,
        map->blocks[map->blockWidth * by + bx].flags &
            TERRAIN_MAP_BLOCK_ROTATED,
        1, &vertex);
    *position = vertex.position;
}

// FUNCTION: LEGORR 0x004500d0
int terrain_map_block_vertices_get(TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   TerrainMapVector3* positions)
{
    TerrainMapVertex vertices[4];

    if (bx < map->gridWidth && by < map->gridHeight) {
        lwo_layer_polygon_vertex_data_read(
            map->mesh, by * map->gridWidth + bx, 0, 4, vertices);
        if (map->blocks[map->blockWidth * by + bx].flags &
            TERRAIN_MAP_BLOCK_ROTATED) {
            positions[0] = vertices[1].position;
            positions[1] = vertices[2].position;
            positions[2] = vertices[3].position;
            positions[3] = vertices[0].position;
            return 1;
        }
        positions[0] = vertices[0].position;
        positions[1] = vertices[1].position;
        positions[2] = vertices[2].position;
        positions[3] = vertices[3].position;
        return 0;
    }
}

// FUNCTION: LEGORR 0x00450220
void terrain_map_block_vertices_raw_get(TerrainMap* map, int bx, int by,
                                        TerrainMapVector3* positions)
{
    TerrainMapVertex vertices[4];

    lwo_layer_polygon_vertex_data_read(map->mesh, by * map->gridWidth + bx, 0,
                                       4, vertices);
    positions[0] = vertices[0].position;
    positions[1] = vertices[1].position;
    positions[2] = vertices[2].position;
    positions[3] = vertices[3].position;
}

// FUNCTION: LEGORR 0x004502c0
int level_coord_pair_within_bounds_target(const unsigned int* dimensions,
                                          unsigned int cx, unsigned int cy)
{
    if (cx >= dimensions[0]) return 0;
    return cy < dimensions[1];
}

// FUNCTION: LEGORR 0x004502e0
int terrain_map_screen_pick(TerrainMap* map, void* viewport,
                            unsigned int mx, unsigned int my,
                            unsigned int* bx, unsigned int* by,
                            TerrainMapVector3* intersection)
{
    TerrainMapPoint2I block;
    int ox, oy;
    TerrainMapPoint2 mouse;
    TerrainMapVector3 origin;
    TerrainMapVector3 direction;
    float transformed[4];
    float nearPoint[4];
    float farPoint[4];
    TerrainMapPoint2 polygon[5];
    TerrainMapVector3 vertices[4];
    float scale;
    unsigned int i;

    mouse.x = (float)mx;
    mouse.y = (float)my;
    nearPoint[2] = 0.0f;
    nearPoint[3] = 1.0f;
    farPoint[2] = 1.0f;
    farPoint[3] = 1.0f;
    nearPoint[0] = mouse.x;
    nearPoint[1] = mouse.y;
    farPoint[0] = mouse.x;
    farPoint[1] = mouse.y;
    camera_screen_coordinate_project(
        (const ViewProjectionAdapter*)viewport, &origin.x, nearPoint);
    camera_screen_coordinate_project(
        (const ViewProjectionAdapter*)viewport, &direction.x, farPoint);
    direction.x -= origin.x;
    direction.y -= origin.y;
    direction.z -= origin.z;
    scale = 1.0f / (float)sqrt(direction.z * direction.z +
                               direction.y * direction.y +
                               direction.x * direction.x);
    direction.x = scale * direction.x;
    direction.y = scale * direction.y;
    direction.z = scale * direction.z;
    if (!terrain_map_ray_intersection(map, &origin, &direction, intersection,
                                      &block, 20)) {
        return 0;
    }
    for (oy = -2; oy < 3; oy++) {
        for (ox = -2; ox < 3; ox++) {
            if ((unsigned int)(ox + block.x) < map->gridWidth &&
                (unsigned int)(oy + block.y) < map->gridHeight) {
                terrain_map_block_vertices_get(map, ox + block.x,
                                               oy + block.y, vertices);
                for (i = 0; i < 4; i++) {
                    render_interface_transform_point(viewport, transformed,
                                                     &vertices[i]);
                    polygon[i].x = transformed[0] / transformed[3];
                    polygon[i].y = transformed[1] / transformed[3];
                }
                polygon[4] = polygon[0];
                if (point_in_polygon_2d(&mouse.x, &polygon[0].x,
                                        &polygon[1].x, 4)) {
                    *bx = ox + block.x;
                    *by = oy + block.y;
                    terrain_map_block_ray_intersection(map, *bx, *by, &origin,
                                                       &direction,
                                                       intersection);
                    return 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00450560
int terrain_map_ray_intersection(TerrainMap* map,
                                 const TerrainMapVector3* origin,
                                 const TerrainMapVector3* direction,
                                 TerrainMapVector3* endPoint,
                                 TerrainMapPoint2I* block, unsigned int iterations)
{
    TerrainMapVector3 planePoint;
    TerrainMapVector3 planeNormal;
    unsigned int i;

    planePoint.x = 0.0f;
    planePoint.y = 0.0f;
    planePoint.z = -50.0f;
    planeNormal.x = 0.0f;
    planeNormal.y = 0.0f;
    planeNormal.z = -1.0f;
    for (i = 0; i < iterations; i++) {
        view_ray_intersection_test(&endPoint->x, &origin->x, &direction->x,
                                   &planePoint.x, &planeNormal.x);
        planePoint.z = terrain_map_world_z_get(map, endPoint->x, endPoint->y);
    }
    return terrain_map_world_to_block(map, endPoint->x, endPoint->y,
                                      &block->x, &block->y);
}

// FUNCTION: LEGORR 0x00450600
void terrain_map_visible_radius_add(TerrainMap* map, int bx, int by,
                                    int radius,
                                    TerrainMapBlockCallback callback)
{
    int x, y;
    int r2 = radius * radius;
    int y2;

    for (y = -radius; y <= radius; y++) {
        y2 = y * y;
        for (x = -radius; x <= radius; x++) {
            if (x * x + y2 <= r2 && bx + x >= 0 &&
                bx + x < (int)map->gridWidth && by + y >= 0 &&
                by + y < (int)map->gridHeight) {
                terrain_map_block_hide(map, bx + x, by + y, 0);
                if (callback) callback(bx + x, by + y);
                map->visible[map->visibleCount].x =
                    (unsigned short)(x + bx);
                map->visible[map->visibleCount].y =
                    (unsigned short)(y + by);
                map->visibleCount++;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00450700
void terrain_map_block_hide(TerrainMap* map, int bx, int by, int hide)
{
    container_mesh_hide_group(map->mesh, by * map->gridWidth + bx, hide);
}

// FUNCTION: LEGORR 0x00450730
void terrain_map_visible_add(TerrainMap* map, int bx, int by)
{
    terrain_map_block_hide(map, bx, by, 0);
    map->visible[map->visibleCount].x = (unsigned short)bx;
    map->visible[map->visibleCount].y = (unsigned short)by;
    map->visibleCount++;
}

// FUNCTION: LEGORR 0x00450780
void terrain_map_visible_clear(TerrainMap* map)
{
    unsigned int i;

    for (i = 0; i < map->visibleCount; i++) {
        terrain_map_block_hide(map, map->visible[i].x, map->visible[i].y, 1);
    }
    map->visibleCount = 0;
}

// FUNCTION: LEGORR 0x004507d0
void terrain_map_vertex_position_get(TerrainMap* map, unsigned int vx,
                                     unsigned int vy, float* x, float* y,
                                     float* z)
{
    TerrainMapVertex vertex;
    int index;

    if (vx >= map->blockWidth) return;
    if (vy >= map->blockHeight) return;
    index = 0;
    if (vx == map->gridWidth && vy == map->gridHeight) {
        vx--;
        vy--;
        index = 2;
    }
    else if (vx == map->gridWidth) {
        vx--;
        index = 1;
    }
    else if (vy == map->gridHeight) {
        vy--;
        index = -1;
    }
    if (map->blocks[vy * map->blockWidth + vx].flags &
        TERRAIN_MAP_BLOCK_ROTATED) {
        index++;
    }
    if (index == -1) index = 3;
    lwo_layer_polygon_vertex_data_read(map->mesh, vy * map->gridWidth + vx,
                                       index, 1, &vertex);
    *x = vertex.position.x;
    *y = vertex.position.y;
    *z = vertex.position.z;
}

// FUNCTION: LEGORR 0x00450890
float terrain_context_scale_get(const void* context)
{
    return ((const TerrainMap*)context)->blockSize;
}

// FUNCTION: LEGORR 0x004508a0
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2)
{
    int result = -1;

    if (x1 == x2) {
        if (y1 - 1 == y2) return 0;
        if (y1 + 1 == y2) return 2;
    }
    else if (y1 == y2) {
        if (x1 + 1 == x2) return 1;
        if (x1 - 1 == x2) return 3;
    }
    else {
        if (x1 + 1 == x2 && y1 - 1 == y2) return 4;
        if (x1 + 1 == x2 && y1 + 1 == y2) return 5;
        if (x1 - 1 == x2 && y1 + 1 == y2) return 6;
        if (x1 - 1 == x2 && y1 - 1 == y2) result = 7;
    }
    return result;
}

// FUNCTION: LEGORR 0x00450960
void terrain_map_block_uv_wobble_set(TerrainMap* map, unsigned int bx,
                                     unsigned int by, int on)
{
    unsigned int i;

    if (on) {
        if (!(map->blocks[by * map->blockWidth + bx].flags &
              TERRAIN_MAP_BLOCK_UVWOBBLE)) {
            map->uvWobbles[map->uvWobbleCount].x = (unsigned short)bx;
            map->uvWobbles[map->uvWobbleCount].y = (unsigned short)by;
            map->uvWobbleCount++;
            map->blocks[by * map->blockWidth + bx].flags |=
                TERRAIN_MAP_BLOCK_UVWOBBLE;
        }
    }
    else if (map->blocks[by * map->blockWidth + bx].flags &
             TERRAIN_MAP_BLOCK_UVWOBBLE) {
        for (i = 0; i < map->uvWobbleCount; i++) {
            if (map->uvWobbles[i].x == bx && map->uvWobbles[i].y == by) {
                map->uvWobbleCount--;
                map->uvWobbles[i].x = map->uvWobbles[map->uvWobbleCount].x;
                map->uvWobbles[i].y = map->uvWobbles[map->uvWobbleCount].y;
                map->blocks[by * map->blockWidth + bx].flags &=
                    ~TERRAIN_MAP_BLOCK_UVWOBBLE;
                break;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00450a80
void terrain_map_wobble_emissive_set(TerrainMap* map, int on)
{
    unsigned int i;

    if (on && !(map->flags & TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE)) {
        for (i = 0; i < map->uvWobbleCount; i++) {
            scene_mesh_color_set(map->mesh,
                                 map->uvWobbles[i].y * map->gridWidth +
                                     map->uvWobbles[i].x,
                                 0.3f, 0.3f, 0.3f);
        }
        map->flags |= TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE;
    }
    else if (!on && (map->flags & TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE)) {
        for (i = 0; i < map->uvWobbleCount; i++) {
            scene_mesh_color_set(map->mesh,
                                 map->uvWobbles[i].y * map->gridWidth +
                                     map->uvWobbles[i].x,
                                 0.0f, 0.0f, 0.0f);
        }
        map->flags &= ~TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE;
    }
}

// FUNCTION: LEGORR 0x00450b60
void terrain_map_uv_wobble_update(TerrainMap* map, float elapsed)
{
    TerrainMapPoint2 uvOffsets[4];
    TerrainMapPoint2I blockOffsets[4];
    TerrainMapVertex vertices[4];
    TerrainMapPoint2* wobble;
    float speed, scale, dist, r1, r2, len, wx, wy;
    float ux, uy, tx, ty, cx, cy;
    unsigned int i, j, k, x, y, group;
    int rotated;
    int index;

    if (elapsed > 0.001f) {
        blockOffsets[1].x = 1;
        blockOffsets[2].x = 1;
        blockOffsets[2].y = 1;
        blockOffsets[3].y = 1;
        uvOffsets[0].x = 0.0f;
        uvOffsets[0].y = 1.0f;
        uvOffsets[1].x = 1.0f;
        uvOffsets[1].y = 1.0f;
        uvOffsets[2].x = 1.0f;
        uvOffsets[2].y = 0.0f;
        uvOffsets[3].x = 0.0f;
        uvOffsets[3].y = 0.0f;
        blockOffsets[0].x = 0;
        blockOffsets[0].y = 0;
        blockOffsets[1].y = 0;
        blockOffsets[3].x = 0;
        for (i = 0; i < map->uvWobbleCount; i++) {
            speed = 50.0f / elapsed;
            scale = 1.0f / (speed - -1.0f);
            x = map->uvWobbles[i].x;
            y = map->uvWobbles[i].y;
            group = map->gridWidth * y + x;
            rotated = map->blocks[y * map->blockWidth + x].flags &
                      TERRAIN_MAP_BLOCK_ROTATED;
            lwo_layer_polygon_vertex_data_read(map->mesh, group, 0, 4,
                                               vertices);
            for (j = 0; j < 4; j++) {
                k = j;
                if (rotated) k = (j + 1) & 3;
                index = (y + blockOffsets[j].y) * map->blockWidth + x +
                        blockOffsets[j].x;
                ux = vertices[k].tu;
                tx = map->blocks[index].uvWobble.x + uvOffsets[j].x;
                uy = vertices[k].tv;
                ty = map->blocks[index].uvWobble.y + uvOffsets[j].y;
                dist = (float)sqrt((uy - ty) * (uy - ty) +
                                   (ux - tx) * (ux - tx));
                cx = tx;
                cy = ty;
                if (dist <= 0.36f) {
                    cx = ux;
                    cy = uy;
                    if (dist < 0.05f) {
                        r1 = random_float_range(0.0f, 1.0f);
                        r2 = random_float_range(0.0f, 1.0f);
                        wobble = &map->blocks[index].uvWobble;
                        len = 1.0f / (float)sqrt(r2 * r2 + r1 * r1);
                        wx = (wobble->x + len * r1) * 0.3f;
                        wy = (wobble->y + len * r2) * 0.3f;
                        ty = wy + uvOffsets[j].y;
                        tx = wx + uvOffsets[j].x;
                        wobble->x = wx;
                        wobble->y = wy;
                    }
                }
                vertices[k].tu = scale * (speed * cx + tx);
                vertices[k].tv = scale * (speed * cy + ty);
            }
            container_mesh_set_vertices(map->mesh, group, 0, 4, vertices);
        }
    }
}

#include "terrain_map_maths.h"

// FUNCTION: LEGORR 0x00450f30
void terrain_map_block_vertex_normals_update(TerrainMap* map,
                                             unsigned int vx,
                                             unsigned int vy)
{
    TerrainMapPoint2 blockOffsets[4];
    TerrainMapVector3 normals[4];
    TerrainMapVector3 faceNormals[4];
    unsigned int i, j;
    float scale;

    blockOffsets[0].x = 0.0f;
    blockOffsets[0].y = 0.0f;
    blockOffsets[1].x = -1.0f;
    blockOffsets[1].y = 0.0f;
    blockOffsets[2].x = -1.0f;
    blockOffsets[2].y = -1.0f;
    blockOffsets[3].x = 0.0f;
    blockOffsets[3].y = -1.0f;
    normals[0].x = 0.0f;
    normals[0].y = 0.0f;
    normals[0].z = 0.0f;
    normals[1].x = 0.0f;
    normals[1].y = 0.0f;
    normals[1].z = 0.0f;
    normals[2].x = 0.0f;
    normals[2].y = 0.0f;
    normals[2].z = 0.0f;
    normals[3].x = 0.0f;
    normals[3].y = 0.0f;
    normals[3].z = 0.0f;
    for (i = 0; i < 4; i++) {
        blockOffsets[i].x += (float)vx;
        blockOffsets[i].y = (float)vy + blockOffsets[i].y;
    }
    for (i = 0; i < 4; i++) {
        terrain_map_block_vertex_normal_get(map, (int)blockOffsets[i].x,
                                            (int)blockOffsets[i].y, i,
                                            &faceNormals[i]);
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (!terrain_map_block_pair_fades(map, (int)blockOffsets[i].x,
                                              (int)blockOffsets[i].y,
                                              (int)blockOffsets[j].x,
                                              (int)blockOffsets[j].y)) {
                terrain_map_vector3d_add(&normals[i], &faceNormals[j],
                                         &normals[i]);
            }
        }
        terrain_map_vector3d_normalise(&normals[i]);
    }
    for (i = 0; i < 4; i++) {
        terrain_map_block_vertex_normal_set(map, (int)blockOffsets[i].x,
                                            (int)blockOffsets[i].y, i,
                                            &normals[i]);
    }
}

// FUNCTION: LEGORR 0x00451180
int terrain_map_block_pair_fades(TerrainMap* map, unsigned int bx1,
                                 unsigned int by1, unsigned int bx2,
                                 unsigned int by2)
{
    unsigned char texA;
    char texB;
    unsigned int i;

    if (by1 != by2 || bx1 != bx2) {
        if (bx1 < map->gridWidth && by1 < map->gridHeight &&
            bx2 < map->gridWidth && by2 < map->gridHeight) {
            texA = map->blocks[by1 * map->blockWidth + bx1].texture;
            texB = (char)map->blocks[by2 * map->blockWidth + bx2].texture;
            if (texA != 0xff && texB != -1) {
                for (i = 0; i < map->fadePairCount[texA]; i++) {
                    if ((char)map->fadePairs[texA][i] == texB) return 1;
                }
                return 0;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00451230
void terrain_map_block_vertex_normal_set(TerrainMap* map, unsigned int bx,
                                         unsigned int by,
                                         unsigned int vertex,
                                         const TerrainMapVector3* normal)
{
    TerrainMapVertex v;
    unsigned int group = map->gridWidth * by + bx;

    if (bx < map->gridWidth && by < map->gridHeight) {
        if (map->blocks[map->blockWidth * by + bx].flags &
            TERRAIN_MAP_BLOCK_ROTATED) {
            vertex = (vertex + 1) & 3;
        }
        lwo_layer_polygon_vertex_data_read(map->mesh, group, vertex, 1, &v);
        v.normal = *normal;
        container_mesh_set_vertices(map->mesh, group, vertex, 1, &v);
    }
}

// FUNCTION: LEGORR 0x004512d0
int terrain_map_block_vertex_normal_get(TerrainMap* map, unsigned int bx,
                                        unsigned int by,
                                        unsigned int vertex,
                                        TerrainMapVector3* normal)
{
    TerrainMapBlock* block;
    float scale;

    if (bx < map->gridWidth && by < map->gridHeight) {
        block = &map->blocks[by * map->blockWidth + bx];
        if (block->flags & TERRAIN_MAP_BLOCK_ROTATED) {
            vertex = (vertex + 1) & 3;
        }
        if (vertex == 0) {
            *normal = block->normalA;
        }
        if (vertex == 2) {
            *normal = map->blocks[by * map->blockWidth + bx].normalB;
        }
        if (vertex == 1 || vertex == 3) {
            *normal = map->blocks[by * map->blockWidth + bx].normalA;
            terrain_map_vector3d_add(
                normal, &map->blocks[by * map->blockWidth + bx].normalB,
                normal);
            terrain_map_vector3d_normalise(normal);
        }
        return 1;
    }
    normal->z = 0.0f;
    normal->y = 0.0f;
    normal->x = 0.0f;
    return 0;
}

// FUNCTION: LEGORR 0x00451450
void terrain_map_block_vertex_offset(TerrainMap* map, unsigned int bx,
                                     unsigned int by, unsigned int vertex,
                                     const TerrainMapVector3* offset)
{
    TerrainMapVertex v;
    unsigned int group = map->gridWidth * by + bx;

    if (bx < map->gridWidth && by < map->gridHeight) {
        if (map->blocks[map->blockWidth * by + bx].flags &
            TERRAIN_MAP_BLOCK_ROTATED) {
            vertex = (vertex + 1) & 3;
        }
        lwo_layer_polygon_vertex_data_read(map->mesh, group, vertex, 1, &v);
        v.position.x = offset->x + v.position.x;
        v.position.y = offset->y + v.position.y;
        v.position.z = offset->z + v.position.z;
        container_mesh_set_vertices(map->mesh, group, vertex, 1, &v);
    }
}

// FUNCTION: LEGORR 0x004514f0
void terrain_map_block_face_normals_update(TerrainMap* map,
                                           unsigned int bx,
                                           unsigned int by)
{
    TerrainMapVertex vertices[4];

    if (bx < map->gridWidth && by < map->gridHeight) {
        lwo_layer_polygon_vertex_data_read(map->mesh, by * map->gridWidth + bx,
                                           0, 4, vertices);
        surface_triangle_normal_calculate(
            &map->blocks[by * map->blockWidth + bx].normalA.x,
            &vertices[0].position.x, &vertices[1].position.x,
            &vertices[3].position.x);
        surface_triangle_normal_calculate(
            &map->blocks[by * map->blockWidth + bx].normalB.x,
            &vertices[1].position.x, &vertices[2].position.x,
            &vertices[3].position.x);
    }
}

// FUNCTION: LEGORR 0x004515a0
void terrain_map_file_specs_get(const TerrainMapFile* file,
                                unsigned int* width, unsigned int* height)
{
    *width = file->width;
    *height = file->height;
}

// FUNCTION: LEGORR 0x004515c0
unsigned short terrain_map_file_block_value(const TerrainMapFile* file,
                                            unsigned int bx,
                                            unsigned int by,
                                            unsigned int width)
{
    return file->cells[width * by + bx];
}

// FUNCTION: LEGORR 0x004515e0
int terrain_map_block_ray_intersection(TerrainMap* map, int bx, int by,
                                       const TerrainMapVector3* origin,
                                       const TerrainMapVector3* direction,
                                       TerrainMapVector3* intersection)
{
    TerrainMapVector3 v[4];
    TerrainMapVector3 normal;
    TerrainMapVector3 hitA;
    TerrainMapVector3 hitB;
    TerrainMapPoint2 point;
    TerrainMapPoint2 list[4];
    int rotated;
    int hitAFound = 0;
    int hitBFound = 0;

    rotated = terrain_map_block_vertices_get(map, bx, by, v);
    normal = map->blocks[by * map->blockWidth + bx].normalA;
    if (view_ray_intersection_test(&hitA.x, &origin->x, &direction->x,
                                   &v[0].x, &normal.x)) {
        if (!rotated) {
            list[0].x = v[0].x;
            list[0].y = v[0].y;
            list[1].x = v[1].x;
            list[1].y = v[1].y;
            list[2].x = v[3].x;
            list[2].y = v[3].y;
            list[3].x = v[0].x;
            list[3].y = v[0].y;
        }
        else {
            list[0].x = v[0].x;
            list[0].y = v[0].y;
            list[1].x = v[2].x;
            list[1].y = v[2].y;
            list[2].x = v[3].x;
            list[2].y = v[3].y;
            list[3].x = v[0].x;
            list[3].y = v[0].y;
        }
        point = *(TerrainMapPoint2*)&hitA;
        if (point_in_polygon_2d(&point.x, &list[0].x, &list[1].x, 3)) {
            hitAFound = 1;
        }
    }
    normal = map->blocks[by * map->blockWidth + bx].normalB;
    if (view_ray_intersection_test(&hitB.x, &origin->x, &direction->x,
                                   &v[2].x, &normal.x)) {
        if (!rotated) {
            list[0].x = v[1].x;
            list[0].y = v[1].y;
            list[1].x = v[2].x;
            list[1].y = v[2].y;
            list[2].x = v[3].x;
            list[2].y = v[3].y;
            list[3].x = v[1].x;
            list[3].y = v[1].y;
        }
        else {
            list[0].x = v[0].x;
            list[0].y = v[0].y;
            list[1].x = v[1].x;
            list[1].y = v[1].y;
            list[2].x = v[2].x;
            list[2].y = v[2].y;
            list[3].x = v[0].x;
            list[3].y = v[0].y;
        }
        point = *(TerrainMapPoint2*)&hitB;
        if (point_in_polygon_2d(&point.x, &list[0].x, &list[1].x, 3)) {
            hitBFound = 1;
        }
    }
    if (hitAFound) {
        if (!hitBFound) {
            if (hitAFound) {
                *intersection = hitA;
                return 1;
            }
        }
        else if (terrain_map_vector3d_distance(&hitA, origin) <
                 terrain_map_vector3d_distance(&hitB, origin)) {
            *intersection = hitA;
            return 1;
        }
    }
    else if (!hitBFound) {
        return 0;
    }
    *intersection = hitB;
    return 1;
}

// FUNCTION: LEGORR 0x004519b0
ModelEntryRecord* model_entry_record_load(ModelEntryRecord* previous,
                                          const char* partName,
                                          const char* dir,
                                          const char* meshName, int setID)
{
    char path[256];
    void* container;
    ModelEntryRecord* record;

    sprintf(path, "%s\\%s", dir, meshName);
    if ((container = container_load(NULL, path, "LWO", 1)) == NULL) {
        container = container_load(NULL, path, "MESH", 1);
    }
    if (container) {
        record = (ModelEntryRecord*)malloc(sizeof(ModelEntryRecord));
        record->flags = 2;
        if (previous) previous->next = record;
        record->object = container;
        record->next = NULL;
        record->kind = setID;
        record->name = (char*)malloc(strlen(partName) + 1);
        strcpy(record->name, partName);
        record->instance = NULL;
        return record;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00451ab0
ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind)
{
    ModelEntryRecord* record;

    record = (ModelEntryRecord*)malloc(sizeof(ModelEntryRecord));
    record->flags = 2;
    if (previous) previous->next = record;
    record->kind = kind;
    record->next = NULL;
    record->object = NULL;
    record->name = (char*)malloc(strlen(name) + 1);
    strcpy(record->name, name);
    record->instance = NULL;
    return record;
}

// FUNCTION: LEGORR 0x00451b30
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head)
{
    const ModelEntryRecord* node;
    ModelEntryRecord* clone;
    unsigned int count = 0;
    unsigned int i;

    for (node = head; node; node = node->next) count++;
    if (count) {
        clone = (ModelEntryRecord*)malloc(count * sizeof(ModelEntryRecord));
        i = 0;
        for (node = head; node; node = node->next) {
            clone[i] = *node;
            clone[i].instance = NULL;
            clone[i].flags = (clone[i].flags & ~2) | 1;
            clone[i].next = &clone[i + 1];
            i++;
        }
        clone[0].flags |= 2;
        clone[i - 1].next = NULL;
        return clone;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00451bc0
void resource_list_update_matching_key(void* list, void* library,
                                       void* update_value,
                                       unsigned int key)
{
    ModelEntryRecord* record;

    for (record = (ModelEntryRecord*)list; record; record = record->next) {
        if (key == (unsigned int)record->kind) {
            if (!record->instance) {
                const char* name = xf_texture_name_format(library, record->name, NULL);
                record->instance = resource_name_register_or_query_target(
                    (void**)library, name, 0, NULL);
            }
            if (record->instance) {
                interface_array_forward_sync(record->instance, record->object,
                                             update_value);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00451c30
void linked_record_field4_clear(void* record)
{
    ModelEntryRecord* node;

    for (node = (ModelEntryRecord*)record; node; node = node->next) {
        node->instance = NULL;
    }
}

// FUNCTION: LEGORR 0x00451c50
void owned_object_reference_chain_release(void* head)
{
    ModelEntryRecord* node;
    ModelEntryRecord* next;
    ModelEntryRecord* last = NULL;
    ModelEntryRecord* root = (ModelEntryRecord*)head;

    for (node = root; node; node = node->next) {
        if (!(node->flags & 1)) {
            if (node->name) free(node->name);
            if (node->object) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(node->object);
            }
        }
        if (node->flags & 2) {
            last = node;
        }
        else if (last) {
            last->next = node->next;
        }
    }
    if (last) {
        while (root) {
            next = root->next;
            free(root);
            root = next;
        }
    }
}
