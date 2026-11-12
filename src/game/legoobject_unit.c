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

/* Whether obj is the object the "Docks" definition names (its type and
 * index). The spatial query callback below passes the object itself. */
// FUNCTION: LEGORR 0x00438f60
int legoobject_dock_definition_matches_cell(Lego_Object* obj)
{
    int type;
    int index;

    // STRING: LEGORR 0x004a5060
    if (entity_type_pair_lookup("Docks", &type, &index, 0) &&
        obj->type == type && obj->id == index) {
        return 1;
    }
    return 0;
}

/* LegoObject_RunThroughLists callback of the spatial queries: keeps the
 * nearest object that the search mask accepts. Always returns 0, so the
 * walk visits every object. */
// FUNCTION: LEGORR 0x00438fb0
int legoobject_route_marker_candidate_visit(Lego_Object* obj, void* data)
{
    LegoObjGE_RouteMarkerSearch* search = (LegoObjGE_RouteMarkerSearch*)data;
    unsigned int flags = search->flags;
    int found = 0;
    float pos[2];
    float distance;

    if (legoobject_shared_eligibility_check(obj, flags & 0x400)) {
        flags &= ~0x400;
        if (!(flags & 0x4000) || !(obj->flags4 & 0x10)) {
            if (flags == 0) {
                if (obj->type == 4) found = 1;
            } else {
                if (flags & 0x1) {
                    if ((entity_stats_word_get((struct EntitySlot*)obj) & 0x8) &&
                        legoobject_action_kind_supported(
                            obj, search->actionKind, search->actionArgument,
                            search->actionExtra)) {
                        found = 1;
                    }
                }
                if (flags & 0x1000) {
                    if (legoobject_dock_definition_matches_cell(obj)) found = 1;
                }
                if (flags & 0x2000) {
                    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x400) &&
                        !(entity_state_flags_get((struct EntitySlot*)obj) & 0x200) &&
                        legoobject_shared_eligibility_check(obj, 0)) {
                        found = 1;
                    }
                }
                if (flags & 0x100) {
                    if (legoobject_flags_contain_requested_single_bit(obj, search->selector)) found = 1;
                }
                if (flags & 0x40) {
                    if (!(entity_stats_word_get((struct EntitySlot*)obj) & 0x8) && obj->type == 4) found = 1;
                }
                if (flags & 0x80) {
                    if ((entity_state_flags_get((struct EntitySlot*)obj) & 0x2) &&
                        obj->interactObject == NULL) {
                        found = 1;
                    }
                }
                if (flags & 0x800) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x80000000) found = 1;
                }
                if (flags & 0x10) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x10) found = 1;
                }
                if (flags & 0x20) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x20) found = 1;
                }
                if (flags & 0x200) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x40) found = 1;
                }
                if (flags & 0x2) {
                    if (((entity_stats_word_get((struct EntitySlot*)obj) & 0x2) && search->actionKind == 7) ||
                        ((entity_stats_word_get((struct EntitySlot*)obj) & 0x4) && search->actionKind == 6)) {
                        found = 1;
                    }
                }
                if (flags & 0x4) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4000) found = 1;
                }
                if (flags & 0x8) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x100000) found = 1;
                }
            }
            if (found) {
                entity_transform_position_get_outputs((struct EntitySlot*)obj, &pos[0], &pos[1]);
                if ((distance = (float)sqrt((search->x - pos[0]) * (search->x - pos[0]) +
                                            (search->y - pos[1]) * (search->y - pos[1]))) <
                    search->bestDistance) {
                    search->bestDistance = distance;
                    search->result = obj;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439200
int legoobject_task_kind_filtered_search(int selector)
{
    if (selector != 1 && selector != 2 && selector != 4) {
        if (legoobject_run_through_lists_skip_upgrade_parts(
                legoobject_task_flags_and_cell_task_match, &selector)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439240
int legoobject_task_flags_and_cell_task_match(Lego_Object* obj, void* task)
{
    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x8) {
        if (legoobject_action_kind_supported(
                obj, *(int*)task, 0, 0)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439280
void legoobject_build_placement_command_batch_expand(int* cell)
{
    uint32_t crystals;
    uint32_t crystalsAlt;
    uint32_t ores;
    uint32_t oresAlt;
    uint32_t i;

    if (terrain_cell_byte_state_classify_current(cell, &crystals, &crystalsAlt,
                                                 &ores, &oresAlt)) {
        for (i = 0; i < crystals; i++) {
            legoobject_build_placement_command_setup(cell, 0, NULL, 1);
        }
        for (i = 0; i < crystalsAlt; i++) {
            legoobject_build_placement_command_setup(cell, 1, NULL, 1);
        }
        for (i = 0; i < ores; i++) {
            legoobject_build_placement_alternate_command_setup(cell, 0, NULL, 1);
        }
        for (i = 0; i < oresAlt; i++) {
            legoobject_build_placement_alternate_command_setup(cell, 1, NULL, 1);
        }
    }
}

// FUNCTION: LEGORR 0x00439340
void legoobject_build_placement_command_pair_create(int* cell, int id)
{
    legoobject_build_placement_command_setup(cell, id, NULL, 1);
    legoobject_build_placement_alternate_command_setup(cell, id, NULL, 1);
}

// FUNCTION: LEGORR 0x00439370
void legoobject_build_placement_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update)
{
    float inset;
    LegoObjGE_Point2F pos;
    float vertices[4][3];
    Lego_Object* obj;

    inset = terrain_context_scale_get(Lego_GetMap()) * 0.5f -
            terrain_context_scale_get(Lego_GetMap()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(Lego_GetMap(), cell[0], cell[1],
                                       (struct TerrainMapVector3*)vertices);
        pos.x = random_float_range(vertices[0][0] + inset, vertices[2][0] - inset);
        pos.y = random_float_range(vertices[0][1] - inset, vertices[2][1] + inset);
    }
    obj = legoobject_create_in_world(legoGlobs_contCrystal, 6, 0, id, pos.x, pos.y, 0.0f);
    cmd_produce(0x16, obj, 0, 0);
    if (update) {
        info_send(0, NULL, NULL, (struct InfoPoint2I*)cell);
        advisor_text_animation_update(0x11, 1, 0);
        LEGOOBJ_FIELD(level_current_owner_get(), LegoObjGE_Block*, 0xb8)
            [LEGOOBJ_FIELD(level_current_owner_get(), int, 0x14) * cell[1] + cell[0]].field_2c++;
    }
}

// FUNCTION: LEGORR 0x004394b0
void legoobject_build_placement_alternate_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update)
{
    float inset;
    LegoObjGE_Point2F pos;
    float vertices[4][3];
    Lego_Object* obj;

    inset = terrain_context_scale_get(Lego_GetMap()) * 0.5f -
            terrain_context_scale_get(Lego_GetMap()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(Lego_GetMap(), cell[0], cell[1],
                                       (struct TerrainMapVector3*)vertices);
        pos.x = random_float_range(vertices[0][0] + inset, vertices[2][0] - inset);
        pos.y = random_float_range(vertices[0][1] - inset, vertices[2][1] + inset);
    }
    obj = legoobject_create_in_world(legoGlobs_contOresTable, 7, 0, id, pos.x, pos.y, 0.0f);
    cmd_produce(0x1c, obj, 0, 0);
    advisor_text_animation_update(0x13, 1, 0);
    if (update) {
        LEGOOBJ_FIELD(level_current_owner_get(), LegoObjGE_Block*, 0xb8)
            [LEGOOBJ_FIELD(level_current_owner_get(), int, 0x14) * cell[1] + cell[0]].field_30++;
    }
}

// FUNCTION: LEGORR 0x004395e0
const char* legoobject_display_name_lookup(Lego_Object* obj)
{
    switch (obj->type) {
    case 2: return s_object_name_kind2[*(int*)obj->miniFigure];
    case 3: return s_object_name_kind3[*(int*)obj->rockMonster];
    case 1: return s_object_name_kind1[*(int*)obj->vehicle];
    case 4: return s_object_name_kind4[*(int*)obj->building];
    case 10: return s_object_name_kind10[*(int*)obj->upgradePart];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[obj->id + 1];
    case 8: return s_object_name_special[3];
    case 9: return s_object_name_special[4];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004396c0
const char* legoobject_item_name_by_kind(unsigned int kind, int index)
{
    switch (kind) {
    case 6: return "PowerCrystal";
    case 7: return index == 0 ? "Ore" : "ProcessedOre";
    case 1: return legoGlobs_vehicleName[index];
    case 2: return legoGlobs_miniFigureName[index];
    case 3: return legoGlobs_rockMonsterName[index];
    case 4: return legoGlobs_buildingName[index];
    case 8: return "Dynamite";
    case 9: return "Barrier";
    case 11: return "ElectricFence";
    case 12: return "SpiderWeb";
    case 13: return "OohScary";
    case 0xffffffff: return "tvcamera";
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439790
const char* legoobject_the_name_get(unsigned int kind, int index)
{
    switch (kind) {
    case 2: return s_object_the_name_kind2[index];
    case 3: return s_object_the_name_kind3[index];
    case 1: return s_object_the_name_kind1[index];
    case 4: return s_object_the_name_kind4[index];
    case 10: return s_object_the_name_kind10[index];
    case 6: return s_object_the_name_special[0];
    case 7: return s_object_the_name_special[index + 1];
    case 8: return s_object_the_name_special[3];
    case 9: return s_object_the_name_special[4];
    case 11: return s_object_the_name_special[5];
    case 12: return s_object_the_name_special[6];
    case 13: return s_object_the_name_special[7];
    case 15: return s_object_the_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439860
const char* legoobject_name_get(unsigned int kind, int index)
{
    switch (kind) {
    case 2: return s_object_name_kind2[index];
    case 3: return s_object_name_kind3[index];
    case 1: return s_object_name_kind1[index];
    case 4: return s_object_name_kind4[index];
    case 10: return s_object_name_kind10[index];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[index + 1];
    case 8: return s_object_name_special[3];
    case 9: return s_object_name_special[4];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439930
void legoobject_copy_two_dword_pair(void* record, unsigned int* outA, unsigned int* outB)
{
    *outA = ((unsigned int*)record)[0];
    *outB = ((unsigned int*)record)[1];
}


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

// FUNCTION: LEGORR 0x00439e70
void legoobject_attached_object_list_detach(Lego_Object* obj, int resetMode)
{
    unsigned int flags;
    unsigned int noRemoveTasks;
    Lego_Object* routeTo;
    unsigned int routeToStats;
    unsigned int i;
    Lego_Object* carried;
    LegoObjF_Point2I blockPos;
    float pos[3];
    float contPos[3];
    void* cont;
    unsigned char* routeFlag;

    flags = obj->flags1;
    noRemoveTasks = flags & 0x200;
    if ((flags & 0x400) && (!(flags & 0x2000) || resetMode)) {
        routeTo = obj->routeToObject;
        routeToStats = 0;
        if (routeTo != NULL)
            routeToStats = entity_stats_word_get(LEGOOBJ_SLOT(routeTo)) & 0x8;
        if (routeToStats == 0 && resetMode) {
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 6, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 9, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 15, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 20, NULL, 1);
        }
        for (i = 0; i < obj->numCarriedObjects; i++) {
            carried = obj->carriedObjects[i];
            obj->carriedObjects[i] = NULL;
            carried->carryingThisObject = NULL;
            carried->interactObject = NULL;
            if (routeToStats && resetMode) {
                entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 5, carried, 1);
                legoobject_action_record_consume_and_cleanup(routeTo, carried);
                legoobject_activity_scene_state_update();
            } else {
                legoobject_block_position_get(LEGOOBJ_SLOT(carried), &blockPos.x, &blockPos.y);
                if (!terrain_cell_flag_8_bounded_current(blockPos.x, blockPos.y) ||
                    terrain_cell_state_equals_six_current(&blockPos.x) ||
                    terrain_cell_occupied_or_flag_current(blockPos.x, blockPos.y, 0))
                {
                    cont = entity_type_component_get(carried);
                    entity_transform_position_get_outputs(LEGOOBJ_SLOT(obj), &pos[0], &pos[1]);
                    ddiscene_object_position_get(cont, NULL, contPos);
                    ddiscene_object_position_set(cont, NULL, pos[0], pos[1], contPos[2]);
                }
                if (resetMode) {
                    entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 7, carried,
                                                  i == obj->numCarriedObjects - 1);
                    if (carried->type != 7 && carried->type != 6)
                        carried->flags1 |= 0x80000;
                } else if (noRemoveTasks) {
                    entity_activity_record_remove_by_type(obj, 3, carried);
                } else {
                    entity_activity_record_remove_by_type(obj, 5, carried);
                    entity_activity_record_remove_by_type(obj, 6, carried);
                    entity_activity_record_remove_by_type(obj, 9, carried);
                    entity_activity_record_remove_by_type(obj, 15, carried);
                    entity_activity_record_remove_by_type(obj, 7, carried);
                    entity_activity_record_remove_by_type(obj, 4, NULL);
                    entity_activity_record_remove_by_type(obj, 20, carried);
                }
                if (carried->type == 5)
                    entity_type5_finalize_if_allowed(LEGOOBJ_SLOT(carried));
            }
        }
        obj->numCarriedObjects = 0;
        obj->flags1 &= ~0x400;
        for (i = 0; i < obj->routeBlocksTotal; i++) {
            routeFlag = &((LegoObjG_RouteBlock*)obj->routeBlocks)[i].actionFlags;
            if (*routeFlag == 7 || *routeFlag == 5)
                *routeFlag = 0;
        }
    }
}

// FUNCTION: LEGORR 0x0043a300
int legoobject_deposit_activity_gate(Lego_Object* obj, Lego_Object* target)
{
    int fromY;
    int* routeX;
    int* routeY;
    int fromX;
    unsigned int count;
    int result;
    int toX;
    int toY;
    int dir;
    void* cont;
    float targetPos[2];
    float pos[2];
    float facing[3];
    float contPos[3];

    if (obj->flags1 & 0x400) {
        if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x4) ||
            (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2))
        {
            obj->flags1 |= 0x2000;
            ai_set_activity(LEGOOBJ_SLOT(obj), 0x1d, 1);
            activity_register_bookkeeping(LEGOOBJ_SLOT(obj));
            legoobject_activity_tick_adapter(LEGOOBJ_SLOT(obj), 1.0f);
            return 1;
        } else if (legoobject_accepts_target_for_task_check(target, obj->carriedObjects[0], 0) ||
            (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8))
        {
            cont = NULL;
            if (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8)
                cont = scene_object_indexed_secondary_handle_get_or_create(target->building, s_activity_resource_slots[10]);
            else if (target->type == 4)
                cont = scene_object_primary_handle_get_or_create(target->building);

            if (cont != NULL) {
                ddiscene_object_position_get(cont, NULL, contPos);
                ddiscene_object_facing_pair_get(cont, NULL, facing, NULL);
                targetPos[0] = contPos[0];
                targetPos[1] = contPos[1];
                if (fabs(facing[0]) > fabs(facing[1]))
                    dir = (facing[0] > 0.0f) ? 3 : 1;
                else
                    dir = (facing[1] > 0.0f) ? 2 : 0;
            } else {
                entity_transform_position_get_outputs(LEGOOBJ_SLOT(target), &targetPos[0], &targetPos[1]);
            }

            terrain_map_world_to_block(Lego_GetMap(), targetPos[0], targetPos[1], &toX, &toY);
            entity_transform_position_get_outputs(LEGOOBJ_SLOT(obj), &pos[0], &pos[1]);
            terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &fromX, &fromY);

            if (fromX == toX && fromY == toY) {
                result = entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), 1,
                    (const float*)&toX, (const float*)&toY, targetPos);
            } else {
                if (!terrain_neighbor_path_check_current(obj, fromX, fromY, toX, toY,
                        &routeX, &routeY, (float*)&count, NULL, NULL))
                    return 0;
                result = entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), count,
                    (const float*)routeX, (const float*)routeY, targetPos);
                free(routeX);
                free(routeY);
            }

            if (result) {
                obj->routeToObject = target;
                if (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8) {
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 9;
                    obj->routeToObject = target;
                    return 1;
                }
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 7;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 4;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= dir & 3;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043a5f0
void legoobject_visual_resource_get_or_create(Lego_Object* obj)
{
    void* handle;

    handle = NULL;
    if (obj->type == 4)
        handle = scene_object_primary_handle_get_or_create(obj->building);
    else if (obj->type == 3)
        handle = renderer_cached_handle_50_get_or_create((struct AnimationObject*)obj->rockMonster);
    else if (obj->type == 2)
        handle = renderer_cached_handle_50_get_or_create((struct AnimationObject*)obj->miniFigure);
    else if (obj->type == 1)
        handle = scene_resource_cached_slot_b8_get((struct SceneResourceAnimationRecord*)obj->vehicle);
    if (handle == NULL)
        entity_type_component_get(obj);
}

// FUNCTION: LEGORR 0x0043a120
int legoobject_cell_step_command_issue_with_jitter(Lego_Object* obj, int* target, float* input, int mode, int markPending)
{
    int result;
    int* routeX;
    int* routeY;
    int blockX;
    int blockY;
    unsigned int count;
    float offset[2];
    float jitter;
    int side;

    result = 0;
    if (obj->flags1 & 0x400) {
        legoobject_block_position_get(LEGOOBJ_SLOT(obj), &blockX, &blockY);
        if (blockX == target[0] && blockY == target[1]) {
            result = entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), 1,
                (const float*)&target[0], (const float*)&target[1], NULL);
        } else if (terrain_neighbor_path_check_current(obj, blockX, blockY,
                       target[0], target[1], &routeX, &routeY, (float*)&count, NULL, NULL)) {
            result = entity_route_setup_and_marker_associate(LEGOOBJ_SLOT(obj), count,
                (const float*)routeX, (const float*)routeY, NULL);
            free(routeX);
            free(routeY);
        }

        if (result) {
            if (obj->type == 1) {
                jitter = random_float_range(0.55f, 0.75f);
                side = rng_thunk_draw15() % 4;
                markPending = 1;
                switch (side) {
                case 0:
                    offset[0] = 0.5f;
                    offset[1] = jitter;
                    break;
                case 1:
                    offset[1] = 0.5f;
                    offset[0] = jitter;
                    break;
                case 2:
                    offset[0] = 0.5f;
                    offset[1] = 1.0f - jitter;
                    break;
                case 3:
                    offset[1] = 0.5f;
                    offset[0] = 1.0f - jitter;
                    break;
                }
            } else {
                offset[0] = input[0];
                offset[1] = input[1];
            }

            if (mode)
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 9;
            else
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 8;
            if (input != NULL) {
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = offset[0];
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = offset[1];
            }
            if (markPending)
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 8;
        }
    }
    return result;
}

// FUNCTION: LEGORR 0x0043a650
Lego_Object* legoobject_action_kind_create_and_command_issue(Lego_Object* source, int kind, int argument, int last)
{
    int slot;
    void* handle;
    void* model;
    float pos[3];
    Lego_Object* obj;

    if (!legoobject_action_kind_supported(source, kind, argument, last))
        return NULL;

    switch (kind) {
    case 9:
        slot = 8;
        break;
    case 6:
    case 7:
        slot = 10;
        break;
    case 8:
        slot = 9;
        break;
    default:
        slot = 0;
        break;
    }
    handle = scene_object_indexed_secondary_handle_get_or_create(source->building, s_activity_resource_slots[slot]);
    ddiscene_object_position_get(handle, NULL, pos);

    if (kind == 6) {
        model = legoGlobs_contCrystal;
        legoobject_activity_scene_state_update();
        global_object_field8c_subtract(1);
    } else if (kind == 7) {
        model = (&legoGlobs_contOresTable)[argument];
        lego_upgrade_resource_debit(argument, 1);
    } else if (kind == 9) {
        model = legoGlobs_contBarrier;
    } else if (kind == 8) {
        model = legoGlobs_contDynamite;
    } else if (kind == 11) {
        model = legoGlobs_contElectricFence;
    } else if (kind == 13) {
        model = legoGlobs_contOohScary;
    } else {
        return NULL;
    }

    obj = legoobject_create_in_world(model, kind, argument, last, pos[0], pos[1], 0.0f);
    if (kind == 7)
        cmd_produce(0x1c, obj, 0, NULL);
    else
        cmd_produce(0x16, obj, 0, NULL);
    if (kind == 9) {
        // STRING: LEGORR 0x004a507c
        scene_action_name_set(obj->other, "Short");
        return obj;
    }
    if (kind == 8)
        // STRING: LEGORR 0x004a5074
        scene_action_name_set(obj->other, "Normal");
    return obj;
}

// FUNCTION: LEGORR 0x0043a7c0
int legoobject_action_kind_supported(Lego_Object* obj, int kind, int argument, int last)
{
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x8) {
        if (kind == 6 && Lego_GetCrystalCount(1))
            return 1;
        if (kind == 7 && lego_upgrade_resource_check(argument))
            return 1;
        if (kind == 9)
            return 1;
        if (kind == 8)
            return 1;
        if (kind == 11)
            return 1;
        if (kind == 13)
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043a850
void legoobject_action_record_consume_and_cleanup(Lego_Object* obj, Lego_Object* record)
{
    if (obj->type == 3 && obj->field_348 == NULL) {
        obj->field_348 = malloc(6 * sizeof(int));
        memset(obj->field_348, 0, 6 * sizeof(int));
    }
    if (record->type == 6) {
        if (obj->type == 3)
            ((int*)obj->field_348)[record->objLevel]++;
        else
            global_object_field8c_increment();
    } else if (record->type == 7 && obj->type != 3) {
        lego_level_resource_balance_add_and_sound(record->id);
    }
    record->carryingThisObject = NULL;
    legoobject_remove(record);
    if (obj->type != 3)
        terrain_cell_record_counts_prune(obj);
}

// FUNCTION: LEGORR 0x0043a8f0
void legoobject_simulation_update_counter_add(int unused, unsigned int count)
{
    while (count-- > 0)
        global_object_field8c_increment();
    legoobject_activity_scene_state_update();
}

// FUNCTION: LEGORR 0x0043a910
void legoobject_simulation_mode_counter_add(int unused, unsigned int count)
{
    while (count-- > 0)
        lego_level_resource_balance_add_and_sound(0);
}

// FUNCTION: LEGORR 0x0043a930
void legoobject_tracked_pointer_remove(void* pointer)
{
    unsigned int i;

    for (i = 0; i < LegoObjG_liveCount; i++) {
        if (pointer == LegoObjG_liveObjs[i])
            LegoObjG_liveObjs[i] = NULL;
    }
}

// FUNCTION: LEGORR 0x0043a960
void legoobject_lava_crust_activity_register(Lego_Object* obj)
{
    LegoObjF_Point2I blockPos;

    if (obj->type == 1 && !(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2000000)) {
        legoobject_block_position_get(LEGOOBJ_SLOT(obj), &blockPos.x, &blockPos.y);
        if (LegoObjG_currLevel->blocks[LegoObjG_currLevel->width * blockPos.y + blockPos.x].terrain == 9 &&
            LegoObjG_liveCount < 100)
        {
            LegoObjG_liveObjs[LegoObjG_liveCount] = obj;
            LegoObjG_liveCount++;
        }
    }
}
