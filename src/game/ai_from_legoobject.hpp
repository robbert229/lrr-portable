// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what ai.cpp uses from legoobject.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by ai.cpp only. */
#ifndef LRR_GAME_AI_FROM_LEGOOBJECT_HPP
#define LRR_GAME_AI_FROM_LEGOOBJECT_HPP

#include "ai_for_legoobject.hpp"

extern const char s_tool_name_drill[];
extern const char s_tool_name_spade[];
extern const char s_tool_name_hammer[];
extern const char s_tool_name_spanner[];
extern const char s_tool_name_laser[];
extern const char s_tool_name_pushergun[];
extern const char s_tool_name_birdscarer[];
extern const char* s_activity[AI_ACTIVITY_COUNT];
extern const char* s_ability_type_names[AI_ABILITY_TYPE_COUNT];
extern uint32_t s_activity_resource_slots[AI_ACTIVITY_RESOURCE_SLOT_COUNT];
extern EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
extern int s_idle100_n;
void ai_lava_crust_activity_register(GameContext* ctx, EntitySlot* e);
void ai_attached_entity_wake(EntitySlot* parent);

#endif /* LRR_GAME_AI_FROM_LEGOOBJECT_HPP */
