// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * the mouse words of Input.c's state (crt.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_CRT_CRT_MOUSE_H
#define LEGORRR_CRT_CRT_MOUSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t s_renderer_word_a;      /* DAT_0076ab24 mouse x */
extern uint32_t s_renderer_word_b;      /* DAT_0076ab28 mouse y */
extern uint32_t g_renderer_word_c;      /* DAT_0076ab3c left button held */
extern uint32_t s_window_mouse_word_4c; /* DAT_0076ab4c left button clicked */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_CRT_CRT_MOUSE_H */
