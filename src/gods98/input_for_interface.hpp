// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from input.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_INPUT_FOR_INTERFACE_HPP
#define LRR_GODS98_INPUT_FOR_INTERFACE_HPP

/* --- input-entry / player-input flag-8 leaves (see input.hpp) --------------
 * Zero-initialized module statics with the documented window; the corpus
 * indexes through (possibly heap) pointers blindly. */
#define INPUT_FLAG_TABLE_SLOTS 74

/* IndexedEntityValueGet (FUN_0041cab0) — see input.hpp. The three binding
 * windows stride in the corpus's data section as adjacent 74-dword blocks
 * (DAT_004dcff8 / DAT_004dd120 / DAT_004dd248); their only writer, the
 * interface-images config load, is a recorded file-seam boundary, so the
 * port models the windows as own storage installed through the hook below.
 * Deviations: the corpus indexes blindly (the port answers zero outside
 * its 74-slot window), and the NULL-context gate deviation inherited
 * from InputActionPressedGet (FUN_0041c8f0) applies. */
#define INPUT_INDEXED_WINDOW_SLOTS 74

extern uint32_t s_player_kind1_click_counts[INPUT_FLAG_TABLE_SLOTS];
extern uint32_t s_player_kind4_click_counts[INPUT_FLAG_TABLE_SLOTS];
uint32_t* player_flags_table(int kind);
extern InputActionStateQueryFn s_input_action_state_query;
extern void* s_input_action_state_query_context;

#endif /* LRR_GODS98_INPUT_FOR_INTERFACE_HPP */
