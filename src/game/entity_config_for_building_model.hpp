// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what building_model.cpp sees from
 * entity_config.cpp. A verbatim snapshot of entity_config_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_ENTITY_CONFIG_FOR_BUILDING_MODEL_HPP
#define LRR_GAME_ENTITY_CONFIG_FOR_BUILDING_MODEL_HPP

extern TypeConfigTable s_minifigure_type_config;
extern TypeConfigTable s_building_type_config;
extern TypeConfigTable s_vehicle_type_config;
extern TypeConfigTable s_rockmonster_type_config;
extern EntityTypeConfigRuntimeOps s_entity_type_runtime_ops;
void entity_type_config_word_set(void* row, uint32_t offset, uint32_t value);
int entity_type_config_row_parse_path(void* context, const char* asset_path, void* destination, uint32_t row_kind);


#endif /* LRR_GAME_ENTITY_CONFIG_FOR_BUILDING_MODEL_HPP */
