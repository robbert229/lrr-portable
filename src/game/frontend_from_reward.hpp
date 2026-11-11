// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from reward.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_REWARD_HPP
#define LRR_GAME_FRONTEND_FROM_REWARD_HPP

namespace reference_target {
void save_menu_open_from_reward(void);
} // namespace reference_target

#endif /* LRR_GAME_FRONTEND_FROM_REWARD_HPP */
