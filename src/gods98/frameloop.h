// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Main.c's frame pump (frameloop.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_FRAMELOOP_H
#define LEGORRR_GODS98_FRAMELOOP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void main_loop_frame_pump(int draw_projection_overlay);    /* 00477f30 */

/* Main.c's millisecond clock: a tail call to WINMM's timeGetTime, which
 * Reward.c calls directly (finding 574). */
unsigned long main_time_get(void);                          /* 00478000 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FRAMELOOP_H */
