// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapons uses from weapon.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_WEAPON_FOR_WEAPONS_HPP
#define LRR_GAME_WEAPON_FOR_WEAPONS_HPP

namespace lego {
extern WeaponDefinition s_weapon_definitions[WEAPON_TYPE_CAPACITY];
extern uint32_t s_weapon_type_count;
extern WeaponDefinitionsParseHook s_weapon_parse_hook;
extern WeaponIndexedTypeRecord* s_weapon_owned_records;
extern uint32_t s_weapon_indexed_record_count;
} // namespace lego

#endif /* LRR_GAME_WEAPON_FOR_WEAPONS_HPP */
