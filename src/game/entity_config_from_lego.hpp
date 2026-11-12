// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what entity_config.cpp uses from lego.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by entity_config.cpp only. */
#ifndef LRR_GAME_ENTITY_CONFIG_FROM_LEGO_HPP
#define LRR_GAME_ENTITY_CONFIG_FROM_LEGO_HPP

int entity_minifigure_type_table_load(void);
int entity_building_types_load(void);
void entity_object_names_load(BlockFile* config);
void entity_object_the_names_load(BlockFile* config);
int entity_rockmonster_type_table_load(void);
int entity_vehicle_type_table_load(void);

#endif /* LRR_GAME_ENTITY_CONFIG_FROM_LEGO_HPP */
