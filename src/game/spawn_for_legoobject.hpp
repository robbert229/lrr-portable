// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from spawn.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_SPAWN_FOR_LEGOOBJECT_HPP
#define LRR_GAME_SPAWN_FOR_LEGOOBJECT_HPP

namespace lego {
extern SpawnRandomTriggerHook s_trigger_hook;
extern SpawnScatterHook s_scatter_hook;
extern void* s_hook_context;
extern SpawnTypeFiveFrameResourceOps s_type_five_frame_resource_ops;
extern SpawnScenePositionGetHook s_scene_position_get;
extern void* s_scene_position_context;
extern GlobalScatterSpawnFindHook s_global_scatter_find_hook;
extern void* s_global_scatter_find_context;
extern int s_global_scatter_flag_a;
extern int s_global_scatter_flag_b;
} // namespace lego
namespace unported_ledger_stubs {
extern EmergenceSpawnHooks s_emergence_spawn_hooks;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_SPAWN_FOR_LEGOOBJECT_HPP */
