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

#endif /* LRR_GAME_ENTITY_SHARED_HPP */
