// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from entity.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_FOR_LEGO_HPP
#define LRR_GAME_ENTITY_FOR_LEGO_HPP

#define OBJECT_POINTER_REGISTRY_CAPACITY 10u

int entity_pool_find_by_callback_filtered_current( int (*callback)(EntitySlot*, void*), void* payload);
extern EntityCommandStatusFeedbackHooks s_entity_command_feedback_hooks;
extern EntitySlot* s_entity_command_feedback_last_entity;
extern char s_entity_command_feedback_last_text[256];
int entity_tab(int type);
extern uint32_t s_object_pointer_registry_words[ OBJECT_POINTER_REGISTRY_CAPACITY];
extern uint32_t s_object_pointer_registry_count;
extern EntitySlot* s_object_pointer_registry_entities[ OBJECT_POINTER_REGISTRY_CAPACITY];
extern PeriodicEntityCallback s_periodic_entity_callback;
extern void* s_periodic_entity_callback_context;
EntitySlot* deferred_olist_live_building_create( const DeferredOListEntityRecord* record, unsigned int, void* context);
extern uint32_t s_ctx_sel_ptr_first;
extern uint32_t s_ctx_sel_ptr_second;
extern uint32_t s_catalog_counts[5];

#endif /* LRR_GAME_ENTITY_FOR_LEGO_HPP */
