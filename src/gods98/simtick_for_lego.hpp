// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_LEGO_HPP
#define LRR_GODS98_SIMTICK_FOR_LEGO_HPP

extern void* s_minion_list;
extern int (*s_minion_dist)(int, float);
extern void (*s_minion_act)(int);
extern void (*s_minion_fail)(void*);
extern int s_minion_actions;
extern int s_minion_fails;
extern SimtickRenderPassHook s_render_pass_hook;
extern void* s_render_pass_user;
extern int (*s_pick)(int, int);
extern int s_picks;
extern int s_camera_detail_collection_ready;

#endif /* LRR_GODS98_SIMTICK_FOR_LEGO_HPP */
