// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what select.cpp uses from legoobject.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by select.cpp only. */
#ifndef LRR_GAME_SELECT_FROM_LEGOOBJECT_HPP
#define LRR_GAME_SELECT_FROM_LEGOOBJECT_HPP

int selection_candidate_filter_pool_refresh_callback( EntitySlot* entity, void* payload);

#endif /* LRR_GAME_SELECT_FROM_LEGOOBJECT_HPP */
