// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mission.cpp uses from reward.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by mission.cpp only. */
#ifndef LRR_GAME_MISSION_FROM_REWARD_HPP
#define LRR_GAME_MISSION_FROM_REWARD_HPP

void mission_task_importance_refresh(GameTask* t);
void mission_task_quota_refresh(GameTask* t);

#endif /* LRR_GAME_MISSION_FROM_REWARD_HPP */
