// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from spawn.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_SPAWN_FOR_LEGO_HPP
#define LRR_GAME_SPAWN_FOR_LEGO_HPP

namespace unported_ledger_stubs {
const char* emergence_type_name_get(int owner_index);
extern uint32_t s_start_level_pilot_grid_count;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_SPAWN_FOR_LEGO_HPP */
