// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/engine.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_ENGINE_SHARED_HPP
#define LRR_GODS98_ENGINE_SHARED_HPP

extern int32_t s_application_mode;

extern uint32_t s_window_mode_flag;

extern uint32_t s_overlay_word_03610;
extern uint32_t s_overlay_word_03614;

#endif /* LRR_GODS98_ENGINE_SHARED_HPP */
