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

/* objectGlobs.recordObjs (004e8544): the level's deferred object records,
 * 0x2c bytes each, that LegoObject_CreateAndLinkDeferred realises when
 * their block is uncovered. The port keeps its own queue. */
// GLOBAL: LEGORR 0x004e8544
LegoObjGB_DeferredRecord objectGlobs_recordObjs[200];

/* objectGlobs.recordObjsCount (004ea7a4). */
// GLOBAL: LEGORR 0x004ea7a4
unsigned int objectGlobs_recordObjsCount;


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

// FUNCTION: LEGORR 0x00437540
int legoobject_remove(Lego_Object* obj)
{
    s_grid_count_first[(obj->id + obj->type * 15) * 16 + obj->objLevel]++;

    if (obj->flags4 & 0x100000) {
        sound_handle_stop_forward(obj->drillSoundHandle);
    }
    if (obj->flags4 & 0x80000) {
        sound_handle_stop_forward(LEGOOBJ_FIELD(obj, int, 0x354));
    }

    legoobject_run_through_lists_skip_upgrade_parts(
        (Lego_ObjectCallback)legoobject_gb_weapon_target_clear_callback, obj);

    if (obj->type == 3) {
        argument_callback_dispatch(obj);
    }
    if (LEGOOBJ_FIELD(obj, void*, 0x2e4) != NULL) {
        ledger_stub_00472a40_DDiSceneObjectDestroy(LEGOOBJ_FIELD(obj, void*, 0x2e4));
    }
    if (obj->carryingThisObject != NULL) {
        legoobject_attached_object_list_detach(obj->carryingThisObject, 0);
    }

    if (obj->flags4 & 0x40) {
        obj->flags4 &= ~0x40;
        if (obj->routeToObject != NULL) {
            obj->routeToObject->flags4 &= ~0x40;
        }
    }

    legoobject_tracked_pointer_remove(obj);

    if (obj->flags4 & 0x1) {
        legoGlobs_flags1 &= ~0x10000000;
    }
    if (obj->flags4 & 0x80) {
        if (obj->routeToObject != NULL) {
            obj->routeToObject->flags4 &= ~0x80;
        }
        obj->routeToObject = NULL;
    }

    entity_action_sequence_reset((struct EntitySlot*)obj, 0);
    entity_type_four_callback_dispatch((struct EntitySlot*)obj);
    legoobject_attached_object_list_detach(obj, 0);
    entity_drain_command_chain((struct EntitySlot*)obj);
    legoobject_gb_pool_find_with_local_predicate_adapter(obj);
    global_value_clear_if_equal((int)(uintptr_t)obj);
    if ((entity_stats_word_get((struct EntitySlot*)obj) & 4) ||
        (entity_stats_word_get((struct EntitySlot*)obj) & 2)) {
        task_callback_dispatch(obj);
    }
    selection_object_detach(obj);
    info_remove_object_references(obj);
    entity_pointer_registries_remove((struct EntitySlot*)obj);
    legoobject_callback_dispatch_unless_type_five(obj);
    object_pointer_registry_clear_match((int32_t)(uintptr_t)obj);

    if (obj->type == 1) {
        entity_payload_type1_release(
            (struct ScenePairedAnimationState*)obj->vehicle);
        free(obj->vehicle);
    } else if (obj->type == 2) {
        entity_payload_release_subslots((struct EntitySlot*)obj->miniFigure);
        free(obj->miniFigure);
    } else if (obj->type == 3) {
        if (entity_stats_word_get((struct EntitySlot*)obj) & 0x200000) {
            legoobject_gb_auxiliary_payload_release_adapter(obj);
        }
        entity_payload_release_subslots((struct EntitySlot*)obj->rockMonster);
        free(obj->rockMonster);
    } else if (obj->type == 4) {
        entity_payload_teardown((struct EntitySlot*)obj->building);
        free(obj->building);
    } else if (obj->flags3 & 0x20000) {
        ledger_stub_00472a40_DDiSceneObjectDestroy(obj->other);
    } else if (obj->type == 10) {
        render_resource_binding_release(
            (struct RenderResourceBinding*)obj->upgradePart);
        free(obj->upgradePart);
    }

    obj->nextFree = (Lego_Object*)s_pool_record_head;
    s_pool_record_head = obj;
    return 0;
}

// FUNCTION: LEGORR 0x004377b0
int legoobject_run_through_lists_skip_upgrade_parts(Lego_ObjectCallback callback, void* data)
{
    return legoobject_run_through_lists(callback, data, 1);
}

