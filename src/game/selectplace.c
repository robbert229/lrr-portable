// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-040: the building placement marker (the original's SelectPlace.c, a
 * name inferred from OpenLRR's), rebuilt in original shape (ADR 0005,
 * finding 633). Text 00463f00-00464740, .rdata 0049e718-0049e738,
 * .bss 005013c8-00501468.
 *
 * SelectPlace_Create builds a transparent mesh of ten groups, one per block
 * a building can cover, each a five-sided box (four walls and a lid) of
 * twenty vertices. SelectPlace_CheckAndUpdate rotates the building's shape
 * to the cursor block, checks every block (a path next to it, the terrain
 * tolerance, the water entrances), drapes one box over each block's corners
 * in the colour of the result, and returns the rotated shape when the
 * building can go there.
 *
 * The port's models of the same functions stay in selectplace_port.cpp,
 * unannotated: natively they are the live ones and nothing calls the unit.
 * In the matching build the four functions take the port's names
 * (selectplace_link_names.h). */
#include "selectplace_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "selectplace.h"
#include "selectplace_imports.h"

/* The most blocks a building covers: one mesh group each. */
#define SELECTPLACE_MAXGROUPS 10
/* The shape buffer SelectPlace_TransformShape fills. */
#define SELECTPLACE_MAXSHAPEPOINTS 20

/* Container_MeshType_Transparent and Container_Quality_Gouraud. */
#define SELECTPLACE_MESHTYPE_TRANSPARENT 3
#define SELECTPLACE_QUALITY_GOURAUD 3

typedef struct SelectPlace_Vector3F {
    float x, y, z;
} SelectPlace_Vector3F;

/* Container.c's mesh vertex (0x24 bytes). */
typedef struct SelectPlace_Vertex {
    SelectPlace_Vector3F position;  /* +0x00 */
    SelectPlace_Vector3F normal;    /* +0x0c */
    float tu, tv;       /* +0x18 */
    unsigned int colour; /* +0x20 */
} SelectPlace_Vertex;

// GLOBAL: LEGORR 0x005013c8
static SelectPlace_Point2I selectPlaceShape[SELECTPLACE_MAXSHAPEPOINTS];

