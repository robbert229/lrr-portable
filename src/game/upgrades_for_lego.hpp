// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from upgrades.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UPGRADES_FOR_LEGO_HPP
#define LRR_GAME_UPGRADES_FOR_LEGO_HPP

namespace lego {
extern UpgradeConfigEntryHook s_upgrade_entry_hook;
extern uint32_t (*s_upgrade_entry_count_hook)(void*);
extern uint32_t s_level_upgrade_names[UPGRADE_NAME_SLOT_COUNT];
extern char* s_level_upgrade_name_strings[UPGRADE_NAME_SLOT_COUNT];
extern UpgradeTypeRecord* s_upgrade_types;
extern char** s_upgrade_type_names;
extern uint32_t s_upgrade_type_count;
extern UpgradeTypeLoadHook s_upgrade_type_load_hook;
} // namespace lego

#endif /* LRR_GAME_UPGRADES_FOR_LEGO_HPP */
