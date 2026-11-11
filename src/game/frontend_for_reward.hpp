// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what reward uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_REWARD_HPP
#define LRR_GAME_FRONTEND_FOR_REWARD_HPP

extern FrontEnd* s_reward_save_frontend;

#endif /* LRR_GAME_FRONTEND_FOR_REWARD_HPP */