// FUNCTION: LEGORR 0x00463f00
SelectPlace* selectplace_create(struct Container* root, float tileDepth)
{
    SelectPlace* selectPlace;
    unsigned int loop, sub, face;
    float tvList[4];
    float tuList[4];
    unsigned int faceIndices[6];
    SelectPlace_Vector3F normalList[5];
    unsigned int faceData[5 * 6];
    SelectPlace_Vertex vertices[4];

    if ((selectPlace = (SelectPlace*)malloc(sizeof(SelectPlace)))) {
        memset(selectPlace, 0, sizeof(SelectPlace));
        if ((selectPlace->contMesh = Container_MakeMesh2(root, SELECTPLACE_MESHTYPE_TRANSPARENT))) {
            tuList[0] = 0.0f; tuList[1] = 1.0f; tuList[2] = 1.0f; tuList[3] = 0.0f;
            tvList[0] = 0.0f; tvList[1] = 0.0f; tvList[2] = 1.0f; tvList[3] = 1.0f;

            normalList[0].x = 0.0f; normalList[0].y = 0.0f; normalList[0].z = -1.0f;
            normalList[1].x = 0.0f; normalList[1].y = 1.0f; normalList[1].z = 0.0f;
            normalList[2].x = 1.0f; normalList[2].y = 0.0f; normalList[2].z = 0.0f;
            normalList[3].x = 0.0f; normalList[3].y = -1.0f; normalList[3].z = 0.0f;
            normalList[4].x = -1.0f; normalList[4].y = 0.0f; normalList[4].z = 0.0f;

            faceIndices[0] = 0; faceIndices[1] = 1; faceIndices[2] = 3;
            faceIndices[3] = 1; faceIndices[4] = 2; faceIndices[5] = 3;

            for (face = 0; face < 5; face++) {
                for (sub = 0; sub < 6; sub++) {
                    faceData[(face * 6) + sub] = faceIndices[sub] + (face * 4);
                }
            }

            selectPlace->tileDepth = tileDepth;

            for (loop = 0; loop < SELECTPLACE_MAXGROUPS; loop++) {
                Container_Mesh_AddGroup(selectPlace->contMesh, 5 * 4, 5 * 2, 3, (unsigned long*)faceData);
                for (face = 0; face < 5; face++) {
                    for (sub = 0; sub < 4; sub++) {
                        vertices[sub].normal = normalList[face];
                        vertices[sub].colour = 0xffffffff;
                        vertices[sub].tu = tuList[sub];
                        vertices[sub].tv = tvList[sub];
                    }
                    Container_Mesh_SetVertices(selectPlace->contMesh, loop, face * 4, 4, vertices);
                }
                Container_Mesh_SetQuality(selectPlace->contMesh, loop, SELECTPLACE_QUALITY_GOURAUD);
            }

            Container_Hide(selectPlace->contMesh, 1);
            return selectPlace;
        }
        free(selectPlace);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00464110
SelectPlace_Point2I* selectplace_transform_shape(const SelectPlace_Point2I* blockPos,
                                                 const SelectPlace_Point2I* shapePoints,
                                                 unsigned int shapeCount,
                                                 int rotation)
{
    unsigned int loop;
    int temp;

    for (loop = 0; loop < shapeCount; loop++) {
        selectPlaceShape[loop] = shapePoints[loop];
        switch ((rotation + 0x8000) % 4) {
        case 0:
            break;
        case 1:
            temp = selectPlaceShape[loop].x;
            selectPlaceShape[loop].x = -selectPlaceShape[loop].y;
            selectPlaceShape[loop].y = temp;
            break;
        case 2:
            selectPlaceShape[loop].x = -selectPlaceShape[loop].x;
            selectPlaceShape[loop].y = -selectPlaceShape[loop].y;
            break;
        case 3:
            temp = selectPlaceShape[loop].x;
            selectPlaceShape[loop].x = selectPlaceShape[loop].y;
            selectPlaceShape[loop].y = -temp;
            break;
        }
        selectPlaceShape[loop].x += blockPos->x;
        selectPlaceShape[loop].y += blockPos->y;
    }
    return selectPlaceShape;
}

// FUNCTION: LEGORR 0x004641c0
SelectPlace_Point2I* selectplace_check_and_update(SelectPlace* selectPlace,
                                                  SelectPlace_Point2I* blockPos,
                                                  const SelectPlace_Point2I* shapePoints,
                                                  unsigned int shapeCount, int rotation,
                                                  void* map, unsigned int waterEntrances)
{
    SelectPlace_Vertex vertices[5 * 4];
    TerrainMapVector3 wallList[8];
    unsigned int vertexIndices[5 * 4];
    SelectPlace_Point2I* shape;
    int validTolerance;
    int pathFound = 0;
    int doubleFound = 0;
    int doublePathFound = 0;
    int valid = 1;
    unsigned int loop, sub, face;
    unsigned int group = 0;
    int isDouble, isWater;
    float red, green, blue;

    vertexIndices[0] = 0; vertexIndices[1] = 1; vertexIndices[2] = 2; vertexIndices[3] = 3;
    vertexIndices[4] = 0; vertexIndices[5] = 4; vertexIndices[6] = 5; vertexIndices[7] = 1;
    vertexIndices[8] = 1; vertexIndices[9] = 5; vertexIndices[10] = 6; vertexIndices[11] = 2;
    vertexIndices[12] = 2; vertexIndices[13] = 6; vertexIndices[14] = 7; vertexIndices[15] = 3;
    vertexIndices[16] = 3; vertexIndices[17] = 7; vertexIndices[18] = 4; vertexIndices[19] = 0;

    shape = selectplace_transform_shape(blockPos, shapePoints, shapeCount, rotation);
    validTolerance = terrain_map_shape_tolerance_check(LEGO_LEVEL_PTR(TerrainMap, legoGlobs_currLevel->map), (TerrainMapPoint2I*)shape, shapeCount,
                                                       legoGlobs_currLevel->buildingTolerance,
                                                       legoGlobs_currLevel->buildingMaxVariation);

    if (shapeCount) {
        SelectPlace_Point2I offsets[4];
        SelectPlace_Point2I pos;
        int pathHere;

        offsets[0].x = 0; offsets[0].y = -1;
        offsets[1].x = 1; offsets[1].y = 0;
        offsets[2].x = 0; offsets[2].y = 1;
        offsets[3].x = -1; offsets[3].y = 0;

        for (loop = 0; loop < shapeCount; loop++) {
            pathHere = 0;
            for (sub = 0; sub < 4; sub++) {
                pos.x = shape[loop].x + offsets[sub].x;
                pos.y = shape[loop].y + offsets[sub].y;
                if (level_cell_occupied_at_current(&pos.x) && !terrain_cell_flag_8000_current(&pos.x)) {
                    pathFound = 1;
                    pathHere = 1;
                }
            }
            if (loop < shapeCount - 1 && shape[loop].x == shape[loop + 1].x && shape[loop].y == shape[loop + 1].y) {
                if (pathHere) doublePathFound = 1;
                loop++;
                doubleFound = 1;
            }
        }
    }

    if (doubleFound) pathFound = doublePathFound;

    for (loop = 0; loop < shapeCount; loop++) {
        if (loop < shapeCount - 1 && shape[loop].x == shape[loop + 1].x && shape[loop].y == shape[loop + 1].y) {
            isDouble = 1;
            isWater = (group < waterEntrances + 1);
        } else {
            isDouble = 0;
            isWater = 0;
        }

        if (!lego_terrain_placement_blocked_adjacent_cell_scan(shape[loop].x, shape[loop].y, isDouble, isWater) ||
            !pathFound || lego_terrain_cell_command_predicate(&blockPos->x)) {
            valid = 0;
            red = 0.6f; green = 0.0f; blue = 0.0f;
        } else if (!isWater) {
            if (isDouble) {
                red = 0.7f; green = 0.7f; blue = 0.0f;
            } else if (!validTolerance) {
                valid = 0;
                red = 0.7f; green = 0.0f; blue = 0.7f;
            } else {
                red = 0.0f; green = 0.7f; blue = 0.1f;
            }
        } else {
            red = 0.0f; green = 0.7f; blue = 0.9f;
        }

        Container_Mesh_SetColourAlpha(selectPlace->contMesh, group, red, green, blue, 0.2f);
        Container_Mesh_SetEmissive(selectPlace->contMesh, group, red * 0.5f, green * 0.5f, blue * 0.5f);

        if (level_coord_pair_within_bounds_target((const unsigned int*)map, shape[loop].x, shape[loop].y)) {
            terrain_map_block_vertices_raw_get((TerrainMap*)map, shape[loop].x, shape[loop].y, wallList);
            for (sub = 0; sub < 4; sub++) {
                wallList[sub + 4] = wallList[sub];
                wallList[sub].z -= selectPlace->tileDepth;
            }
            Container_Mesh_GetVertices(selectPlace->contMesh, group, 0, 5 * 4, vertices);
            for (face = 0; face < 5; face++) {
                for (sub = 0; sub < 4; sub++) {
                    vertices[face * 4 + sub].position.x = wallList[vertexIndices[face * 4 + sub]].x;
                    vertices[face * 4 + sub].position.y = wallList[vertexIndices[face * 4 + sub]].y;
                    vertices[face * 4 + sub].position.z = wallList[vertexIndices[face * 4 + sub]].z;
                }
            }
            Container_Mesh_SetVertices(selectPlace->contMesh, group, 0, 5 * 4, vertices);
            Container_Mesh_HideGroup(selectPlace->contMesh, group, 0);
        } else {
            Container_Mesh_HideGroup(selectPlace->contMesh, group, 1);
        }

        group++;
        if (isDouble) loop++;
    }

    for (; group < SELECTPLACE_MAXGROUPS; group++) {
        Container_Mesh_HideGroup(selectPlace->contMesh, group, 1);
    }

    return valid ? shape : NULL;
}

// FUNCTION: LEGORR 0x00464720
void selectplace_hide(SelectPlace* selectPlace, int hide)
{
    Container_Hide(selectPlace->contMesh, hide);
}
