// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what pointer.cpp sees from
 * ui_list.cpp. A verbatim snapshot of ui_list_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_UI_LIST_FOR_POINTER_HPP
#define LRR_GAME_UI_LIST_FOR_POINTER_HPP

extern const char* s_pointer_cursor_names[56];

extern int s_ui_track_pointer_outer_gate;

#endif /* LRR_GAME_UI_LIST_FOR_POINTER_HPP */