// FUNCTION: LEGORR 0x004377d0
int legoobject_run_through_lists(Lego_ObjectCallback callback, void* data, int skipUpgradeParts)
{
    unsigned int list;
    unsigned int count;
    unsigned int i;
    Lego_Object* obj;

    for (list = 0; list < s_pool_chunk_count; list++) {
        if (s_pool_chunks[list] != NULL) {
            count = 1 << list;
            for (i = 0; i < count; i++) {
                obj = &((Lego_Object*)s_pool_chunks[list])[i];
                if (obj != NULL && obj->nextFree == obj) {
                    if (!skipUpgradeParts || !(obj->flags3 & 0x80000)) {
                        if (callback(obj, data)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00437880
void legoobject_rename(Lego_Object* obj, const char* name)
{
    if (name != NULL && *name != '\0') {
        if (obj->customName == NULL) {
            obj->customName = (char*)malloc(11);
        }
        strncpy(obj->customName, name, 11);
        obj->customName[11] = '\0';
    } else {
        if (obj->customName != NULL) {
            free(obj->customName);
        }
        obj->customName = NULL;
    }
}

// FUNCTION: LEGORR 0x004378e0
void legoobject_grid_object_records_release(void)
{
    unsigned int i;

    for (i = 0; i < objectGlobs_recordObjsCount; i++) {
        if (objectGlobs_recordObjs[i].linkName != NULL) {
            free(objectGlobs_recordObjs[i].linkName);
        }
        if (objectGlobs_recordObjs[i].name != NULL) {
            free(objectGlobs_recordObjs[i].name);
        }
    }
    objectGlobs_recordObjsCount = 0;
    memset(objectGlobs_recordObjs, 0, 200 * sizeof(int));
    global_counter_004ff990_reset();
}

// FUNCTION: LEGORR 0x00437940
void legoobject_deferred_olist_create_and_link(int* blockPos)
{
    Lego_Object* objs[200];
    char* linkNames[200];
    char* names[200];
    unsigned int count;
    unsigned int i;
    unsigned int j;
    uint32_t shapeCount;
    unsigned int direction;
    void* shape;
    void* points;
    LegoObjGB_DeferredRecord* rec;
    Lego_Object* obj;
    Lego_Object* target;
    unsigned int mode;

    count = 0;
    for (i = 0; i < objectGlobs_recordObjsCount; i++) {
        rec = &objectGlobs_recordObjs[i];
        if (rec->blockPos[0] == blockPos[0] && rec->blockPos[1] == blockPos[1]) {
            if (rec->type == 4) {
                shape = (void*)task_field84_get_and_field88_out(
                    (char*)legoGlobs_buildingData + rec->id * 0x14c, &shapeCount);
                direction = (unsigned int)(rec->heading * 0.15915494f * 8.0f);
                direction = ((direction & 1) + direction) >> 1;
                points = rotated_pairs_scratch_fill((const int*)rec, (const int*)shape,
                                                    shapeCount, direction);
                obj = (Lego_Object*)legoobject_gb_world_object_create_adapter(
                    rec->id, rec, direction, points, shapeCount, 0);
            } else {
                obj = legoobject_create(rec->model, rec->type, rec->id);
            }

            if (obj != NULL) {
                if (rec->type != 4) {
                    entity_world_position_set((struct EntitySlot*)obj, rec->worldPos[0],
                                              rec->worldPos[1], rec->heading, 1);
                }
                if (rec->type == 6 || rec->type == 7 || rec->type == 8 || rec->type == 9) {
                    cmd_create_task((struct EntitySlot*)obj, 0);
                }
                obj->health = rec->health;
                if (obj->type == 2) {
                    legoobject_gb_tutorial_counter_increment_adapter(obj->type);
                    info_send(0x21, NULL, obj, NULL);
                }
                obj->flags4 |= 0x200;
                legoobject_kind_record_update_dispatch(obj);
                if (obj->type == 11) {
                    electric_fence_add((struct ElectricFenceObject*)obj);
                }
                objs[count] = obj;
                names[count] = rec->name;
                linkNames[count] = rec->linkName;
                count++;
                rec->name = NULL;
            }

            objectGlobs_recordObjsCount--;
            *rec = objectGlobs_recordObjs[objectGlobs_recordObjsCount];
            i--;
        }
    }

    for (i = 0; i < count; i++) {
        if (linkNames[i] != NULL) {
            for (j = 0; j < count; j++) {
                if (i != j && _strcmpi(names[j], linkNames[i]) == 0) {
                    obj = objs[i];
                    target = objs[j];
                    obj->routeToObject = target;
                    obj->flags2 |= 0x4;
                    if (entity_capability_flags_get((struct EntitySlot*)target) & 0x100) {
                        mode = 1;
                    } else if (!(entity_stats_word_get((struct EntitySlot*)target) & 0x1000000)) {
                        mode = 4;
                    } else if (!(entity_stats_word_get((struct EntitySlot*)target) & 0x2000000)) {
                        mode = 2;
                    } else {
                        mode = 1;
                    }
                    entity_linked_activity_event_dispatch((struct EntitySlot*)obj, mode);
                    entity_linked_route_flag_clear((struct GameContext*)obj,
                                                   (struct EntitySlot*)target);
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (linkNames[i] != NULL) {
            free(linkNames[i]);
        }
        if (names[i] != NULL) {
            free(names[i]);
        }
    }
}

// FUNCTION: LEGORR 0x00437c20
void legoobject_deferred_olist_queue_add(void* model, int type, int id, float* worldPos, float heading, float health, const char* name, const char* linkName)
{
    LegoObjGB_DeferredRecord* rec;

    rec = &objectGlobs_recordObjs[objectGlobs_recordObjsCount++];
    rec->worldPos[0] = worldPos[0];
    rec->worldPos[1] = worldPos[1];
    rec->heading = heading;
    rec->model = model;
    rec->type = type;
    rec->id = id;
    rec->health = health;
    if (name != NULL) {
        rec->name = string_duplicate(name);
    }
    if (linkName != NULL) {
        rec->linkName = string_duplicate(linkName);
    }
    terrain_map_world_to_block((struct TerrainMap*)level_current_terrain_get_original(),
                               worldPos[0], worldPos[1],
                               &rec->blockPos[0], &rec->blockPos[1]);
}

// FUNCTION: LEGORR 0x00437cc0
int legoobject_flag40000_threshold_compare(Lego_Object* obj)
{
    if (entity_state_flags_get((struct EntitySlot*)obj) & 0x40000) {
        if (entity_stats_float_ate8((struct EntitySlot*)obj) < obj->health) {
            return 1;
        }
    }
    return 0;
}


/* ---- functions, group C (00437d00-00438580) ---- */


/* ---- functions, group D (004385b0-00438ed0) ---- */


/* ---- functions, group E (00438f60-00439930) ---- */


/* ---- functions, group F (00439950-00439e40) ---- */


/* ---- functions, group G (00439e70-0043a960) ---- */

