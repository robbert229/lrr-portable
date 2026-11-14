// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008 GAME-008: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-008.csv. */
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "crt/crt_for_entity_cell_effects.hpp"
#include <stdint.h>
#include "ai.hpp"
#include "entity.hpp"
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
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_for_entity_cell_effects.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/simtick.hpp"
#include "gods98/surface.hpp"
#include "gods98/camera.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_entity_cell_effects.hpp"
#include "crt/crt_for_entity_cell_effects.hpp"
#include "entity_for_entity_cell_effects.hpp"
#include "level_for_entity_cell_effects.hpp"

// GLOBAL: LEGORR 0x004b8a54
uint32_t s_level_stored_word = 0u;  /* DAT_004b8a54 */

// FUNCTION: LEGORR 0x00409d40
void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: 00409d50 and 00409f20 in the original's shape, calling
 * the original LegoObject.c, Lego.c, Stats.c and terrain functions (#562,
 * finding 642). The callees have C linkage; the parameter types here are
 * this file's, the arguments the original's. Six callees are called by
 * their matching-build link names, which the port's headers already
 * declare: Building_GetShapePoints (00408360,
 * task_field84_get_and_field88_out), SelectPlace_TransformShapePoints
 * (00464110, rotated_pairs_scratch_fill), Effect_Spawn_Particle (0040cce0,
 * lava_effect_select_and_place) and Stats_GetCostOre/Crystal/RefinedOre
 * (00469ac0/af0/b20, entity_indexed_record_word_at_b0_get/_b4_get,
 * stats_table_word_at_b8). Natively the port's models below stay the live
 * ones. */
struct Gods_Point2I { int x, y; };
struct Gods_Vector3F { float x, y, z; };
/* Lego_GetCurrentMapTerrain (00431570) takes no argument; lego_unit.h
 * declares it with two (aitask_imports.h). */
#define ENTITY_CELL_CURRENT_TERRAIN() \
    (((void* (*)(void))level_current_terrain_get)())
extern "C" {
extern void* legoGlobs_buildingData;                                     /* 005561e0 */
void legoobject_surface_vector_normalize(EntitySlot* obj, float* outDir); /* 004422a0 */
int legoobject_position_to_terrain_cell(EntitySlot* obj, int* outX,
                                        int* outY);                      /* 004424f0 */
void legoobject_transform_position_get(EntitySlot* obj, float* outX,
                                       float* outY);                     /* 00442260 */
void lego_terrain_cell_primary_flags_mask_clear_and_refresh(Gods_Point2I* blockPos); /* 00432360 */
void lego_terrain_cell_secondary_bit0_clear(Gods_Point2I* blockPos);      /* 004323c0 */
void* level_current_terrain_get(int cell_x, int cell_y);                 /* 00431570 */
int terrain_map_block_to_world(void* map, unsigned int bx, unsigned int by,
                               float* xOut, float* yOut);                /* 0044f640 */
float terrain_map_world_z_get(void* map, float x, float y);              /* 0044f940 */
void electric_fence_block_surfaces_update(int bx, int by);               /* 0040cea0 */
void legoobject_build_placement_command_setup(int* cell, int id,
                                              void* world, int update);  /* 00439370 */
void legoobject_build_placement_alternate_command_setup(int* cell, int id,
                                                        void* world,
                                                        int update);     /* 004394b0 */
extern uint32_t legoGlobs_f_55644c;                                      /* 0055644c */
extern uint32_t legoGlobs_f_556450;                                      /* 00556450 */
extern void* legoGlobs_f_556454;                                         /* 00556454 */
int legoobject_remove(void* obj);                                        /* 00437540 */
void* legoobject_query_center_mask20(float* center);                     /* 00438cd0 */
void* legoobject_query_center_mask10(float* center);                     /* 00438d30 */
void* legoobject_query_center_mask200(float* center);                    /* 00438d90 */
void legoobject_activity_scene_state_update(void);                       /* 00439990 */
}

void entity_placement_commands_build_once(EntitySlot* entity);

