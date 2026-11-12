// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from ai.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_AI_FOR_LEGOOBJECT_HPP
#define LRR_GAME_AI_FOR_LEGOOBJECT_HPP

/* FUN_0043a960's idle table, DAT_004eabcc, max 100 (DAT_004ead5c). */
#define AI_IDLE_TABLE_MAX 100

extern const char s_tool_name_freezergun[];
extern const char* s_tool_type_names[AI_TOOL_TYPE_COUNT];
const LevelState* grid_of(const GameContext* ctx);

#endif /* LRR_GAME_AI_FOR_LEGOOBJECT_HPP */
