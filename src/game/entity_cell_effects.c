// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008: a small unit of building teardown between Construction.c and
 * Credits.c, rebuilt in original shape (ADR 0005, finding 652). Text
 * 00409d40-0040a0c0, .rdata 0049e0a0, .bss 004b8a54. The binary does not
 * name the file, and the functions' original names are not known; the
 * names here describe them.
 *
 * When a building is destroyed, its cells are cleared (with a rubble effect
 * on each unless the building is flagged +0x3ec & 8) and, once per
 * building, its crystal and ore cost is dropped on its cell unless the
 * level disabled that (004b8a54, which Lego.c and Objective.c set). When
 * the destroyed object is one of the three tracked objects in legoGlobs,
 * the nearest other one replaces it.
 *
 * The port's models stay in entity_cell_effects_port.cpp, unannotated:
 * natively they are the live ones and nothing calls the unit. In the
 * matching build three functions take the names their callers use
 * (entity_cell_effects_link_names.h). */
#include "entity_cell_effects_link_names.h"

#include <math.h>
#include <stddef.h>

#include "entity_cell_effects.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The callees, by the names they have in the matching build. Their units'
 * headers declare them with the port's types or not at all. */
typedef struct EntityCell_Point2I {
    int x;
    int y;
} EntityCell_Point2I;

typedef struct EntityCell_Vector3F {
    float x;
    float y;
    float z;
} EntityCell_Vector3F;

/* GAME-006 Building.c: Building_GetShapePoints. */
EntityCell_Point2I* building_get_shape_points(void* building,
                                              unsigned int* count); /* 00408360 */
/* GAME-012: Effect_Spawn_Particle. */
int effect_spawn_particle(unsigned int type, EntityCell_Vector3F* wPos,
                          EntityCell_Vector3F* dir);                /* 0040cce0 */
/* GAME-013: ElectricFence_UpdateBlockSurfaces. */
void electric_fence_block_surfaces_update(int bx, int by);          /* 0040cea0 */
/* GAME-021 Lego.c. */
void terrain_cell_work_flag_clear_current(EntityCell_Point2I* blockPos,
                                          unsigned int flags);       /* 00430320 */
/* Lego_GetCurrentMapTerrain takes no argument; lego_unit.c defines it
 * with two. */
void* level_current_terrain_get(void);                              /* 00431570 */
void lego_terrain_cell_primary_flags_mask_clear_and_refresh(EntityCell_Point2I* blockPos); /* 00432360 */
void lego_terrain_cell_secondary_bit0_clear(EntityCell_Point2I* blockPos); /* 004323c0 */
extern char* legoGlobs_buildingData;                                /* 005561e0 */
extern Lego_Object* legoGlobs_f_55644c;                             /* 0055644c */
extern Lego_Object* legoGlobs_f_556450;                             /* 00556450 */
extern Lego_Object* legoGlobs_f_556454;                             /* 00556454 */
/* GAME-026 Map3D.c. */
int terrain_map_block_to_world(void* map, unsigned int bx, unsigned int by,
                               float* xOut, float* yOut);            /* 0044f640 */
float terrain_map_world_z_get(void* map, float x, float y);         /* 0044f940 */
/* GAME-040 SelectPlace.c: SelectPlace_TransformShapePoints. */
EntityCell_Point2I* selectplace_transform_shape(const EntityCell_Point2I* blockPos,
                                                const EntityCell_Point2I* shapePoints,
                                                unsigned int shapeCount,
                                                int rotation);       /* 00464110 */
/* GAME-043 Stats.c. */
int stats_get_cost_ore(int objType, int objID, unsigned int objLevel);         /* 00469ac0 */
int stats_get_cost_crystal(int objType, int objID, unsigned int objLevel);     /* 00469af0 */
int stats_get_cost_refined_ore(int objType, int objID, unsigned int objLevel); /* 00469b20 */
unsigned int stats_object_get_stats_flags1(Lego_Object* obj);                  /* 00469e60 */
/* gods98 Maths.c: Maths_Vector3DRandom. */
float* d3drm_vector_random(float* r);                               /* 00479500 */

#ifdef __cplusplus
}
#endif

/* The level's switch for the building cost drop (004b8a54). */
// GLOBAL: LEGORR 0x004b8a54
static int entityCell_costDropDisabled;

/* Set whether destroyed buildings drop their cost. */
// FUNCTION: LEGORR 0x00409d40
void building_set_cost_drop_disabled(int disabled)
{
    entityCell_costDropDisabled = disabled;
}

/* Clear each cell of the building's rotated shape, spawn a rubble effect on
 * it unless the building is flagged +0x3ec & 8, then drop the building's
 * cost. */
