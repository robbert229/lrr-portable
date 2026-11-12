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
                        ((LegoObjGE_ActionKindSupported)legoobject_action_kind_supported)(
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
        if (((LegoObjGE_ActionKindSupported)legoobject_action_kind_supported)(
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

    inset = terrain_context_scale_get(level_current_terrain_get_original()) * 0.5f -
            terrain_context_scale_get(level_current_terrain_get_original()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(level_current_terrain_get_original(), cell[0], cell[1],
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

    inset = terrain_context_scale_get(level_current_terrain_get_original()) * 0.5f -
            terrain_context_scale_get(level_current_terrain_get_original()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(level_current_terrain_get_original(), cell[0], cell[1],
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


/* ---- functions, group G (00439e70-0043a960) ---- */

