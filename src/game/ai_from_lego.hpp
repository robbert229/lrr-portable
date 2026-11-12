// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what ai.cpp uses from lego.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by ai.cpp only. */
#ifndef LRR_GAME_AI_FROM_LEGO_HPP
#define LRR_GAME_AI_FROM_LEGO_HPP

extern const char s_activity_name_activity_stand[];

#endif /* LRR_GAME_AI_FROM_LEGO_HPP */
