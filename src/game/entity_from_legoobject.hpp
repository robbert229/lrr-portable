// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what entity.cpp uses from legoobject.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by entity.cpp only. */
#ifndef LRR_GAME_ENTITY_FROM_LEGOOBJECT_HPP
#define LRR_GAME_ENTITY_FROM_LEGOOBJECT_HPP

#include "entity_for_lego.hpp"
#include "entity_for_legoobject.hpp"

int entity_pool_find_by_callback_current( int (*callback)(EntitySlot*, void*), void* payload, int filter_flagged);
int entity_pool_find_by_callback_filtered_current( int (*callback)(EntitySlot*, void*), void* payload);
namespace unported_ledger_stubs {
void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet( void* entity, float delta, float* movement_delta, float* movement_scale);
float ledger_stub_00447b30_EntityActivityTick( EntitySlot* entity, float delta);
int entity_step_movement_vector_resolve(EntitySlot* entity, int advance, int use_saved_vector, int steering_mode);
float entity_path_step_advance_and_arrival_check(EntitySlot* entity, float delta);
int ledger_stub_0044a910_BuildingPlacementPreviewUpdate( const BuildingPlacementPreviewHooks* hooks);
} // namespace unported_ledger_stubs
int entity_route_marker_candidate_visit(EntitySlot* candidate, void* payload);
extern EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records;
extern uint32_t s_global_animation_state;
extern uint32_t s_pool_chunk_count;
extern void* s_pool_chunks[POOL_CHUNK_COUNT_SLOTS];
extern void* s_pool_record_head;
namespace unported_ledger_stubs {
int spatial_entity_damage_radius_callback(EntitySlot* candidate, void* payload);
int spatial_entity_vector_step_callback(EntitySlot* candidate, void* payload);
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_ENTITY_FROM_LEGOOBJECT_HPP */
