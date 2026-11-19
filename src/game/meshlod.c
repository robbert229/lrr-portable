// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-053: the level-of-detail mesh lists (the original's MeshLOD.c, a name
 * from OpenLRR's), rebuilt in original shape (ADR 0005). Text
 * 004519b0-00451cd0, .rdata 0049e520 (one 1.0f, never read); no .data or
 * .bss of its own.
 *
 * A mesh list is a chain of 0x18-byte records, one per part of an
 * animation or vehicle model, loaded from LWO or MESH files
 * (MeshLOD_Create) or left empty (MeshLOD_CreateEmpty). A clone shares the
 * meshes with the original list (MeshLOD_Clone); MeshLOD_SwapTarget puts a
 * list's meshes onto the model's named parts, and MeshLOD_Free frees what a
 * list owns.
 *
 * Its own object, not the end of Map3D.c (GAME-026, terrain_map.c):
 * - The original has three never-read 1.0f literals at 0049e520-0049e528
 *   between Map3D.c's pool (0049e4e4-0049e51c) and the tutorial unit's
 *   (GAME-028, 0049e52c). Each is a separate object's Maths.h 1.0
 *   (gods98/maths_normalise_head.h; finding 693), and MSVC 5.0 pools a
 *   literal once an object, so three objects lie between the two pools:
 *   this one, Message.c (GAME-027) and the code-less object after it
 *   (message_data.c).
 * - The call graph is closed: only GAME-004 (00406c00-00406ec0), LegoObject.c
 *   (0044a7d0) and GAME-048 (0046c3f0-0046cff0) call these six functions,
 *   and they call only gods98; nothing in Map3D.c calls them or is called
 *   by them.
 * - They read no .rdata, .data or .bss: Map3D.c's .bss ends at 004eaf20,
 *   where Message.c's begins, and their strings are pooled with earlier
 *   units (004a1cbc, 004a1d5c, 004a4014).
 * - OpenLRR names them MeshLOD_*, and the original links its game units in
 *   name order (Map3D.c, MeshLOD.c, Message.c). */
#include <stdio.h>
#include <stdlib.h>
#include "gods98/mem_free.h"
#include <string.h>

#include "meshlod.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/mesh.h"
#include "gods98/resource.h"
#include "gods98/texture.h"
#include "crt/vc5names.h"
#include "gods98/maths_normalise_head.h"

// FUNCTION: LEGORR 0x004519b0
ModelEntryRecord* model_entry_record_load(ModelEntryRecord* previous,
                                          const char* partName,
                                          const char* dir,
                                          const char* meshName, int setID)
{
    char path[256];
    void* container;
    ModelEntryRecord* record;

    sprintf(path, "%s\\%s", dir, meshName);
    if ((container = container_load(NULL, path, "LWO", 1)) == NULL) {
        container = container_load(NULL, path, "MESH", 1);
    }
    if (container) {
        record = (ModelEntryRecord*)malloc(sizeof(ModelEntryRecord));
        record->flags = 2;
        if (previous) previous->next = record;
        record->object = container;
        record->next = NULL;
        record->kind = setID;
        record->name = (char*)malloc(strlen(partName) + 1);
        strcpy(record->name, partName);
        record->instance = NULL;
        return record;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00451ab0
ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind)
{
    ModelEntryRecord* record;

    record = (ModelEntryRecord*)malloc(sizeof(ModelEntryRecord));
    record->flags = 2;
    if (previous) previous->next = record;
    record->kind = kind;
    record->next = NULL;
    record->object = NULL;
    record->name = (char*)malloc(strlen(name) + 1);
    strcpy(record->name, name);
    record->instance = NULL;
    return record;
}

// FUNCTION: LEGORR 0x00451b30
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head)
{
    const ModelEntryRecord* node;
    ModelEntryRecord* clone;
    unsigned int count = 0;
    unsigned int i;

    for (node = head; node; node = node->next) count++;
    if (count) {
        clone = (ModelEntryRecord*)malloc(count * sizeof(ModelEntryRecord));
        i = 0;
        for (node = head; node; node = node->next) {
            clone[i] = *node;
            clone[i].instance = NULL;
            clone[i].flags = (clone[i].flags & ~2) | 1;
            clone[i].next = &clone[i + 1];
            i++;
        }
        clone[0].flags |= 2;
        clone[i - 1].next = NULL;
        return clone;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00451bc0
void resource_list_update_matching_key(void* list, void* library,
                                       void* update_value,
                                       unsigned int key)
{
    ModelEntryRecord* record;

    for (record = (ModelEntryRecord*)list; record; record = record->next) {
        if (key == (unsigned int)record->kind) {
            if (!record->instance) {
                const char* name = xf_texture_name_format(library, record->name, NULL);
                record->instance = resource_name_register_or_query_target(
                    (void**)library, name, 0, NULL);
            }
            if (record->instance) {
                interface_array_forward_sync(record->instance, record->object,
                                             update_value);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00451c30
void linked_record_field4_clear(void* record)
{
    ModelEntryRecord* node;

    for (node = (ModelEntryRecord*)record; node; node = node->next) {
        node->instance = NULL;
    }
}

// FUNCTION: LEGORR 0x00451c50
void owned_object_reference_chain_release(void* head)
{
    ModelEntryRecord* node;
    ModelEntryRecord* next;
    ModelEntryRecord* last = NULL;
    ModelEntryRecord* root = (ModelEntryRecord*)head;

    for (node = root; node; node = node->next) {
        if (!(node->flags & 1)) {
            if (node->name) free(node->name);
            if (node->object) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(node->object);
            }
        }
        if (node->flags & 2) {
            last = node;
        }
        else if (last) {
            last->next = node->next;
        }
    }
    if (last) {
        for (node = root; node; node = next) {
            next = node->next;
            free(node);
        }
    }
}
