// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the mesh-list (MeshLOD) helpers that
 * original-shape units call; the port keeps them in model.hpp, record.hpp
 * and resource.hpp. Shared C++ headers do not include it;
 * src/game/test_c_boundaries.cpp checks it against those headers. These
 * declarations are not added to record.h or resource.h because other C
 * units include those, and every declaration shifts their symbol numbering
 * (finding 538). */
#ifndef LEGORRR_GODS98_MODEL_H
#define LEGORRR_GODS98_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* The six-word mesh-list node; model.hpp defines it. */
typedef struct ModelEntryRecord ModelEntryRecord;

/* MeshLOD_Clone (FUN_00451b30, GAME-026): a flat copy of the list. */
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head);
/* MeshLOD_RemoveTargets (FUN_00451c30, GAME-026). */
void linked_record_field4_clear(void* record);
/* MeshLOD_SwapTarget (FUN_00451bc0, GAME-026). */
void resource_list_update_matching_key(void* list, void* library,
                                       void* update_value,
                                       unsigned int key);
/* MeshLOD_Free (FUN_00451c50, GAME-026). */
void owned_object_reference_chain_release(void* head);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MODEL_H */