// FUNCTION: LEGORR 0x00409d50
void building_remove_from_terrain(Lego_Object* buildingObj)
{
    unsigned int count;
    float dir[2];
    EntityCell_Point2I blockPos;
    float pos2D[2];
    EntityCell_Vector3F wPos;
    EntityCell_Vector3F randDir;
    int direction;
    EntityCell_Point2I* shapePoints;
    EntityCell_Point2I* shape;
    unsigned int i;
    int skip;
    EntityCell_Point2I* next;

    legoobject_surface_vector_normalize(buildingObj, dir);
    legoobject_position_to_terrain_cell(buildingObj, &blockPos.x, &blockPos.y);
    legoobject_transform_position_get(buildingObj, &pos2D[0], &pos2D[1]);
    if (fabs(dir[0]) > fabs(dir[1])) {
        if (dir[0] > 0.0f) direction = 1;
        else direction = 3;
    } else {
        if (dir[1] > 0.0f) direction = 0;
        else direction = 2;
    }

    shapePoints = building_get_shape_points(
        legoGlobs_buildingData + buildingObj->id * 0x14c, &count);
    shape = selectplace_transform_shape(&blockPos, shapePoints, count, direction);
    for (i = 0; i < count; i++) {
        next = &shape[i + 1];
        if (i != count - 1 && shape[i].x == next->x && shape[i].y == next->y)
            skip = 1;
        else skip = 0;

        lego_terrain_cell_primary_flags_mask_clear_and_refresh(&shape[i]);
        lego_terrain_cell_secondary_bit0_clear(&shape[i]);
        if (!(buildingObj->flags4 & 8)) {
            terrain_map_block_to_world(level_current_terrain_get(),
                                       shape[i].x, shape[i].y,
                                       &wPos.x, &wPos.y);
            wPos.z = terrain_map_world_z_get(level_current_terrain_get(),
                                             wPos.x, wPos.y);
            d3drm_vector_random(&randDir.x);
            randDir.z = 0.0f;
            effect_spawn_particle(3, &wPos, &randDir);
            terrain_cell_work_flag_clear_current(&shape[i], 0x3b);
        }
        electric_fence_block_surfaces_update(shape[i].x, shape[i].y);
        if (skip) i++;
    }

    building_drop_cost(buildingObj);
}

/* Once per building, drop its crystal and ore cost on its cell unless the
 * level disabled the drop. */
// FUNCTION: LEGORR 0x00409f20
void building_drop_cost(Lego_Object* buildingObj)
{
    int blockPos[2];
    unsigned int ore, crystals, i;

    if (!(buildingObj->flags4 & 0x800)) {
        legoobject_position_to_terrain_cell(buildingObj, &blockPos[0], &blockPos[1]);
        ore = stats_get_cost_ore(4, buildingObj->id, buildingObj->objLevel);
        crystals = stats_get_cost_crystal(4, buildingObj->id, buildingObj->objLevel);
        stats_get_cost_refined_ore(4, buildingObj->id, buildingObj->objLevel);
        if (entityCell_costDropDisabled == 0) {
            for (i = 0; i < crystals; i++)
                legoobject_build_placement_command_setup(blockPos, 0, NULL, 0);
            for (i = 0; i < ore; i++)
                legoobject_build_placement_alternate_command_setup(blockPos, 0,
                                                                   NULL, 0);
        }
        buildingObj->flags4 |= 0x800;
    }
}

/* When the object is one of the three tracked objects (stats flags 0x10,
 * 0x20 and 0x40), replace it with the nearest other one, then remove it
 * and refresh the activity scenes. */
// FUNCTION: LEGORR 0x00409ff0
void object_tracked_replace_and_remove(Lego_Object* liveObj)
{
    float position[2];
    int replaceC = 0, replaceA = 0, replaceB = 0;

    legoobject_transform_position_get(liveObj, &position[0], &position[1]);
    if ((stats_object_get_stats_flags1(liveObj) & 0x10) &&
        legoGlobs_f_55644c == liveObj) replaceA = 1;
    if ((stats_object_get_stats_flags1(liveObj) & 0x20) &&
        legoGlobs_f_556450 == liveObj) replaceB = 1;
    if ((stats_object_get_stats_flags1(liveObj) & 0x40) &&
        legoGlobs_f_556454 == liveObj) replaceC = 1;
    if (replaceA)
        legoGlobs_f_55644c = legoobject_query_center_mask10(position);
    if (replaceB)
        legoGlobs_f_556450 = legoobject_query_center_mask20(position);
    if (replaceC)
        legoGlobs_f_556454 = legoobject_query_center_mask200(position);
    legoobject_remove(liveObj);
    legoobject_activity_scene_state_update();
}
