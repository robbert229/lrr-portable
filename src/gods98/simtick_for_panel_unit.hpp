// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what panel_unit uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_PANEL_UNIT_HPP
#define LRR_GODS98_SIMTICK_FOR_PANEL_UNIT_HPP

extern SimtickManagerSlot* s_mgr;
extern int s_mgr_count;
extern int (*s_repos)(int, int, int);
extern int (*s_attach)(int, int, int, int);
extern int s_metrics[2];
extern int s_repos_n;
extern int s_attach_n;
extern const uint32_t s_timer_bits[4];

#endif /* LRR_GODS98_SIMTICK_FOR_PANEL_UNIT_HPP */
