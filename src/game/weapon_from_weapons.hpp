// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapon.cpp uses from weapons.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by weapon.cpp only. */
#ifndef LRR_GAME_WEAPON_FROM_WEAPONS_HPP
#define LRR_GAME_WEAPON_FROM_WEAPONS_HPP

namespace lego {
extern WeaponIndexedTypeRecord* s_weapon_indexed_records;
float weapon_type_recharge_time_get_target(int type);
float weapon_type_discharge_rate_get_target(int type);
float weapon_type_range_get_target(int type);
float weapon_type_wall_destroy_time_get_target(int type, int wall_type);
} // namespace lego

#endif /* LRR_GAME_WEAPON_FROM_WEAPONS_HPP */
