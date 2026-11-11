// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what reward uses from objective.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_OBJECTIVE_FOR_REWARD_HPP
#define LRR_GAME_OBJECTIVE_FOR_REWARD_HPP

extern RewardTaskResourceRecord s_reward_task_resources[10];

#endif /* LRR_GAME_OBJECTIVE_FOR_REWARD_HPP */
