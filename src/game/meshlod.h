// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-053 MeshLOD.c: the level-of-detail mesh lists (meshlod.c says why
 * this is its own object).
 *
 * C-compatible header (ADR 0005). meshlod.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). The callers in other units declare
 * the functions in gods98/model.h, model.hpp, record.hpp or their own
 * imports; src/game/test_c_boundaries.cpp checks this file against them. */
#ifndef LEGORRR_GAME_MESHLOD_H
#define LEGORRR_GAME_MESHLOD_H

#include "gods98/model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A level-of-detail mesh list entry (MeshLOD, 0x18 bytes); the C view of
 * gods98/model.hpp's ModelEntryRecord, which defines it when included
 * first (test_c_boundaries.cpp). */
#ifndef LRR_MODEL_ENTRY_RECORD_DEFINED
#define LRR_MODEL_ENTRY_RECORD_DEFINED 1
struct ModelEntryRecord {
    void* object;              /* +0x00 the loaded mesh container */
    void* instance;            /* +0x04 the target it is swapped onto */
    char* name;                /* +0x08 the part name */
    int kind;                  /* +0x0c the set ID */
    unsigned int flags;        /* +0x10 bit 0 clone, bit 1 block head */
    struct ModelEntryRecord* next; /* +0x14 */
};
#endif

/* The port's names and contracts: their callers already use them. */
/* FUN_004519b0 MeshLOD_Create. */
ModelEntryRecord* model_entry_record_load(ModelEntryRecord* previous,
                                          const char* partName,
                                          const char* dir,
                                          const char* meshName,
                                          int setID);
/* FUN_00451ab0 MeshLOD_CreateEmpty. */
ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind);
/* FUN_00451b30 MeshLOD_Clone. */
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head);
/* FUN_00451bc0 MeshLOD_SwapTarget. */
void resource_list_update_matching_key(void* list, void* library,
                                       void* update_value,
                                       unsigned int key);
/* FUN_00451c30 MeshLOD_RemoveTargets. */
void linked_record_field4_clear(void* record);
/* FUN_00451c50 MeshLOD_Free. */
void owned_object_reference_chain_release(void* head);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MESHLOD_H */
