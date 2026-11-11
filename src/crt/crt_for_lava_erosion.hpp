// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what lava_erosion.cpp sees from
 * crt.cpp. A verbatim snapshot of crt_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_CRT_CRT_FOR_LAVA_EROSION_HPP
#define LRR_CRT_CRT_FOR_LAVA_EROSION_HPP

extern uint32_t s_advisor_state_word;
/* Pointer.c's current cursor type (DAT_00500a98), defined in
 * gods98/media.cpp. */
extern int s_cursor_slot;

extern uint32_t s_mode_zero_flag;

extern uint32_t s_config_word;

extern uint32_t s_ui_flag_word;
extern int s_global_value_word;

extern uint32_t s_level_stored_word;

extern uint32_t s_rockfall_base;

extern void* s_marker_table[32];
extern uint32_t s_marker_count;
extern void* s_marker_descriptor;

extern uint32_t s_task_refresh_flag;

#endif /* LRR_CRT_CRT_FOR_LAVA_EROSION_HPP */
