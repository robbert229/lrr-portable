// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-016 GAME-016: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-016.csv. */
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "view.hpp"
#include "record.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_for_steering.hpp"
#include <float.h>
#include <stdint.h>
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
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/engine.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_for_steering.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include <errno.h>
#include <locale.h>
#include "ptrstack.hpp"
#include <limits.h>
#include "record_for_steering.hpp"
#include "gods98/view_for_steering.hpp"
#include "entity_for_steering.hpp"
#include "gods98/record_for_steering.hpp"

/* Maths_RandRange (FUN_004798b0, Maths.c). */
extern "C" float random_float_range(float low, float high);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void scene_object_list_apply(void* list, uint32_t operation, void* payload);
static void ptrstack_link_set(void* item, void* next);
static void* ptrstack_link_get(const void* item);

/* --- the record-creation leaves (2026-10-25) --------------------------------
 * SceneNodeRecordCreate (FUN_0040f6c0): allocates the 0xa0-byte scene-node
 * record through the engine heap, zeroes the counter slots, pins slot 8 to
 * 1.0, copies the param_2 triple into +0/+4/+8 (zeros when null) and the
 * param_1 triple into +12/+16/+20, duplicates param_3..param_6 into the
 * paired dword slots, stores param_7 at dword 0x26, and answers the record.
 * Deviations: the whole block rides zero-init (the corpus leaves the tail
 * dwords as heap garbage), and the null allocation answers NULL (the corpus
 * heap would assert). All record fields are 32-bit values, so the block
 * keeps the corpus layout exactly.
 *
 * CallbackStateRecordCreate (FUN_0040f7d0): allocates the 0x28-byte state
 * record, creates the scene-node record over (param_1, param_2, param_3,
 * param_4, param_5, 0, param_6) into slot +0, and records which of the two
 * triples was non-null at dwords 3/4. Pinned quirk: the corpus stores the
 * node result through the record even when that create's own allocation
 * fails (slot +0 lands NULL) while the flag dwords still land. Deviation:
 * the corpus writes dwords 1/2 explicitly zero — LP64 widens the +0 slot to
 * a native pointer, whose high half covers dword 1, and the port zeroes
 * the block.
 *
 * CallbackRecordAllocateInitialize (FUN_004628e0): allocates the 0x28-byte
 * record, zeroes ten dwords, allocates a link slot holding a zeroed
 * 0x20-byte buffer, stores the slot at dword 8, and fills dwords 0..5 with
 * param_2..param_6 — with param_3 riding twice, at dwords 0 and 2 (pinned).
 * Pinned quirk: the corpus writes the caller's out slot through *outside*
 * the allocation-success branch, so the slot lands NULL when the record
 * allocation fails. Deviations: the caller slot write is guarded, and the
 * slot (corpus 4 bytes) and its buffer pointer widen to native pointers. */
// FUNCTION: LEGORR 0x0040f6c0
void* scene_node_record_create(const uint32_t* triple1,
                               const uint32_t* triple2, uint32_t param_3,
                               uint32_t param_4, uint32_t param_5,
                               uint32_t param_6, uint32_t param_7)
{
    void* record = engine_heap_alloc(0xa0);
    if (record == NULL) {
        return NULL; /* deviation: the corpus heap would assert */
    }
    uint32_t* d = (uint32_t*)record;
    memset(record, 0, 0xa0); /* deviation: zero the whole block */
    d[8] = 0x3f800000u;      /* pinned: slot 8 is 1.0 */
    for (int i = 0; i < 3; ++i) {
        d[i] = triple2 != NULL ? triple2[i] : 0u;
        d[3 + i] = triple1 != NULL ? triple1[i] : 0u;
    }
    d[0x0e] = param_6;
    d[0x12] = param_6;
    d[0x0f] = param_3;
    d[0x13] = param_3;
    d[0x10] = param_4;
    d[0x14] = param_4;
    d[0x11] = param_5;
    d[0x15] = param_5;
    d[0x26] = param_7;
    return record;
}

/* OptionalResourceDestroy (FUN_0040f7b0) — see heap.hpp. */
// FUNCTION: LEGORR 0x0040f7b0
void optional_resource_destroy(void* record)
{
    if (record != NULL) {
        engine_heap_free(record);
    }
}

