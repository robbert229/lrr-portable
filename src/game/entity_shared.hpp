// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/entity.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_ENTITY_SHARED_HPP
#define LRR_GAME_ENTITY_SHARED_HPP

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

#endif /* LRR_GAME_ENTITY_SHARED_HPP */
