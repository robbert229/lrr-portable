// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what placement.cpp sees from
 * entity.cpp. A verbatim snapshot of entity_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_ENTITY_FOR_PLACEMENT_HPP
#define LRR_GAME_ENTITY_FOR_PLACEMENT_HPP

extern EntityPayloadReleaseFn s_payload_scene_release_hook;
extern EntityPayloadReleaseFn s_payload_chain_release_hook;

extern EntitySceneSlotRuntimeOps s_entity_scene_slot_runtime_ops;

extern const char s_item_name_ore[];
extern const char s_item_name_power_crystal[];

namespace unported_ledger_stubs {
uint32_t motion_config_profile_entries_release( void* config, MotionConfigProfileEntry* entries, uint32_t count, const MotionConfigProfileReleaseHooks* hooks);
} // namespace unported_ledger_stubs

extern EntityStateOverrideEntry s_entity_state_override_entries[64];
extern uint32_t s_entity_state_override_count;
extern EntityStateOverrideValue330ApplyHook s_entity_state_override_value_330_apply;
extern EntityStateOverrideRenameHook s_entity_state_override_rename;
extern void* s_entity_state_override_context;

extern EntityQuadCollisionHooks s_entity_quad_collision_hooks;

extern uint32_t s_scene_dispatch_point_low;
extern uint32_t s_scene_dispatch_point_high;
extern uint32_t s_scene_dispatch_word_c;
extern uint32_t s_scene_dispatch_word14;

#endif /* LRR_GAME_ENTITY_FOR_PLACEMENT_HPP */
