// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mesh_unit uses from spawn.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_SPAWN_FOR_MESH_UNIT_HPP
#define LRR_GAME_SPAWN_FOR_MESH_UNIT_HPP

namespace lego {
extern LegacyResultCheckHook s_result_check_hook;
extern void* s_result_check_context;
} // namespace lego

#endif /* LRR_GAME_SPAWN_FOR_MESH_UNIT_HPP */
