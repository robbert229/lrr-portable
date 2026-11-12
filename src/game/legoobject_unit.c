// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 -- original translation unit (text 00436c20-0044c710; finding
 * 505): the original's LegoObject.c, the object manager (the object pool,
 * object creation and removal, the per-object activity and AI updates).
 *
 * Rebuilt in the original's shape (ADR 0005) in three parts by address. This
 * file holds part 1 (issue #575): the functions 00436c20-0043a960, in
 * original order, in the C-compatible subset, so the matching build compiles
 * it as C. The native port compiles it as C++. Parts 2 and 3 move the rest
 * of the unit here from legoobject.cpp.
 *
 * The port's own model of LegoObject.c (EntitySlot, the GameContext and the
 * hook tables) is a different contract. Its implementations stay,
 * unannotated and under the port's names, in legoobject.cpp, which becomes
 * the unit's port tail once parts 2 and 3 have drained it. Native callers
 * keep using them. The functions here that keep a port name are the ones
 * whose callers already use the original's contract.
 *
 * The original keeps its state in one LegoObject_Globs struct (objectGlobs,
 * 004de790-004eadd0). The port models its fields as separate globals in
 * legoobject.cpp, which this unit reads with C linkage
 * (legoobject_unit_imports.h). Fields the port does not model are defined
 * here, one annotated global each. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "legoobject_unit.h"
#include "legoobject_unit_imports.h"

/* ------------------------------------------------------------------ */
/* State: objectGlobs fields the port does not model, and legoGlobs     */
/* stand-ins.                                                           */
/* ------------------------------------------------------------------ */

/* ---- state, group A ---- */


/* ---- state, group B ---- */


/* ---- state, group C ---- */


/* ---- state, group D ---- */


/* ---- state, group E ---- */


/* ---- state, group F ---- */


/* ---- state, group G ---- */


/* ------------------------------------------------------------------ */
/* Functions, in original order.                                       */
/* ------------------------------------------------------------------ */

/* ---- functions, group A (00436c20-00437510) ---- */


/* ---- functions, group B (00437540-00437cc0) ---- */


/* ---- functions, group C (00437d00-00438580) ---- */


/* ---- functions, group D (004385b0-00438ed0) ---- */


/* ---- functions, group E (00438f60-00439930) ---- */


/* ---- functions, group F (00439950-00439e40) ---- */

/* 00439950: the tools' display names from the ToolNames block, into
 * legoGlobs.langToolName (00556228), which follows legoGlobs.toolName
 * (005561fc) in the original's Lego_Globs. */
// FUNCTION: LEGORR 0x00439950
void legoobject_tool_name_strings_load(void* config, const char* gameName)
{
    unsigned int i;

    for (i = 0; i < LEGO_TOOL_TYPE_COUNT; i++) {
        legoGlobs_langToolName[i] = block_get_str_copy(
            (struct BlockRec*)config,
            // STRING: LEGORR 0x004a5068
            string_path_join(gameName, "ToolNames", legoGlobs_toolName[i], NULL));
    }
}

// FUNCTION: LEGORR 0x00439990
void legoobject_activity_scene_state_update(void)
{
    if (s_entity_state_flags & 0x10) {
        s_entity_state_flags |= 0x8;
        return;
    }
    s_entity_state_flags |= 0x4;
    global_object_field98_clear_current();
}

// FUNCTION: LEGORR 0x004399c0
int legoobject_flag400_and_health_threshold_check(Lego_Object* obj)
{
    unsigned int capacity;

    if (obj->flags1 & 0x400) {
        if (obj->type == 1) {
            capacity = (obj->carryNullFrames < stats_instance_word_at_ec(obj))
                ? obj->carryNullFrames : stats_instance_word_at_ec(obj);
            if (obj->numCarriedObjects < capacity && obj->field_3d4 < 75.0f)
                return 0;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439a20
int legoobject_gated_step_command_issue(Lego_Object* obj, Lego_Object* target)
{
    int* routeX;
    int* routeY;
    int fromX;
    int fromY;
    unsigned int count;
    float targetPos[2];
    float pos[2];

    if (legoobject_flag400_and_health_threshold_check(obj) == 0 &&
        legoobject_accepts_target_for_task_check(obj, target, 1))
    {
        int toX;
        int toY;

        entity_transform_position_get_outputs(LEGOOBJ_SLOT(target), &targetPos[0], &targetPos[1]);
        terrain_map_world_to_block(Lego_GetMap(), targetPos[0], targetPos[1], &toX, &toY);
        entity_transform_position_get_outputs(LEGOOBJ_SLOT(obj), &pos[0], &pos[1]);
        terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &fromX, &fromY);

        if (fromX == toX && fromY == toY) {
            entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), 1,
                (const float*)&toX, (const float*)&toY, targetPos);
        } else {
            if (!terrain_neighbor_path_check_current(obj, fromX, fromY, toX, toY,
                    &routeX, &routeY, (float*)&count, NULL, NULL))
                return 0;
            entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), count,
                (const float*)routeX, (const float*)routeY, targetPos);
            free(routeX);
            free(routeY);
        }
        ((unsigned char*)obj->routeBlocks)[obj->routeBlocksTotal * 0x14 - 3] = 3;
        obj->routeToObject = target;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439b80