// FUNCTION: LEGORR 0x0040f7d0
void* callback_state_record_create(const uint32_t* triple1,
                                   const uint32_t* triple2, uint32_t param_3,
                                   uint32_t param_4, uint32_t param_5,
                                   uint32_t param_6)
{
    void* record = engine_heap_alloc(0x28);
    if (record == NULL) {
        return NULL; /* deviation: the corpus heap would assert */
    }
    memset(record, 0, 0x28);
    void* inner = scene_node_record_create(triple1, triple2, param_3,
                                           param_4, param_5, 0u, param_6);
    memcpy(record, &inner, sizeof inner);
    const uint32_t has_triple1 = triple1 != NULL ? 1u : 0u;
    const uint32_t has_triple2 = triple2 != NULL ? 1u : 0u;
    __builtin_memcpy((char*)record + 12, &has_triple1, 4); /* corpus +0xc */
    __builtin_memcpy((char*)record + 16, &has_triple2, 4); /* corpus +0x10 */
    return record;
}
/* CallbackStateListDispose (FUN_0040f850) — see ptrstack.hpp. */
// FUNCTION: LEGORR 0x0040f850
void callback_state_list_dispose(void* stack_allocation)
{
    if (stack_allocation == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    char* block = (char*)stack_allocation;
    PtrStack stack;
    __builtin_memcpy(&stack, block + sizeof(void*), sizeof stack);
    void* item = ptrstack_pop(&stack);
    while (item != NULL) {
        optional_resource_destroy(item);
        item = ptrstack_pop(&stack);
    }
    void* head_state = NULL;
    __builtin_memcpy(&head_state, block, sizeof head_state);
    engine_heap_free(head_state);
    engine_heap_free(block);
}

// FUNCTION: LEGORR 0x0040fb00
void matrix_rotation_compose_xyz(float* record)
{
    float scratch[64];
    float* rotation_x = scratch + 32;
    float* rotation_y = scratch;
    float* rotation_z = scratch + 16;
    float* intermediate = scratch + 48;

    matrix4_set_rotation_x(rotation_x, record[0x28 / 4]);
    matrix4_set_rotation_y(rotation_y, record[0x24 / 4]);
    matrix4_set_rotation_z(rotation_z, record[0x2c / 4]);
    /* Matrix4Multiply takes (out, param2, param3) and computes param3 ×
     * param2. The retail calls therefore produce X × Y, then Z × that
     * intermediate; keep the seemingly reversed argument order explicit. */
    matrix4_multiply(intermediate, rotation_y, rotation_x);
    matrix4_multiply(record + 0x58 / 4, intermediate, rotation_z);
}

// FUNCTION: LEGORR 0x0040fcc0
void vector_scale_components_in_place(float* vector, float sx, float sy,
                                      float sz)
{
    if (vector == NULL) {
        return;
    }
    d3drm_vector_random(vector);
    /* Keep the scale operand first, matching FUN_0040fcc0's
     * param_scale * vector_component order on the x87 path. */
    vector[0] = sx * vector[0];
    vector[1] = sy * vector[1];
    vector[2] = sz * vector[2];
}

/* ThreeValueRecordCopyAndMarkReady (FUN_0040fdb0). */
// FUNCTION: LEGORR 0x0040fdb0
void three_value_record_copy_and_mark_ready(void* record,
                                            const uint32_t values[3])
{
    void* destination_pointer = *(void**)record;
    uint32_t* destination = (uint32_t*)destination_pointer;
    destination[0] = values[0];
    destination[1] = values[1];
    destination[2] = values[2];
    ((uint32_t*)record)[4] = 1;
}

// FUNCTION: LEGORR 0x0040fde0
void set_vector_and_notify_listeners(float* state, const float* delta)
{
    steering_physics_step(NULL, state, delta);
}

// FUNCTION: LEGORR 0x0040fe00
void scene_object_list_update_transforms(void* list, void* payload)
{
    void* first;
    if (list == NULL) return;
    first = *(void**)list;
    if (s_scene_object_list_apply_hook != NULL) {
        s_scene_object_list_apply_hook(s_scene_object_list_apply_context,
                                       list, first, 0x0040fde0u, payload);
    }
    scene_object_list_apply(list, 0x0040fb90u, payload);
    scene_object_list_apply(list, 0x0040fd00u, payload);
}

/* PointerStackPush (FUN_0040fe40): link item → current head, make item
 * the head, count++. Corpus guard: both pointers nonzero. */
// FUNCTION: LEGORR 0x0040fe40
void ptrstack_push(PtrStack* stack, void* item)
{
    if (stack == NULL || item == NULL) {
        return;
    }
    ptrstack_link_set(item, stack->head);
    stack->head = item;
    stack->count = stack->count + 1;
}

/* PointerStackPop (FUN_0040fe60): nonempty stack answers the head and
 * advances it through the item link, decrementing the count. */
// FUNCTION: LEGORR 0x0040fe60
void* ptrstack_pop(PtrStack* stack)
{
    if (stack == NULL || stack->count == 0) {
        return NULL;
    }
    void* item = stack->head;
    stack->head = ptrstack_link_get(item);
    stack->count = stack->count - 1;
    return item;
}

/* PointerStackForEach (FUN_0040fe90): callback walk. The corpus's for-
 * loop fetches item+0x9c AFTER the callback body runs — reproduced. */
// FUNCTION: LEGORR 0x0040fe90
void ptrstack_for_each(PtrStack* stack, PtrStackCallback cb, void* ctx)
{
    if (stack == NULL || cb == NULL) {
        return;
    }
    for (void* item = stack->head; item != NULL;) {
        cb(stack, item, ctx);
        item = ptrstack_link_get(item);
    }
}

/* SetGlobalScalarToOne (FUN_0040fec0) — see view.hpp. */
// FUNCTION: LEGORR 0x0040fec0
void set_global_scalar_to_one(void)
{
    (void)random_float_range(0.0f, 1.0f);
}
// FUNCTION: LEGORR 0x0040ff50
void scene_object_list_apply_float_callback(void* list, float value)
{
    scene_object_list_apply(list, 0x0040fed0u, &value);
}

/* The corpus reloads the base slot for each store and dereferences it
 * blindly. The outer NULL guard is retained because test_record pins it. */
// FUNCTION: LEGORR 0x0040ff70
void object_vector3_set(void* object_block, float x, float y, float z)
{
    if (object_block == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    void* base;
    __builtin_memcpy(&base, (char*)object_block, sizeof(base));
    __builtin_memcpy((char*)base + 0x3c, &x, sizeof(x));
    __builtin_memcpy(&base, (char*)object_block, sizeof(base));
    __builtin_memcpy((char*)base + 0x40, &y, sizeof(y));
    __builtin_memcpy(&base, (char*)object_block, sizeof(base));
    __builtin_memcpy((char*)base + 0x44, &z, sizeof(z));
}

/* SceneNodePositionSet (FUN_00410020) — see view.hpp. The inner object
 * rides the node's widened slot at +0; the corpus strides a dword. */
// FUNCTION: LEGORR 0x00410020
void scene_node_position_set(void* node, const float pos[3])
{
    if (node == NULL || pos == NULL) {
        return;
    }
    void* inner = NULL;
    const uint32_t valid = 1u;
    __builtin_memcpy((char*)node + 0x0c, &valid, sizeof valid);
    __builtin_memcpy(&inner, (const char*)node, sizeof inner);
    if (inner == NULL) {
        return; /* deviation: the corpus strides the handle blindly */
    }
    __builtin_memcpy((char*)inner + 0x0c, &pos[0], sizeof pos[0]);
    __builtin_memcpy((char*)inner + 0x10, &pos[1], sizeof pos[1]);
    __builtin_memcpy((char*)inner + 0x14, &pos[2], sizeof pos[2]);
}
// FUNCTION: LEGORR 0x00410070
void scene_object_list_apply_position_callback(void* list,
                                              const float position[3])
{
    scene_object_list_apply(list, 0x00410050u, (void*)position);
}

// FUNCTION: LEGORR 0x004100b0
void scene_object_list_apply_second_position_callback(
    void* list, const float position[3])
{
    scene_object_list_apply(list, 0x00410090u, (void*)position);
}

// FUNCTION: LEGORR 0x00410280
int callback_operation_run_and_return_status(void* stack_owner,
                                             CallbackOperationNode* first,
                                             CallbackOperationVisit visit)
{
    int status = 1;
    CallbackOperationNode* node;
    if (visit == NULL) return status;
    for (node = first; node != NULL; node = node->next)
        visit(stack_owner, node, &status);
    return status;
}

static void scene_object_list_apply(void* list, uint32_t operation,
                                    void* payload)
{
    void* object;
    unsigned int count;
    if (list == NULL || s_scene_object_list_apply_hook == NULL) return;
    object = *(void**)((char*)list + 4);
    count = 0;
    while (object != NULL && count < 65536u) {
        void* next = *(void**)((char*)object + 0x9c);
        s_scene_object_list_apply_hook(s_scene_object_list_apply_context,
                                       list, object, operation, payload);
        object = next;
        ++count;
    }
}

/* Item link slot: corpus item+0x9c. The corpus reads/writes a 32-bit
 * dword there; the port moves native pointers with memcpy so access is
 * byte-safe even though +0x9c is not pointer-aligned on 64-bit hosts
 * (documented deviation in the header). */
static void* ptrstack_link_get(const void* item)
{
    void* next;
    __builtin_memcpy(&next, (const char*)item + PTRSTACK_LINK_OFFSET,
                     sizeof(next));
    return next;
}

static void ptrstack_link_set(void* item, void* next)
{
    __builtin_memcpy((char*)item + PTRSTACK_LINK_OFFSET, &next,
                     sizeof(next));
}
