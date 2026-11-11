// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/entity_config.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_ENTITY_CONFIG_SHARED_HPP
#define LRR_GAME_ENTITY_CONFIG_SHARED_HPP

extern TypeConfigTable s_minifigure_type_config;
extern TypeConfigTable s_building_type_config;
extern TypeConfigTable s_vehicle_type_config;
extern TypeConfigTable s_rockmonster_type_config;
extern EntityTypeConfigRuntimeOps s_entity_type_runtime_ops;
void entity_type_config_word_set(void* row, uint32_t offset, uint32_t value);
int entity_type_config_row_parse_path(void* context, const char* asset_path, void* destination, uint32_t row_kind);

#endif /* LRR_GAME_ENTITY_CONFIG_SHARED_HPP */