int legoobject_task_target_cell_eligibility_check(Lego_Object* obj, Lego_Object* target)
{
    if (obj->type == 1) {
        if (target->id == 0) {
            if (!(entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x200))
                return legoobject_query_center_mask02(obj, NULL, target->type, target->objLevel) != NULL;
        } else {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00439bd0
int legoobject_accepts_target_for_task_check(Lego_Object* obj, Lego_Object* target, int extra)
{
    if (obj->type == 3) {
        if (!(obj->flags3 & 0x8000000))
            return 0;
    } else if (!(obj->flags3 & 0x20)) {
        return 0;
    }
    if (obj->flags1 & 0x1)
        return 0;
    if (extra && entity_flags_mask107c33de_or_1c00203_any_test(LEGOOBJ_SLOT(obj)))
        return 0;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x4) && target->type != 6)
        return 0;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2) && target->type != 7)
        return 0;
    if (obj->numCarriedObjects != 0 && obj->carriedObjects[0]->type != target->type)
        return 0;
    return 1;
}

// FUNCTION: LEGORR 0x00439c80
void legoobject_indexed_flag_set_and_refresh(Lego_Object* obj)
{
    unsigned int level;

    if (obj->type == 1) {
        level = entity_type_mask_get(obj->vehicle);
        level |= 1 << obj->field_404;
        scene_record_indexed_entries_rebuild_original(obj->vehicle, level);
        entity_building_index_set(LEGOOBJ_SLOT(obj), level);
        advisor_text_animation_update(0x19, 1, 0);
    }
}

// FUNCTION: LEGORR 0x00439cd0
void legoobject_animation_state_set(Lego_Object* obj, unsigned int state)
{
    entity_building_index_set(LEGOOBJ_SLOT(obj), state);
}

// FUNCTION: LEGORR 0x00439cf0
unsigned int legoobject_stats_flag10_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x10;
}

// FUNCTION: LEGORR 0x00439d10
unsigned int legoobject_stats_flag20_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20;
}

// FUNCTION: LEGORR 0x00439d30
unsigned int legoobject_stats_flag40_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x40;
}

// FUNCTION: LEGORR 0x00439d50
int legoobject_vehicle_unload_adjacent_cell_find(Lego_Object* obj, int* outCell)
{
    LegoObjF_Point2I blockPos;
    LegoObjF_Point2I offsets[4];
    unsigned int i;

    offsets[0].x = 0;
    offsets[0].y = -1;
    offsets[1].x = 1;
    offsets[1].y = 0;
    offsets[2].x = 0;
    offsets[2].y = 1;
    offsets[3].x = -1;
    offsets[3].y = 0;

    legoobject_block_position_get(LEGOOBJ_SLOT(obj), &blockPos.x, &blockPos.y);
    for (i = 0; i < 4; i++) {
        offsets[i].x += blockPos.x;
        offsets[i].y += blockPos.y;
        if (terrain_cell_flag_8_bounded_current(offsets[i].x, offsets[i].y) &&
            terrain_cell_route_and_occupancy_check_current(NULL, offsets[i].x, offsets[i].y,
                offsets[i].x, offsets[i].y, 0))
        {
            if (outCell != NULL)
                *(LegoObjF_Point2I*)outCell = offsets[i];
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439e10
int legoobject_vehicle_unload_cell_find_if_eligible(Lego_Object* obj, int* outCell)
{
    if (entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x10)
        return legoobject_vehicle_unload_adjacent_cell_find(obj, outCell);
    return 0;
}

// FUNCTION: LEGORR 0x00439e40
int legoobject_record_flag20_query(Lego_Object* obj, int* outCell)
{
    if (entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x20)
        return legoobject_vehicle_unload_adjacent_cell_find(obj, outCell);
    return 0;
}

/* ---- functions, group G (00439e70-0043a960) ---- */