namespace unported_ledger_stubs {
/* LegoObject_DestroyBuilding's cell pass (FUN_00409d50): clear each cell of
 * the building's rotated shape, spawn a rubble effect on it unless the
 * building is flagged +0x3ec & 8, then drop the building's cost. */
// FUNCTION: LEGORR 0x00409d50
extern "C" void entity_cell_path_effects_apply_current(EntitySlot* entity)
{
    unsigned int count;
    float dir[2];
    Gods_Point2I blockPos;
    float pos2D[2];
    Gods_Vector3F wPos;
    Gods_Vector3F randDir;
    int direction;
    Gods_Point2I* shapePoints;
    Gods_Point2I* shape;
    unsigned int i;
    int skip;
    Gods_Point2I* next;

    legoobject_surface_vector_normalize(entity, dir);
    legoobject_position_to_terrain_cell(entity, &blockPos.x, &blockPos.y);
    legoobject_transform_position_get(entity, &pos2D[0], &pos2D[1]);
    if (fabs(dir[0]) > fabs(dir[1])) {
        if (dir[0] > 0.0f) direction = 1;
        else direction = 3;
    } else {
        if (dir[1] > 0.0f) direction = 0;
        else direction = 2;
    }

    shapePoints = (Gods_Point2I*)task_field84_get_and_field88_out(
        (char*)legoGlobs_buildingData + entity->etype * 0x14c, &count);
    shape = (Gods_Point2I*)rotated_pairs_scratch_fill(&blockPos.x, &shapePoints->x,
                                                      count, direction);
    for (i = 0; i < count; i++) {
        next = &shape[i + 1];
        if (i != count - 1 && shape[i].x == next->x && shape[i].y == next->y)
            skip = 1;
        else skip = 0;

        lego_terrain_cell_primary_flags_mask_clear_and_refresh(&shape[i]);
        lego_terrain_cell_secondary_bit0_clear(&shape[i]);
        if (!(entity->flags_3ec & 8)) {
            terrain_map_block_to_world(ENTITY_CELL_CURRENT_TERRAIN(),
                                       shape[i].x, shape[i].y,
                                       &wPos.x, &wPos.y);
            wPos.z = terrain_map_world_z_get(ENTITY_CELL_CURRENT_TERRAIN(),
                                             wPos.x, wPos.y);
            d3drm_vector_random(&randDir.x);         /* view.hpp */
            randDir.z = 0.0f;
            lava_effect_select_and_place(3, &wPos.x, &randDir.x);
            terrain_cell_work_flag_clear_current(&shape[i].x, 0x3b); /* level.hpp */
        }
        electric_fence_block_surfaces_update(shape[i].x, shape[i].y);
        if (skip) i++;
    }

    entity_placement_commands_build_once(entity);
}
} // namespace unported_ledger_stubs

/* LegoObject_DropBuildingCost (FUN_00409f20): once per building, drop its
 * crystal and ore cost on its cell unless the level stored word is set. */
// FUNCTION: LEGORR 0x00409f20
void entity_placement_commands_build_once(EntitySlot* entity)
{
    int blockPos[2];
    unsigned int ore, crystals, i;

    if (!(entity->flags_3ec & 0x800)) {
        legoobject_position_to_terrain_cell(entity, &blockPos[0], &blockPos[1]);
        ore = entity_indexed_record_word_at_b0_get(
            4, entity->etype, entity->indexed_record_slot_330);
        crystals = entity_indexed_record_word_at_b4_get(
            4, entity->etype, entity->indexed_record_slot_330);
        stats_table_word_at_b8(4, entity->etype, entity->indexed_record_slot_330);
        if (s_level_stored_word == 0) {
            for (i = 0; i < crystals; i++)
                legoobject_build_placement_command_setup(blockPos, 0, NULL, 0);
            for (i = 0; i < ore; i++)
                legoobject_build_placement_alternate_command_setup(blockPos, 0,
                                                                   NULL, 0);
        }
        entity->flags_3ec |= 0x800;
    }
}

