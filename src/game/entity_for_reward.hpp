// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what reward uses from entity.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_FOR_REWARD_HPP
#define LRR_GAME_ENTITY_FOR_REWARD_HPP

int entity_type_pair_for_name(const char* name, int* type_out, int* row_out, int* zero_out);

#endif /* LRR_GAME_ENTITY_FOR_REWARD_HPP */
