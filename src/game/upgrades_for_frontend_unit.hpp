// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from upgrades.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UPGRADES_FOR_FRONTEND_UNIT_HPP
#define LRR_GAME_UPGRADES_FOR_FRONTEND_UNIT_HPP

namespace lego {
extern UpgradeAverageCountHook s_average_count_hook;
extern UpgradeAverageValueHook s_average_value_hook;
} // namespace lego

#endif /* LRR_GAME_UPGRADES_FOR_FRONTEND_UNIT_HPP */
