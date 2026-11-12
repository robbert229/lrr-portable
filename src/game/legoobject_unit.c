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


/* ---- functions, group F (00439950-00439e40) ---- */


/* ---- functions, group G (00439e70-0043a960) ---- */