/* LegoObject_Destroy's tracked-pointer pass (FUN_00409ff0): when the object
 * is one of the three tracked objects (stats flags 0x10, 0x20 and 0x40,
 * legoGlobs 0055644c-00556454), replace it with the nearest other one, then
 * remove it and refresh the activity scenes. It takes the port's name,
 * which LegoObject.c, Message.c and the port call; entity.cpp compiles its
 * port model out of the matching build. Stats_GetStatsFlags1 (00469e60)
 * is entity_stats_word_get there (entity_stats_link_names.h). */
// FUNCTION: LEGORR 0x00409ff0
extern "C" void entity_destroy_tracked_pointer_cleanup(void* entity)
{
    float position[2];
    int replaceC = 0, replaceA = 0, replaceB = 0;

    legoobject_transform_position_get((EntitySlot*)entity, &position[0], &position[1]);
    if ((entity_stats_word_get((EntitySlot*)entity) & 0x10) &&
        legoGlobs_f_55644c == (uint32_t)entity) replaceA = 1;
    if ((entity_stats_word_get((EntitySlot*)entity) & 0x20) &&
        legoGlobs_f_556450 == (uint32_t)entity) replaceB = 1;
    if ((entity_stats_word_get((EntitySlot*)entity) & 0x40) &&
        legoGlobs_f_556454 == entity) replaceC = 1;
    if (replaceA)
        legoGlobs_f_55644c = (uint32_t)legoobject_query_center_mask10(position);
    if (replaceB)
        legoGlobs_f_556450 = (uint32_t)legoobject_query_center_mask20(position);
    if (replaceC)
        legoGlobs_f_556454 = legoobject_query_center_mask200(position);
    legoobject_remove(entity);
    legoobject_activity_scene_state_update();
}
#else
namespace unported_ledger_stubs {
/* EntityCellPathEffectsApply (FUN_00409d50) uses one entity argument and the
 * active level/type tables; there is no caller-supplied hook bundle. */
// PORT-ONLY: the native model of 0x00409d50; the matching build has the original's shape above.
void entity_cell_path_effects_apply_current(EntitySlot* entity)
{
    GameContext* context = entity != NULL ? entity->owner_ctx : NULL;
    if (context == NULL) context = entity_pool_callback_context_get();
    entity_cell_path_effects_apply_level(
        entity, context != NULL ? context->level : NULL);
}
} // namespace unported_ledger_stubs

/* EntityPlacementCommandsBuildOnce (FUN_00409f20): read the three indexed
 * stats words for this entity, enqueue the corresponding type-6/type-7
 * placement commands at its current terrain cell, and latch the per-entity
 * once bit. Empty/unloaded stats cells read as zero through the live table
 * seam, matching the other stats consumers in this reconstruction. */
// PORT-ONLY: the native model of 0x00409f20; the matching build has the original's shape above.
void entity_placement_commands_build_once(EntitySlot* entity)
{
    GameContext* context;
    LevelState* level;
    int cell_xy[2];
    int32_t type6_count;
    int32_t type7_count;
    int index;
    if (entity == NULL || (entity->flags_3ec & 0x800u) != 0u) return;
    context = entity_pool_callback_context_get();
    if (context == NULL) context = entity->owner_ctx;
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    entity_position_to_terrain_cell(entity, level, &cell_xy[0], &cell_xy[1]);
    if (!level_coord_pair_within_bounds(level, cell_xy[0], cell_xy[1])) return;

    /* The reference getters index through entity +0 (stats type), +4
     * (stats row), and +0x330 (strided stats instance). */
    type6_count = (int32_t)stats_table_word_at_b4(
        4, entity->etype, entity->indexed_record_slot_330);
    type7_count = (int32_t)stats_table_word_at_b0(
        4, entity->etype, entity->indexed_record_slot_330);
    (void)stats_table_word_at_b8(
        4, entity->etype, entity->indexed_record_slot_330);

    for (index = type6_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 0);
    for (index = type7_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 1);
    entity->flags_3ec |= 0x800u;
}
#endif
