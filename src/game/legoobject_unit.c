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

/* The third primary teleporter (legoGlobs, DAT_00556454); lego_unit.c
 * defines the first two (0055644c, 00556450). */
// GLOBAL: LEGORR 0x00556454
Lego_Object* legoGlobs_f_556454;

/* Stats.c's static ObjectStats records (0x150 bytes each, first word 0x30)
 * that LegoObject_Create points objects without a stats row at: upgrade
 * parts, dynamite, barriers and the small object types. Only this unit
 * reads them; they stay here until Stats.c is converted. */
// GLOBAL: LEGORR 0x004a97a0
static uint32_t s_stats_upgrade_part[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a98f0
static uint32_t s_stats_dynamite[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a9a40
static uint32_t s_stats_barrier[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a9b90
static uint32_t s_stats_small_object[0x54] = { 0x30 };


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

// FUNCTION: LEGORR 0x00437d00
Lego_Object* legoobject_create(void* model, int type, int id)
{
    Lego_Object* obj;
    int soundOn;

    if ((obj = legoobject_pool_allocate()) != NULL) {
        obj->type = type;
        obj->id = id;
        obj->flags3 = 0;
        obj->health = 100.0f;
        obj->energy = 100.0f;

        renderer_flag_bit6_set(0);
        soundOn = sound_subsystem_enabled();
        sound_subsystem_enable_set(0, 0);

        if (obj->type == 1) {
            obj->vehicle = malloc(0x1ec);
            scene_record_template_instantiate((struct ScenePairedAnimationState*)model, (struct ScenePairedAnimationState*)obj->vehicle);
            scene_assign_owner_from_model((struct ScenePairedAnimationState*)obj->vehicle, obj);
            obj->carryNullFrames = object_record_field11c_get(obj->vehicle);
            scene_record_indexed_entries_rebuild_original(obj->vehicle, 0);
            obj->abilityFlags = 0x20;
            obj->flags3 |= 0x441190;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (LEGOOBJ_FIELD(model, int, 0xb4)) obj->flags3 |= 0x2;
            if (LEGOOBJ_FIELD(model, int, 0xdc)) {
                obj->flags3 |= 0x20;
                if (entity_stats_word_get((struct EntitySlot*)obj) & 0x2000000) obj->flags3 |= 0x40;
            }
        } else if (obj->type == 2) {
            obj->miniFigure = malloc(0x74);
            renderer_record_clone((struct AnimationObject*)model, (struct AnimationObject*)obj->miniFigure);
            scene_assign_entity_owner(obj->miniFigure, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = ((uint32_t (*)(void*))flic_opcode25_success_stub)(obj->miniFigure);
            obj->flags3 |= 0x4401f5;
            if (LEGOOBJ_FIELD(model, int, 0x2c)) obj->flags3 |= 0x2;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            obj->flags3 |= 0x8000;
            legoobject_gc_activity_grant_adapter((struct EntitySlot*)obj, 0);
        } else if (obj->type == 3) {
            obj->rockMonster = malloc(0x74);
            renderer_record_clone((struct AnimationObject*)model, (struct AnimationObject*)obj->rockMonster);
            scene_assign_entity_owner(obj->rockMonster, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = ((uint32_t (*)(void*))flic_opcode25_success_stub)(obj->rockMonster);
            obj->flags3 |= 0x8440200;
            if (LEGOOBJ_FIELD(model, int, 0x2c)) obj->flags3 |= 0x2;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x200000) {
                legoobject_gc_scene_object_tree_attach_adapter((struct EntitySlot*)obj);
                if (!(entity_stats_word_get((struct EntitySlot*)obj) & 0x400000)) {
                    entity_type_operation_dispatch_adapter((struct EntitySlot*)obj, 1);
                    obj->flags3 &= ~0x100;
                }
            } else {
                obj->field_32c = 0;
            }
            if (legoobject_flag40000_threshold_compare(obj)) entity_command15_validated_record_queue((struct EntitySlot*)obj);
            obj->flags3 |= 0x80000000;
        } else if (obj->type == 4) {
            obj->building = malloc(0x14c);
            scene_object_record_clone_retain_handles((struct SceneObjectRecord*)model, (struct SceneObjectRecord*)obj->building);
            scene_assign_entity_owner(obj->building, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = scene_object_flags_get(obj->building);
            obj->flags3 |= 0x40100;
            entity_payload_state_set((struct EntitySlot*)obj->building, 0);
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x10) legoGlobs_f_55644c = (uint32_t)(uintptr_t)obj;
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x20) legoGlobs_f_556450 = (uint32_t)(uintptr_t)obj;
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x40) legoGlobs_f_556454 = obj;
            entity_stats_word_get((struct EntitySlot*)obj);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4000) task_record_kind11_timed_queue(obj);
            task_record_kind10_queue(obj);
            obj->abilityFlags = 0x20;
            legoobject_activity_scene_state_update();
        } else if (obj->type == 10) {
            obj->upgradePart = malloc(0x10);
            render_resource_binding_copy_and_convert((struct RenderResourceBinding*)model, (struct RenderResourceBinding*)obj->upgradePart);
            obj->stats = s_stats_upgrade_part;
            obj->flags3 |= 0x80000;
        } else {
            obj->other = entity_visual_resource_load(model);
            scene_object_owner_set(obj->other, (uint32_t)(uintptr_t)obj);
            obj->flags3 |= 0x20000;
            if (obj->type == 11) obj->flags3 |= 0x40100;
            if (obj->type == 7 || obj->type == 6) obj->flags3 |= 0x100100;
            else if (obj->type == 8 || obj->type == 13) obj->flags3 |= 0x40000;
            legoobject_rover_work_queue_update(obj, 1);
            switch (obj->type) {
            case 8:
                obj->stats = s_stats_dynamite;
                break;
            case 9:
                obj->stats = s_stats_barrier;
                break;
            case 12:
            case 13:
            case 14:
            case 16:
            case 17:
            case 18:
            case 19:
                obj->stats = s_stats_small_object;
                break;
            default:
                entity_building_index_set((struct EntitySlot*)obj, 0);
                break;
            }
        }

        LEGOOBJ_FIELD(obj, const char*, 0x2ec) = s_activity[0];
        LEGOOBJ_FIELD(obj, const char*, 0x2e8) = s_activity[9];
        if (activity_register_bookkeeping((struct EntitySlot*)obj)) obj->flags3 |= 0x8;
        LEGOOBJ_FIELD(obj, const char*, 0x2e8) = s_activity[16];
        if (activity_register_bookkeeping((struct EntitySlot*)obj)) obj->flags3 |= 0x4000000;
        LEGOOBJ_FIELD(obj, const char*, 0x2e8) = s_activity[2];
        if (activity_register_bookkeeping((struct EntitySlot*)obj)) obj->flags3 |= 0x10000000;
        LEGOOBJ_FIELD(obj, const char*, 0x2e8) = s_activity[0];
        activity_register_bookkeeping((struct EntitySlot*)obj);

        task_record_kind14_remove_if_head((struct EntitySlot*)obj);
        renderer_flag_bit6_set(1);
        if (soundOn) sound_subsystem_enable_set(1, 0);

        entity_kind_four_post_create_resolve((struct EntitySlot*)obj);
        s_grid_count_second[(obj->type * 15 + obj->id) * 16]++;
        if (entity_state_flags_get((struct EntitySlot*)obj) & 0x80000000) obj->flags3 &= ~0x100;

        return obj;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x004382c0
Lego_Object* legoobject_pool_allocate(void)
{
    Lego_Object* newObj;

    if (s_pool_record_head == NULL) legoobject_pool_chunk_allocate();

    newObj = (Lego_Object*)s_pool_record_head;
    s_pool_record_head = newObj->nextFree;
    memset(newObj, 0, sizeof(Lego_Object));
    newObj->nextFree = newObj;
    newObj->targetBlockPos[0] = newObj->targetBlockPos[1] = -1.0f;

    return newObj;
}

/* objectGlobs.listCount, re-read after the listSet store. In the original
 * both are fields of objectGlobs, so MSVC must assume the indexed store
 * may alias the count and loads it again. The port keeps them as separate
 * globals; a volatile read restores the reloads (finding: effective match,
 * the count load is scheduled one instruction earlier). */
#define LEGOOBJ_GC_LIST_COUNT (*(volatile uint32_t*)&s_pool_chunk_count)

// FUNCTION: LEGORR 0x00438310
void legoobject_pool_chunk_allocate(void)
{
    Lego_Object* list;
    unsigned int count, loop;

    count = 1 << s_pool_chunk_count;
    s_pool_chunks[s_pool_chunk_count] = malloc(sizeof(Lego_Object) * count);
    if ((list = (Lego_Object*)s_pool_chunks[LEGOOBJ_GC_LIST_COUNT]) != NULL) {
        LEGOOBJ_GC_LIST_COUNT++;
        for (loop = 1; loop < count; loop++) list[loop - 1].nextFree = &list[loop];
        list[count - 1].nextFree = (Lego_Object*)s_pool_record_head;
        s_pool_record_head = list;
    }
}

// FUNCTION: LEGORR 0x00438390
unsigned int legoobject_global_animation_state_get(void)
{
    return s_global_animation_state;
}

// FUNCTION: LEGORR 0x004383a0
void legoobject_global_animation_state_set(unsigned int state)
{
    s_global_animation_state = state;
}

// FUNCTION: LEGORR 0x004383b0
void legoobject_rover_work_queue_update(Lego_Object* obj, int mode)
{
    float r, g, b;
    unsigned int loop;

    if (obj->type == 6) {
        if (mode) {
            r = legoGlobs_PowerCrystalRGB[0];
            g = legoGlobs_PowerCrystalRGB[1];
            b = legoGlobs_PowerCrystalRGB[2];
        } else {
            r = legoGlobs_UnpoweredCrystalRGB[0];
            g = legoGlobs_UnpoweredCrystalRGB[1];
            b = legoGlobs_UnpoweredCrystalRGB[2];
        }
        for (loop = 0; loop < (unsigned int)animated_texture_frame_count(obj->other); loop++) {
            scene_mesh_color_and_alpha_set(obj->other, loop, r, g, b, 1.0f);
            scene_mesh_color_set(obj->other, loop, r, g, b);
        }
    }
}

// FUNCTION: LEGORR 0x00438460
void legoobject_kind_record_update_dispatch(Lego_Object* obj)
{
    if (obj->type == 4) {
        activity_register_bookkeeping((struct EntitySlot*)obj);
        obj->flags3 |= 0x20;
        if (entity_stats_word_get((struct EntitySlot*)obj) & 0x8) task_record_create_and_scan_grid(obj);
        else if (entity_stats_word_get((struct EntitySlot*)obj) & 0x2) timed_task_record_initialize_and_link(obj, 7, 0, 0);
        else if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4) timed_task_record_initialize_and_link(obj, 6, 0, 0);
        else obj->flags3 &= ~0x20;
        legoobject_activity_scene_state_update();
        obj->flags4 |= 0x200000;
    } else if (obj->type == 1) {
        six_resource_slots_set_state(obj->vehicle, 0);
    }

    if (obj->type == 2 || obj->type == 1) legoobject_gc_pool_find_with_local_predicate_adapter((struct EntitySlot*)obj);

    building_upgrade_state_changed_dispatch(obj->type, obj->id, 0, obj->flags4 & 0x200);

    if (obj->type == 4) s_global_animation_state++;
    if (obj->type == 2 || obj->type == 1) scalar_task_record_queue(obj);
    if (obj->type == 1 || obj->type == 4) obj->flags4 |= 0x40000;
}

// FUNCTION: LEGORR 0x00438580
void legoobject_flag80000000_set_or_clear(Lego_Object* obj, int on)
{
    if (on) obj->flags3 &= 0x7fffffff;
    else obj->flags3 |= 0x80000000;
    legoobject_activity_scene_state_update();
}


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
