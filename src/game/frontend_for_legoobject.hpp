// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_LEGOOBJECT_HPP
#define LRR_GAME_FRONTEND_FOR_LEGOOBJECT_HPP

extern const char* s_tooltip_primary_names[11];
extern const char* s_tooltip_secondary_names[6];
extern void* s_tooltip_primary_handles[11];
extern void* s_tooltip_secondary_handles[6];
extern void* s_tooltip_blank_handle;
extern void* s_tooltip_cursor_handle;

#endif /* LRR_GAME_FRONTEND_FOR_LEGOOBJECT_HPP */
