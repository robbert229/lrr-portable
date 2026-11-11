// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what reward uses from crt.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_CRT_CRT_FOR_REWARD_HPP
#define LRR_CRT_CRT_FOR_REWARD_HPP

extern float s_float_00554adc;
extern float s_float_00554efc;
extern uint32_t s_counter_00555324;
extern uint32_t s_counter_00555328;
extern uint32_t s_counter_0055532c;
extern float s_float_00555330;
extern float s_float_00555120;
extern float s_reward_score;
extern float s_reward_count;

#endif /* LRR_CRT_CRT_FOR_REWARD_HPP */
