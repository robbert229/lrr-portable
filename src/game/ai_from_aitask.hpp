// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what ai.cpp uses from aitask.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by ai.cpp only. */
#ifndef LRR_GAME_AI_FROM_AITASK_HPP
#define LRR_GAME_AI_FROM_AITASK_HPP

#include "ai_for_aitask.hpp"

extern EntitySlot* s_active_list[ACTIVE_ENTITY_LIST_MAX];
extern int s_active_list_used;

#endif /* LRR_GAME_AI_FROM_AITASK_HPP */
