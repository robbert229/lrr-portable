// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/crt/crt.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_CRT_CRT_SHARED_HPP
#define LRR_CRT_CRT_SHARED_HPP

extern uint32_t s_advisor_state_word;

extern uint32_t s_mode_zero_flag;

#endif /* LRR_CRT_CRT_SHARED_HPP */
