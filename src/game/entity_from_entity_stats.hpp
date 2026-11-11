// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what entity.cpp uses from entity_stats.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by entity.cpp only. */
#ifndef LRR_GAME_ENTITY_FROM_ENTITY_STATS_HPP
#define LRR_GAME_ENTITY_FROM_ENTITY_STATS_HPP

#include "entity_for_entity_stats.hpp"

extern const void** s_stats_type_table[STATS_TYPE_TABLE_TYPES];
extern float s_stats_cell_level_limit[STATS_TYPE_TABLE_TYPES] [STATS_TYPE_TABLE_ROWS];
int entity_building_index_set(EntitySlot* entity, uint32_t state);
extern EntityIndexedValueGroup s_entity_indexed_value_groups[ENTITY_INDEXED_VALUE_GROUPS];
void* entity_stats_field_at_a0_corpus(const EntitySlot* e);

#endif /* LRR_GAME_ENTITY_FROM_ENTITY_STATS_HPP */
