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
/* LegoObject.c's pooled 0.0f in .rdata, which the unit's float compares
 * read (about a hundred references, 004385b0-0044c710). Annotated, so a
 * read of it is a named operand on both sides; the original reads a
 * literal. */
// GLOBAL: LEGORR 0x0049e3a4
static const float legoobject_zero = 0.0f;


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

/* LegoObject_IsActive-style check: not dead, not being removed, and (unless
 * relaxed) ready for work. */
// FUNCTION: LEGORR 0x004385b0
int legoobject_shared_eligibility_check(Lego_Object* obj, int relaxed)
{
    if (!(obj->flags1 & 0x700100) && !(obj->flags2 & 0x100000) &&
        obj->health >= legoobject_zero && !(obj->flags1 & 0x20000) &&
        (relaxed || ((obj->flags3 & 0x20000000) && !(obj->flags3 & 0x80000000))))
    {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00438610
Lego_Object* legoobject_create_in_world(void* model, int type, int id, int last, float x, float y, float heading)
{
    Lego_Object* obj;

    if ((obj = legoobject_create(model, type, id)) != NULL) {
        entity_world_position_set((struct EntitySlot*)obj, x, y, heading, 1);
        if (obj->type == 6 || obj->type == 7) legoobject_animation_state_set(obj, last);
        return obj;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00438670
Lego_Object* legoobject_at_grid_position_find(int* cell)
{
    LegoObjGD_BlockSearch search;

    search.result = NULL;
    search.bx = cell[0];
    search.by = cell[1];
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_grid_position_match_callback, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x004386b0
int legoobject_grid_position_match_callback(Lego_Object* obj, void* data)
{
    LegoObjGD_BlockSearch* search = (LegoObjGD_BlockSearch*)data;
    int blockPos[2];

    if (obj->type == 4 && (entity_state_flags_get((struct EntitySlot*)obj) & 0x200) &&
        legoobject_shared_eligibility_check(obj, 0))
    {
        legoobject_block_position_get((struct EntitySlot*)obj, &blockPos[0], &blockPos[1]);
        if (blockPos[0] == search->bx && blockPos[1] == search->by) {
            search->result = obj;
            return 1;
        }
    }
    return 0;
}

/* 0041f8e0 is empty in this build (noop_callback); the original compares
 * whatever it leaves in eax, so it is called through an int-returning
 * type. Only the matching build runs this function. */
// FUNCTION: LEGORR 0x00438720
int legoobject_available_resource_spend(Lego_Object* obj, int amount)
{
    if (LEGOOBJ_FIELD(g_active_level_retail, int, 0x1f8)) amount = 0;
    if (((int (*)(int))noop_callback)(0) >= amount) {
        global_object_field98_add(amount);
        return 1;
    }
    info_send(0xe, NULL, obj, NULL);
    return 0;
}

// FUNCTION: LEGORR 0x00438770
int legoobject_building_upgrade_resource_admission_check(Lego_Object* building, unsigned int* outCost)
{
    if (building->objLevel + 1 < building_upgrade_admission_check(building->type, building->id)) {
        if ((unsigned int)lego_upgrade_resource_check(0) >= (unsigned int)legoGlobs_BuildingUpgradeCostOre) {
            if (outCost) *outCost = 0;
            return 1;
        } else if ((unsigned int)lego_upgrade_resource_check(1) >= (unsigned int)legoGlobs_BuildingUpgradeCostStuds) {
            if (outCost) *outCost = legoGlobs_BuildingUpgradeCostStuds;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004387f0
void legoobject_building_upgrade_commit(Lego_Object* building)
{
    unsigned int cost;
    float wPos[3];

    if (building->type == 4) {
        if (legoobject_building_upgrade_resource_admission_check(building, &cost)) {
            lego_upgrade_resource_debit(cost != 0, cost ? cost : legoGlobs_BuildingUpgradeCostOre);
            building->field_3d8 = 0.0f;
            entity_payload_state_set((struct EntitySlot*)building->building, building->objLevel + 1);
            legoobject_gd_building_index_set_adapter((struct EntitySlot*)building, building->objLevel + 1);
            ddiscene_object_position_get(entity_type_component_get(building), NULL, wPos);
            lava_effect_select_and_place(9, wPos, NULL);
            building_upgrade_state_changed_dispatch(building->type, building->id, building->objLevel, 0);
        }
    }
}

// FUNCTION: LEGORR 0x004388b0
int legoobject_action_kind_prerequisite_check(Lego_Object* obj, int action)
{
    unsigned int studs, ore;

    if (obj->type == 1 && record_flag_bit_test(obj->vehicle, action, 1)) {
        studs = (entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action) % 5 != 0);
        studs += entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action) / 5;
        ore = entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action);
        if ((unsigned int)lego_upgrade_resource_check(0) < ore &&
            (unsigned int)lego_upgrade_resource_check(1) < studs)
        {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 36%: the original reloads the mask and ORs 0x4000 into it in memory, and
 * keeps alt in ecx; MSVC 5.0 folds every C form tried here into one store
 * of 0x6000 (finding notes in the GAME-023 group D report). */
// FUNCTION: LEGORR 0x00438960
Lego_Object* legoobject_unknown_00438960(Lego_Object* obj, int alt)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x2000;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    if (alt) search.mask |= 0x4000;
    if (obj) entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x004389e0
Lego_Object* legoobject_route_marker_candidate_find(Lego_Object* source, int secondary)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x1000;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    entity_transform_position_get_outputs((struct EntitySlot*)source, &search.x, &search.y);
    if (secondary) search.mask |= 0x400;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438a60
Lego_Object* legoobject_query_center_mask01(int* cell, int a, int b, int c)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x1;
    search.arg14 = a;
    search.arg18 = b;
    search.arg1c = c;
    search.arg20 = 0;
    terrain_map_block_to_world((struct TerrainMap*)level_current_terrain_get_original(), cell[0], cell[1], &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438ae0
Lego_Object* legoobject_query_center_mask02(Lego_Object* obj, int* cell, int a, int b)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x2;
    search.arg14 = a;
    search.arg18 = 0;
    search.arg1c = b;
    search.arg20 = 0;
    if (obj) entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    else terrain_map_block_to_world((struct TerrainMap*)level_current_terrain_get_original(), cell[0], cell[1], &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438b80
Lego_Object* legoobject_query_center_mask_from_argument(Lego_Object* obj, int mask)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = mask;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438bf0
Lego_Object* legoobject_query_center_mask80(Lego_Object* obj)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x80;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438c60
Lego_Object* legoobject_candidate_search(Lego_Object* obj)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x4;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438cd0
Lego_Object* legoobject_query_center_mask20(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x20;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438d30
Lego_Object* legoobject_query_center_mask10(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x10;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438d90
Lego_Object* legoobject_query_center_mask200(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x200;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438df0
Lego_Object* legoobject_lookup_at_world_position(float* position)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = position[0];
    search.y = position[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x8;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438e50
Lego_Object* legoobject_query_center_mask100(Lego_Object* obj, float* center, int extra)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x100;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = extra;
    if (center) {
        search.x = center[0];
        search.y = center[1];
    }
    if (obj) entity_transform_position_get_outputs((struct EntitySlot*)obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438ed0
int legoobject_flags_contain_requested_single_bit(Lego_Object* obj, int selector)
{
    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x4) || selector != 1) {
        if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x8) || selector != 2) {
            if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x10) || selector != 4) {
                if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x20) || selector != 8) {
                    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x40) || selector != 0x10) {
                        if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x80) || selector != 0x20) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}


/* ---- functions, group E (00438f60-00439930) ---- */


/* ---- functions, group F (00439950-00439e40) ---- */


/* ---- functions, group G (00439e70-0043a960) ---- */

