// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what helpwindow uses from upgrades.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UPGRADES_FOR_HELPWINDOW_HPP
#define LRR_GAME_UPGRADES_FOR_HELPWINDOW_HPP

namespace lego {
extern void (*s_building_upgrade_begin_hook)(void);
extern int (*s_building_upgrade_changed_hook)(int, int, int);
extern void (*s_building_upgrade_apply_hook)(int, int, int);
extern BuildingUpgradeEventHook s_building_upgrade_event_hook;
} // namespace lego

#endif /* LRR_GAME_UPGRADES_FOR_HELPWINDOW_HPP */
