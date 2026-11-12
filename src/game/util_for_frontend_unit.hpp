// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from util.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UTIL_FOR_FRONTEND_UNIT_HPP
#define LRR_GAME_UTIL_FOR_FRONTEND_UNIT_HPP

extern unsigned int s_string_queue_count;
extern StringQueueFlushHook s_string_queue_flush_hook;

#endif /* LRR_GAME_UTIL_FOR_FRONTEND_UNIT_HPP */
