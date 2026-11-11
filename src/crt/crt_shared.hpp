// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/crt/crt.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_CRT_CRT_SHARED_HPP
#define LRR_CRT_CRT_SHARED_HPP

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

extern uint32_t s_counter_004ff990;
extern uint32_t s_flag_004ff978;

#endif /* LRR_CRT_CRT_SHARED_HPP */
