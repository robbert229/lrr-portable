// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what input.cpp uses from interface.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by input.cpp only. */
#ifndef LRR_GODS98_INPUT_FROM_INTERFACE_HPP
#define LRR_GODS98_INPUT_FROM_INTERFACE_HPP

#include "input_for_interface.hpp"

extern uint32_t s_input_entry_flags[INPUT_FLAG_TABLE_SLOTS];
extern uint32_t s_input_action_click_counts[INPUT_FLAG_TABLE_SLOTS];
extern uint32_t s_indexed_window_c[INPUT_INDEXED_WINDOW_SLOTS];
extern uint32_t s_indexed_window_a[INPUT_INDEXED_WINDOW_SLOTS];
extern uint32_t s_indexed_window_b[INPUT_INDEXED_WINDOW_SLOTS];

#endif /* LRR_GODS98_INPUT_FROM_INTERFACE_HPP */
